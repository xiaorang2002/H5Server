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

const char * g_Base64Char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
using namespace std;


namespace Landy
{


# define likely(x)      __builtin_expect(!!(x), 1)
# define unlikely(x)    __builtin_expect(!!(x), 0)


Crypto::Crypto()
{
}

char *Crypto::Base64Encode(const unsigned char * bindata, char * base64, int binlength)
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
int Crypto::Base64Decode(const char * base64, unsigned char * bindata)
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

void Crypto::CharToHex(const char* pSrc, unsigned char* pDest)
{
    int nSrcLen = 0;
    if (pSrc != 0)
    {
        nSrcLen = strlen(pSrc);
        memcpy(pDest, pSrc, nSrcLen);
    }
}

void Crypto::ClearPadding(unsigned char *&pSrc, int nSrcLen)
{
    int nEnd = nSrcLen;
    int nStart = ((nSrcLen / 16) - 1) * 16;
    if (pSrc[nSrcLen - 1] != pSrc[nSrcLen - 2])
    {
        for (int i = 0; i < 16; i++)
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


void Crypto::Padding(char* pSrc, int nSrcLen, int nPadLen)
{
    unsigned char ucPad = nPadLen - nSrcLen;
    for (int nID = nPadLen; nID > nSrcLen; --nID)
    {
        pSrc[nID - 1] = ucPad;
    }
}
bool Crypto::ECBEncrypt(string &key,unsigned char *szIn,  string & strOut)
{
    AES_KEY aes;
    int result=private_AES_set_encrypt_key((unsigned char *)key.c_str(),128,&aes);//设置加密密钥
    if(result<0)//如果设置失败,退出
    {
        return "";
    }
    unsigned char szIvec[AES_BLOCK_SIZE];
    //memcpy(szIvec, m_szIvec, AES_BLOCK_SIZE);
    int nSetDataLen = 0;
    int nDataLen = strlen((char *)szIn);
    if ((nDataLen%AES_BLOCK_SIZE) == 0)
    {
        nSetDataLen = nDataLen + 16;
    }
    else
    {
        nSetDataLen = ((nDataLen / AES_BLOCK_SIZE) + 1) * AES_BLOCK_SIZE;
    }
    char *szInputData = (char *)calloc(nSetDataLen + 1, sizeof(char));
    memcpy(szInputData, szIn, nDataLen);
    Padding(szInputData, nDataLen, nSetDataLen);

    unsigned char *szOutputData = (unsigned char *)calloc(32 * (nSetDataLen / AES_BLOCK_SIZE), sizeof(unsigned char));


    int nDataLenNew = 32 * (nSetDataLen / AES_BLOCK_SIZE) *2;


    int lengthInput = ((strlen((char *)szIn)+AES_BLOCK_SIZE-1)/AES_BLOCK_SIZE)*AES_BLOCK_SIZE;  //对齐分组
    int len = 0;
      /*循环加密，每次只能加密AES_BLOCK_SIZE长度的数据*/
      while(len < lengthInput) {
        AES_ecb_encrypt((unsigned char *)szInputData+len, (unsigned char *)szOutputData+len, &aes, AES_ENCRYPT);
        len += AES_BLOCK_SIZE;
      }
    //AES_ecb_encrypt((unsigned char *)szInputData, (unsigned char *)szOutputData, &aes, AES_ENCRYPT);
    //AES_encrypt((unsigned char *)szInputData, (unsigned char *)szOutputData, &aes);
    //AES_cbc_encrypt((unsigned char *)szInputData, (unsigned char *)szOutputData, nSetDataLen + 1, &aes, szIvec, AES_ENCRYPT);

 
    char * szOutTmp;
    Base64Encode((unsigned char *)szOutputData,szOutTmp, lengthInput);//nSetDataLen);
    strOut=(char *) szOutTmp;
    if (szInputData)
    {
        free(szInputData);
        szInputData = NULL;
    }

    if (szOutputData)
    {
        free(szOutputData);
        szOutputData = NULL;
    }

    return true;
}

//string Crypto::ECBDecrypt(std::string const& data, std::string const& key)
string Crypto::ECBDecrypt(string const& key, unsigned char const* szIn)
{
    //SetDecryptKey(0);
    AES_KEY aes;
#if 0
    int keyleng=AES_set_decrypt_key((unsigned char *)key.c_str(), key.length()*8, &aes);
    if(keyleng<0)
    {
        printf("AES_set_decrypt_key error\n");
        return "";
    }
    unsigned char szIvec[AES_BLOCK_SIZE];
    //memcpy(szIvec, m_szIvec, AES_BLOCK_SIZE);
    //int nDataLen = strlen((char *)szIn);
    //unsigned char *EncryptData = (unsigned char *)calloc(nDataLen + 1, sizeof(char));
    //int length =Base64Decode((char *)szIn, EncryptData);
    int len=0;
    unsigned char *decrypt_result = (unsigned char *)malloc(data.length());
    memset(decrypt_result, 0, data.length());
	while (len < data.length()) {
		AES_decrypt((const unsigned char*)(&data[len]), decrypt_result + len, &aes);
		len += AES_BLOCK_SIZE;
	}
    std::string overStr="";
    overStr=(char *)decrypt_result;
    //if (EncryptData)
    //{
    //    free(EncryptData);
    //    EncryptData = NULL;
    //}
    //printf("\n\n-------------------ECBDecrypt-------------\n%*.s\n\n", decrypt_result, data.length());
    //ClearPadding(decrypt_result, length);
    //free(decrypt_result);
    //std::string overStr(decrypt_result, decrypt_result + length);
#else
	int keyleng = AES_set_decrypt_key((unsigned char*)key.c_str(), key.length() * 8, &aes);
	if (keyleng < 0)
	{
        printf("AES_set_decrypt_key error\n");
		return "";
	}
	unsigned char szIvec[AES_BLOCK_SIZE];
	//memcpy(szIvec, m_szIvec, AES_BLOCK_SIZE);
	int nDataLen = strlen((char*)szIn);
	unsigned char* EncryptData = (unsigned char*)calloc(nDataLen + 1, sizeof(char));
	int length = Base64Decode((char*)szIn, EncryptData);
	int len = 0;
	unsigned char* decrypt_result = (unsigned char*)malloc(length);
	while (len < length) {
		AES_decrypt(EncryptData + len, decrypt_result + len, &aes);
		len += AES_BLOCK_SIZE;
	}
    ClearPadding(decrypt_result, length);
    decrypt_result[len] = '\0';
	string overStr((char*)decrypt_result);
	if (EncryptData)
	{
		free(EncryptData);
		EncryptData = NULL;
	}
    return overStr;
#endif
    
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
            *(int16_t*)&encryptedText[0] = len;
            *(int16_t*)&encryptedText[2] = 1;
            *(int16_t*)&encryptedText[4] = clearText.size();
        }else
            encryptedText.resize(0);
        BIO_free_all(keybio);
        RSA_free(rsa);

        ret = ret>0?ret+6:0;
    }catch(...)
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

    }catch(...)
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

int Crypto::aesEncrypt(const string &key, const unsigned char *in, int inLen,
                       vector<unsigned char> &out, internal_prev_header *internal_header)
{
    AES_KEY aes;
//    unsigned char key[AES_BLOCK_SIZE];        // AES_BLOCK_SIZE = 16
    unsigned char iv[AES_BLOCK_SIZE];        // init vector

    memset(iv, 0, sizeof(iv));
    if (unlikely(AES_set_encrypt_key((unsigned char *)key.c_str(), key.length()*8, &aes) < 0))
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
    memcpy(&out[0+sizeof(internal_prev_header)], &header, sizeof(Header));
    return outLen;
}

int Crypto::aesEncrypt(const string &key, const vector<unsigned char> &in, vector<unsigned char> &out, internal_prev_header *internal_header)
{
    AES_KEY aes;
//    unsigned char key[AES_BLOCK_SIZE];        // AES_BLOCK_SIZE = 16
    unsigned char iv[AES_BLOCK_SIZE];        // init vector

    memset(iv, 0, sizeof(iv));
    if (unlikely(AES_set_encrypt_key((unsigned char *)key.c_str(), key.size()*8, &aes) < 0))
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
    memcpy(&out[0+sizeof(internal_prev_header)], &header, sizeof(Header));
    return outLen;
}

int Crypto::aesEncrypt(const string &key, const vector<unsigned char> &in, vector<unsigned char> &out)
{
    AES_KEY aes;
//    unsigned char key[AES_BLOCK_SIZE];        // AES_BLOCK_SIZE = 16
    unsigned char iv[AES_BLOCK_SIZE];        // init vector

    memset(iv, 0, sizeof(iv));
    if (unlikely(AES_set_encrypt_key((unsigned char *)key.c_str(), key.length()*8, &aes) < 0))
    {
        cout << "Unable to set encryption key in AES" << endl;
        return -1;
    }
    int outLen =0;
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
    unsigned char iv[AES_BLOCK_SIZE];        // init vector

    memset(iv, 0, sizeof(iv));
    if (unlikely(AES_set_decrypt_key((unsigned char *)key.c_str(), key.length()*8, &aes) < 0))
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

int Crypto::aesDecrypt(const string &key, const unsigned char *in, int inLen, unsigned char*out, int &outLen)
{
    AES_KEY aes;
//    unsigned char key[AES_BLOCK_SIZE];        // AES_BLOCK_SIZE = 16
    unsigned char iv[AES_BLOCK_SIZE];        // init vector

    memset(iv, 0, sizeof(iv));
    if (unlikely(AES_set_decrypt_key((unsigned char *)key.c_str(), key.length()*8, &aes) < 0))
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
        if ((!data)||(!size))
            break;
        unsigned char out[16]={0};
        MD5_CTX ctx={0};
        MD5_Init(&ctx);
        MD5_Update(&ctx,data,size);
        MD5_Final(out,&ctx);
        hash = BufferToHexString(out,16);
    }while (0);
//Cleanup:
    return (hash);
}


string Crypto::BufferToHexString(unsigned char *buf, int len)
{
    ostringstream oss;
    oss<<hex<<uppercase<<setfill('0');
    for(int i = 0; i< len; ++i)
    {
        oss<<setw(2)<<(unsigned int)(buf[i]);
    }
    return oss.str();
}

void Crypto::HexStringToBuffer(const string &source, unsigned char* dest, int &len)
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


bool Crypto::base64Encode(const string& input, string& output)
{
    typedef boost::archive::iterators::base64_from_binary<boost::archive::iterators::transform_width<string::const_iterator, 6, 8> > Base64EncodeIterator;

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

bool Crypto::base64Decode(const string& input, string& output)
{
    typedef boost::archive::iterators::transform_width<boost::archive::iterators::binary_from_base64<string::const_iterator>, 8, 6> Base64DecodeIterator;
    stringstream result;
    try
    {
        copy(Base64DecodeIterator(input.begin()) , Base64DecodeIterator(input.end()), ostream_iterator<char>(result));
    }catch(...)
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

int Crypto::aesEncryptEx(const string &key, unsigned char * in, unsigned char * out)
{
    AES_KEY aes;
    int result=AES_set_encrypt_key((unsigned char *)key.c_str(),128,&aes);//设置加密密钥
     if(result<0)//如果设置失败,退出
     {
         return -1;
     }

    AES_ecb_encrypt(in,out, &aes, AES_ENCRYPT);
    return 0;
}

int Crypto::aesDecryptEx(const string &key, const unsigned char *in, int inLen, unsigned char*out, int &outLen)
{
    AES_KEY aes;
//    unsigned char key[AES_BLOCK_SIZE];        // AES_BLOCK_SIZE = 16
    unsigned char iv[AES_BLOCK_SIZE];        // init vector

    memset(iv, 0, sizeof(iv));
    if (unlikely(AES_set_decrypt_key((unsigned char *)key.c_str(), key.length()*8, &aes) < 0))
    {
        cout << "Unable to set encryption key in AES" << endl;
        return -1;
    }
    //encrypt (iv will change)
    outLen = ((inLen + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
    AES_ecb_encrypt(in, out, &aes,AES_DECRYPT);
    return outLen;
}

}
