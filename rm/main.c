#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

void remove_recursive(char *name, int supress_errors)
{
	char full_path[1024];
	struct stat sb;

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
					sprintf(full_path, "%s/%s", name, entry->d_name);		
					remove_recursive(full_path, supress_errors);
				}

			}
			closedir(dir);
			remove(name);
		}
		else
		{
			int exam_file = remove(name);

			if(exam_file != 0 && !supress_errors)
			{
				perror(name);
			}
		}
	}
	else
	{
		if(!supress_errors)
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
		return 1;
	}

	if(!strcmp(argv[1], "-f"))
	{
		start_index = 2;
		supress_errors = 1;
	}

	if(!strcmp(argv[1], "-r"))
	{
		start_index = 2;
		supress_recursive = 1;
	}

	if(strcmp(argv[1], "-rf") == 0 || strcmp(argv[1], "-fr") == 0)
	{
		supress_errors = 1;
		supress_recursive = 1;
	}

	for(int i = start_index; i < argc; i++)
	{
		struct stat sb;

		if(stat(argv[i], &sb) == 0 && S_ISDIR(sb.st_mode) && !supress_recursive)
		{
			if(!supress_errors)
			{
				fprintf(stderr, "rm: %s: Is a directory\n", argv[i]);
			}
		}
		else
		{
			remove_recursive(argv[i], supress_errors);
		}
	}
	return 0;
}
