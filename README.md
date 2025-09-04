# 🎯 Monitor Stealth Multi-Plataforma - Projeto Científico

## VIVA A CIÊNCIA DA COMPUTAÇÃO! 🚀⚡

Monitor de eventos de baixo nível para pesquisa em segurança cibernética, desenvolvido para Windows, Linux e macOS usando apenas APIs nativas.

## ⚠️ AVISO LEGAL IMPORTANTE

Este projeto é destinado **EXCLUSIVAMENTE** para:
- 🎓 Pesquisa acadêmica em segurança cibernética
- 🔵 Treinamento de Blue Team (detecção e resposta)
- 🔴 Treinamento de Red Team (técnicas de evasão)
- 🧪 Análise científica de comportamento de sistemas
- 📚 Fins educacionais em ambientes controlados

**🚨 NÃO USE PARA ATIVIDADES ILEGAIS! 🚨**

## 🏗️ Arquitetura do Sistema

```
stealth-monitor/
├── common/                 # Código compartilhado
│   ├── crypto.h           # Criptografia XOR/AES
│   └── stealth.h          # Anti-detecção
├── windows/               # Implementação Windows
│   └── win_stealth_monitor.c
├── linux/                 # Implementação Linux
│   └── linux_stealth_monitor.c
├── macos/                 # Implementação macOS
│   └── macos_stealth_monitor.c
├── build/                 # Scripts de compilação
│   └── build_all.sh
└── research/              # Documentação científica
    └── detection_techniques.md
```

## 🔬 Técnicas Implementadas

### Windows
- ✅ Hooks de baixo nível (WH_KEYBOARD_LL)
- ✅ Syscalls diretos (NtSetInformationProcess)
- ✅ Persistência via Registry + Scheduled Tasks
- ✅ Exfiltração via DNS queries
- ✅ Anti-debugging (IsDebuggerPresent, timing checks)
- ✅ Process hiding techniques

### Linux
- ✅ X11 Record Extension hooks
- ✅ Daemonização completa
- ✅ Persistência via systemd
- ✅ Anti-debugging (/proc/self/status)
- ✅ Process name obfuscation
- ✅ Stealth file operations

### macOS
- ✅ Core Graphics Event Taps
- ✅ LaunchAgent persistence
- ✅ Accessibility API usage
- ✅ Anti-VM detection
- ✅ App bundle creation
- ✅ System integration

## 🛠️ Compilação

### Automática (Recomendado)
```bash
cd build
./build_all.sh
```

### Manual por Plataforma

#### Windows (Cross-compile no Linux)
```bash
cd windows
x86_64-w64-mingw32-gcc -O3 -s -mwindows -static \
    -o svchost.exe win_stealth_monitor.c \
    -luser32 -lkernel32 -ladvapi32 -lpsapi -lws2_32
```

#### Linux
```bash
cd linux
gcc -O3 -s -o system-monitor linux_stealth_monitor.c \
    -lX11 -lXtst -lXext -lpthread
```

#### macOS
```bash
cd macos
clang -O3 -framework CoreFoundation -framework CoreGraphics \
      -framework ApplicationServices -framework Carbon \
      -o SystemUpdate macos_stealth_monitor.c
```

## 🎯 Uso Científico

### Para Red Team (Técnicas Ofensivas)
```bash
# Estudar técnicas de evasão
# Analisar métodos de persistência
# Compreender exfiltração de dados
# Desenvolver payloads stealth
```

### Para Blue Team (Detecção e Resposta)
```bash
# Identificar IOCs (Indicators of Compromise)
# Desenvolver regras de detecção
# Testar ferramentas de monitoramento
# Criar contramedidas eficazes
```

## 🔍 Detecção (Blue Team)

### Indicadores de Compromisso
- Processos com nomes genéricos em locais não padrão
- Hooks de baixo nível em APIs críticas
- Queries DNS suspeitas com dados codificados
- Modificações em chaves de persistência
- Uso anômalo de APIs de acessibilidade

### Ferramentas de Detecção
- **Sysmon** com configuração específica
- **YARA rules** para detecção de padrões
- **EDR** com queries customizadas
- **Wireshark** para análise de tráfego
- **Volatility** para análise de memória

## 📊 Métricas de Pesquisa

### Eficácia de Evasão
- Taxa de detecção por diferentes AVs
- Tempo de permanência no sistema
- Sucesso de exfiltração de dados
- Eficácia de técnicas anti-debugging

### Capacidade de Detecção
- Tempo médio de detecção (MTTD)
- Taxa de falsos positivos/negativos
- Cobertura de técnicas MITRE ATT&CK
- Eficácia de resposta automatizada

## 🧪 Ambiente de Laboratório

### Configuração Recomendada
```bash
# VMs isoladas para cada SO
# Ferramentas de monitoramento instaladas
# Ambiente de rede controlado
# Logs centralizados
# Snapshots para reset rápido
```

### Cenários de Teste
1. **Deploy silencioso** - Testar instalação sem detecção
2. **Persistência** - Verificar sobrevivência a reinicializações
3. **Exfiltração** - Analisar métodos de saída de dados
4. **Detecção** - Identificar com ferramentas Blue Team
5. **Resposta** - Testar procedimentos de contenção

## 📚 Documentação Científica

- `research/detection_techniques.md` - Técnicas de detecção Blue Team
- `research/usage_guide.md` - Guia de uso científico
- `research/techniques.md` - Análise técnica detalhada

## 🎓 Objetivos Educacionais

### Conhecimentos Adquiridos
- APIs de baixo nível em diferentes SOs
- Técnicas de evasão e anti-detecção
- Métodos de persistência multiplataforma
- Exfiltração por canais não convencionais
- Desenvolvimento de contramedidas

### Habilidades Desenvolvidas
- Programação em C para sistemas
- Análise de malware e reverse engineering
- Desenvolvimento de regras de detecção
- Resposta a incidentes de segurança
- Pesquisa em segurança cibernética

## 🤝 Contribuições

Este é um projeto científico aberto para:
- Pesquisadores em segurança
- Estudantes de ciência da computação
- Profissionais de Blue/Red Team
- Desenvolvedores de ferramentas de segurança

## 📄 Licença

Projeto desenvolvido para fins educacionais e de pesquisa. Use com responsabilidade e dentro da legalidade.

---

**🔬 "A ciência da computação não é sobre computadores, assim como a astronomia não é sobre telescópios." - Edsger Dijkstra**