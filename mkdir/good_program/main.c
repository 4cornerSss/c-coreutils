#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

static void report_error(const char *path)
{
	int saved_errno = errno;

	fprintf(stderr, "mkdir: cannot create directory '%s': %s\n", path, strerror(saved_errno));
}

static int parse_mode(const char *text, mode_t *mode)
{
	char *end = NULL;
	unsigned long value;

	if(text == NULL || *text == '\0')
	{
		fprintf(stderr, "mkdir: invalid mode\n");
		return -1;
	}

	for(const char *p = text; *p != '\0'; p++)
	{
		if(*p < '0' || *p > '7')
		{
			fprintf(stderr, "mkdir: invalid mode '%s'\n", text);
			return -1;
		}
	}

	errno = 0;
	value = strtoul(text, &end, 8);

	if(errno == ERANGE || end == text || *end != '\0' || value > 07777UL)
	{
		fprintf(stderr, "mkdir: invalid mode '%s'\n", text);
		return -1;
	}

	*mode = (mode_t)value;

	return 0;
}

static int create_directory(const char *path, mode_t mode)
{
	if(mkdir(path, mode) == 0)
	{
		return 0;
	}

	report_error(path);
	return -1;
}

static int ensure_directory(const char *path, mode_t mode)
{
	struct stat st;

	if(mkdir(path, mode) == 0)
	{
		return 0;
	}

	if(errno != EEXIST)
	{
		report_error(path);
		return -1;
	}

	if(stat(path, &st) == 0)
	{
		if(S_ISDIR(st.st_mode))
		{
			return 0;
		}

		errno = ENOTDIR;
	}

	report_error(path);

	return -1;
}

static int make_path(const char *path, mode_t mode)
{
	char *copy;

	copy = strdup(path);

	if(copy == NULL)
	{
		report_error(path);
		return -1;
	}

	for(char *p = copy; *p != '\0'; p++)
	{
		if(*p != '/')
		{
			continue;
		}

		if(p == copy || p[-1] == '/')
		{
			continue;
		}

		*p = '\0';

		if(ensure_directory(copy, mode) != 0)
		{
			free(copy);
			return -1;
		}

		*p = '/';
	}

	if(ensure_directory(copy, mode) != 0)
	{
		free(copy);
		return -1;
	}

	free(copy);
	return 0;
}

static int parse_arguments(int argc, char *argv[], char **operands,
				int *operand_count, int *recursive, mode_t *mode)
{
	*operand_count = 0;
	*recursive = 0;
	*mode = 0775;

	for(int i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "--") == 0)
		{
			for(i++; i < argc; i++)
			{
				operands[(*operand_count)++] = argv[i];
			}

			break;
		}

		if(strcmp(argv[i], "-p") == 0)
		{
			*recursive = 1;
		}
		else if(strcmp(argv[i], "-m") == 0)
		{
			if(i + 1 >= argc)
			{
				fprintf(stderr, "mkdir: option '-m' requires am argument\n");
				return -1;
			}

			if(parse_mode(argv[++i], mode) != 0)
			{
				return -1;
			}
		}
		else if(argv[i][0] == '-' && argv[i][1] != '\0')
		{
			fprintf(stderr, "mkdir: invalid option -- '%s'\n", argv[i]);
			return -1;
		}
		else
		{
			operands[(*operand_count)++] = argv[i];
		}
	}

	if(*operand_count == 0)
	{
		fprintf(stderr, "mkdir: missing operand\n");
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	char **operands;
	int operand_count;
	int recursive;
	int status = 0;

	mode_t mode;

	operands = malloc((size_t)argc * sizeof(*operands));

	if(operands == NULL)
	{
		perror("mkdir: malloc");
		return EXIT_FAILURE;
	}

	if(parse_arguments(argc, argv, operands, &operand_count, &recursive, &mode) != 0)
	{
		free(operands);
		return EXIT_FAILURE;
	}

	for(int i = 0; i < operand_count; i++)
	{
		int result;

		if(recursive)
		{
			result = make_path(operands[i], mode);
		}
		else
		{
			result = create_directory(operands[i], mode);
		}

		if(result != 0)
		{
			status = EXIT_FAILURE;
		}
	}

	free(operands);

	return status;
}
