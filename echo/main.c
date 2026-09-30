#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int start_index = 1;
	int newline = 1;

	if(argc > 1 && !strcmp(argv[1], "-n"))
	{
		start_index = 2;
		newline = 0;
	}

	for(int i = start_index; i < argc; i++)
	{
		printf("%s", argv[i]);

		if(i < argc - 1)
		{
			printf(" ");
		}
	}

	if(newline)
	{
		printf("\n");
	}

	return 0;
}
