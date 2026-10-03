#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int create_dir(char *name, mode_t mode, int flag)
{
	int result_mkdir = 0;
	int result_status = 0;

	struct stat st;

	if(stat(name, &st) != 0)
	{
		if(errno == ENOENT)
		{
			result_mkdir = mkdir(name, mode);

			if(result_mkdir != 0)
			{
				perror("mkdir");
				result_status = 1;
			}
		}
		else
		{
			perror("mkdir");
			result_status = 1;
		}
	}
	else
	{
		if((S_ISDIR(st.st_mode)) && flag)
		{
		}
		else
		{
			perror("mkdir");
			return EXIT_FAILURE;
		}
	}

	return result_status;
}

int main(int argc, char *argv[])
{
	int start_index = 1;
	int recursive = 0;
	int result;
	char *mode = NULL;
	char *mode_8 = NULL;
	char *part_str = NULL;
	mode_t result_mode = 0755;

	if(argc < 2)
	{
		fprintf(stderr, "mkdir: missing operand\n");
		return 1;
	}

	if(argv[1][0] == '-')
	{
		if(strcmp(argv[1], "-m") == 0)
		{
			start_index = 3;

			if(argv[2] != NULL)
			{
				mode = argv[2];
				result_mode = strtol(mode, &mode_8, 8);

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
			}
			else
			{
				fprintf(stderr, "mkdir: missing operand\n");
				return 1;
			}
		}
		else if(strcmp(argv[1], "-p") == 0)
		{
			start_index = 2;
			recursive = 1;
		}
		else
		{
			fprintf(stderr, "mkdir: invalid option -- '%s'\n", argv[1]);
			return 1;
		}
	}

	if(argv[start_index] == NULL)
	{
		fprintf(stderr, "mkdir: missing operand\n");
		return 1;
	}

	for(int i = start_index; i < argc; i++)
	{
		if(recursive)
		{
			char *search_pos = argv[i];
			part_str = strchr(argv[i], '/');

			while(part_str != NULL)
			{
				*part_str = '\0';

				if(argv[i][0] != '\0')
				{
					result = create_dir(argv[i], result_mode, recursive);
				
					if(result != 0)
					{
						return 1;
					}
				}
				
				*part_str = '/';
				search_pos = part_str + 1;
				part_str = strchr(search_pos, '/');
			}

			result = create_dir(argv[i], result_mode, recursive);

			if(result != 0)
			{
				return 1;
			}
		}
		else
		{
			result = create_dir(argv[i], result_mode, recursive);

			if(result != 0)
			{
				return 1;
			}
		}
	}

	return 0;
}
