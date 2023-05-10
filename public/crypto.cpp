#include "crypto.h"

#include <string>
#include <cstring>
#include <iostream>
//#include <assert.h>
#include <vector>
#include <sstream>
#include <iomanip>

#include "boost/uuid/uuid.hpp"
#include "boost/uuid/uuid_generators.hpp"
#include "boost/uuid/uuid_io.hpp"

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <string>

#include "GlobalFunc.h"
#include "base64.h"

const char *g_Base64Char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
using namespace std;

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

Crypto::Crypto()
{
}

char *Crypto::Base64Encode(const unsigned char *bindata, char *base64, int binlength)
{
    int i, j;
    unsigned char current;

    for (i = 0, j = 0; i < binlength; i += 3)
    {
        current = (bindata[i] >> 2);
        current &= (unsigned char)0x3F;
        base64[j++] = g_Base64Char[(int)current];

        current = ((unsigned char)(bindata[i] << 4)) & ((unsigned char)0x30);
        if (i + 1 >= binlength)
        {
            base64[j++] = g_Base64Char[(int)current];
            base64[j++] = '=';
            base64[j++] = '=';
            break;
        }
        current |= ((unsigned char)(bindata[i + 1] >> 4)) & ((unsigned char)0x0F);
        base64[j++] = g_Base64Char[(int)current];

        current = ((unsigned char)(bindata[i + 1] << 2)) & ((unsigned char)0x3C);
        if (i + 2 >= binlength)
        {
            base64[j++] = g_Base64Char[(int)current];
            base64[j++] = '=';
            break;
        }
        current |= ((unsigned char)(bindata[i + 2] >> 6)) & ((unsigned char)0x03);
        base64[j++] = g_Base64Char[(int)current];

        current = ((unsigned char)bindata[i + 2]) & ((unsigned char)0x3F);
        base64[j++] = g_Base64Char[(int)current];
    }
    base64[j] = '\0';
    return base64;
}
int Crypto::Base64Decode(const char *base64, unsigned char *bindata)
{
    int i, j;
    unsigned char k;
    unsigned char temp[4];
    for (i = 0, j = 0; base64[i] != '\0'; i += 4)
    {
        memset(temp, 0xFF, sizeof(temp));
        for (k = 0; k < 64; k++)
        {
            if (g_Base64Char[k] == base64[i])
                temp[0] = k;
        }
        for (k = 0; k < 64; k++)
        {
            if (g_Base64Char[k] == base64[i + 1])
                temp[1] = k;
        }
        for (k = 0; k < 64; k++)
        {
            if (g_Base64Char[k] == base64[i + 2])
                temp[2] = k;
        }
        for (k = 0; k < 64; k++)
        {
            if (g_Base64Char[k] == base64[i + 3])
                temp[3] = k;
        }

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[0] << 2)) & 0xFC)) |
                       ((unsigned char)((unsigned char)(temp[1] >> 4) & 0x03));
        if (base64[i + 2] == '=')
            break;

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[1] << 4)) & 0xF0)) |
                       ((unsigned char)((unsigned char)(temp[2] >> 2) & 0x0F));
        if (base64[i + 3] == '=')
            break;

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[2] << 6)) & 0xF0)) |
                       ((unsigned char)(temp[3] & 0x3F));
    }
    return j;
}

void Crypto::CharToHex(const char *pSrc, unsigned char *pDest)
{
    int nSrcLen = 0;
    if (pSrc != 0)
    {
        nSrcLen = strlen(pSrc);
        memcpy(pDest, pSrc, nSrcLen);
    }
}

void Crypto::ClearPadding(unsigned char *pSrc, int nSrcLen)
{
    int nEnd = nSrcLen;
    int nStart = ((nSrcLen / AES_BLOCK_SIZE) - 1) * AES_BLOCK_SIZE;
    if (pSrc[nSrcLen - 1] != pSrc[nSrcLen - 2])
    {
        for (int i = 0; i < AES_BLOCK_SIZE; i++)
        {
            if (pSrc[nSrcLen - 1] == i)
            {
                pSrc[nSrcLen - 1] = '\0';
                break;
            }
        }
    }
    else
    {
        for (int i = 2; i <= nEnd - nStart; i++)
        {
            if (pSrc[nSrcLen - 1] != pSrc[nSrcLen - 1 - i])
            {
                pSrc[nSrcLen - i] = '\0';
                break;
            }
        }
    }
}

