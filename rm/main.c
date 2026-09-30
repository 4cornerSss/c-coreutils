#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

void remove_recursive(char *name, int error)
{
	struct stat sb;
	char buffer[1024];

	if(stat(name, &sb) == 0)
	{
		if(S_ISDIR(sb.st_mode))
		{
			DIR *dir = opendir(name);
			struct dirent *entry;

			while((entry = readdir(dir)) != NULL)
			{
				if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
				{
					sprintf(buffer, "%s/%s", name, entry->d_name);
					remove_recursive(buffer, error);
				}
			}

			closedir(dir);
			remove(name);
		}
		else
		{
			remove(name);
		}
	}
	else
	{
		if(!error)
		{
			perror(name);
		}
	}
}

int main(int argc, char *argv[])
{
	int start_index = 1;
	int supress_errors = 0;
	int supress_recursive = 0;
	if(argc < 2)
	{
		fprintf(stderr, "rm: missing operand\n");
	}

	while(start_index < argc && argv[start_index][0] == '-')
	{
		if(strcmp(argv[start_index], "-f") == 0)
		{
			supress_errors = 1;
		}
		else if(strcmp(argv[start_index], "-r") == 0)
		{
			supress_recursive = 1;
		}
		else if(strcmp(argv[start_index], "-fr") == 0 || strcmp(argv[start_index], "-rf") == 0)
		{
			supress_errors = 1;
			supress_recursive = 1;
		}

		start_index++;
	}

	for(int i = start_index; i < argc; i++)
	{
		struct stat sb;

		if(stat(argv[i], &sb) == 0 && S_ISDIR(sb.st_mode) && !supress_recursive)
		{
			if(!supress_errors)
			{
				fprintf(stderr, "rm: %s is a directory\n", argv[i]);
			}
		}
		else
		{
			remove_recursive(argv[i], supress_errors);
		}

	}

	return 0;
}
