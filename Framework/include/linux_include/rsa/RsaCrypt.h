/*
	RSA���ܶԳ���Կ
	RSA�ܼ��ܵ����ݳ�����KEY�����������,����1024λ�������Լ���64�ֽ�.

	���ģʽʹ��: RSA_PKCS1_PADDINGʱ�����ĳ���ֻ���� 1024/8-11=117�ֽ�.

	����Ƚ��鷳����,117�ֽڵ�����,���ܳ�����128�ֽڵ�����,

	����128�ֽڵ�����,�����ȴֻ��117�ֽڵ�����,���ԱȽ��Ѹ�.


History:
	V2.0 2015-06-19 11:25 ������˽Կ���ܴ������,��Կ���ܴ������,����ע����������Ҫ̫��,��ΪRSA�����ٶȺ���!.

*/


#ifndef __RSACRYPT_HEADER__
#define __RSACRYPT_HEADER__


#include <file/FileHelp.h>
#include <openssl/rsa.h>


#define RSACRYPT_KEYSIZE (1024) // KEY size.
#define RSACRYPT_KEYBYTE (RSACRYPT_KEYSIZE>>3)
#define RSACRYPT_DATAMAX (RSACRYPT_KEYBYTE-11)

#ifndef EFS_SIGN
#define EFS_SIGN	(0x32784B50)
#endif//EFS_SIGN

#define ERROR_INSUFFICIENT_BUFFER 122L

enum enumRSAError
{
    RSAERROR_OK         =  0, // OK
    RSAERROR_UNKNOWN    =  1, // unknown.
    RSAERROR_INVALIDKEY =  2, // invalid key.
    RSAERROR_RSAGENKFAIL=  3, // gen RSA key fail
    RSAERROR_CREATEFILE =  4, // create file error.
    RSAERROR_OPENFILE   =  5, // open file error.
    RSAERROR_READFILE   =  6, // read file failed.
    RSAERROR_WRITEFILE  =  7, // write file failed.
    RSAERROR_NOKEYFILE  =  8, // open key file failed.
    RSAERROR_ENUMDIRFAIL=  9, // enum special dir fail
    RSAERROR_NOFILEEXIST= 10, // no file has exist now.
    RSAERROR_INVALIDFILE= 11, // invalidate data file.
    RSAERROR_PARSEERROR = 12, // parse x file error.
    RSAERROR_INVALIDDATA= 13, // invalid data or decrypt failed.
    RSAERROR_NOMEMTOUSE = 14, // no memory to use.
    RSAERROR_OPENSSLFAIL= 15, // open SSL failed.
    RSAERROR_INVALIDHEAD= 16, // header invalid.
    RSAERROR_CHKSUMFAIL = 20, // check sum fail.
};


class CRsaCrypt
{
public:
    CRsaCrypt()
    {
    }

public:
    LONG GenKEY(const char* pubfile="p.x",const char* pivfile="v.x")
    {
        int  ret,i;
        LONG nStatus=-1;
        int  bits=RSACRYPT_KEYSIZE;
        unsigned long e=RSA_F4;
        BIGNUM *bne;
        RSA *r=NULL;
        BYTE bufPub[4096]={0};
        BYTE bufPrv[4096]={0};

        do 
        {
            // =BN_new();
            bne=BN_new();
            ret=BN_set_word(bne,e);
            r  =RSA_new();
            // r  =RSA_generate_key(1024,0x10001,NULL,NULL);
            ret=RSA_generate_key_ex(r,bits,bne,NULL);
            i=RSA_size(r);

            if (1 != ret)
            {
                nStatus=RSAERROR_RSAGENKFAIL;
                break;
            }
            
            // try to read the private.
            i=i2d_RSAPublicKey(r,NULL);
            if (i)
            {
                LPBYTE lpsz = bufPub;
                // try to get public key.
                i=i2d_RSAPublicKey(r,&lpsz);
                // save back the file now.
                nStatus = SaveFile(pubfile,bufPub,i);
                if (RSAERROR_OK != nStatus)
                {
                    break;
                }
            }

            i=i2d_RSAPrivateKey(r,NULL);
            if (i)
            {
                LPBYTE lpsz= bufPrv;
                // try to get public key.
                i2d_RSAPrivateKey(r,&lpsz);
                // save back the file now.
                nStatus = SaveFile(pivfile,bufPrv,i);
                if (RSAERROR_OK != nStatus)
                {
                    break;
                }
            }
        } while (0);

    //Cleanup:
        RSA_free(r);
        return (nStatus);
    }