void Crypto::Padding(char *pSrc, int nSrcLen, int nPadLen)
{
    unsigned char ucPad = nPadLen - nSrcLen;
    for (int nID = nPadLen; nID > nSrcLen; --nID)
    {
        pSrc[nID - 1] = ucPad;
    }
}
 
void Crypto::AES_ECB_Encrypt(string const &key, unsigned char *szIn, string &str)
{
    AES_KEY aes; //private_
    int result = AES_set_encrypt_key((unsigned char *)key.c_str(), key.length() * 8, &aes); //设置加密密钥
    if (result < 0)
    {
        printf("private_AES_set_encrypt_key error\n");
        return;
    }

    try
    {
        //取长度
        unsigned int nSetDataLen = 0;
        int nDataLen = strlen((char *)szIn);
        if ((nDataLen % AES_BLOCK_SIZE) == 0)
        {
            nSetDataLen = nDataLen + AES_BLOCK_SIZE;
        }
        else
        {
            nSetDataLen = ((nDataLen / AES_BLOCK_SIZE) + 1) * AES_BLOCK_SIZE;
        }

        // 填充
        char szInputData[nSetDataLen + 1];
        memcpy(szInputData, szIn, nDataLen);
        Padding(szInputData, nDataLen, nSetDataLen);

        int len = 0;
        unsigned char szOutputData[32 * (nSetDataLen / AES_BLOCK_SIZE)];
        memset(szOutputData, 0, sizeof(szOutputData));

        /*循环加密，每次只能加密AES_BLOCK_SIZE长度的数据*/
        while (len < nSetDataLen )
        {
            AES_ecb_encrypt((unsigned char *)szInputData + len, (unsigned char *)szOutputData + len, &aes, AES_ENCRYPT);
            len += AES_BLOCK_SIZE;
        }

        // base64加密
        str = base64_encode((unsigned char *)szOutputData, nSetDataLen);
    }
    catch (const std::exception &e)
    {
        printf("AES_ECB_Encrypt error,%s\n",e.what());
    }
}
string Crypto::AES_ECB_Decrypt(string const &key, string const &szIn)
{
    //SetDecryptKey(0);
    AES_KEY aes;
    //设置加密密钥private_
    int keyleng = AES_set_decrypt_key((unsigned char *)key.c_str(),key.length() * 8, &aes);
    if (keyleng < 0)
    {
        printf("AES_set_decrypt_key error,len:%d\n", keyleng);
        return szIn;
    }

    try
    {
        // 数据长度
        int nDataLen = szIn.length(); // strlen((char *)szIn);
        unsigned char szEncryptData[nDataLen];
        memset(szEncryptData, 0, sizeof(szEncryptData));

        // base64解密
        std::string strSrc;
        strSrc = base64_decode(szIn);
        const char *pSrc = strSrc.data();
        memcpy(szEncryptData, pSrc, nDataLen);

        int len = 0, length = strSrc.length();
        unsigned char decrypt_result[length];
        memset(decrypt_result, 0, length);

        while (len < length)
        {
            AES_decrypt(&szEncryptData[len], decrypt_result + len, &aes);
            len += AES_BLOCK_SIZE;
        }

        //清除填充数据
        ClearPadding((unsigned char *)decrypt_result, length);

        // 取结束长度
        decrypt_result[len] = '\0';
        string overStr((char *)decrypt_result);
        return overStr;
    }
    catch (const std::exception &e)
    {
        printf("AES_ECB_Decrypt error,%s\n",e.what());
        return szIn;
    }
}
//string Crypto::ECBDecrypt(std::string const& data, std::string const& key)
string Crypto::AES_ECB_Decrypt(string const &key, unsigned char const *szIn)
{
    //SetDecryptKey(0);
    AES_KEY aes;
    //设置加密密钥
    int keyleng = AES_set_decrypt_key((unsigned char *)key.c_str(), key.length() * 8, &aes);
    if (keyleng < 0)
    {
        printf("AES_set_decrypt_key error,len:%d\n", key.length());
        return "";
    }

    unsigned char szIvec[AES_BLOCK_SIZE];
    int nDataLen = strlen((char *)szIn);
    unsigned char szEncryptData[nDataLen + 1]; //= (unsigned char *)calloc(nDataLen + 1, sizeof(char));
    memset(szEncryptData, 0, sizeof(szEncryptData));
    // base64解密
    int length = Base64Decode((char *)szIn, szEncryptData);
    int len = 0;
    unsigned char *decrypt_result = (unsigned char *)malloc(length);
    memset(decrypt_result, 0, length);

    while (len < length)
    {
        AES_decrypt(&szEncryptData[len], decrypt_result + len, &aes);
        len += AES_BLOCK_SIZE;
    }
    //清除填充数据
    ClearPadding((unsigned char *)decrypt_result, length);
    // 取结束长度
    decrypt_result[len] = '\0';
    string overStr((char *)decrypt_result);
    if (decrypt_result)
    {
        free(decrypt_result);
        decrypt_result = NULL;
    }
    return overStr;
}
void Crypto::generateRSAKey(string &pubKey, string &priKey)
{
    size_t pri_len;
    size_t pub_len;
    char *pri_key = NULL;
    char *pub_key = NULL;

    RSA *keypair = RSA_generate_key(KEY_LENGTH, RSA_3, NULL, NULL);

    BIO *pri = BIO_new(BIO_s_mem());
    BIO *pub = BIO_new(BIO_s_mem());

    PEM_write_bio_RSAPrivateKey(pri, keypair, NULL, NULL, 0, NULL, NULL);
    PEM_write_bio_RSAPublicKey(pub, keypair);

    pri_len = BIO_pending(pri);
    pub_len = BIO_pending(pub);

    pri_key = (char *)malloc(pri_len + 1);
    pub_key = (char *)malloc(pub_len + 1);

    BIO_read(pri, pri_key, pri_len);
    BIO_read(pub, pub_key, pub_len);

    pri_key[pri_len] = '\0';
    pub_key[pub_len] = '\0';

    pubKey.assign(pub_key, pub_len);
    priKey.assign(pri_key, pri_len);

    //    FILE *pubFile = fopen(PUB_KEY_FILE, "w");
    //    if (pubFile == NULL)
    //    {
    //        assert(false);
    //        return;
    //    }
    //    fputs(pub_key, pubFile);
    //    fclose(pubFile);

    //    FILE *priFile = fopen(PRI_KEY_FILE, "w");
    //    if (priFile == NULL)
    //    {
    //        assert(false);
    //        return;
    //    }
    //    fputs(pri_key, priFile);
    //    fclose(priFile);

    RSA_free(keypair);
    BIO_free_all(pub);
    BIO_free_all(pri);

    free(pri_key);
    free(pub_key);
}

