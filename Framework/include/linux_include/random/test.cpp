

#include <stdio.h>
#include <random/Random.h>
#include <random/Random2.h>
#include <random/Random3.h>
#include <random/RandomMT32.h>
#include <tick.h>

int main()
{
	/*
	int n = 0;
	DWORD dwTick = GetTickCount();

	for (int i=0;i<10000;i++)
	{
		// float fr = CRandom3::Instance().Random_Float(0,1.0f);
		float fr = CRandom::Instance().Random_Float(0,1.0f);
		printf("fr:[%0.02lf]\n",fr);
		getchar();

		if (fr <= 0.01f) {
			n++;
		}
	}

	dwTick = GetTickCount() - dwTick;

	printf("n=[%d],time used:[%d]\n",n,dwTick);
	*/


	// CRandomMT32 random;
	while (1)
	{
		// float fr = random.Random_Float(0,1.0f);

		float fr = CRandomMT32::Instance().Random_Float(0,1.0f);
		if (fr < 1.0/8.0f) {
			printf("killed!\n");
		}

		getchar();
		// printf("fr:[%0.02lf]\n",fr);
	}

	return 0;
}
