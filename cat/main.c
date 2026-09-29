#include <stdio.h>

#define BUF_SIZE 4096

int main(int argc, char *argv[])
{
	char buf[BUF_SIZE];
	if(argc == 1)
	{
		while((fgets(buf, sizeof(buf), stdin)) != NULL)
		{
			printf("%s", buf);
		}
	}
	else
	{
		int exit_code = 0;

		for(int i = 1; i < argc; i++)
		{
			FILE* fl = fopen(argv[i], "r");

		        if(fl != NULL)
			{
				while(fgets(buf, sizeof(buf), fl) != NULL)
				{
					printf("%s", buf);
				}
				fclose(fl);
			}
			else
			{
				perror(argv[i]);
				exit_code = 1;
			}
		}

		return exit_code;
	}
	return 0;
}
