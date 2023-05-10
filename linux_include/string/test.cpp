
#include <stdio.h>
#include <string/SplitString.h>

int main(int argc,char* argv[])
{
	CSplitString split;

	split.SetSplit("4000",",");

	int count = split.count();
	for (int i=0;i<count;i++) 
	{
		printf("item:[%d],string:[%s]\n",i,split[i]);
	}

	getchar();

	return 0;
}


