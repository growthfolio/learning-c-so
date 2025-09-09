# 🎯 Estrutura do Projeto Científico Multi-Plataforma

## Arquitetura do Sistema
```
stealth-monitor/
├── common/                 # Código compartilhado
│   ├── crypto.h           # Criptografia XOR/AES
│   ├── network.h          # Exfiltração de dados
│   ├── stealth.h          # Anti-detecção
│   └── logger.h           # Sistema de logs
├── windows/               # Implementação Windows
│   ├── win_hooks.c        # Hooks nativos Win32
│   ├── win_stealth.c      # Técnicas Windows
│   └── win_persistence.c  # Persistência Windows
├── linux/                 # Implementação Linux
│   ├── linux_hooks.c     # X11/Wayland hooks
│   ├── linux_stealth.c   # Técnicas Linux
│   └── linux_persistence.c # Persistência Linux
├── macos/                 # Implementação macOS
│   ├── macos_hooks.c      # Core Graphics/Events
│   ├── macos_stealth.c    # Técnicas macOS
│   └── macos_persistence.c # Persistência macOS
├── build/                 # Scripts de compilação
│   ├── build_windows.bat
│   ├── build_linux.sh
│   └── build_macos.sh
└── research/              # Documentação científica
    ├── techniques.md      # Técnicas implementadas
    ├── detection.md       # Métodos de detecção
    └── countermeasures.md # Contramedidas
```

## Objetivos Científicos
1. Estudar APIs de baixo nível em diferentes SOs
2. Implementar técnicas de evasão multiplataforma
3. Desenvolver métodos de detecção (Blue Team)
4. Criar contramedidas eficazes
5. Documentar comportamentos do sistema

## Metodologia
- Código nativo sem bibliotecas externas
- Técnicas de ofuscação avançadas
- Exfiltração por canais não convencionais
- Anti-debugging e anti-VM
- Persistência stealth