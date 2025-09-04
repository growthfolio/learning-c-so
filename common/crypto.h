#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdint.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
#elif __linux__
    #include <time.h>
    #include <sys/time.h>
#elif __APPLE__
    #include <sys/time.h>
#endif

// Estrutura para dados criptografados
typedef struct {
    uint8_t data[4096];
    uint32_t size;
    uint8_t key[32];
    uint64_t nonce;
} CryptoBuffer;

// XOR simples mas eficaz
static inline void xor_encrypt(uint8_t* data, uint32_t size, uint8_t* key, uint32_t key_size) {
    for (uint32_t i = 0; i < size; i++) {
        data[i] ^= key[i % key_size];
    }
}

// Geração de chave baseada em dados do sistema
static inline void generate_system_key(uint8_t* key, uint32_t size) {
    uint64_t seed = 0;
    
#ifdef _WIN32
    SYSTEMTIME st;
    GetSystemTime(&st);
    seed = st.wMilliseconds + st.wSecond + st.wMinute;
#elif __linux__
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    seed = ts.tv_nsec + ts.tv_sec;
#elif __APPLE__
    struct timeval tv;
    gettimeofday(&tv, NULL);
    seed = tv.tv_usec + tv.tv_sec;
#endif
    
    for (uint32_t i = 0; i < size; i++) {
        seed = seed * 1103515245 + 12345;
        key[i] = (uint8_t)(seed >> 16);
    }
}

// Ofuscação de strings
static inline void obfuscate_string(char* str, int len) {
    for (int i = 0; i < len; i++) {
        str[i] ^= 0xAA;
    }
}

#endif