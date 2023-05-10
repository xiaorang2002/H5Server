
#ifndef __IPADDRESS_HEADER__
#define __IPADDRESS_HEADER__

#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

class CIpAddress
{
public:
    CIpAddress()
    {
		memset(&m_ifc,0,sizeof(m_ifc));
    	m_bEnumAdapter=FALSE;
		m_nAdapter=0;
	}

    ~CIpAddress()
    {
    }


public:
    LPCTSTR GetHostName()
    {
		static char host[128]="";
		int err = gethostname(host,sizeof(host));
		if (err != 0) {
			return 0;
		}    
//Cleanup:
        return (host);
    }

	/// get domain ip address content.
	LPCTSTR Domain2ip(LPCTSTR domain)
	{
		LPCTSTR lpsz=0;
		char ip[32]={0};
		char **pptr=0;
		struct hostent* phost=0;

		do
		{
			phost = gethostbyname(domain);
			if (0 == phost) break;

			for (pptr=phost->h_addr_list; *pptr!=NULL,pptr++;)
			{
				if (NULL != inet_ntop(phost->h_addrtype, *pptr, ip, sizeof(ip)))
				{
					// Succeeded.
					lpsz = ip;
					break;
				}
			}			

		} while (0);
	//Cleanup:
		return (lpsz);
	}

    /// get adapter count.
    int GetMacAdapterNum()
    {
		EnumAdapter();
        return (m_nAdapter);
    }

    /// try to get the mac address.
    LPCTSTR GetMac(LONG nIndex=0)
    {
        LPCTSTR lpsz=NULL;
        struct ifreq ifr;
		static char name[32]={0};
		static char mac[32]={0};
        int sd = 0;
        do
        {
            // try to check the index value data item now.
            if ((nIndex<0)||(nIndex>=m_nAdapter)) break;
            ifreq* it = m_ifc.ifc_req;
            it += nIndex;

			memset(&ifr,0,sizeof(ifr));
			sd = socket(AF_INET,SOCK_STREAM,0); 
			lstrcpyn(ifr.ifr_name,it->ifr_name,sizeof(mac));
			int err = ioctl(sd,SIOCGIFHWADDR, &ifr);
			if (err != 0) break;

			// try to format the special mac address content value.
			snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",  
		 			(unsigned char)ifr.ifr_hwaddr.sa_data[0],   
		 			(unsigned char)ifr.ifr_hwaddr.sa_data[1],  
		 			(unsigned char)ifr.ifr_hwaddr.sa_data[2],   
		 			(unsigned char)ifr.ifr_hwaddr.sa_data[3],  
		 			(unsigned char)ifr.ifr_hwaddr.sa_data[4],  
		 			(unsigned char)ifr.ifr_hwaddr.sa_data[5]);  

			// address.
            lpsz = mac;
        } while (0);
    //Cleanup:
        if (sd) close(sd);
        return (lpsz);


    }

    // get local host ipaddress.
    LPCTSTR GetIp(int nIndex=0)
    {
		LPCTSTR lpsz=NULL;
		struct sockaddr_in sin;
		static char ip[32]={0};
		int sd = 0;
		do
		{
			// try to check the index value data item now.
			if ((nIndex<0)||(nIndex>=m_nAdapter)) break;
			ifreq* ifr = m_ifc.ifc_req;
			ifr += nIndex;

			// try to get the special content value.
			memcpy(&sin, &ifr->ifr_addr,sizeof(sin));
			snprintf(ip,sizeof(ip),"%s", inet_ntoa(sin.sin_addr));
			lpsz = ip;
		} while (0);
    //Cleanup:
		if (sd) close(sd);
        return (lpsz);
    }

    /// get the integer ip address.
    uint32 GetIpInt(int nIndex=0)
    {
		uint32 naddr = 0;
        struct sockaddr_in sin;
        int sd = 0;
        do
        {
            // try to check the index value data item now.
            if ((nIndex<0)||(nIndex>=m_nAdapter)) break;
            ifreq* ifr = m_ifc.ifc_req;
            ifr += nIndex;

            // try to get the special content value.
            memcpy(&sin, &ifr->ifr_addr,sizeof(sin));
            naddr = sin.sin_addr.s_addr;
        } while (0);
    //Cleanup:
        if (sd) close(sd);
        return (naddr);
    }

    ///// convert the string IP address content.
    static uint32 StringIpToInt(LPCTSTR lpszIp)
    {
        uint32 nStatus = 0;
		nStatus = inet_addr(lpszIp);
    //Cleanup:
        return (nStatus);
    }

    // convert the DWORD IP to String value.
    static LPCTSTR IntIpToString(uint32 dwIp)
    {
        BYTE b1,b2,b3,b4;
		static char ip[32]={0};
        if (!dwIp) return (NULL);
        /// try to get all the value now.
        b1 = (BYTE)(( dwIp & 0xFF000000 ) >> 24);
        b2 = (BYTE)(( dwIp & 0xFF0000 ) >> 16);
        b3 = (BYTE)(( dwIp & 0xFF00 ) >> 8);
        b4 = (BYTE)(( dwIp & 0xFF ));
        /// try to build the full IP value now.
        sprintf(ip,("%d.%d.%d.%d"),b1,b2,b3,b4);
        if (lstrlen(ip)) {
            return (ip);
        }
    //Cleanup:
        return (NULL);
    }

protected:
	int32 EnumAdapter()
	{
		int32 status = -1;
		int sd = 0;
		do
		{
			// check the enum adapter.
			if (m_bEnumAdapter) break;
			sd = socket(AF_INET, SOCK_DGRAM, 0);
			if (sd<=0) break;

			//// try to enum all the adapter.
			m_ifc.ifc_len = sizeof(m_ifcbuf);
			m_ifc.ifc_buf = m_ifcbuf;

			// try to get the parameter item now.
			status = ioctl(sd,SIOCGIFCONF,&m_ifc);
			if (0 == status) {
				m_bEnumAdapter=TRUE;
			}

			// try to get the adapter count now content now.
			m_nAdapter=(m_ifc.ifc_len/sizeof(struct ifreq));
		} while (0);
	//Cleanup:
		if (sd) close(sd);
		return (m_bEnumAdapter);
	}

protected:
	ifconf m_ifc;				// the ifconf data.
	TCHAR  m_ifcbuf[2048];		// the ifconf buffer.
	DWORD  m_nAdapter;			// count of adapter.
	BOOL   m_bEnumAdapter;		// check if adapter enumted.
};



#endif//__IPADDRESS_HEADER__