	// ��Կ����.
    LONG Pubenc(LPCTSTR  pubfile,
                unsigned char* lpszData,int nDatalen,
                unsigned char* lpszOut,int* pnOutlen)
    {
        CFileHelp help;
        LONG nStatus=RSAERROR_UNKNOWN;

        do 
        {
            if (!help.Open(pubfile)) {
                nStatus=RSAERROR_NOKEYFILE;
                break;
            }

            // try to call the real public key encrypt now.
            nStatus=Pubenc(help.GetBuffer(),help.GetSize(),
                lpszData,nDatalen,
                lpszOut,pnOutlen);

        } while (0);

    //Cleanup:
        return (nStatus);
    }

	// ��Կ����.
    LONG Pubenc(const unsigned char* lpPubkey,int nPublen,
                unsigned char* lpszData,int nDatalen,
                unsigned char* lpszOut,int* pnOutlen)
    {
        int i;
        RSA* r=NULL;
        LONG nStatus=-1;
        do 
        {
            if (pnOutlen){
               *pnOutlen=0;
            }

            r = RSA_new();
            const unsigned char* p = lpPubkey;
            r = d2i_RSAPublicKey(&r,&p,nPublen);
            i = RSA_public_encrypt(nDatalen,lpszData,lpszOut,r,RSA_PKCS1_PADDING);
            if (i>0)
            {
                nStatus = S_OK;
                if (pnOutlen) {
                   *pnOutlen=i;
                }
            }
        } while (0);

    //Cleanup:
        RSA_free(r);
        return (nStatus);
    }

	// ˽Կ����.
    LONG Prvdec(LPCTSTR  prvfile,
                unsigned char* lpszEnc,int nEnclen,
                unsigned char* lpszOut,int* pnOutlen)
    {
        CFileHelp help;
        LONG nStatus=RSAERROR_UNKNOWN;
        
        do 
        {
            if (!help.Open(prvfile)) {
                nStatus=RSAERROR_NOKEYFILE;
                break;
            }
            
            // try to call the real public key encrypt now.
            nStatus=Prvdec(help.GetBuffer(),help.GetSize(),
                lpszEnc,nEnclen,
                lpszOut,pnOutlen);
            
        } while (0);
        
    //Cleanup:
        return (nStatus);

    }

	// ˽Կ����.
    // for test,on real version,private key decrypt must implement on hardware.
    LONG Prvdec(const unsigned char* lpPrvkey,int nPrvlen,
                unsigned char* lpszEnc,int nEnclen,
                unsigned char* lpszOut,int* pnOutlen)
    {
        int i;
        RSA* r=NULL;
        LONG nStatus=-1;
        do 
        {
            r = RSA_new();
            const unsigned char* p = lpPrvkey;
            r = d2i_RSAPrivateKey(&r,&p,nPrvlen);
            i = RSA_private_decrypt(nEnclen,lpszEnc,lpszOut,r,RSA_PKCS1_PADDING);
            if (i>0)
            {
                nStatus = S_OK;
                if (pnOutlen) {
                   *pnOutlen=i;
                }
            }
        } while (0);
        
    //Cleanup:
        RSA_free(r);
        return (nStatus);
    }

    // ˽Կ����.
	// �Լ�������Ҫ���������ڴ��С,���Ĵ�С117ʱ,���Ĵ�С128�ֽ�.
	LONG PrivKEncHuge(const unsigned char* lpPrvkey,int nPrvlen,
					unsigned char* lpszPlain,int nPlainLen,
					unsigned char* lpszEnc,int* pnOutlen)
	{
		LONG nStatus = -1;
		int   enclen = 0;
		do 
		{
			DWORD dwBlockSize=((RSACRYPT_KEYSIZE>>3)-11);				// RSA˽Կ����,���ĳ��ȱ�������Կ���ȼ�ȥ11�ֽ�.
			DWORD dwBlockCount=(nPlainLen/dwBlockSize);					// ���Ա����ܵĿ���.
			DWORD dwAllocSize=((dwBlockCount+1)*(RSACRYPT_KEYSIZE>>3));	// ��Ҫ�����ڴ��С.
			DWORD dwBlockDiva=(nPlainLen-(dwBlockCount*dwBlockSize));	// ʣ�಻�ܼ����ڲ�.
			// allocate buffer.
			if (!lpszEnc) {
				*pnOutlen = dwAllocSize;
				nStatus = ERROR_INSUFFICIENT_BUFFER;
				break;
			}


			*pnOutlen = 0;
			LPBYTE lpsrc = lpszPlain;
			LPBYTE lpdst = lpszEnc;
			// loop to encrypt all the data now.
			for (DWORD i=0;i<dwBlockCount;i++) {
				nStatus = PrivkEncBlock(lpPrvkey,nPrvlen,lpsrc,dwBlockSize,lpdst,&enclen);
				if (S_OK != nStatus) {
					break;
				}
			
				*pnOutlen += enclen;
				// move to next block.
				lpsrc += dwBlockSize;
				lpdst += enclen;
			}

			// get div data now.
			if (dwBlockDiva) {
				// try to copy div block data.
				memcpy(lpdst,lpsrc,dwBlockDiva);
				*pnOutlen += dwBlockDiva;
			}

			// encrypt OK.
			nStatus = S_OK;

		} while (0);
	//Cleanup:
		return (nStatus);
	}

