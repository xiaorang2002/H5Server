
#include <types.h>
#include <stdio.h>
#include <ini/INIHelp.h>


int main()
{
	CINIHelp ini;
        if (ini.Open("StaticBlackList.ini"))
        {
            LPCTSTR lpsz = ini.GetString("Config","StartNumber");
            printf("Condition:[%s]\n",lpsz);
	
		lpsz = ini.GetString("Config","Condition");
		printf("Interval:[%s]\n",lpsz);

       }

	/* 
	if (ini.Open("StaticBlackList.ini")) 
	{

		ini.SetString("BlackList","101","12351");
		ini.SetString("BlackList","102","12352");
		ini.SetString("BlackList","103","12353");
                ini.SetString("BlackList","104","12354");
                ini.SetString("BlackList","105","12355");
                ini.SetString("BlackList","106","12356");
		ini.SetString("BlackList","107","12347");

		LPCTSTR lpsz = ini.GetString("BlackList","1");
		printf("read 1, value:[%s]\n",lpsz);
	}
	*/

	printf("all done!\n");
	return 0;
}
