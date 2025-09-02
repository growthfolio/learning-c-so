#include <windows.h>
#include <stdio.h>
#include <stdbool.h>

// Variáveis globais para os hooks
HHOOK hKeyboardHook = NULL;
HHOOK hMouseHook = NULL;
bool running = true;

// Procedimento do hook de teclado
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* kbStruct = (KBDLLHOOKSTRUCT*)lParam;
        
        if (wParam == WM_KEYDOWN) {
            printf("Tecla pressionada: VK_CODE = %lu\n", kbStruct->vkCode);
            
            // Exemplo: detectar ESC para sair
            if (kbStruct->vkCode == VK_ESCAPE) {
                printf("ESC pressionado - encerrando programa...\n");
                running = false;
                PostQuitMessage(0);
            }
            
            // Exemplo: detectar algumas teclas específicas
            switch (kbStruct->vkCode) {
                case VK_SPACE:
                    printf("  -> Barra de espaço\n");
                    break;
                case VK_RETURN:
                    printf("  -> Enter\n");
                    break;
                case 'A':
                    printf("  -> Letra A\n");
                    break;
            }
        }
        else if (wParam == WM_KEYUP) {
            printf("Tecla liberada: VK_CODE = %lu\n", kbStruct->vkCode);
        }
    }
    
    // Chama o próximo hook na cadeia
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

// Procedimento do hook de mouse
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSLLHOOKSTRUCT* mouseStruct = (MSLLHOOKSTRUCT*)lParam;
        
        switch (wParam) {
            case WM_LBUTTONDOWN:
                printf("Botão esquerdo pressionado em (%ld, %ld)\n", 
                       mouseStruct->pt.x, mouseStruct->pt.y);
                break;
                
            case WM_LBUTTONUP:
                printf("Botão esquerdo liberado em (%ld, %ld)\n", 
                       mouseStruct->pt.x, mouseStruct->pt.y);
                break;
                
            case WM_RBUTTONDOWN:
                printf("Botão direito pressionado em (%ld, %ld)\n", 
                       mouseStruct->pt.x, mouseStruct->pt.y);
                break;
                
            case WM_RBUTTONUP:
                printf("Botão direito liberado em (%ld, %ld)\n", 
                       mouseStruct->pt.x, mouseStruct->pt.y);
                break;
                
            case WM_MOUSEMOVE:
                // Descomente a linha abaixo se quiser ver movimento do mouse
                // printf("Mouse movido para (%ld, %ld)\n", mouseStruct->pt.x, mouseStruct->pt.y);
                break;
                
            case WM_MOUSEWHEEL:
                {
                    int delta = GET_WHEEL_DELTA_WPARAM(mouseStruct->mouseData);
                    printf("Scroll do mouse: %s (delta: %d) em (%ld, %ld)\n",
                           delta > 0 ? "para cima" : "para baixo", delta,
                           mouseStruct->pt.x, mouseStruct->pt.y);
                }
                break;
        }
    }
    
    // Chama o próximo hook na cadeia
    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

// Função para instalar os hooks
bool InstallHooks() {
    // Hook de teclado de baixo nível (global)
    hKeyboardHook = SetWindowsHookEx(
        WH_KEYBOARD_LL,           // Tipo do hook
        KeyboardProc,             // Procedimento do hook
        GetModuleHandle(NULL),    // Handle do módulo
        0                         // ID da thread (0 = global)
    );
    
    if (hKeyboardHook == NULL) {
        printf("Erro ao instalar hook de teclado: %lu\n", GetLastError());
        return false;
    }
    
    // Hook de mouse de baixo nível (global)
    hMouseHook = SetWindowsHookEx(
        WH_MOUSE_LL,              // Tipo do hook
        MouseProc,                // Procedimento do hook
        GetModuleHandle(NULL),    // Handle do módulo
        0                         // ID da thread (0 = global)
    );
    
    if (hMouseHook == NULL) {
        printf("Erro ao instalar hook de mouse: %lu\n", GetLastError());
        UnhookWindowsHookEx(hKeyboardHook);
        return false;
    }
    
    printf("Hooks instalados com sucesso!\n");
    printf("Pressione ESC para sair do programa.\n");
    printf("Monitorando eventos de teclado e mouse...\n\n");
    
    return true;
}

// Função para remover os hooks
void UninstallHooks() {
    if (hKeyboardHook != NULL) {
        UnhookWindowsHookEx(hKeyboardHook);
        hKeyboardHook = NULL;
    }
    
    if (hMouseHook != NULL) {
        UnhookWindowsHookEx(hMouseHook);
        hMouseHook = NULL;
    }
    
    printf("Hooks removidos.\n");
}

// Função principal
int main() {
    printf("=== Monitor Global de Teclado e Mouse ===\n");
    printf("Usando apenas API Win32 nativa\n\n");
    
    // Instalar os hooks
    if (!InstallHooks()) {
        printf("Falha ao instalar hooks. Encerrando...\n");
        return 1;
    }
    
    // Loop de mensagens
    MSG msg;
    while (running && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Limpar recursos
    UninstallHooks();
    printf("Programa encerrado.\n");
    
    return 0;
}

// Instruções de compilação:
// gcc -o global_hook.exe global_hook.c -luser32
// 
// Ou usando Visual Studio:
// cl global_hook.c user32.lib
//
// IMPORTANTE: 
// - Execute como Administrador para hooks globais funcionarem corretamente
// - Este código funciona apenas no Windows
// - Para Linux, seria necessário usar X11 ou outras APIs específicas
