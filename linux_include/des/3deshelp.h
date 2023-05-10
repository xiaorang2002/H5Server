
#ifndef __3DESHELP_HEADER__
#define __3DESHELP_HEADER__

#include <types.h>
#include <des/3des8bit.h>
#include <string.h>

class C3DEShelp
{
public:
    /// lpszKey  : must be 24 bytes.
    /// lpszData : must be align with 8 bytes.
    static int32 Encrypt(LPBYTE lpszKey,LPBYTE lpszData,int32 nSize,LPBYTE lpszOut)
    {
        int32 nIndex     = 0;
        int32 nDecrypted = 0;
        des3_context ctx = {0};
        // try to encrypt all the text content value item value now.
        des3_set_3keys(&ctx,&lpszKey[0],&lpszKey[8],&lpszKey[16]);    
        // initialize the loop.
        int32 nLoop = nSize / 8;
        LPBYTE lpszTemp = lpszOut;
        // try to empty the data.
        memset(lpszOut,0,nSize);
        // loop to encrypt all content now.
        for (nIndex=0;nIndex<nLoop;nIndex++)
        {
            des3_encrypt(&ctx,lpszData,lpszTemp);
            nDecrypted+=8;
            lpszData+=8;
            lpszTemp+=8;
        }
    //Cleanup:
        return (nDecrypted);
    }

    /// lpszKey : must be 24 bytes.
    /// lpszEnc : must be align with 8 bytes.
    static int32 Decrypt(LPBYTE lpszKey,LPBYTE lpszEnc,int32 nSize,LPBYTE lpszOut)
    {
        int32 nIndex     = 0;
        int32 nEncrypted = 0;
        des3_context ctx = {0};
        // try to encrypt all the text content value now.
        des3_set_3keys(&ctx,lpszKey,lpszKey+8,lpszKey+16);    
        // initialize the loop.
        int32 nLoop = nSize>>3;
        LPBYTE lpszTemp = lpszOut;
        // try to empty the data.
        memset(lpszOut,0,nSize);
        // loop to encrypt all content now.
        for (nIndex=0;nIndex<nLoop;nIndex++)
        {
            des3_decrypt(&ctx,lpszEnc,lpszTemp);
            nEncrypted+=8;
            lpszEnc+=8;
            lpszTemp+=8;
        }
    //Cleanup:
        return (nEncrypted);
    }
};



#endif//__3DESHELP_HEADER__
