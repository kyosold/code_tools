#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/conf.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>



/*
 * @param in_str        需要md5的源字符串
 * @param in_str_len    源字符串的长度
 * @param out_str       保存md5后的字符串buf
 * @param out_str_size  out_str的空间长度
 *
 * @return 0 成功  1 失败
 */
int ct_md5(char *in_str, size_t in_str_len, char *out_str, size_t out_str_size)
{
    MD5_CTX         hash_ctx;  
    unsigned char   hash_ret[16];
    int             i;

    if (*in_str == '\0' || in_str_len == 0) {
        return 1;
    }

    // initialize a hash context
    if (MD5_Init(&hash_ctx) == 0) {
        return 1;
    }

    // update the input string to the hash context (you can update
    // more string to the hash context)
    if (MD5_Update(&hash_ctx, in_str, in_str_len) == 0) {
        return 1;
    }

    // compute the hash result
    if (MD5_Final(hash_ret, &hash_ctx) == 0) {
        return 1;
    }

    char *pout = out_str;
    int pout_len = 0;
    for (i=0; i<32; i++) {
        if (i%2 == 0) {
            pout_len += snprintf(pout+pout_len, out_str_size - pout_len, "%x", (hash_ret[i/2] >> 4) & 0xf);
        } else {
            pout_len += snprintf(pout+pout_len, out_str_size - pout_len, "%x", (hash_ret[i/2]) & 0xf);
        }
    } 
    
    return 0;
}



/*
 * sha1加密
 * @param in_str        需要sha1的源字符串
 * @param in_str_len    源字符串的长度
 * @param out_str       保存sha1后的字符串buf
 * @param out_str_size  out_str的空间长度
 *
 * @return 0 成功  1 失败
 */
int ct_sha1(char *in_str, size_t in_str_len, char *out_str, size_t out_str_size)
{
    SHA_CTX         hash_ctx;
    unsigned char   hash_ret[20];
    int             i;

    // initialize a hash context
    if (SHA1_Init(&hash_ctx) == 0) {
        return 1;
    }

    if (SHA1_Update(&hash_ctx, in_str, in_str_len) == 0) {
        return 1;
    }

    if (SHA1_Final(hash_ret, &hash_ctx) == 0) {
        return 1;
    }

    char *pout = out_str;
    int pout_len = 0;
    for (i=0; i<40; i++) {
        if (i%2 == 0) {
            pout_len += snprintf(pout+pout_len, out_str_size - pout_len, "%x", (hash_ret[i/2] >> 4) & 0xf);
        } else {
            pout_len += snprintf(pout+pout_len, out_str_size - pout_len, "%x", (hash_ret[i/2]) & 0xf);
        }
    } 

    return 0;
}


/*
 * hash_hmac加密
 * @param algo          使用hash的算法: 
 *                          md2, md4, md5, sha, sha1, dss, dss1, ecdsa, sha224,
 *                          sha256, sha384, sha512, ripemd160, whirlpool
 * @param data          被hash的数据
 * @param data_len      被hash的数据的长度
 * @param key           Shared secret key used for generating the HMAC variant of the message digest.
 * @param key_len       key的长度
 * @param out_data      保存hash后的字符串buf
 * @param out_data_size out_data的空间长度
 *
 * @return 0 成功  1 失败
 */
int ct_hash_hmac(char *algo, char *data, size_t data_len, char *key, size_t key_len, char *out_data, size_t out_data_size)
{
    HMAC_CTX        hash_ctx; 
    unsigned char   hash_ret[20];
    unsigned int    hash_ret_len = 20;
    int             i;
    const EVP_MD    *evp_md;

    if (algo == NULL) {
        evp_md = EVP_md_null();
    } else if (strcasecmp(algo, "md2") == 0) {
        evp_md = EVP_md2();
    } else if (strcasecmp(algo, "md4") == 0) {
        evp_md = EVP_md4();
    } else if (strcasecmp(algo, "md5") == 0) {
        evp_md = EVP_md5();
    } else if (strcasecmp(algo, "sha") == 0) {
        evp_md = EVP_sha();
    } else if (strcasecmp(algo, "sha1") == 0) {
        evp_md = EVP_sha1();
    } else if (strcasecmp(algo, "dss") == 0) {
        evp_md = EVP_dss();
    } else if (strcasecmp(algo, "dss1") == 0) {
        evp_md = EVP_dss1();
    } else if (strcasecmp(algo, "ecdsa") == 0) {
        evp_md = EVP_ecdsa();
    } else if (strcasecmp(algo, "sha224") == 0) {
        evp_md = EVP_sha224();
    } else if (strcasecmp(algo, "sha256") == 0) {
        evp_md = EVP_sha256();
    } else if (strcasecmp(algo, "sha384") == 0) {
        evp_md = EVP_sha384();
    } else if (strcasecmp(algo, "sha512") == 0) {
        evp_md = EVP_sha512();
    //} else if (strcasecmp(algo, "mdc2") == 0) {
    //    evp_md = EVP_mdc2();
    } else if (strcasecmp(algo, "ripemd160") == 0) {
        evp_md = EVP_ripemd160();
    } else if (strcasecmp(algo, "whirlpool") == 0) {
        evp_md = EVP_whirlpool();
    }


    HMAC_CTX_init(&hash_ctx);

    if (HMAC_Init_ex(&hash_ctx, key, key_len, evp_md, NULL) == 0) {
        return 1;
    }

    if (HMAC_Update(&hash_ctx, (unsigned char *)data, data_len) == 0) {
        return 1;
    }

    if (HMAC_Final(&hash_ctx, hash_ret, &hash_ret_len) == 0) {
        return 1;
    }

    HMAC_CTX_cleanup(&hash_ctx);
    
    char *pout = out_data;
    int pout_len = 0;
    for (i=0; i<hash_ret_len; i++) {
        pout_len += snprintf(pout+pout_len, out_data_size - pout_len, "%02x", (unsigned int)hash_ret[i]);
        
    }

    return 0;
}

