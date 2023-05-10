
/*
  This optimized 3DES implementation conforms to FIPS-46-3. 

3Des.h  
*/
#ifndef _DES_H
#define _DES_H

#ifndef uint8
#define uint8  unsigned char
#endif

#ifndef uint32
#define uint32 unsigned int
#endif

typedef struct
{
    uint32 esk[32];     /* DES encryption subkeys */
    uint32 dsk[32];     /* DES decryption subkeys */
}
des_context;

typedef struct
{
    uint32 esk[96];     /* Triple-DES encryption subkeys */
    uint32 dsk[96];     /* Triple-DES decryption subkeys */
}
des3_context;

int  des_set_key( des_context *ctx, uint8 key[8] );
void des_encrypt( des_context *ctx, uint8 input[8], uint8 output[8] );
void des_decrypt( des_context *ctx, uint8 input[8], uint8 output[8] );

int  des3_set_2keys( des3_context *ctx, uint8 key1[8], uint8 key2[8] );
int  des3_set_3keys( des3_context *ctx, uint8 key1[8], uint8 key2[8],
                                        uint8 key3[8] );

void des3_encrypt( des3_context *ctx, uint8 input[8], uint8 output[8] );
void des3_decrypt( des3_context *ctx, uint8 input[8], uint8 output[8] );

#endif /* 3Des.h */

/* Validation sets:
*
* Single-length key, single-length plaintext -
* Key	  : 0123 4567 89ab cdef
* Plain  : 0123 4567 89ab cde7
* Cipher : c957 4425 6a5e d31d
*
* Double-length key, single-length plaintext -
* Key	  : 0123 4567 89ab cdef fedc ba98 7654 3210
* Plain  : 0123 4567 89ab cde7
* Cipher : 7f1d 0a77 826b 8aff
*
* Double-length key, double-length plaintext -
* Key	  : 0123 4567 89ab cdef fedc ba98 7654 3210
* Plain  : 0123 4567 89ab cdef 0123 4567 89ab cdff
* Cipher : 27a0 8440 406a df60 278f 47cf 42d6 15d7
*
* Triple-length key, single-length plaintext -
* Key	  : 0123 4567 89ab cdef fedc ba98 7654 3210 89ab cdef 0123 4567
* Plain  : 0123 4567 89ab cde7
* Cipher : de0b 7c06 ae5e 0ed5
*
* Triple-length key, double-length plaintext -
* Key	  : 0123 4567 89ab cdef fedc ba98 7654 3210 89ab cdef 0123 4567
* Plain  : 0123 4567 89ab cdef 0123 4567 89ab cdff
* Cipher : ad0d 1b30 ac17 cf07 0ed1 1c63 81e4 4de5
*
* d3des V5.0a rwo 9208.07 18:44 Graven Imagery
**********************************************************************/
