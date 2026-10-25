#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#include <unistd.h>

#include <limits.h>

int processed_output(const char *str, size_t size)
{
	write(STDOUT_FILENO, str, size);

	return 0;
}

int newline()
{
	if(processed_output("\n", 1) != 0)
	{
		return 1;
	}

	return 0;
}

int header(const char *name)
{
	if(processed_output(name, strlen(name)) != 0)
	{
		return 1;
	}

	if(newline() != 0)
	{
		return 1;
	}
	
	return 0;
}

char *gluing_str(char *buf, int size, const char *str_1, const char *str_2)
{
	int written = snprintf(buf, size, "%s/%s", str_1, str_2);

	if(written < 0)
	{
		perror("find");
		return NULL;
	}
	else if(written == 0)
	{
		return "";
	}

	return buf;
}

int format_processed_out(const char *file_name, const char *str, size_t size)
{
	char buf[PATH_MAX];

	char *path = gluing_str(buf, sizeof(buf), file_name, str);

	if(path == NULL)
	{
		return 1;
	}
	else if(path[0] == '\0')
	{
		return 0;
	}

	if(processed_output(path, strlen(path)) != 0)
	{
		return 1;
	}

	return 0;
}

int exam_file(const char *file_name)
{
	struct stat st_file;

	if(stat(file_name, &st_file) == 0)
	{
		if(S_ISDIR(st_file.st_mode))
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		if(errno == ENOENT)
		{
			fprintf(stderr, "find: ‘%s’: No such file or directory\n", file_name);
			return 1;
		}
		else if(errno == EACCES)
		{
			fprintf(stderr, "find: ‘%s’: Permission denied\n", file_name);
			return 1;
		}
		else
		{
			perror("find");
			return 1;
		}
	}

	return 0;
}

int free_list_error(struct dirent **list, int count, size_t start)
{
	for(size_t i = start; i < count; i++)
	{
		free(list[i]);
	}

	free(list);

	return 0;
}

int comparasion_string(char const *str_1, char const *str_2)
{
	size_t size_str_1 = strlen(str_1);
	size_t size_str_2 = strlen(str_2);

	int result = 0;

	if(size_str_1 < size_str_2)
	{
		return 1;
	}

	for(size_t i = 0; i < size_str_1; i++)
	{
		if((size_str_1 - i) >= size_str_2)
		{
			if(str_1[i] == str_2[0])
			{
				for(size_t k = 0; k < size_str_2; k++)
				{
					if(str_1[i + k] == str_2[k])
					{
						result += 1;
					}
					else
					{
						i += k;
					}
				}

				if(result == size_str_2)
				{
					return 0;
				}
			}
		}

		result = 0;
	}
	
	return 1;
}

int processed_directory(const char *directory, const char *str)
{
	struct dirent **namelist;

	char buf[PATH_MAX];
	char *name;
	int exam_dir;

	int count_files = scandir(directory, &namelist, 0, alphasort);

	if(count_files < 0)
	{
		perror("find");
		return 1;
	}

	for(size_t i = 0; i < count_files; i++)
	{
		name = namelist[i]->d_name;

		char *path = gluing_str(buf, sizeof(buf), directory, name);

		if(path == NULL)
		{
			if(free_list_error(namelist, count_files, i) != 0)
			{
				return 1;
			}

			return 1;
		}
		else if(path[0] == '\0')
		{
			if(free_list_error(namelist, count_files, i) != 0)
			{
				return 1;
			}

			return 0;
		}
		else
		{
			exam_dir = exam_file(path);

			if(exam_dir == 1)
			{
				if(free_list_error(namelist, count_files, i) != 0)
				{
					return 1;
				}

				return 1;
			}
		}

		if(strcmp(name, ".") && strcmp(name, ".."))
		{
			if(str != NULL)
			{
				int result_comp = comparasion_string(name, str);

				if(result_comp == 0)
				{
					if(format_processed_out(directory, name, strlen(name)) != 0)
					{
						if(free_list_error(namelist, count_files, i) != 0)
						{
							return 1;
						}

						return 1;
					}
					if(newline() != 0)
					{
						if(free_list_error(namelist, count_files, i) != 0)
						{
							return 1;
						}

						return 1;
					}
				}
			}
			else
			{
				if(format_processed_out(directory, name, strlen(name)) != 0)
				{
					if(free_list_error(namelist, count_files, i) != 0)
					{
						return 1;
					}

					return 1;
				}
				if(newline() != 0)
				{
					if(free_list_error(namelist, count_files, i) != 0)
					{
						return 1;
					}

					return 1;
				}
			}

			if(exam_dir == 0)
			{

				if(processed_directory(path, str) != 0)
				{
					if(free_list_error(namelist, count_files, i) != 0)
					{
						return 1;
					}

					return 1;
				}

			}
		}

		free(namelist[i]);
	}

	free(namelist);

	return 0;
}

int output_processed_files(const char *file_name, const char *str)
{
	int result_func = exam_file(file_name);

	if(result_func == 0)
	{
		if(str == NULL)
		{
			if(header(file_name) != 0)
			{
				return 1;
			}
		}

		if(processed_directory(file_name, str) != 0)
		{
			return 1;
		}
	}
	else if(result_func == -1)
	{
		if(header(file_name) != 0)
		{
			return 1;
		}
	}
	else
	{
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	char *search_str = NULL;

	if(argc == 1)
	{
		if(output_processed_files(".", search_str) != 0)
		{
			return 1;
		}

		return 0;
	}

	char *options[argc - 1];
	size_t index = 0;

	for(size_t i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
		{
			if(!strcmp(argv[i], "-name"))
			{
				if(argv[i + 1] == NULL)
				{
					fprintf(stderr, "find: missing argument to '%s'\n", argv[i]);
					return 1;
				}

				search_str = argv[i + 1];

				i += 1;
			}
			else
			{
				fprintf(stderr, "find: unknown predicate '%s'\n", argv[i]);
				return 1;
			}
		}
		else
		{
			options[index] = argv[i];

			index += 1;
		}
	}

	for(size_t i = 0; i < index; i++)
	{
		if(output_processed_files(options[i], search_str) != 0)
		{
			return 1;
		}
	}


	return 0;
}
