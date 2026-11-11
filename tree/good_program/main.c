#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <dirent.h>
#include <sys/stat.h>

static void free_entries(struct dirent **entries, int count);
static char *join_paths(const char *directory, const char *name);
static int print_entry(const char *path, size_t depth);
static int report_error(const char *path);
static int walk_directory(const char *path, size_t depth);

static void free_entries(struct dirent **entries, int count)
{
	for(int i = 0; i < count; i++)
	{
		free(entries[i]);
	}

	free(entries);
}

static char *join_paths(const char *directory, const char *name)
{
	size_t directory_length = strlen(directory);
	size_t name_length = strlen(name);

	size_t separator = directory[directory_length - 1] != '/';

	if(directory_length > SIZE_MAX - separator - 1)
	{
		errno = EOVERFLOW;
		return NULL;
	}

	size_t required_size = directory_length + separator + 1;

	if(name_length > SIZE_MAX - required_size)
	{
		errno = EOVERFLOW;
		return NULL;
	}

	required_size += name_length;

	char *path = malloc(required_size);

	if(path == NULL)
	{
		return NULL;
	}

	memcpy(path, directory, directory_length);

	if(separator)
	{
		path[directory_length] = '/';
	}

	memcpy(path + directory_length + separator, name, name_length + 1);

	return path;
}

static int print_entry(const char *path, size_t depth)
{
	for(size_t i = 0; i < depth; i++)
	{
		if(fputc('\t', stdout) == EOF)
		{
			return -1;
		}
	}

	if(fprintf(stdout, "%s\n", path) < 0)
	{
		return -1;
	}

	return 0;
}

static int report_error(const char *path)
{
	int error = errno;

	fprintf(stderr, "tree: '%s': %s\n", path, strerror(error));

	return -1;
}

static int walk_directory(const char *path, size_t depth)
{
	struct stat file_info;

	if(lstat(path, &file_info) == -1)
	{
		return report_error(path);
	}

	if(!S_ISDIR(file_info.st_mode))
	{
		if(print_entry(path, depth) == -1)
		{
			return report_error("stdout");
		}

		return 0;
	}

	struct dirent **entries = NULL;

	int count = scandir(path, &entries, NULL, alphasort);

	if(count == -1)
	{
		return report_error(path);
	}

	if(print_entry(path, depth) == -1)
	{
		free_entries(entries, count);
		return report_error("stdout");
	}

	if(depth == SIZE_MAX)
	{
		free_entries(entries, count);
		errno = EOVERFLOW;
		return report_error(path);
	}

	int result = 0;

	for(int i = 0; i < count; i++)
	{
		const char *name = entries[i]->d_name;

		if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		{
			continue;
		}

		char *child_path = join_paths(path, name);

		if(child_path == NULL)
		{
			result = report_error(path);
			break;
		}

		if(walk_directory(child_path, depth + 1) == -1)
		{
			free(child_path);
			result = -1;
			break;
		}

		free(child_path);
	}

	free_entries(entries, count);

	return result;
}

int main(int argc, char *argv[])
{
	if(argc == 1)
	{
		if(walk_directory(".", 0) == -1)
		{
			return EXIT_FAILURE;
		}
	}
	else
	{
		for(int i = 1; i < argc; i++)
		{
			if(walk_directory(argv[i], 0) == -1)
			{
				return EXIT_FAILURE;
			}
		}
	}

	if(fflush(stdout) == EOF)
	{
		perror("tree: stdout");
		return EXIT_FAILURE;
	}

	return EXIT_FAILURE;
}
