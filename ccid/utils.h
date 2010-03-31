#ifndef _UTIL_H_
#define _UTIL_H_
/**
 * @date 2009-11-30
 * @version 0.1
 * @author Frank Morgner <morgner@informatik.hu-berlin.de>
 * @author Dominik Oepen <oepen@informatik.hu-berlin.de>
 */

#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/cmac.h>
#include <openssl/ec.h>
#include <openssl/pace.h>

/**
 * @brief Wrapper for the OpenSSL hash functions.
 *
 * @param md
 * @param ctx (optional)
 * @param impl (optional)
 * @param in
 *
 * @return message digest or NULL if an error occurred
 */
BUF_MEM *
hash(const EVP_MD * md, EVP_MD_CTX * ctx, ENGINE * impl, const BUF_MEM * in);
/**
 * @brief Wrapper to the OpenSSL encryption functions.
 *
 * @param ctx (optional)
 * @param type
 * @param impl only evaluated if init is 1. (optional)
 * @param key only evaluated if init is 1.
 * @param iv only evaluated if init is 1. (optional)
 * @param enc only evaluated if init is 1.
 * @param in
 *
 * @return cipher of in or NULL if an error occurred
 *
 * @note automatic padding is disabled
 */
BUF_MEM *
cipher(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type, ENGINE *impl,
        unsigned char *key, unsigned char *iv, int enc, const BUF_MEM * in);
/**
 * @brief Padds a buffer using ISO/IEC 9797-1 padding method 2.
 *
 * @param m buffer to padd
 * @param block_size pad to this block size
 *
 * @return new buffer with the padded input or NULL if an error occurred
 */
BUF_MEM *
add_iso_pad(const BUF_MEM * m, int block_size);
/**
 * @brief Encodes a send sequence counter according to TR-3110 F.3
 *
 * @param ssc Send sequence counter to encode (Formatted in big endian)
 * @param ctx PACE_CTX object
 *
 * @return BUF_MEM object containing the send sequence counter or NULL if an error occurred
 *
 * @note This function is automatically called during PACE, normally you should not need to use it.
 */
BUF_MEM * encoded_ssc(const uint16_t ssc, const PACE_CTX *ctx);

#include <openssl/buffer.h>
#include <openssl/ec.h>

/**
 * @brief Creates a BUF_MEM object
 *
 * @param len required length of the buffer
 *
 * @return Initialized BUF_MEM object or NULL if an error occurred
 */
BUF_MEM *
BUF_MEM_create(size_t len);
/**
 * @brief Creates and initializes a BUF_MEM object
 *
 * @param buf Initial data
 * @param len Length of buf
 *
 * @return Initialized BUF_MEM object or NULL if an error occurred
 */
BUF_MEM *
BUF_MEM_create_init(const void *buf, size_t len);
/**
 * @brief converts an EC_POINT object to a BUF_MEM object
 *
 * @param ecdh EC_KEY object
 * @param bn_ctx object (optional)
 * @param ecp elliptic curve point to convert
 *
 * @return converted elliptic curve point or NULL if an error occurred
 */
BUF_MEM *
point2buf(const EC_KEY * ecdh, BN_CTX * bn_ctx, const EC_POINT * ecp);
/**
 * @brief converts an BIGNUM object to a BUF_MEM object
 *
 * @param bn bignumber to convert
 *
 * @return converted bignumber or NULL if an error occurred
 */
BUF_MEM *
bn2buf(const BIGNUM *bn);
/**
 * @brief duplicates a BUF_MEM structure
 *
 * @param in BUF_MEM to duplicate
 *
 * @return pointer to the new BUF_MEM or NULL in case of error
 */
BUF_MEM *
BUF_MEM_dup(const BUF_MEM * in);
#endif
