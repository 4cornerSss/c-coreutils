#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

int create_file(const char *file_name)
{
	int exam_close;
	int fd = open(file_name, O_CREAT, 0644);

	if(fd == -1)
	{
		perror("touch");
		return EXIT_FAILURE;
	}

	exam_close = close(fd);

	if(exam_close != 0)
	{
		perror("touch");
		return EXIT_FAILURE;
	}

	return 0;
}

int edit_time_file(const char *file_name)
{
	if(utimensat(AT_FDCWD, file_name, NULL, 0) != 0)
	{
		perror("touch");
		return EXIT_FAILURE;
	}

	return 0;
}

int processed_file(const char *file_name, int const not_create)
{
	if(not_create)
	{
		struct stat st;

		if(stat(file_name, &st) != 0)
		{
			if(errno == ENOENT)
			{
				return 0;
			}
			else
			{
				return 1;
			}
		}
	}
	else
	{
		if(create_file(file_name) != 0)
		{
			return 1;
		}
	}

	if(edit_time_file(file_name) != 0)
	{
		return 1;
	}
}

int main(int argc, char *argv[])
{
	int status = 0;
	int exam_func;

	int start_index = 0;
	int not_create = 0;
	char *arguments[argc - 1];

	for(size_t i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
		{
			if(!strcmp(argv[i], "-c"))
			{
				not_create = 1;

			}
			else
			{
				fprintf(stderr, "touch: invalid option -- '%s'\n", argv[i]);
				return 1;
			}
		}
		else
		{
			arguments[start_index] = argv[i];

			start_index += 1;
		}
	}

	if(start_index == 0)
	{
		fprintf(stderr, "touch: missing file operand\n");
		return 1;
	}

	for(size_t i = 0; i < start_index; i++)
	{
		exam_func = processed_file(arguments[i], not_create);

		if(exam_func != 0)
		{
			status = exam_func;
		}
	}

	return status;
}
