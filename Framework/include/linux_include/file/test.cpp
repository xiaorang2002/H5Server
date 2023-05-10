
#include <stdio.h>
#include <file/FileExist.h>

//#include <file/FileEnum.h>
//
//#include <unistd.h>
//#include <dirent.h>

int main(int argc,char* argv[])
{
	bool bSuccess = CFileExist::IsFile("./test.txt");
	printf("is test.txt exist:[%d]\n",bSuccess);
	
	bool bisSpec  = CFileExist::IsSpecialFileA("test.txt","exe");
	printf("is special file:[%d]\n",bisSpec);

	/*
	DIR* p = opendir(".");
	printf("dirent:[%d]\n",p);

	CFileEnum file;

	file.Find(".");

	char name[1024]={0};

	while (file.Next(name,sizeof(name),FALSE))
	{
		printf("file:[%s]\n",name);
	}
	*/

	printf("all done!\n");

	return 0;
}


