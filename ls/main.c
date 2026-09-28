#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

int main(int argc, char *argv[])
{
	const char *PATH = NULL;
	const char *OP = "";

	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
		{
			OP = argv[i];
		}
		else
		{
			PATH = argv[i];
		}
	}

	if(PATH == NULL)
	{
		PATH = ".";
	}
	
	DIR *dir = opendir(PATH);

	if(dir == NULL)
	{
		perror("Not open dir!");
		return 1;
	}

	struct dirent *entry;
	struct stat st;
	struct tm *tm_info = localtime(&st.st_mtime);
	char full_path[1024];
	char time_buf[64];
	while((entry = readdir(dir)) != NULL)
	{
		if(entry->d_name[0] != '.' || !strcmp(OP, "-a"))
		{
			if(!strcmp(OP, "-l"))
			{
				snprintf(full_path, sizeof(full_path), "%s/%s", PATH, entry->d_name);
				stat(full_path, &st); 
				strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", tm_info);
				printf("%o \t %ld \t %s \t",st.st_mode, st.st_size, time_buf);
			}
			printf("%s \n", entry->d_name);
		}
	}

	closedir(dir);
	return 0;
}
