#ifndef CRYPTO_H
#define CRYPTO_H


#include <openssl/rsa.h>
#include <openssl/aes.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

#include <string>
#include <vector>

#include "public/gameDefine.h"

#define KEY_LENGTH  1024


using namespace std;


namespace Landy
{


class Crypto
{

public:
    Crypto();

public:
    static void generateRSAKey(string &pubKey, string &priKey);
//    static int rsaPublicNoEncrypt(const string &pubKey, const vector<unsigned char> &clearText, vector<unsigned char> &encryptedText);
    static int rsaPublicEncrypt(const string &pubKey, const vector<unsigned char> &clearText, vector<unsigned char> &encryptedText);
    static int rsaPrivateDecrypt(const std::string &priKey, const unsigned char *in, int inLen, vector<unsigned char> &out);
//    static int rsaPrivateEncrypt(const string &priKey, const string &clearText, string &encryptedText);
//    static int rsaPublicDecrypt(const std::string &pubKey, const std::string &cipherText, string &decryptedText);

    static string generateAESKey();
    static string generateAESKeyEx();
    static int aesEncrypt(const string &key, const unsigned char *in, int inLen, vector<unsigned char> &out, internal_prev_header *internal_header);
    static int aesEncrypt(const string &key, const vector<unsigned char> &in, vector<unsigned char> &out, internal_prev_header *internal_header);
    static int aesEncrypt(const string &key, const vector<unsigned char> &in, vector<unsigned char> &out);
//    static int aesDecrypt(unsigned char *key, int keyLen, const unsigned char *in, int inLen, unsigned char *out, int &outLen);
    static int aesDecrypt(const string &key, const unsigned char *in, int inLen, vector<unsigned char> &out);
    static int aesDecrypt(const string &key, const unsigned char *in, int inLen, unsigned char*out, int &outLen);

    static int aes_cbc_decrypt(const string &key, unsigned char* in,int inLen,unsigned char* out);

    static string BufferToHexString(unsigned char *buf, int len);
    static void HexStringToBuffer(const string &source, unsigned char* dest, int &len);

    static string MD5HashToString(const char* data, int size);
    static bool base64Encode(const string& input, string& output);
    static bool base64Decode(const string& input, string& output);

    static void ClearPadding(unsigned char *pSrc, int nSrcLen);
    static void Padding(unsigned char *pSrc, int nSrcLen, int nPadLen);
    static string AES_ECB_Decrypt(string const &key, string const &szIn);
    static int AES_ECB_Decrypt_EX(string const &key, unsigned char *szIn,int nEncryptDataLen,vector<unsigned char> &out);
    static int AES_ECB_Encrypt_EX(string const &key, unsigned char *szIn, int nInLen,vector<unsigned char> &out);
 
};

}

#endif // CRYPTO_H