    // ��Կ����.
	// ������RSA˽Կ���ܵ�����,���ﲻ�õ��Ļ������Ĵ�С����,
	LONG PubKDecHuge(const unsigned char* lpPubkey,int nPubklen,
			unsigned char* lpszEnc,int nEncLen,
			unsigned char* lpszOut,int* pnOutlen)
	{
		LONG nStatus = -1;
		int   declen = 0;
		do 
		{
			if ((!lpszEnc)||(!lpszOut)) break;
			DWORD dwBlockSize=(RSACRYPT_KEYSIZE>>3);				// RSA˽Կ����,���ĳ��ȱ�������Կ���ȼ�ȥ11�ֽ�.
			DWORD dwBlockCount=(nEncLen/dwBlockSize);				// ���Ա����ܵĿ���.
			DWORD dwBlockDiva=(nEncLen-(dwBlockCount*dwBlockSize));	// ʣ�಻�ܼ����ڲ�.

			*pnOutlen = 0;
			LPBYTE lpsrc = lpszEnc;
			LPBYTE lpdst = lpszOut;
			// loop to encrypt all the data now.
			for (DWORD i=0;i<dwBlockCount;i++) {
				nStatus = PubKDecBlock(lpPubkey,nPubklen,lpsrc,dwBlockSize,lpdst,&declen);
				if (S_OK != nStatus) {
					break;
				}

				*pnOutlen += declen;
				// move to next block.
				lpsrc += dwBlockSize;
				lpdst += declen;
			}

			// get div data now.
			if (dwBlockDiva) {
				// try to copy div block data.
				memcpy(lpdst,lpsrc,dwBlockDiva);
				*pnOutlen += dwBlockDiva;
			}

			// decrypt  OK.
			nStatus = S_OK;

		} while (0);
	//Cleanup:
		return (nStatus);
	}


protected:
	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ˽Կ����.����117�ֽ�,����֮�����128�ֽ�.
	LONG PrivkEncBlock(const unsigned char* lpPrvkey,int nPrvlen,
		 unsigned char* lpszPlain,int nPlainLen,
		 unsigned char* lpszEnc,int* pnOutlen)
	{
		int i;
		RSA* r=NULL;
		LONG nStatus=-1;
		do 
		{
			r = RSA_new();
			const unsigned char* p = lpPrvkey;
			r = d2i_RSAPrivateKey(&r,&p,nPrvlen);

			int flen = RSA_size(r);
			i = RSA_private_encrypt(nPlainLen,lpszPlain,lpszEnc,r,RSA_PKCS1_PADDING);
			if (i>0)
			{
				nStatus = S_OK;
				if (pnOutlen) {
				   *pnOutlen=i;
				}
			}
		} while (0);

	//Cleanup:
		RSA_free(r);
		return (nStatus);
	}

	// ��Կ����,����128�ֽ�,����֮����117�ֽڵ�����.
	LONG PubKDecBlock(const unsigned char* lpPubkey,int nKeysize,
		 unsigned char* lpszEnc,int nEnclen,
		 unsigned char* lpszOut,int* pnOutlen)
	{
		int i;
		RSA* r=NULL;
		LONG nStatus=-1;
		do 
		{
			if (pnOutlen){
				*pnOutlen=0;
			}

			r = RSA_new();
			const unsigned char* p = lpPubkey;
			r = d2i_RSAPublicKey(&r,&p,nKeysize);

			int flen = RSA_size(r);
			i = RSA_public_decrypt(flen,lpszEnc,lpszOut,r,RSA_PKCS1_PADDING);
			if (i>0)
			{
				nStatus = S_OK;
				if (pnOutlen) {
				   *pnOutlen=i;
				}
			}
		} while (0);

		//Cleanup:
		RSA_free(r);
		return (nStatus);
	}

protected:
    LONG SaveFile(const char* lpszFile,unsigned char* lpszData,int nSize)
    {
        LONG nStatus=RSAERROR_UNKNOWN;

        do 
        {
            FILE* fp = fopen(lpszFile,"wb");
            if  (!fp) {
                nStatus=RSAERROR_CREATEFILE;
                break;
            }

            // try to write the special file now.
            nStatus=fwrite(lpszData,1,nSize,fp);
            if (nSize == nSize)
            {
                nStatus = RSAERROR_OK;
            }   else
            {
                nStatus = RSAERROR_WRITEFILE;
            }

            //// close.
            fclose(fp);
        }   while (0);

    //Cleanup:
        return (nStatus);
    }
};

#endif//__RSACRYPT_HEADER__