//int Crypto::rsaPublicNoEncrypt(const string &pubKey, const vector<unsigned char> &clearText, vector<unsigned char> &encryptedText)
//{
//    int16_t len = clearText.size() + 6;
//    encryptedText.resize(len);
//    *(int16_t*)&encryptedText[0] = len;
//    *(int16_t*)&encryptedText[2] = 0;
//    *(int16_t*)&encryptedText[4] = clearText.size();

//    memcpy(&encryptedText[6], &clearText[0], clearText.size());

//    return len;
//}

int Crypto::rsaPublicEncrypt(const string &pubKey, const vector<unsigned char> &clearText, vector<unsigned char> &encryptedText)
{
    int ret = 0;
    try
    {
        RSA *rsa = NULL;
        BIO *keybio = BIO_new_mem_buf((unsigned char *)pubKey.c_str(), -1);
        rsa = PEM_read_bio_RSAPublicKey(keybio, &rsa, NULL, NULL);
        int len = RSA_size(rsa);
        len += 6;
        encryptedText.resize(len);
        ret = RSA_public_encrypt(clearText.size(), &clearText[0], &encryptedText[6], rsa, RSA_PKCS1_PADDING);
        if (likely(ret > 0))
        {
            *(int16_t *)&encryptedText[0] = len;
            *(int16_t *)&encryptedText[2] = 1;
            *(int16_t *)&encryptedText[4] = clearText.size();
        }
        else
            encryptedText.resize(0);
        BIO_free_all(keybio);
        RSA_free(rsa);

        ret = ret > 0 ? ret + 6 : 0;
    }
    catch (...)
    {
        encryptedText.resize(0);
        ret = 0;
    }
    return ret;
}

