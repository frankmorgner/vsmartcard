#include <opensc/opensc.h>
#include <openssl/evp.h>

#define SM_ISO_PADDING 0x01
#define SM_NO_PADDING  0x02

struct sm_ctx {
    u8 padding_indicator;
    u8 *key_mac;
    size_t key_mac_len;
    u8 *key_enc;
    size_t key_enc_len;

    const EVP_CIPHER *cipher;
    EVP_CIPHER_CTX *cipher_ctx;
    ENGINE *cipher_engine;
    unsigned char *iv;

    const EVP_MD * md;
    EVP_MD_CTX * md_ctx;
    ENGINE *md_engine;
};
