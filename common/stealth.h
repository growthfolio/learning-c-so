#ifndef STEALTH_H
#define STEALTH_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
    #include <windows.h>
#elif __linux__
    #include <sys/sysctl.h>
    #include <time.h>
#elif __APPLE__
    #include <sys/sysctl.h>
    #include <sys/types.h>
    #include <sys/proc.h>
#endif

// Anti-debugging genérico
static inline bool is_debugger_present() {
#ifdef _WIN32
    return IsDebuggerPresent() || CheckRemoteDebuggerPresent(GetCurrentProcess(), NULL);
#elif __linux__
    FILE* status = fopen("/proc/self/status", "r");
    if (!status) return false;
    
    char line[256];
    while (fgets(line, sizeof(line), status)) {
        if (strstr(line, "TracerPid:")) {
            int tracer_pid = atoi(line + 10);
            fclose(status);
            return tracer_pid != 0;
        }
    }
    fclose(status);
    return false;
#elif __APPLE__
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid()};
    struct kinfo_proc info;
    size_t size = sizeof(info);
    
    if (sysctl(mib, 4, &info, &size, NULL, 0) == 0) {
        return (info.kp_proc.p_flag & P_TRACED) != 0;
    }
    return false;
#endif
}

// Verificação de VM/Sandbox
static inline bool is_virtual_machine() {
#ifdef _WIN32
    // Verificar registry keys de VMs conhecidas
    HKEY key;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\VBoxService", 
                     0, KEY_READ, &key) == ERROR_SUCCESS) {
        RegCloseKey(key);
        return true;
    }
    return false;
#elif __linux__
    // Verificar DMI info
    FILE* dmi = fopen("/sys/class/dmi/id/product_name", "r");
    if (dmi) {
        char product[256];
        if (fgets(product, sizeof(product), dmi)) {
            fclose(dmi);
            return strstr(product, "VirtualBox") || strstr(product, "VMware");
        }
        fclose(dmi);
    }
    return false;
#elif __APPLE__
    // Verificar modelo do hardware
    size_t size = 0;
    sysctlbyname("hw.model", NULL, &size, NULL, 0);
    char* model = malloc(size);
    sysctlbyname("hw.model", model, &size, NULL, 0);
    bool is_vm = strstr(model, "VMware") != NULL;
    free(model);
    return is_vm;
#endif
}

// Delay anti-análise
static inline void anti_analysis_delay() {
    uint64_t start = 0, end = 0;
    
#ifdef _WIN32
    start = GetTickCount64();
    Sleep(100);
    end = GetTickCount64();
#elif __linux__ || __APPLE__
    struct timespec ts_start, ts_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);
    usleep(100000);
    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    
    start = ts_start.tv_sec * 1000 + ts_start.tv_nsec / 1000000;
    end = ts_end.tv_sec * 1000 + ts_end.tv_nsec / 1000000;
#endif
    
    // Se o delay foi muito longo, provavelmente está sendo analisado
    if ((end - start) > 200) {
        exit(0);
    }
}

#endif