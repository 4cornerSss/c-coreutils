#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

char *processed_P(char *name, size_t size);

char *processed_L(char *name, size_t size)
{
	int invalid = 0;
	struct stat st1;
	struct stat st2;

	char *PWD = getenv("PWD");

	if(PWD != NULL)
	{
		if(stat(PWD, &st1) == 0)
		{
			if(stat(".", &st2) == 0)
			{
				if(st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino)
				{
					return PWD;
				}
				else
				{
					invalid = 1;
				}
			}
			else
			{
				invalid = 1;
			}
		}
		else
		{
			invalid = 1;
		}
	}
	else
	{
		invalid = 1;
	}

	if(invalid)
	{
		PWD = processed_P(name, size);

		if(PWD == NULL)
		{
			return NULL;
		}
	}

	return PWD;
}

char *processed_P(char *name, size_t size)
{

	if(getcwd(name, size) == NULL)
	{
		perror("pwd");
		return NULL;
	}

	return name;
}

int processed_out(const char *name, size_t size)
{
	size_t count_write;
	size_t offset = 0;
	size_t count = size;
	
	while(count != 0)
	{
		if((count_write = fwrite(name + offset, sizeof(name[0]), count, stdout)) == 0)
		{
			if(ferror(stdout))
			{
				perror("pwd");
				return EXIT_FAILURE;
			}
			else
			{
				return 1;
			}
		}

		offset += count_write;
		count -= count_write;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	const char *newline = "\n";
	int result;
	char cwd[PATH_MAX];
	char *result_path;

	if(argc == 1)
	{
		result_path = processed_L(cwd, sizeof(cwd));

		if(result_path == NULL)
		{
			return 1;
		}
	}

	if(argc > 1)
	{
		if(argv[1][0] == '-')
		{
			if(!strcmp(argv[1], "-P"))
			{
				result_path = processed_P(cwd, sizeof(cwd));

				if(result_path == NULL)
				{
					return 1;
				}
			}
			else if(!strcmp(argv[1], "-L"))
			{
				result_path = processed_L(cwd, sizeof(cwd));

				if(result_path == NULL)
				{
					return 1;
				}
			}
			else
			{
				fprintf(stderr, "pwd: %s: ininvalid option\n", argv[1]);
				return 1;
			}
		}
		else
		{
			result_path = processed_L(cwd, sizeof(cwd));

			if(result_path == NULL)
			{
				return 1;
			}
		}
	}

	result = processed_out(result_path, strlen(result_path));

	if(result != 0)
	{
		return 1;
	}

	result = processed_out(newline, strlen(newline));

	if(result != 0)
	{
		return 1;
	}

	return 0;
}