int Crypto::rsaPrivateDecrypt(const std::string &priKey, const unsigned char *in, int inLen, vector<unsigned char> &out)
{
    int ret = 0;

    try
    {
        RSA *rsa = RSA_new();
        BIO *keybio;
        keybio = BIO_new_mem_buf((unsigned char *)priKey.c_str(), -1);
        rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
        int len = RSA_size(rsa);
        out.resize(len);
        ret = RSA_private_decrypt(inLen, in, &out[0], rsa, RSA_PKCS1_PADDING);
        if (unlikely(ret <= 0))
            out.resize(0);
        BIO_free_all(keybio);
        RSA_free(rsa);
    }
    catch (...)
    {
        out.resize(0);
        ret = 0;
    }
    return ret;
}

//int Crypto::rsaPrivateEncrypt(const string &priKey, const string &clearText, string &encryptedText)
//{
//    RSA *rsa = NULL;
//    BIO *keybio = BIO_new_mem_buf((unsigned char *)priKey.c_str(), -1);

//    rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);

//    int len = RSA_size(rsa);
//    char *encryptedBuf = (char *)malloc(len + 1);
//    memset(encryptedBuf, 0, len + 1);

//    int ret = RSA_private_encrypt(clearText.length(), (const unsigned char*)clearText.c_str(), (unsigned char*)encryptedBuf, rsa, RSA_PKCS1_PADDING);
//    if (ret >= 0)
//        encryptedText.assign(encryptedBuf, ret);
//    else
//        encryptedText = "";

//    free(encryptedBuf);
//    BIO_free_all(keybio);
//    RSA_free(rsa);

//    return ret;
//}

//int Crypto::rsaPublicDecrypt(const std::string &pubKey, const std::string &cipherText, string &decryptedText)
//{
//    RSA *rsa = RSA_new();
//    BIO *keybio;
//    keybio = BIO_new_mem_buf((unsigned char *)pubKey.c_str(), -1);

//    rsa = PEM_read_bio_RSAPublicKey(keybio, &rsa, NULL, NULL);

//    int len = RSA_size(rsa);
//    char *decryptedBuf = (char *)malloc(len + 1);
//    memset(decryptedBuf, 0, len + 1);

//    int ret = RSA_public_decrypt(cipherText.length(), (const unsigned char*)cipherText.c_str(), (unsigned char*)decryptedBuf, rsa, RSA_PKCS1_PADDING);
//    if (ret >= 0)
//        decryptedText.assign(decryptedBuf, ret);
//    else
//        decryptedText = "";

//    free(decryptedBuf);
//    BIO_free_all(keybio);
//    RSA_free(rsa);

//    return ret;
//}

//void Crypto::generateRSAKey(string &pubKey, string &priKey)

string Crypto::generateAESKey()
{
    //    unsigned char key[AES_BLOCK_SIZE];
    //    memset(key, 0, sizeof(key));
    boost::uuids::random_generator rgen;
    boost::uuids::uuid u = rgen();
    string key;
    key.assign(u.begin(), u.end());
    return key;
}

string Crypto::generateUuid()
{
    boost::uuids::random_generator rgen;
    boost::uuids::uuid _uuid = rgen();
    return to_string(_uuid);
}

