#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

ssize_t write_bytes(char *buf, size_t offset, size_t count_bytes)
{
	ssize_t count = write(STDOUT_FILENO, buf + offset, count_bytes);

	if(count <= 0)
	{
		perror("tree");
		return -1;
	}

	return count;
}

int newline()
{
	char *newline = "\n";

	if(write_bytes(newline, 0, 1) <= 0)
		return 1;

	return 0;
}

int tab()
{
	char *tab = "\t";

	if(write_bytes(tab, 0, 1) <= 0)
		return 1;

	return 0;
}

int processed_write(char *buffer, size_t size_buffer)
{
	size_t count_bytes = size_buffer;
	size_t offset = 0;

	while(count_bytes != 0)
	{
		ssize_t count_write = write_bytes(buffer, offset, count_bytes);
		
		if(count_write <= 0)
			return 1;

		count_bytes -= count_write;
		offset += count_write;
	}

	return 0;
}

int output_file(char *file_name, size_t count)
{
	size_t size = strlen(file_name);

	for(size_t i = 0; i < count; i++)
	{
		if(tab() != 0)
			return 1;
	}

	if(processed_write(file_name, size) != 0)
		return 1;

	if(newline() != 0)
		return 1;

	return 0;
}

char *gluing_path(char *buffer, size_t size_buffer, char *path_1, char *path_2)
{
	int count = snprintf(buffer, size_buffer, "%s/%s", path_1, path_2);

	if(count < 0)
	{
		perror("tree");
		return NULL;
	}

	return buffer;
}

int free_list(struct dirent **list, size_t start, size_t size)
{
	for(size_t i = start; i < size; i++)
	{
		free(list[i]);
	}

	free(list);

	return 0;
}

int examination_processed_file_dir(char *file_name, size_t count)
{
	struct stat st_file;

	if(stat(file_name, &st_file) == 0)
	{
		if(S_ISDIR(st_file.st_mode))
		{
			struct dirent **namelist;

			char buffer[PATH_MAX];
			size_t size_buffer = sizeof(buffer) / sizeof(buffer[0]);

			int count_files = scandir(file_name, &namelist, 0, alphasort);

			if(count_files < 0)
			{
				perror("tree");
				return 1;
			}

			if(output_file(file_name, count) != 0)
				return 1;

			count += 1;

			for(size_t i = 0; i < count_files; i++)
			{
				char *name = namelist[i]->d_name;

				if(strcmp(name, ".") && strcmp(name, ".."))
				{
					char *path = gluing_path(buffer, size_buffer, file_name, name);

					if(path == NULL)
					{
						free_list(namelist, i, count_files);
						return 1;
					}


					if(examination_processed_file_dir(path, count) != 0)
					{
						free_list(namelist, i, count_files);
						return 1;
					}
				}

				free(namelist[i]);
			}

			free(namelist);
		}
		else
		{
			if(output_file(file_name, count) != 0)
				return 1;
		}
	}
	else
	{
		if(errno == ENOENT)
		{
			fprintf(stderr, "tree: File of directories do not exist '%s'\n", file_name);
			return 1;
		}
		else if(errno == EACCES)
		{
			fprintf(stderr, "find: ‘%s’: Permission denied\n", file_name);
			return 1;
		}
		else
		{
			perror("tree");
			return 1;
		}
	}	
	
	return 0;
}

int main(int argc, char *argv[])
{
	if(argc == 1)
	{
		if(examination_processed_file_dir(".", 0) != 0)
			return 1;

		return 0;
	}

	for(size_t i = 1; i < argc; i++)
	{
		if(examination_processed_file_dir(argv[i], 0) != 0)
			return 1;
	}

	return 0;
}
