#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static int create_file(const char *file_name)
{
	int fd = open(file_name, O_WRONLY | O_CREAT | O_EXCL, 0666);

	if(fd == -1)
	{
		return -1;
	}

	if(close(fd) == -1)
	{
		return -1;
	}

	return 0;
}

static void report_error(const char *file_name)
{
	fprintf(stderr, "touch: cannot touch '%s': %s\n",
			file_name, strerror(errno));
}

static int touch_file(const char *file_name, bool no_create)
{
	if(utimensat(AT_FDCWD, file_name, NULL, 0) == 0)
	{
		return 0;
	}

	if(errno != ENOENT)
	{
		report_error(file_name);
		return 1;
	}

	if(no_create)
	{
		return 0;
	}

	if(create_file(file_name) == 0)
	{
		if(utimensat(AT_FDCWD, file_name, NULL, 0) == 0)
		{
			return 0;
		}

		report_error(file_name);
		return 1;
	}

	if(errno == EEXIST)
	{
		if(utimensat(AT_FDCWD, file_name, NULL, 0) == 0)
		{
			return 0;
		}
	}

	report_error(file_name);
	return 1;
}

int main(int argc, char *argv[])
{
	bool no_create = false;
	bool end_options = false;
	size_t file_count = 0;
	int status = 0;

	for(int i = 1; i < argc; i++)
	{
		if(!end_options && strcmp(argv[i], "--") == 0)
		{
			end_options = true;
			continue;
		}

		if(!end_options && strcmp(argv[i], "-c") == 0)
		{
			no_create = true;
			continue;
		}

		if(!end_options && argv[i][0] == '-' && argv[i][1] != '\0')
		{
			fprintf(stderr, "touch: invalid option -- '%s'\n", argv[i]);
			return EXIT_FAILURE;
		}

		file_count++;
	}

	if(file_count == 0)
	{
		fprintf(stderr, "touch: missing file operand\n");
		return EXIT_FAILURE;
	}

	end_options = false;

	for(int i = 1; i < argc; i++)
	{
		if(!end_options && strcmp(argv[i], "--") == 0)
		{
			end_options = true;
			continue;
		}

		if(!end_options && strcmp(argv[i], "-c") == 0)
		{
			continue;
		}

		if(touch_file(argv[i], no_create) != 0)
		{
			status = EXIT_FAILURE;
		}
	}

	return status;
}
