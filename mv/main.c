#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

int move(char *sour, char *des)
{
	if(rename(sour, des) != 0)
	{
		if(errno == ENOENT)
		{
			fprintf(stderr, "mv: cannot stat '%s': No such file or directory\n", sour);
			return 1;
		}
		else
		{
			perror("mv");
			return EXIT_FAILURE;
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int result = 0;

	int start_index = 1;
	int source = start_index;
	int destination = start_index + 1;

	struct stat st_s;
	struct stat st_d;

	if(argv[source] == NULL)
	{
		fprintf(stderr, "mv: missing file operand\n");
		return 1;
	}

	if(argv[destination] == NULL)
	{
		fprintf(stderr, "mv: missing destination file operand after '%s'\n", argv[source]);
		return 1;
	}

	if(stat(argv[destination], &st_d) == 0)
	{
		if(S_ISDIR(st_d.st_mode))
		{
			char path[PATH_MAX];
			char *search_pos;
			char *part_str = strrchr(argv[source], '/');
			if(part_str != NULL)
			{
				search_pos = part_str + 1;
			}
			else
			{
				search_pos = argv[source];
			}

			snprintf(path, sizeof(path), "%s/%s", argv[destination], search_pos);

			result = move(argv[source], path);

			if(result != 0)
			{
				return 1;
			}
		}
		else
		{
			if(stat(argv[source], &st_s) == 0)
			{
				if(S_ISDIR(st_s.st_mode))
				{
					fprintf(stderr, "mv: cannot overwrite non-directory '%s' with directory '%s'\n", argv[destination], argv[source]);
					return 1;
				}
				else
				{
					result = move(argv[source], argv[destination]);

					if(result != 0)
					{
						return 1;
					}
				}
			}
		}

	}
	else
	{
		if(errno == ENOENT)
		{
			result = move(argv[source], argv[destination]);

			if(result != 0)
			{
				return 1;
			}
		}
		else
		{
			perror("mv");
			return EXIT_FAILURE;
		}
	}
	
	return 0;
}
