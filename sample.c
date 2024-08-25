#include <stdio.h>
#include <unistd.h>

int main()
{
	int i=0;
	while (1)
	{
		i++;
		if(i%100000 == 0)
		{
			sleep(5);
			printf("%d\n", i);
		}
	}
	return 0;
}
