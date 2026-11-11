#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fnmatch.h>

struct find_options
{
	const char *name_pattern;
};

static int write_all(int fd, const char *buffer, size_t size);
static int print_path(const char *path);
static void report_error(const char *path, int error_cide);
static char *join_path(const char *directrory, const char *name);
static int name_matches(const char *name, const struct find_options *options);
static const char *get_basename(const char *path);
static int walk_path(const char *path, const struct find_options *options);

static int write_all(int fd, const char *buffer, size_t size)
{
	size_t written = 0;

	while(written < size)
	{
		ssize_t result = write(fd, buffer + written, size - written);

		if(result < 0)
		{
			if(errno == EINTR)
			{
				continue;
			}

			return -1;
		}

		if(result == 0)
		{
			errno = EIO;
			return -1;
		}

		written += (size_t)result;
	}

	return 0;
}

static int print_path(const char *path)
{
	if(write_all(STDOUT_FILENO, path, strlen(path)) == -1)
	{
		return -1;
	}

	if(write_all(STDOUT_FILENO, "\n", 1) == -1)
	{
		return -1;
	}

	return 0;
}

static char *join_path(const char *directory, const char *name)
{
	size_t directory_length = strlen(directory);
	size_t name_length = strlen(name);

	int needs_slash = directory_length > 0 && directory[directory_length - 1] != '/';

	if(name_length > SIZE_MAX - directory_length - 2)
	{
		errno = ENOMEM;
		return NULL;
	}

	size_t total_size = directory_length + name_length + needs_slash + 1;

	char *result = malloc(total_size);

	if(result == NULL)
	{
		return NULL;
	}

	memcpy(result, directory, directory_length);

	size_t offset = directory_length;

	if(needs_slash)
	{
		result[offset++] = '/';
	}

	memcpy(result + offset, name, name_length);
	result[offset + name_length] = '\0';

	return result;
}

static void report_error(const char *path, int error_code)
{
	fprintf(stderr, "find: '%s': %s\n", path, strerror(error_code));
}

static const char *get_basename(const char *path)
{
	const char *slash = strrchr(path, '/');

	return slash != NULL ? slash + 1 : path;
}

static int name_matches(const char *name, const struct find_options *options)
{
	if(options->name_pattern == NULL)
	{
		return 1;
	}

	return fnmatch(options->name_pattern, name, 0) == 0;
}


static int walk_path(const char *path, const struct find_options *options)
{
	int status = 0;
	struct stat file_info;

	if(lstat(path, &file_info) == -1)
	{
		int error_code = errno;
		report_error(path, error_code);
		return 1;
	}

	if(name_matches(get_basename(path), options))
	{
		if(print_path(path) == -1)
		{
			int error_code = errno;
			report_error("standard output", error_code);
			return 1;
		}
	}

	if(!S_ISDIR(file_info.st_mode))
	{
		return 0;
	}

	DIR *directory = opendir(path);

	if(directory == NULL)
	{
		int error_code = errno;
		report_error(path, error_code);
		return 1;
	}

	errno = 0;

	struct dirent *entry;

	while((entry = readdir(directory)) != NULL)
	{
		if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
		{
			continue;
		}

		char *child_path = join_path(path, entry->d_name);

		if(child_path == NULL)
		{
			int error_code = errno;
			report_error(path, error_code);
			status = 1;
			continue;
		}

		if(walk_path(child_path, options) != 0)
		{
			status = 1;
		}

		free(child_path);

		errno = 0;
	}

	if(errno != 0)
	{
		int error_code = errno;
		report_error(path, error_code);
		status = 1;
	}

	if(closedir(directory) == -1)
	{
		int error_code = errno;
		report_error(path, error_code);
		status = 1;
	}

	return status;
}

int main(int argc, char *argv[])
{
	struct find_options options = {.name_pattern = NULL};

	int predicate_index = argc;

	for(int i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "-name") == 0)
		{
			if(i + 1 >= argc)
			{
				fprintf(stderr, "find: missing argument to '-name'\n");
				return EXIT_FAILURE;
			}

			options.name_pattern = argv[i + 1];
			predicate_index = i;
			break;
		}

		if(argv[i][0] == '-')
		{
			fprintf(stderr, "find: unknown predicate '%s'\n",argv[i]);
			return EXIT_FAILURE;
		}
	}

	int status = 0;

	if(predicate_index == 1)
	{
		return walk_path(".", &options) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	for(int i = 1; i < predicate_index; i++)
	{
		if(walk_path(argv[i], &options) != 0)
		{
			status = 1;
		}
	}

	return status == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
