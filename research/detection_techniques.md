# 🔵 Técnicas de Detecção (Blue Team)

## Análise Comportamental

### 1. Monitoramento de APIs Críticas
```c
// APIs suspeitas para monitorar:
// Windows
- SetWindowsHookEx (WH_KEYBOARD_LL, WH_MOUSE_LL)
- CreateRemoteThread
- VirtualAllocEx
- WriteProcessMemory
- NtSetInformationProcess

// Linux
- XRecordEnableContext
- ptrace
- /proc/*/mem access
- LD_PRELOAD usage

// macOS
- CGEventTapCreate
- CGEventPost
- task_for_pid
```

### 2. Padrões de Tráfego de Rede
```bash
# Detecção de exfiltração DNS
# Procurar por:
- Queries DNS com subdomínios longos e aleatórios
- Frequência alta de queries para domínios específicos
- Dados codificados em Base64 nos subdomínios
- Padrões de timing regulares

# Exemplo de regra Suricata:
alert dns any any -> any any (msg:"Possível exfiltração DNS"; 
    dns_query; content:".data."; pcre:"/[A-Za-z0-9+\/]{40,}/"; 
    threshold:type both,track by_src,count 10,seconds 60; 
    sid:1000001;)
```

### 3. Análise de Processos
```bash
# Indicadores suspeitos:
- Processos com nomes genéricos do sistema em locais não padrão
- Processos filhos inesperados de explorer.exe/systemd
- Uso alto de CPU por processos "do sistema"
- Handles abertos para dispositivos de entrada

# PowerShell para Windows:
Get-Process | Where-Object {$_.ProcessName -like "*svchost*" -and $_.Path -notlike "*system32*"}

# Linux:
ps aux | awk '$11 ~ /^\[.*\]$/ && $3 > 1.0'
```

## Ferramentas de Detecção

### 1. YARA Rules
```yara
rule Stealth_Keylogger {
    meta:
        description = "Detecta keylogger stealth"
        author = "Blue Team"
        
    strings:
        $api1 = "SetWindowsHookEx" ascii
        $api2 = "GetAsyncKeyState" ascii
        $api3 = "XRecordEnableContext" ascii
        $crypto = { 48 B8 ?? ?? ?? ?? ?? ?? ?? ?? FF E0 } // JMP RAX pattern
        $dns_exfil = ".data." ascii
        
    condition:
        any of ($api*) and ($crypto or $dns_exfil)
}
```

### 2. Sysmon Configuration
```xml
<Sysmon schemaversion="4.30">
  <EventFiltering>
    <!-- Detectar hooks de baixo nível -->
    <ProcessCreate onmatch="include">
      <Image condition="contains">svchost</Image>
      <Image condition="contains">system-monitor</Image>
      <ParentImage condition="contains">explorer.exe</ParentImage>
    </ProcessCreate>
    
    <!-- Monitorar criação de arquivos suspeitos -->
    <FileCreate onmatch="include">
      <TargetFilename condition="contains">.sys_</TargetFilename>
      <TargetFilename condition="contains">LaunchAgents</TargetFilename>
    </FileCreate>
    
    <!-- Detectar modificações no registry -->
    <RegistryEvent onmatch="include">
      <TargetObject condition="contains">CurrentVersion\Run</TargetObject>
    </RegistryEvent>
  </EventFiltering>
</Sysmon>
```

### 3. EDR Queries (KQL/Splunk)
```kql
// Azure Sentinel / Defender
DeviceProcessEvents
| where ProcessCommandLine contains "svchost" 
    and not(FolderPath startswith "C:\\Windows\\System32")
| where ProcessCommandLine contains any("hook", "record", "tap")

// Splunk
index=windows EventCode=1 
| where match(Image, ".*svchost.*") AND NOT match(Image, ".*system32.*")
| stats count by Computer, Image, CommandLine
```

## Contramedidas Técnicas

