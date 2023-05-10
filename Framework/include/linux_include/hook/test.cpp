
#include <hook/HookFileChanged.h>

int main(int argc,char* argv[])
{
	CHookFileChanged hook;
	hook.StartHook("test.txt");
	while (1)
	{
		getchar();
		if (hook.isChanged()) {
			printf("changed!\n");
		}
		else
		{
			printf("no change!\n");
		}
	}
//Cleanup:
	return 0;
}


