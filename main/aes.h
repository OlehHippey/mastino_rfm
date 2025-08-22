/*
 * aes.h
 *
 *  Created on: 5 мая 2025 г.
 *      Author: ProgMen
 */

#ifndef MAIN_AES_H_
#define MAIN_AES_H_

#include <stdint.h>
typedef unsigned char uchar;
typedef unsigned long ulong;


//#define AES_ENCRYPT	1
//#define AES_DECRYPT	0

/* Because array size can't be a const in C, the following two are macros.
   Both sizes are in bytes. */
#define AES_MAXNR 14
#define AES_BLOCK_SIZE 16

#ifdef  __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER) && !defined(OPENSSL_SYS_WINCE)
# define SWAP(x) (_lrotl(x, 8) & 0x00ff00ff | _lrotr(x, 8) & 0xff00ff00)
# define GETU32(p) SWAP(*((u32 *)(p)))
# define PUTU32(ct, st) { *((u32 *)(ct)) = SWAP((st)); }
#else
# define GETU32(pt) (((u32)(pt)[0] << 24) ^ ((u32)(pt)[1] << 16) ^ ((u32)(pt)[2] <<  8) ^ ((u32)(pt)[3]))
# define PUTU32(ct, st) { (ct)[0] = (u8)((st) >> 24); (ct)[1] = (u8)((st) >> 16); (ct)[2] = (u8)((st) >>  8); (ct)[3] = (u8)(st); }
#endif

typedef unsigned long u32;
typedef unsigned short u16;
typedef unsigned char u8;

#define MAXKC   (256/32)
#define MAXKB   (256/8)
#define MAXNR   14

/* This controls loop-unrolling in aes_core.c */
#undef FULL_UNROLL

/* This should be a hidden type, but EVP requires that the size be known */
struct aes_key_st {
    unsigned long rd_key[4 *(AES_MAXNR + 1)];
    int rounds;
};
typedef struct aes_key_st AES_KEY;


void mastino_aes_set_key(uint8_t *key);
//void aes_encrypt(uchar *buf, int n);
void myaes_decrypt(uchar *buf, int n);
void myaes_encrypt(uchar *buf, int n);

#ifdef  __cplusplus
}
#endif
 



#endif /* MAIN_AES_H_ */
