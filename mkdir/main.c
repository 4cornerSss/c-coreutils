#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

int create_dir(const char *name_dir, const mode_t mode)
{
	if(mkdir(name_dir, mode) != 0)
	{
		perror("mkdir");
		return 1;
	}

	return 0;
}

int processed_dir(const char *name_dir, const mode_t mode)
{
	struct stat st;

	if(stat(name_dir, &st) != 0)
	{
		if(errno == ENOENT)
		{
			if(create_dir(name_dir, mode) != 0)
			{
				return 1;
			}
		}
		else
		{
			fprintf(stderr, "mkdir: cannot create directory ‘%s’: File exists\n", name_dir);
			return 1;
		}
	}
	else
	{
		if(!S_ISDIR(st.st_mode))
		{
			fprintf(stderr, "mkdir: cannot create directory ‘%s’: File exists\n", name_dir);
			return 1;
		}
	}
	
	return 0;
}

int separation_path(char *part , char *path, mode_t mode)
{
	if(part != NULL)
	{
		*part = '\0';

		if(path[0] != '\0')
		{
			if(processed_dir(path, mode) != 0)
			{
				*part = '/';
				return 1;
			}
		}

		*part = '/';

		part = strchr(part + 1, '/');

		if(separation_path(part, path, mode) != 0)
		{
			return 1;
		}
	}
	else
	{
		if(processed_dir(path, mode) != 0)
		{
			return 1;
		}
	}

	return 0;
}

int processed_path(char *trans_path, const mode_t mode)
{
	char *part_str = strchr(trans_path, '/');

	if(separation_path(part_str, trans_path, mode) != 0)
	{
		return 1;
	}

	return 0;
}

mode_t processed_mode(char *mode)
{
	char *mode_8 = NULL;

	mode_t result_mode = strtol(mode, &mode_8, 8);

	if(mode == mode_8)
	{
		fprintf(stderr, "mkdir: invalid mode ‘%s’\n", mode);
		return 1;
	}

	if(*mode_8 != '\0')
	{
		fprintf(stderr, "mkdir: invalid mode ‘%s’\n", mode);
		return 1;
	}

	return result_mode;
}

int main(int argc, char *argv[])
{
	int status = 0;

	int start_index = 0;
	char *arguments[argc - 1];

	int recursive = 0;
	mode_t mode = 0755;

	for(size_t i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
		{
			if(!strcmp(argv[i], "-p"))
			{
				recursive = 1;
			}
			else if(!strcmp(argv[i], "-m"))
			{
				if(argv[i + 1] != NULL)
				{
					if((mode = processed_mode(argv[i + 1])) == 0)
					{
						return 1;
					}
				}
				else
				{
					fprintf(stderr, "mkdir: missing operand\n");
					return 1;
				}
				
				i += 1;
			}
			else
			{
				fprintf(stderr, "mkdir: invalid option -- '%s'\n", argv[i]);
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
		fprintf(stderr, "mkdir: missing operand\n");
		return 1;
	}

	for(size_t i = 0; i < start_index; i++)
	{
		if(recursive)
		{
			if(processed_path(arguments[i], mode) != 0)
			{
				status = 1;
			}
		}
		else
		{
			if(processed_dir(arguments[i], mode) != 0)
			{
				status = 1;
			}
		}
	}

	return status;
}
