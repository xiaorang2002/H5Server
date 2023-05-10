#include <cstdio>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>

#include <string>

typedef int(*DllAddFunc)(int a, int b);

int main(int argc, char* argv[])
{
	printf("--- *** run main starting .....................\n");
	if (argc < 3) {
		printf("--- *** input args error\n");
		return -1;
	}
	printf("--- *** %s %s %s\n", argv[0], argv[1], argv[2]);
	while (true) {
		if (getchar() == 'q') {
			break;
		}
		//加载so
		const char *so_file = "/home/Landy/TianXia/Program/bin/libproj_linux_dll.so";
		void *handle = dlopen(so_file, RTLD_LAZY); //RTLD_NOW | RTLD_DEEPBIND
		if (!handle) {
			printf("--- *** handle == null\n");
			return -1;
		}
		//查找函数
		DllAddFunc fn = (DllAddFunc)dlsym(handle, "DllAdd");
		if (!fn) {
			printf("--- *** DllAdd cannot found\n");
			dlclose(handle);
			return -1;
		}
		int sum = fn(atoi(argv[1]), atoi(argv[2]));
		//调用
		printf("--- *** DllAdd(%s,%s) = %d\n", argv[1], argv[2], sum);
		dlclose(handle);
	}

    return 0;
}