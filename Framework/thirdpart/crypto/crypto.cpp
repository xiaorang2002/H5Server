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


#include "public/GlobalFunc.h"


using namespace std;


namespace Landy
{


# define likely(x)      __builtin_expect(!!(x), 1)
# define unlikely(x)    __builtin_expect(!!(x), 0)


Crypto::Crypto()
{
}


void Crypto::generateRSAKey(string &pubKey, string &priKey)
{
    size_t pri_len;
    size_t pub_len;
    char *pri_key = NULL;
    char *pub_key = NULL;

    //生成密钥对
    RSA *keypair = RSA_generate_key(KEY_LENGTH, RSA_3, NULL, NULL);

    // 内存类型私钥
    BIO *pri = BIO_new(BIO_s_mem());
    BIO *pub = BIO_new(BIO_s_mem());

    // 生成的私钥 RSA_3
    int pri_ret = PEM_write_bio_RSAPrivateKey(pri, keypair, NULL, NULL, 0, NULL, NULL);
    int pub_ret = PEM_write_bio_RSAPublicKey(pub, keypair);

    printf("pri_ret:%d,pub_ret:%d\n",pri_ret,pub_ret);

    //得到缓存数据的数量
    pri_len = BIO_ctrl_pending(pri) ;//BIO_pending(pri); //
    pub_len = BIO_ctrl_pending(pub);//BIO_pending(pub); 

    // 分配内存区域
    pri_key = (char *)malloc(pri_len + 1);
    pub_key = (char *)malloc(pub_len + 1);

    // 从BIO接口中读出指定数量字节len的数据并存储到buf中
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

/*
 // 产生RSA密钥对
    RSA *rsaKey = RSA_generate_key(1024, 65537, NULL, NULL);
	int keySize = RSA_size(rsaKey);

	char fData[]="aaabbbccdskjkfd";
	char tData[128];
	
	int  flen = strlen(fData);
	//flen = 15
		
	int ret =  RSA_public_encrypt(flen, (unsigned char *)fData, (unsigned char *)tData, rsaKey,  RSA_PKCS1_PADDING);
	//ret = 128
		
	ret = RSA_private_decrypt(128, (unsigned char *)tData, (unsigned char *)fData, rsaKey, RSA_PKCS1_PADDING);
	//ret = 15
		

*/

int Crypto::rsaPublicEncrypt(const string &pubKey, const vector<unsigned char> &clearText, vector<unsigned char> &encryptedText)
{
    int ret = 0;
    try
    {
        RSA *rsa = NULL;
        BIO *keybio = BIO_new_mem_buf((unsigned char *)pubKey.c_str(), -1);
        rsa = PEM_read_bio_RSAPublicKey(keybio, &rsa, NULL, NULL);
        int len = RSA_size(rsa);
        encryptedText.resize(len);
        ret = RSA_public_encrypt(clearText.size(), &clearText[0], &encryptedText[0], rsa, RSA_PKCS1_PADDING);
        if (likely(ret > 0))
        {
            // std::cout << "=========公钥加密成功=========ret["<< ret <<"]"<< std::endl;
        }
        else
            encryptedText.resize(0);

        BIO_free_all(keybio);
        RSA_free(rsa);
    }
    catch(...)
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
        if (unlikely(ret <= 0)){
            out.resize(0);
        }
        else
            std::cout << "=========私钥解密成功=========ret["<< ret <<"]"<< std::endl;
        
        BIO_free_all(keybio);
        RSA_free(rsa);
    }
    catch(...)
    {
        out.resize(0);
        ret = 0;
    }
    return ret;
}
/*
rsa_len = RSA_size(privateRsa);
unsigned char *decryptMsg = (unsigned char *)malloc(rsa_len);
memset(decryptMsg, 0, rsa_len);

int mun =  RSA_private_decrypt(rsa_len, encryptMsg, decryptMsg, privateRsa, RSA_PKCS1_PADDING);

if ( mun < 0)
    printf("RSA private decrypt error\n");
else
    printf("RSA private decrypt %s\n", decryptMsg);
*/
 
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

string Crypto::generateAESKeyEx()
{
    boost::uuids::random_generator rgen;
    boost::uuids::uuid u = rgen();
    string tmp_uuid = boost::uuids::to_string(u);
    return tmp_uuid;
}


int Crypto::aesEncrypt(const string &key, const unsigned char *in, int inLen,vector<unsigned char> &out, internal_prev_header *internal_header)
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
    int outLen = ((in.size() + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
    outLen += 6;
    out.resize(outLen+6);
    //encrypt (iv will change)
    AES_cbc_encrypt(&in[0], &out[6], in.size(), &aes, iv, AES_ENCRYPT);
    *(int16_t*)&out[0] = outLen;
    *(int16_t*)&out[2] = 2;
    *(int16_t*)&out[4] = in.size();
    return outLen;
}

// 
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
    unsigned char iv[AES_BLOCK_SIZE];        // init vector
    // printf("\n--------------------------------------------\n");
    // printf("\n---- key in AES:%s\n",key.c_str());
    // printf("\n--------------------------------------------\n");

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

int Crypto::aes_cbc_decrypt(const string &key, unsigned char* in,int inLen,unsigned char* out)
{
    if(!in || key.empty() || !out) return 0;

    unsigned char iv[AES_BLOCK_SIZE];//加密的初始化向量
    for(int i=0; i < AES_BLOCK_SIZE; ++i)//iv一般设置为全0,可以设置其他，但是加密解密要一样就行
        iv[i]=0;

    AES_KEY aes;
    if(AES_set_decrypt_key((unsigned char*)key.c_str(), key.length() * 8, &aes) < 0)
    {
        return 0;
    }
 //   int len=strlen(in);
    int outLen = ((inLen + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
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

void Crypto::Padding(unsigned char *pSrc, int nSrcLen, int nPadLen)
{
    unsigned char ucPad = nPadLen - nSrcLen;
    for (int nID = nPadLen; nID > nSrcLen; --nID)
    {
        pSrc[nID - 1] = ucPad;
    }
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
        Padding(szInputData, nDataLen, nSetDataLen);


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
        // printf("%s end,%ld,%ld,%ld\n",__FUNCTION__,len,nInLen,nSetDataLen);
 
        return len;
    }
    catch (const std::exception &e)
    {
        printf("AES_ECB_Encrypt error,%s\n",e.what());
        return 0;
    }
}


// int Crypto::AES_ECB_Encrypt_EX(string const &key, unsigned char *szIn, int nDataLen,vector<unsigned char> &out)
// {
//     //设置加密密钥
//     AES_KEY aes;
//     if (AES_set_encrypt_key((unsigned char *)key.c_str(), key.length() * 8, &aes) < 0)
//         return 0;

//     try
//     {
//         //取长度
//         unsigned int nSetDataLen = 0;
//         if ((nDataLen % AES_BLOCK_SIZE) == 0)
//             nSetDataLen = nDataLen + AES_BLOCK_SIZE;
//         else
//             nSetDataLen = ((nDataLen / AES_BLOCK_SIZE) + 1) * AES_BLOCK_SIZE;

//         // 填充 
//         Padding((char *)szIn, nDataLen, nSetDataLen);

//         // 数据长度
//         out.resize(nSetDataLen);

//         int len = 0; 
//         while (len < nSetDataLen )
//         {
//             AES_ecb_encrypt(szIn + len, &out[len], &aes, AES_ENCRYPT);
//             len += AES_BLOCK_SIZE;
//         }
 
//         return len;
//     }
//     catch (const std::exception &e)
//     {
//         printf("AES_ECB_Encrypt error,%s\n",e.what());
//         return 0;
//     }
// }
/*
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
*/


}
