/**
 * @date 2009-11-30
 * @version 0.1
 * @author Frank Morgner <morgner@informatik.hu-berlin.de>
 * @author Dominik Oepen <oepen@informatik.hu-berlin.de>
 */

#include <string.h>
#include <openssl/bio.h>
#include <openssl/rand.h>
#include <openssl/asn1.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include "utils.h"

BUF_MEM *
hash(const EVP_MD * md, EVP_MD_CTX * ctx, ENGINE * impl, const BUF_MEM * in)
{
    BUF_MEM * out = NULL;
    EVP_MD_CTX * tmp_ctx = NULL;
    unsigned int tmp_len;

    if (!md || !in)
        goto err;

    if (ctx)
        tmp_ctx = ctx;
    else {
        tmp_ctx = EVP_MD_CTX_create();
        if (!tmp_ctx)
            goto err;
    }

    tmp_len = EVP_MD_size(md);
    out = BUF_MEM_create(tmp_len);
    if (!out || !EVP_DigestInit_ex(tmp_ctx, md, impl) ||
            !EVP_DigestUpdate(tmp_ctx, in->data, in->length) ||
            !EVP_DigestFinal_ex(tmp_ctx, (unsigned char *) out->data,
                &tmp_len))
        goto err;
    out->length = tmp_len;

    if (!ctx)
        EVP_MD_CTX_destroy(tmp_ctx);

    return out;

err:
    if (out)
        BUF_MEM_free(out);
    if (tmp_ctx && !ctx)
        EVP_MD_CTX_destroy(tmp_ctx);

    return NULL;
}

BUF_MEM *
cipher(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type, ENGINE *impl,
        unsigned char *key, unsigned char *iv, int enc, const BUF_MEM * in)
{
    BUF_MEM * out = NULL;
    EVP_CIPHER_CTX * tmp_ctx = NULL;
    int i;

    if (!in)
        goto err;

    if (ctx)
        tmp_ctx = ctx;
    else {
        tmp_ctx = EVP_CIPHER_CTX_new();
        if (!tmp_ctx)
            goto err;
        EVP_CIPHER_CTX_init(tmp_ctx);
    }

    if (tmp_ctx->flags & EVP_CIPH_NO_PADDING) {
        i = in->length;
        if (in->length % EVP_CIPHER_block_size(type) != 0) {
            printf("Input length is not a multiple of block length\n");
            goto err;
        }
    } else
        i = in->length + EVP_CIPHER_block_size(type);

    out = BUF_MEM_create(i);
    if (!out)
        goto err;

    /* get cipher */
    if (!EVP_CipherInit_ex(tmp_ctx, type, impl, key, iv, enc)
        || !EVP_CIPHER_CTX_set_padding(tmp_ctx, 0))
        goto err;

    if (!EVP_CipherUpdate(tmp_ctx, (unsigned char *) out->data, &i,
            (unsigned char *) in->data, in->length))
        goto err;
    out->length = i;

    if (!EVP_CipherFinal_ex(tmp_ctx, (unsigned char *) (out->data + out->length),
            &i))
            goto err;

    if (!(tmp_ctx->flags & EVP_CIPH_NO_PADDING))
        out->length += i;

    if (!ctx) {
        EVP_CIPHER_CTX_cleanup(tmp_ctx);
        EVP_CIPHER_CTX_free(tmp_ctx);
    }

    return out;

err:

    if (out)
        BUF_MEM_free(out);
    if (!ctx && tmp_ctx) {
        EVP_CIPHER_CTX_cleanup(tmp_ctx);
        EVP_CIPHER_CTX_free(tmp_ctx);
    }

    return NULL;
}


BUF_MEM *
add_iso_pad(const BUF_MEM * m, int block_size)
{
    BUF_MEM * out = NULL;
    int p_len;

    if (!m)
        goto err;

    /* calculate length of padded message */
    p_len = (m->length / block_size) * block_size + block_size;

    out = BUF_MEM_create(p_len);
    if (!out)
        goto err;

    memcpy(out->data, m->data, m->length);

    /* now add iso padding */
    memset(out->data + m->length, 0x80, 1);
    memset(out->data + m->length + 1, 0, p_len - m->length - 1);

    return out;

err:
    if (out)
        BUF_MEM_free(out);

    return NULL;
}

BUF_MEM *
encoded_ssc(const uint16_t ssc, const PACE_CTX *ctx)
{
    BUF_MEM * out = NULL;
    size_t len;

    if (!ctx)
        goto err;

    len = EVP_CIPHER_block_size(ctx->cipher);
    out = BUF_MEM_create(len);
    if (!out || len < sizeof ssc)
        goto err;

    /* Copy SSC to the end of buffer and fill the rest with 0 */
    memset(out->data, 0, len);
    memcpy(out->data + len - sizeof ssc, &ssc, sizeof ssc);

    return out;

err:
    if (out)
        BUF_MEM_free(out);

    return NULL;
}

BUF_MEM *
BUF_MEM_create(size_t len)
{
    BUF_MEM *out = BUF_MEM_new();
    if (!out)
        return NULL;

    if (!BUF_MEM_grow(out, len)) {
        BUF_MEM_free(out);
        return NULL;
    }

    return out;
}

BUF_MEM *
BUF_MEM_create_init(const void *buf, size_t len)
{
    BUF_MEM *out;

    if (!buf)
        return NULL;

    out = BUF_MEM_create(len);
    if (!out)
        return NULL;

    memcpy(out->data, buf, len);

    return out;
}

BUF_MEM *
point2buf(const EC_KEY * ecdh, BN_CTX * bn_ctx, const EC_POINT * ecp)
{
    size_t len;
    BUF_MEM * out;

    len = EC_POINT_point2oct(EC_KEY_get0_group(ecdh), ecp,
            EC_KEY_get_conv_form(ecdh), NULL, 0, bn_ctx);
    if (len == 0)
        return NULL;

    out = BUF_MEM_create(len);
    if (!out)
        return NULL;

    out->length = EC_POINT_point2oct(EC_KEY_get0_group(ecdh), ecp,
            EC_KEY_get_conv_form(ecdh), (unsigned char *) out->data, out->max,
            bn_ctx);

    return out;
}

BUF_MEM *
bn2buf(const BIGNUM *bn)
{
    BUF_MEM * out;

    if (!bn)
        return NULL;

    out = BUF_MEM_create(BN_num_bytes(bn));
    if (!out)
        return NULL;

    out->length = BN_bn2bin(bn, (unsigned char *) out->data);

    return out;
}

BUF_MEM *
BUF_MEM_dup(const BUF_MEM * in)
{
    BUF_MEM * out = NULL;

    if (!in)
        return NULL;

    out = BUF_MEM_create(in->length);
    if (!out)
        goto err;

    memcpy(out->data, in->data, in->length);
    out->max = in->max;

    return out;

err:
    if (out)
        BUF_MEM_free(out);

    return NULL;
}
