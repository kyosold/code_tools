#ifndef __CTHASH_H_
#define __CTHASH_H_

/*
 * BKDRHash > APHash > DJBHash > JSHash > RSHash
 * > SDBMHash > PJWHash > ELFHash
 */


// A Simple Hash
unsigned int simple_hash(char *str);

// BKDR Hash
unsigned int BKDR_hash(char *str);

// AP Hash
unsigned int AP_hash(char *str);

// DJB Hash
unsigned int DJB_hash(char *str);

// JS Hash
unsigned int JS_hash(char *str);

// RS Hash
unsigned int RS_hash(char *str);

// SDBM Hash
unsigned int SDBM_hash(char *str);

// P. J. Weinberger Hash
unsigned int PJW_hash(char *str);

// ELF Hash
unsigned int ELF_hash(char *str);

// CRC Hash
unsigned int CRC_hash(char *str);

// CRC32 Hash
unsigned long CRC32_hash(unsigned int crc, const void *buf, size_t size);



#endif