### 1. API Hooking Protection
```c
// Verificar integridade de APIs críticas
bool verify_api_integrity() {
    HMODULE user32 = GetModuleHandle(L"user32.dll");
    FARPROC original_proc = GetProcAddress(user32, "SetWindowsHookExW");
    
    // Verificar se os primeiros bytes foram modificados
    BYTE expected_bytes[] = {0x48, 0x89, 0x5C, 0x24}; // Padrão esperado
    BYTE* actual_bytes = (BYTE*)original_proc;
    
    return memcmp(actual_bytes, expected_bytes, sizeof(expected_bytes)) == 0;
}
```

### 2. Monitoramento de Memória
```c
// Detectar injeção de código
void monitor_memory_changes() {
    MEMORY_BASIC_INFORMATION mbi;
    LPVOID address = 0;
    
    while (VirtualQuery(address, &mbi, sizeof(mbi))) {
        if (mbi.Protect & PAGE_EXECUTE_READWRITE) {
            // Região suspeita - executável e gravável
            log_suspicious_memory(address, mbi.RegionSize);
        }
        address = (LPBYTE)mbi.BaseAddress + mbi.RegionSize;
    }
}
```

### 3. Detecção de Persistência
```bash
#!/bin/bash
# Script de detecção de persistência

echo "🔍 Verificando mecanismos de persistência..."

# Windows
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
    echo "Verificando Registry Run keys..."
    reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" | grep -i "security\|update\|system"
    
    echo "Verificando Scheduled Tasks..."
    schtasks /query /fo csv | grep -i "security\|update\|system"
fi

# Linux
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "Verificando systemd services..."
    systemctl list-units --type=service | grep -E "(monitor|security|update)"
    
    echo "Verificando crontab..."
    crontab -l 2>/dev/null | grep -v "^#"
    
    echo "Verificando autostart..."
    ls -la ~/.config/autostart/ 2>/dev/null
fi

# macOS
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Verificando LaunchAgents..."
    ls -la ~/Library/LaunchAgents/ | grep -E "(system|security|update)"
    
    echo "Verificando LaunchDaemons..."
    sudo ls -la /Library/LaunchDaemons/ | grep -E "(system|security|update)"
fi
```

## Análise Forense

### 1. Coleta de Evidências
```bash
# Capturar estado do sistema
echo "📊 Coletando evidências..."

# Processos em execução
ps aux > processes_$(date +%Y%m%d_%H%M%S).txt

# Conexões de rede
netstat -tulpn > network_$(date +%Y%m%d_%H%M%S).txt

# Arquivos modificados recentemente
find /tmp /var/tmp -type f -mtime -1 > recent_files_$(date +%Y%m%d_%H%M%S).txt

# Logs do sistema
journalctl --since "1 hour ago" > system_logs_$(date +%Y%m%d_%H%M%S).txt
```

### 2. Análise de Memória
```bash
# Usando Volatility
volatility -f memory_dump.raw --profile=Win10x64 pslist
volatility -f memory_dump.raw --profile=Win10x64 malfind
volatility -f memory_dump.raw --profile=Win10x64 apihooks
```

### 3. Timeline Analysis
```bash
# Criar timeline de eventos
log2timeline.py --storage-file timeline.plaso disk_image.dd
psort.py -o dynamic timeline.plaso > timeline_analysis.txt
```

## Métricas de Detecção

### 1. KPIs de Segurança
- Tempo médio de detecção (MTTD)
- Taxa de falsos positivos
- Cobertura de técnicas MITRE ATT&CK
- Eficácia de resposta a incidentes

### 2. Alertas Críticos
```json
{
  "alert_rules": [
    {
      "name": "Keylogger_Detection",
      "severity": "HIGH",
      "conditions": [
        "process_name matches system patterns",
        "api_calls contains hook functions",
        "network_traffic shows dns_exfiltration"
      ],
      "response": "isolate_endpoint"
    }
  ]
}
```

## Treinamento Blue Team

### 1. Cenários de Simulação
- Deploy controlado do keylogger em ambiente de lab
- Análise de logs e identificação de IOCs
- Desenvolvimento de regras de detecção
- Teste de ferramentas de resposta

### 2. Exercícios Práticos
- Análise de malware real
- Reverse engineering de técnicas
- Desenvolvimento de contramedidas
- Simulação de resposta a incidentes

**🎯 Objetivo**: Transformar conhecimento teórico em habilidades práticas de detecção e resposta.