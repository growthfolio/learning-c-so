#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <shellapi.h>

// Variáveis globais para os hooks
HHOOK hKeyboardHook = NULL;
HHOOK hMouseHook = NULL;
bool running = true;

// Função para verificar se está rodando como administrador
bool IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    
    if (AllocateAndInitializeSid(&ntAuthority, 2, 
                                SECURITY_BUILTIN_DOMAIN_RID, 
                                DOMAIN_ALIAS_RID_ADMINS,
                                0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    
    return isAdmin == TRUE;
}

// Função para reexecutar como administrador
bool RestartAsAdmin() {
    char exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);
    
    SHELLEXECUTEINFO sei = {0};
    sei.cbSize = sizeof(SHELLEXECUTEINFO);
    sei.lpVerb = "runas";
    sei.lpFile = exePath;
    sei.lpParameters = "--admin";  // Parâmetro para indicar execução como admin
    sei.nShow = SW_NORMAL;
    
    return ShellExecuteEx(&sei);
}

// Procedimento do hook de teclado
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* kbStruct = (KBDLLHOOKSTRUCT*)lParam;
        
        if (wParam == WM_KEYDOWN) {
            printf("Tecla: VK_%lu ", kbStruct->vkCode);
            
            // Mostrar nome da tecla quando possível
            switch (kbStruct->vkCode) {
                case VK_ESCAPE:
                    printf("(ESC) - Encerrando...\n");
                    running = false;
                    PostQuitMessage(0);
                    break;
                case VK_SPACE:
                    printf("(SPACE)\n");
                    break;
                case VK_RETURN:
                    printf("(ENTER)\n");
                    break;
                case VK_SHIFT:
                    printf("(SHIFT)\n");
                    break;
                case VK_CONTROL:
                    printf("(CTRL)\n");
                    break;
                case VK_MENU:
                    printf("(ALT)\n");
                    break;
                case VK_TAB:
                    printf("(TAB)\n");
                    break;
                case VK_BACK:
                    printf("(BACKSPACE)\n");
                    break;
                case VK_DELETE:
                    printf("(DELETE)\n");
                    break;
                default:
                    if (kbStruct->vkCode >= 'A' && kbStruct->vkCode <= 'Z') {
                        printf("(%c)\n", (char)kbStruct->vkCode);
                    } else if (kbStruct->vkCode >= '0' && kbStruct->vkCode <= '9') {
                        printf("(%c)\n", (char)kbStruct->vkCode);
                    } else {
                        printf("\n");
                    }
                    break;
            }
        }
    }
    
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

// Procedimento do hook de mouse
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSLLHOOKSTRUCT* mouseStruct = (MSLLHOOKSTRUCT*)lParam;
        
        switch (wParam) {
            case WM_LBUTTONDOWN:
                printf("Mouse: Botão ESQUERDO pressionado em (%ld, %ld)\n", 
                       mouseStruct->pt.x, mouseStruct->pt.y);
                break;
                
            case WM_RBUTTONDOWN:
                printf("Mouse: Botão DIREITO pressionado em (%ld, %ld)\n", 
                       mouseStruct->pt.x, mouseStruct->pt.y);
                break;
                
            case WM_MBUTTONDOWN:
                printf("Mouse: Botão MEIO pressionado em (%ld, %ld)\n", 
                       mouseStruct->pt.x, mouseStruct->pt.y);
                break;
                
            case WM_MOUSEWHEEL:
                {
                    int delta = GET_WHEEL_DELTA_WPARAM(mouseStruct->mouseData);
                    printf("Mouse: Scroll %s em (%ld, %ld)\n",
                           delta > 0 ? "CIMA" : "BAIXO",
                           mouseStruct->pt.x, mouseStruct->pt.y);
                }
                break;
        }
    }
    
    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

