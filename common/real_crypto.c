#include "crypto.h"
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

// Real AES-256-GCM encryption
int aes_encrypt_real(const unsigned char* plaintext, int plaintext_len,
                     const unsigned char* key, const unsigned char* iv,
                     unsigned char* ciphertext, unsigned char* tag) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;
    
    // Create and initialize context
    if (!(ctx = EVP_CIPHER_CTX_new())) return -1;
    
    // Initialize encryption operation with AES-256-GCM
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    // Set IV length
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 16, NULL)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    // Initialize key and IV
    if (1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    // Encrypt plaintext
    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    ciphertext_len = len;
    
    // Finalize encryption
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    ciphertext_len += len;
    
    // Get authentication tag
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}

// Real key derivation from system entropy
void generate_system_key_real(unsigned char* key, size_t key_len) {
    // Use OpenSSL's secure random number generator
    if (RAND_bytes(key, key_len) != 1) {
        // Fallback to system entropy
        FILE* urandom = fopen("/dev/urandom", "rb");
        if (urandom) {
            fread(key, 1, key_len, urandom);
            fclose(urandom);
        } else {
            // Last resort - time-based seed (not cryptographically secure)
            srand(time(NULL));
            for (size_t i = 0; i < key_len; i++) {
                key[i] = rand() % 256;
            }
        }
    }
}

// Real SHA-256 hashing
void sha256_hash_real(const unsigned char* data, size_t data_len, unsigned char* hash) {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data, data_len);
    SHA256_Final(hash, &sha256);
}

// Real secure memory wiping
void secure_memset_real(void* ptr, int value, size_t num) {
    volatile unsigned char* p = (volatile unsigned char*)ptr;
    while (num--) {
        *p++ = value;
    }
}

// Real polymorphic code generation
void generate_polymorphic_variant_real(unsigned char* code, size_t code_len, int generation) {
    unsigned char key[32];
    generate_system_key_real(key, 32);
    
    // Apply different mutations based on generation
    switch (generation % 4) {
        case 0: // XOR mutation
            for (size_t i = 0; i < code_len; i++) {
                code[i] ^= key[i % 32];
            }
            break;
            
        case 1: // Substitution cipher
            for (size_t i = 0; i < code_len; i++) {
                code[i] = ((code[i] + key[i % 32]) % 256);
            }
            break;
            
        case 2: // Bit rotation
            for (size_t i = 0; i < code_len; i++) {
                unsigned char byte = code[i];
                int shift = key[i % 32] % 8;
                code[i] = (byte << shift) | (byte >> (8 - shift));
            }
            break;
            
        case 3: // Combined mutation
            for (size_t i = 0; i < code_len; i++) {
                code[i] ^= key[i % 32];
                code[i] = ((code[i] + key[(i + 1) % 32]) % 256);
            }
            break;
    }
}