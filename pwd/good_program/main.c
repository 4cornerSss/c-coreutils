#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

static char *get_physical_pwd(void)
{
	size_t size = 128;
	char *buffer = malloc(size);

	if(buffer == NULL)
	{
		perror("pwd: malloc");
		return NULL;
	}

	while(getcwd(buffer, size) == NULL)
	{
		if(errno != ERANGE)
		{
			perror("pwd: getcwd");
			free(buffer);
			return NULL;
		}

		if(size > SIZE_MAX / 2)
		{
			errno = ENOENT;
			perror("pwd: buffer size");
			free(buffer);
			return NULL;
		}

		size *= 2;

		char *new_buffer = realloc(buffer, size);

		if(new_buffer == NULL)
		{
			perror("pwd: realloc");
			free(buffer);
			return NULL;
		}

		buffer = new_buffer;
	}

	return buffer;
}

static char *copy_string(const char *source)
{
	size_t length = strlen(source) + 1;
	char *copy = malloc(length);

	if(copy == NULL)
	{
		perror("pwd: malloc");
		return NULL;
	}

	memcpy(copy, source, length);
	
	return copy;
}

static int is_valid_pwd(const char *pwd)
{
	struct stat pwd_stat;
	struct stat cwd_stat;

	if(pwd == NULL || pwd[0] != '/')
	{
		return 0;
	}

	if(stat(pwd, &pwd_stat) == -1)
	{
		return 0;
	}

	if(stat(".", &cwd_stat) == -1)
	{
		return 0;
	}

	return (pwd_stat.st_dev == cwd_stat.st_dev) && (pwd_stat.st_ino == cwd_stat.st_ino);
}

static char *get_logical_pwd(void)
{
	const char *pwd = getenv("PWD");

	if(is_valid_pwd(pwd))
	{
		return copy_string(pwd);
	}

	return get_physical_pwd();
}

static int print_pwd(const char *path)
{
	if(puts(path) == EOF)
	{
		perror("pwd: stdout");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	char *pwd;
	int physical = 0;

	if(argc > 2)
	{
		fprintf(stderr, "pwd: too many arguments\n");
		return EXIT_FAILURE;
	}

	if(argc == 2)
	{
		if(strcmp(argv[1], "-P") == 0)
		{
			physical = 1;
		}
		else if(strcmp(argv[1], "-L") != 0)
		{
			fprintf(stderr, "pwd, invalid option: %s\n", argv[1]);
			return EXIT_FAILURE;
		}
	}

	if(physical)
	{
		pwd = get_physical_pwd();
	}
	else
	{
		pwd = get_logical_pwd();
	}

	if(pwd == NULL)
	{
		return EXIT_FAILURE;
	}

	int result = print_pwd(pwd);

	free(pwd);

	return result;
}
