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

#include "gameDefine.h"

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



    static bool ECBEncrypt(string &key,unsigned char *szIn,  string & strOut);
    //static string ECBDecrypt(std::string const& data, std::string const& key);
    static string ECBDecrypt(string const& key, unsigned char const* szIn);
    static char *Base64Encode(const unsigned char * bindata, char * base64, int binlength);
    static int  Base64Decode(const char * base64, unsigned char * bindata);

    static void CharToHex(const char* pSrc, unsigned char* pDest);
    static void Padding(char* pSrc, int nSrcLen, int nPadLen);
    static void ClearPadding(unsigned char *&pSrc, int nSrcLen);
};

}

#endif // CRYPTO_H