int Crypto::aesEncrypt(const string &key, const unsigned char *in, int inLen,
                       vector<unsigned char> &out, internal_prev_header *internal_header)
{
    AES_KEY aes;
    //    unsigned char key[AES_BLOCK_SIZE];        // AES_BLOCK_SIZE = 16
    unsigned char iv[AES_BLOCK_SIZE]; // init vector

    memset(iv, 0, sizeof(iv));
    if (unlikely(AES_set_encrypt_key((unsigned char *)key.c_str(), key.length() * 8, &aes) < 0))
    {
        cout << "Unable to set encryption key in AES" << endl;
        return -1;
    }
    int header_len = sizeof(internal_prev_header) + sizeof(Header);
    int outLen = ((inLen + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
    outLen += header_len;
    out.resize(outLen);

    internal_header->len = outLen;
    GlobalFunc::SetCheckSum(internal_header);

    Header header;
    header.len = outLen;
    header.encType = 2;
    header.realSize = inLen;

    //encrypt (iv will change)
    AES_cbc_encrypt(in, &out[header_len], inLen, &aes, iv, AES_ENCRYPT);
    memcpy(&out[0], internal_header, sizeof(internal_prev_header));
    memcpy(&out[0 + sizeof(internal_prev_header)], &header, sizeof(Header));
    return outLen;
}

int Crypto::aesEncrypt(const string &key, const vector<unsigned char> &in, vector<unsigned char> &out, internal_prev_header *internal_header)
{
    AES_KEY aes;
    //    unsigned char key[AES_BLOCK_SIZE];        // AES_BLOCK_SIZE = 16
    unsigned char iv[AES_BLOCK_SIZE]; // init vector

    memset(iv, 0, sizeof(iv));
    if (unlikely(AES_set_encrypt_key((unsigned char *)key.c_str(), key.size() * 8, &aes) < 0))
    {
        cout << "Unable to set encryption key in AES" << endl;
        return -1;
    }

    int header_len = sizeof(internal_prev_header) + sizeof(Header);
    int outLen = ((in.size() + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
    outLen += header_len;
    out.resize(outLen);

    internal_header->len = outLen;
    GlobalFunc::SetCheckSum(internal_header);

    Header header;
    header.len = outLen - sizeof(internal_prev_header);
    header.encType = 2;
    header.realSize = in.size();

    //encrypt (iv will change)
    AES_cbc_encrypt(&in[0], &out[header_len], in.size(), &aes, iv, AES_ENCRYPT);
    memcpy(&out[0], internal_header, sizeof(internal_prev_header));
    memcpy(&out[0 + sizeof(internal_prev_header)], &header, sizeof(Header));
    return outLen;
}

int Crypto::aesEncrypt(const string &key, const vector<unsigned char> &in, vector<unsigned char> &out)
{
    AES_KEY aes;
    //    unsigned char key[AES_BLOCK_SIZE];        // AES_BLOCK_SIZE = 16
    unsigned char iv[AES_BLOCK_SIZE]; // init vector

    memset(iv, 0, sizeof(iv));
    if (unlikely(AES_set_encrypt_key((unsigned char *)key.c_str(), key.length() * 8, &aes) < 0))
    {
        cout << "Unable to set encryption key in AES" << endl;
        return -1;
    }
    int outLen = 0;
    //    int outLen = ((in.size() + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
    //    outLen += 6;
    //    out.resize(outLen+6);
    //encrypt (iv will change)
    AES_cbc_encrypt(&in[0], &out[6], in.size(), &aes, iv, AES_ENCRYPT);
    //    *(int16_t*)&out[0] = outLen;
    //    *(int16_t*)&out[2] = 2;
    //    *(int16_t*)&out[4] = in.size();
    return outLen;
}

int Crypto::aesDecrypt(const string &key, const unsigned char *in, int inLen, vector<unsigned char> &out)
{
    AES_KEY aes;
    //    unsigned char key[AES_BLOCK_SIZE];        // AES_BLOCK_SIZE = 16
    unsigned char iv[AES_BLOCK_SIZE]; // init vector

    memset(iv, 0, sizeof(iv));
    if (unlikely(AES_set_decrypt_key((unsigned char *)key.c_str(), key.length() * 8, &aes) < 0))
    {
        cout << "Unable to set encryption key in AES" << endl;
        return -1;
    }
    //encrypt (iv will change)
    int outLen = ((inLen + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
    out.resize(outLen);
    AES_cbc_encrypt(in, &out[0], inLen, &aes, iv, AES_DECRYPT);
    return outLen;
}

int Crypto::aesDecrypt(const string &key, const unsigned char *in, int inLen, unsigned char *out, int &outLen)
{
    AES_KEY aes;
    //    unsigned char key[AES_BLOCK_SIZE];        // AES_BLOCK_SIZE = 16
    unsigned char iv[AES_BLOCK_SIZE]; // init vector

    memset(iv, 0, sizeof(iv));
    if (unlikely(AES_set_decrypt_key((unsigned char *)key.c_str(), key.length() * 8, &aes) < 0))
    {
        cout << "Unable to set encryption key in AES" << endl;
        return -1;
    }
    //encrypt (iv will change)
    outLen = ((inLen + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
    AES_cbc_encrypt(in, out, inLen, &aes, iv, AES_DECRYPT);
    return outLen;
}

string Crypto::MD5HashToString(const char *data, int size)
{
    string hash = "";
    do
    {
        // check the input buffer now.
        if ((!data) || (!size))
            break;
        unsigned char out[16] = {0};
        MD5_CTX ctx = {0};
        MD5_Init(&ctx);
        MD5_Update(&ctx, data, size);
        MD5_Final(out, &ctx);
        hash = BufferToHexString(out, 16);
    } while (0);
    //Cleanup:
    return (hash);
}

string Crypto::BufferToHexString(unsigned char *buf, int len)
{
    ostringstream oss;
    oss << hex << uppercase << setfill('0');
    for (int i = 0; i < len; ++i)
    {
        oss << setw(2) << (unsigned int)(buf[i]);
    }
    return oss.str();
}

void Crypto::HexStringToBuffer(const string &source, unsigned char *dest, int &len)
{
    unsigned char highByte, lowByte;

    for (size_t i = 0; i < source.length(); i += 2)
    {
        highByte = toupper(source[i]);
        lowByte = toupper(source[i + 1]);

        if (highByte > 0x39)
            highByte -= 0x37;
        else
            highByte -= 0x30;

        if (lowByte > 0x39)
            lowByte -= 0x37;
        else
            lowByte -= 0x30;

        dest[i / 2] = (highByte << 4) | lowByte;
    }
    len = source.length() / 2;
    return;
}

bool Crypto::base64Encode(const string &input, string &output)
{
    typedef boost::archive::iterators::base64_from_binary<boost::archive::iterators::transform_width<string::const_iterator, 6, 8>> Base64EncodeIterator;

    stringstream result;
    copy(Base64EncodeIterator(input.begin()), Base64EncodeIterator(input.end()), ostream_iterator<char>(result));
    size_t equal_count = (3 - input.length() % 3) % 3;
    for (size_t i = 0; i < equal_count; i++)
    {
        result.put('=');
    }
    output = result.str();
    return !output.empty();
}

bool Crypto::base64Decode(const string &input, string &output)
{
    typedef boost::archive::iterators::transform_width<boost::archive::iterators::binary_from_base64<string::const_iterator>, 8, 6> Base64DecodeIterator;
    stringstream result;
    try
    {
        copy(Base64DecodeIterator(input.begin()), Base64DecodeIterator(input.end()), ostream_iterator<char>(result));
    }
    catch (...)
    {
        return false;
    }
    //Cleanup:
    std::string data = result.str();
    int datasize = data.size();

    output = result.str();
    return !output.empty();
}

//void AES_ecb_encrypt(const unsigned char *in, unsigned char *out,const AES_KEY *key, const int enc);

int Crypto::aesEncryptEx(const string &key, unsigned char *in, unsigned char *out)
{
    AES_KEY aes;
    int result = AES_set_encrypt_key((unsigned char *)key.c_str(), 128, &aes); //设置加密密钥
    if (result < 0)                                                            //如果设置失败,退出
    {
        return -1;
    }

    AES_ecb_encrypt(in, out, &aes, AES_ENCRYPT);
    return 0;
}

int Crypto::aesDecryptEx(const string &key, const unsigned char *in, int inLen, unsigned char *out, int &outLen)
{
    AES_KEY aes;
    //    unsigned char key[AES_BLOCK_SIZE];        // AES_BLOCK_SIZE = 16
    unsigned char iv[AES_BLOCK_SIZE]; // init vector

    memset(iv, 0, sizeof(iv));
    if (unlikely(AES_set_decrypt_key((unsigned char *)key.c_str(), key.length() * 8, &aes) < 0))
    {
        cout << "Unable to set encryption key in AES" << endl;
        return -1;
    }
    //encrypt (iv will change)
    outLen = ((inLen + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
    AES_ecb_encrypt(in, out, &aes, AES_DECRYPT);
    return outLen;
}


/*
int EVP_EncryptInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type,ENGINE *impl, unsigned char *key, unsigned char *iv);
该函数采用ENGINE参数impl的算法来设置并初始化加密结构体。其中，参数ctx必须在调用本函数之前已经进行了初始化。
参数type通常通过函数类型来提供参数，如EVP_des_cbc函数的形式，即对称加密算法的类型。
如果参数impl为NULL，那么就会使用缺省的实现算法。
参数key是用来加密的对称密钥，iv参数是初始化向量（如果需要的话）。
在算法中真正使用的密钥长度和初始化密钥长度是根据算法来决定的。
在调用该函数进行初始化的时候，除了参数type之外，所有其它参数可以设置为NULL，留到以后调用其它函数的时候再提供，这时候参数type就可设置为NULL。
在缺省的加密参数不合适的时候，可以这样处理。操作成功返回1，否则返回0。
*/
/*
int EVP_EncryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl, unsigned char *in, int inl);
该函数执行对数据的加密。该函数加密从参数in输入的长度为inl的数据，并将加密好的数据写入到参数out里面去。
可以通过反复调用该函数来处理一个连续的数据块。
写入到out的数据数量是由已经加密的数据的对齐关系决定的，理论上来说，从0到(inl+cipher_block_size-1)的任何一个数字都有可能（单位是字节），
所以输出的参数out要有足够的空间存储数据。
写入到out中的实际数据长度保存在outl参数中。操作成功返回1，否则返回0。
*/
/*
int EVP_EncryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl);
该函数处理最后（Final）的一段数据。在函数在padding功能打开的时候（缺省）才有效，这时候，它将剩余的最后的所有数据进行加密处理。
该算法使用标志的块padding方式（AKA PKCS padding）。加密后的数据写入到参数out里面，参数out的长度至少应该能够一个加密块。
写入的数据长度信息输入到outl参数里面。该函数调用后，表示所有数据都加密完了，不应该再调用EVP_EncryptUpdate函数。
如果没有设置padding功能，那么本函数不会加密任何数据，如果还有剩余的数据，那么就会返回错误信息，也就是说，这时候数据总长度不是块长度的整数倍。
操作成功返回1，否则返回0。
PKCS padding标准是这样定义的，在被加密的数据后面加上n个值为n的字节，使得加密后的数据长度为加密块长度的整数倍。
无论在什么情况下，都是要加上padding的，也就是说，如果被加密的数据已经是块长度的整数倍，那么这时候n就应该等于块长度。
比如，如果块长度是9，要加密的数据长度是11，那么5个值为5的字节就应该增加在数据的后面。
*/
//DES ECB pkcs5padding 加密
bool Crypto::DES_ECB_Encrypt(unsigned char *key, unsigned char *iv, string & strInput, string & strOutput)
{
    EVP_CIPHER_CTX ctx;                 //EVP算法上下文
    unsigned char sCipher[4096];        //密文缓冲区
    int nCipher = 0,nTmp = 0,rv = 0;    //密文长度
    char sBase64[4096];

    //初始化密码算法结构体
    EVP_CIPHER_CTX_init(&ctx);
    
    //设置算法和密钥
    rv = EVP_EncryptInit_ex(&ctx, EVP_des_ecb(), NULL, key, iv);
    if (rv != 1)
        return true;

    //数据加密
    rv = EVP_EncryptUpdate(&ctx, sCipher, &nCipher,( unsigned char *)strInput.c_str(), strInput.size());
    if (rv != 1)
        return true;

    //结束数据加密，把剩余数据输出
    rv = EVP_EncryptFinal_ex(&ctx, sCipher + nCipher, &nTmp);
    if (rv != 1)
        return true;

    // 密文长度
    nCipher = nCipher + nTmp;

    //可视化输出base64
    Base64Encode(sCipher, sBase64, nCipher);
    strOutput = string((char *)sBase64);

    // 现场清理
    EVP_CIPHER_CTX_cleanup(&ctx);

    return false;
}

bool Crypto::DES_ECB_EncryptEx(unsigned char *key, unsigned char *iv, string & strInput, string & strOutput)//  char *strOutput)
{
    /*
    DES_cblock key;
    //随机密钥
    DES_random_key(&key);
 
    DES_key_schedule schedule;
    //转换成schedule
    DES_set_key_checked(&key, &schedule); 
 
    const_DES_cblock input = "hehehe";
    DES_cblock output;
 
    printf("cleartext: %s\n", input);
 
    //加密
    DES_ecb_encrypt(&input, &output, &schedule, DES_ENCRYPT);
    printf("Encrypted!\n");
 
    printf("ciphertext: ");
    int i;
    for (i = 0; i < sizeof(input); i++)
         printf("%02x", output[i]);
    printf("\n");
 
    //解密
    DES_ecb_encrypt(&output, &input, &schedule, DES_DECRYPT);
    printf("Decrypted!\n");
    printf("cleartext:%s\n", input);
    */

    return true;
}


int Crypto::AES_ECB_Decrypt_EX(string const &key, unsigned char *szIn,int nEncryptDataLen,vector<unsigned char> &out)
{
    AES_KEY aes;
    if (AES_set_decrypt_key((unsigned char *)key.c_str(),key.length() * 8, &aes) < 0) 
        return 0;

    try
    {
        int len = 0;

        // 数据长度
        out.resize(nEncryptDataLen);

        while (len < nEncryptDataLen)
        {
            AES_decrypt(szIn + len, &out[len] , &aes); // &out[0]+ len
            len += AES_BLOCK_SIZE;
        }

        //清除填充数据
        ClearPadding(&out[0], nEncryptDataLen);
        return len;
    }
    catch (const std::exception &e)
    {
        return 0;
    }
}


int Crypto::AES_ECB_Encrypt_EX(string const &key, unsigned char *szIn, int nInLen,vector<unsigned char> &out) //nDataLen
{
    //设置加密密钥
    AES_KEY aes;
    if (AES_set_encrypt_key((unsigned char *)key.c_str(), key.length() * 8, &aes) < 0)
        return 0;

    try
    {
        //取长度
        unsigned int nSetDataLen = 0;
        int nDataLen = nInLen;//strlen((char *)szIn);
        if ((nDataLen % AES_BLOCK_SIZE) == 0)
        {
            nSetDataLen = nDataLen + AES_BLOCK_SIZE;
        }
        else
        {
            nSetDataLen = ((nDataLen / AES_BLOCK_SIZE) + 1) * AES_BLOCK_SIZE;
        }

        // 填充
        unsigned char szInputData[nSetDataLen + 1];
        memcpy(szInputData, szIn, nDataLen);
        Padding((char *)szInputData, nDataLen, nSetDataLen);


        // 数据长度
        out.resize(nSetDataLen);

        int len = 0;
        while (len < nSetDataLen )
        {
            AES_ecb_encrypt(szInputData + len, &out[0] + len, &aes, AES_ENCRYPT);
            len += AES_BLOCK_SIZE;
        }
       
        /*
        //取长度
        unsigned int nSetDataLen = 0;
        int64_t nDataLen = ninLen;
        if ((nDataLen % AES_BLOCK_SIZE) == 0)
            nSetDataLen = nDataLen + AES_BLOCK_SIZE;
        else
            nSetDataLen = ((nDataLen / AES_BLOCK_SIZE) + 1) * AES_BLOCK_SIZE;
        // 填充 
        Padding((char *)szIn, nDataLen, nSetDataLen);
        // 数据长度
        out.resize(nSetDataLen);
        int len = 0; 
        while (len < nSetDataLen )
        {
            AES_ecb_encrypt(szIn + len, &out[0] + len, &aes, AES_ENCRYPT);
            len += AES_BLOCK_SIZE;
        }
        */
        printf("%s end,%ld,%ld,%ld\n",__FUNCTION__,len,nInLen,nSetDataLen);
 
        return len;
    }
    catch (const std::exception &e)
    {
        printf("AES_ECB_Encrypt error,%s\n",e.what());
        return 0;
    }
}