// Função principal do monitor de hooks
int RunHookMonitor() {
    printf("\n=== Iniciando Monitor Global ===\n");
    printf("Instalando hooks...\n");
    
    // Instalar hook de teclado
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, 
                                    GetModuleHandle(NULL), 0);
    if (hKeyboardHook == NULL) {
        printf("❌ Erro ao instalar hook de teclado: %lu\n", GetLastError());
        return 1;
    }
    
    // Instalar hook de mouse
    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, 
                                 GetModuleHandle(NULL), 0);
    if (hMouseHook == NULL) {
        printf("❌ Erro ao instalar hook de mouse: %lu\n", GetLastError());
        UnhookWindowsHookEx(hKeyboardHook);
        return 1;
    }
    
    printf("✓ Hooks instalados com sucesso!\n");
    printf("✓ Monitorando eventos globais...\n");
    printf("➤ Pressione ESC para sair\n\n");
    
    // Loop de mensagens
    MSG msg;
    while (running && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Limpar hooks
    if (hKeyboardHook) UnhookWindowsHookEx(hKeyboardHook);
    if (hMouseHook) UnhookWindowsHookEx(hMouseHook);
    
    printf("\n✓ Hooks removidos. Programa encerrado.\n");
    return 0;
}

int main(int argc, char* argv[]) {
    printf("=== Hook Global com Auto-Elevação ===\n");
    printf("Monitor de Teclado e Mouse - Windows\n");
    
    // Verificar se foi passado o parâmetro --admin (indica que já foi relançado)
    bool wasRelaunched = false;
    if (argc > 1 && strcmp(argv[1], "--admin") == 0) {
        wasRelaunched = true;
    }
    
    // Verificar privilégios
    if (IsRunningAsAdmin()) {
        printf("✓ Executando com privilégios de administrador.\n");
        
        if (wasRelaunched) {
            printf("✓ Elevação de privilégios bem-sucedida.\n");
        }
        
        // Executar o monitor de hooks
        return RunHookMonitor();
        
    } else {
        printf("⚠ Não está executando como administrador.\n");
        printf("➤ Solicitando elevação de privilégios...\n\n");
        
        // Tentar reexecutar como administrador
        if (RestartAsAdmin()) {
            printf("✓ Solicitação de elevação enviada.\n");
            printf("➤ Aguarde a janela do UAC e clique em 'Sim'.\n");
            printf("➤ Este processo será encerrado e o novo será iniciado como admin.\n");
        } else {
            printf("❌ Falha ao solicitar elevação de privilégios.\n");
            printf("\nPossíveis causas:\n");
            printf("• UAC está desabilitado\n");
            printf("• Usuário cancelou a elevação\n");
            printf("• Problemas de permissão no sistema\n");
            printf("\nTente executar manualmente como administrador:\n");
            printf("1. Clique com botão direito no executável\n");
            printf("2. Selecione 'Executar como administrador'\n");
        }
        
        printf("\nPressione Enter para sair...\n");
        getchar();
        return 1;
    }
}

/*
INSTRUÇÕES DE COMPILAÇÃO E USO:

1. Compilar:
   gcc -o hook_monitor.exe self_elevating_hook.c -luser32 -lshell32 -ladvapi32

2. Usar:
   - Execute normalmente: hook_monitor.exe
   - O programa detectará se não é admin
   - Solicitará elevação automaticamente
   - Aparecerá o UAC do Windows
   - Clique "Sim" para executar como admin

COMO FUNCIONA:
1. Programa verifica se já é admin
2. Se não for, usa ShellExecuteEx com "runas"
3. Isso aciona o UAC do Windows
4. Se aprovado, relança como admin com parâmetro --admin
5. Nova instância roda como admin e executa os hooks

VANTAGENS:
✓ Tudo em um único executável
✓ Auto-detecção de privilégios
✓ Relançamento automático
✓ Feedback claro para o usuário
✓ Tratamento de erros

REQUISITOS:
- Windows Vista+ (para UAC)
- Usuário deve ter direitos de administrador
- Usuário deve aceitar o prompt do UAC
*/
