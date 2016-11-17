#ifndef _CTCRYPT_H_
#define _CTCRYPT_H_



/*
 * md5加密
 * @param in_str        需要md5的源字符串
 * @param in_str_len    源字符串的长度
 * @param out_str       保存md5后的字符串buf
 * @param out_str_size  out_str的空间长度
 *
 * @return 0 成功  1 失败
 */
int ct_md5(char *in_str, size_t in_str_len, char *out_str, size_t out_str_size);


/*
 * sha1加密
 * @param in_str        需要sha1的源字符串
 * @param in_str_len    源字符串的长度
 * @param out_str       保存sha1后的字符串buf
 * @param out_str_size  out_str的空间长度
 *
 * @return 0 成功  1 失败
 */
int ct_sha1(char *in_str, size_t in_str_len, char *out_str, size_t out_str_size);


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
int ct_hash_hmac(char *algo, char *data, size_t data_len, char *key, size_t key_len, char *out_data, size_t out_data_size);


#endif
