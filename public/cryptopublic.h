#ifndef CRYPTO_PUBLIC_H
#define CRYPTO_PUBLIC_H

#include <openssl/rsa.h>
#include <openssl/aes.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/des.h>

// #include <cryptopp/rosrngsa.h>
// #include <cryptopp/hex.h>
// #include <cryptopp/rsa.h>
// #include <cryptopp/files.h>

// #include <cryptopp/default.h>
// #include <cryptopp/filters.h>
// #include <cryptopp/osrng.h>


#include <string>
#include <vector>

#include "gameDefine.h"

#define KEY_LENGTH  1024

using namespace std;
// using namespace CryptoPP;

class CryptoPublic
{
public:
    CryptoPublic();

public:
    static void generateRSAKey(string &pubKey, string &priKey);
//    static int rsaPublicNoEncrypt(const string &pubKey, const vector<unsigned char> &clearText, vector<unsigned char> &encryptedText);
    static int rsaPublicEncrypt(const string &pubKey, const vector<unsigned char> &clearText, vector<unsigned char> &encryptedText);
    static int rsaPrivateDecrypt(const std::string &priKey, const unsigned char *in, int inLen, vector<unsigned char> &out);
//    static int rsaPrivateEncrypt(const string &priKey, const string &clearText, string &encryptedText);
//    static int rsaPublicDecrypt(const std::string &pubKey, const std::string &cipherText, string &decryptedText);
    static string generateUuid();
    static string generateAESKey();
    static int aesEncrypt(const string &key, const unsigned char *in, int inLen, vector<unsigned char> &out, internal_prev_header *internal_header);
    static int aesEncrypt(const string &key, const vector<unsigned char> &in, vector<unsigned char> &out, internal_prev_header *internal_header);
    static int aesEncrypt(const string &key, const vector<unsigned char> &in, vector<unsigned char> &out);
//    static int aesDecrypt(unsigned char *key, int keyLen, const unsigned char *in, int inLen, unsigned char *out, int &outLen);
    static int aesDecrypt(const string &key, const unsigned char *in, int inLen, vector<unsigned char> &out);
    static int aesDecrypt(const string &key, const unsigned char *in, int inLen, unsigned char*out, int &outLen);

    static string BufferToHexString(unsigned char *buf, int len);
    static void HexStringToBuffer(const string &source, unsigned char* dest, int &len);

    static string MD5HashToString(const char* data, int size);
    static bool base64Encode(const string& input, string& output);
    static bool base64Decode(const string& input, string& output);

    static int aesEncryptEx(const string &key, unsigned char * in, unsigned char * out);
    static int aesDecryptEx(const string &key, const unsigned char *in, int inLen, unsigned char*out, int &outLen);

    static void AES_ECB_Encrypt(string const &key,unsigned char *szIn,string &str);
    static string AES_ECB_Decrypt(string const &key, unsigned char const* szIn);
    static string AES_ECB_Decrypt(string const &key, string const &szIn);

    static char *Base64Encode(const unsigned char * bindata, char * base64, int binlength);
    static int  Base64Decode(const char * base64, unsigned char * bindata);

    static void CharToHex(const char* pSrc, unsigned char* pDest);
    static void Padding(char* pSrc, int nSrcLen, int nPadLen);
    static void ClearPadding(unsigned char *pSrc, int nSrcLen);
    // 
    static bool DES_ECB_Encrypt(unsigned char *key, unsigned char *iv,  string & strInput, string & strOutput);
    static bool DES_ECB_EncryptEx(unsigned char *key, unsigned char *iv, string & strInput, string & strOutput);

    static int AES_ECB_Decrypt_EX(string const &key, unsigned char *szIn,int nEncryptDataLen,vector<unsigned char> &out);
    static int AES_ECB_Encrypt_EX(string const &key, unsigned char *szIn, int nInLen,vector<unsigned char> &out) ;
};


#endif // CRYPTO_PUBLIC_H
