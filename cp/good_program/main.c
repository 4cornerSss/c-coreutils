#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUFFER_SIZE 65536

static int copy_entry(const char *source, const char *destination, int recursive);
static int copy_file(const char *source, const char *destination);
static int copy_directory(const char *source, const char *destination);
static char *join_path(const char *directory, const char *name);

static char *join_path(const char *directory, const char *name)
{
	size_t dir_len = strlen(directory);
	size_t name_len = strlen(name);

	int separator = (dir_len > 0 && directory[dir_len - 1] != '/');

	if(dir_len > SIZE_MAX - name_len - (size_t) separator - 1)
	{
		errno = ENAMETOOLONG;
		return NULL;
	}

	size_t total_len = dir_len + name_len + separator + 1;

	char *path = malloc(total_len);

	if(path == NULL)
	{
		return NULL;
	}

	snprintf(path, total_len, "%s%s%s", directory, separator ? "/" : "", name);

	return path;
}

static int copy_file(const char *source, const char *destination)
{
	int src_fd = -1;
	int dst_fd = -1;
	int result = EXIT_FAILURE;

	char buffer[BUFFER_SIZE];

	struct stat src_stat;
	struct stat dst_stat;

	src_fd = open(source, O_RDONLY);

	if(src_fd == -1)
	{
		perror(source);
		goto cleanup;
	}

	if(fstat(src_fd, &src_stat) == -1)
	{
		perror(source);
		goto cleanup;
	}

	if(!S_ISREG(src_stat.st_mode))
	{
		fprintf(stderr, "cp: '%s' is not a regular file\n", source);
		goto cleanup;
	}

	dst_fd = open(destination, O_WRONLY | O_CREAT, 0666);

	if(dst_fd == -1)
	{
		perror(destination);
		goto cleanup;
	}

	if(fstat(dst_fd, &dst_stat) == -1)
	{
		perror(destination);
		goto cleanup;
	}

	if(src_stat.st_dev == dst_stat.st_dev &&
			src_stat.st_dev == dst_stat.st_ino)
	{
		fprintf(stderr, "cp: '%s' and '%s' are the same file\n",
				source, destination);
		goto cleanup;
	}

	if(ftruncate(dst_fd, 0) == -1)
	{
		perror(destination);
		goto cleanup;
	}

	for(;;)
	{
		ssize_t bytes_read = read(src_fd, buffer, sizeof(buffer));

		if(bytes_read == -1)
		{
			if(errno == EINTR)
			{
				continue;
			}

			perror(source);
			goto cleanup;
		}

		if(bytes_read == 0)
		{
			break;
		}

		ssize_t total_written = 0;

		while(total_written < bytes_read)
		{
			ssize_t bytes_written = write(dst_fd, buffer + total_written, (size_t)(bytes_read - total_written));
			
			if(bytes_written == -1)
			{
				if(errno == EINTR)
				{
					continue;
				}

				perror(destination);
				goto cleanup;
			}

			if(bytes_written == 0)
			{
				fprintf(stderr, "cp: write returned zero\n");
				goto cleanup;
			}

			total_written += bytes_written;
		}
	}

	result = EXIT_SUCCESS;

cleanup:

	if(src_fd != -1 && close(src_fd) == -1)
	{
		perror(source);
		result = EXIT_FAILURE;
	}

	if(dst_fd != -1 && close(dst_fd) == -1)
	{
		perror(destination);
		result = EXIT_FAILURE;
	}

	return result;
}

static int copy_directory(const char *source, const char *destination)
{
	int result = EXIT_FAILURE;

	DIR *dir = NULL;
	struct dirent *entry;

	struct stat src_stat;
	struct stat dst_stat;

	char *src_path = NULL;
	char *dst_path = NULL;

	char *real_source = NULL;
	char *real_destination = NULL;

	if(stat(source, &src_stat) == -1)
	{
		perror(source);
		goto cleanup;
	}

	if(mkdir(destination, src_stat.st_mode & 0777) == -1)
	{
		if(errno != EEXIST)
		{
			perror(destination);
			goto cleanup;
		}
	}

	if(stat(destination, &dst_stat) == -1)
	{
		perror(destination);
		goto cleanup;
	}

	if(!S_ISDIR(dst_stat.st_mode))
	{
		fprintf(stderr, "cp: '%s' is not a directory\n", destination);
		goto cleanup;
	}

	real_source = realpath(source, NULL);
	real_destination = realpath(destination, NULL);

	if(real_source == NULL || real_destination == NULL)
	{
		perror("cp: realpath");
		goto cleanup;
	}

	size_t source_len = strlen(real_source);

	if(strncmp(real_source, real_destination, source_len) == 0 &&
			(real_destination[source_len] == '\0' ||
			 real_destination[source_len] == '/'))
	{
		fprintf(stderr, "cp: cannot copy a directory into itself\n");
		goto cleanup;
	}

	dir = opendir(source);

	if(dir == NULL)
	{
		perror(source);
		goto cleanup;
	}

	for(;;)
	{
		errno = 0;
		entry = readdir(dir);

		if(entry == NULL)
		{
			if(errno != 0)
			{
				perror(source);
				goto cleanup;
			}

			break;
		}

		if(strcmp(entry->d_name, ".") == 0 ||
				strcmp(entry->d_name, "..") == 0)
		{
			continue;
		}

		src_path = join_path(source, entry->d_name);
		dst_path = join_path(destination, entry->d_name);

		if(src_path == NULL || dst_path == NULL)
		{
			perror("cp: join_path");
			goto cleanup;
		}

		if(copy_entry(src_path, dst_path, 1) != EXIT_SUCCESS)
		{
			goto cleanup;
		}

		free(src_path);
		free(dst_path);

		src_path = NULL;
		dst_path = NULL;
	}

	result = EXIT_SUCCESS;

cleanup:
	
	free(src_path);
	free(dst_path);

	free(real_source);
	free(real_destination);

	if(dir != NULL && closedir(dir) == -1)
	{
		perror(source);
		result = EXIT_FAILURE;
	}

	return result;
}

static int copy_entry(const char *source, const char *destination, int recursive)
{
	struct stat src_stat;

	if(lstat(source, &src_stat) == -1)
	{
		perror(source);
		return EXIT_FAILURE;
	}

	if(S_ISDIR(src_stat.st_mode))
	{
		if(!recursive)
		{
			fprintf(stderr, "cp: -r not specified; omitting directory '%s'\n", source);
			return EXIT_FAILURE;
		}

		return copy_directory(source, destination);
	}

	if(S_ISREG(src_stat.st_mode))
	{
		return copy_file(source, destination);
	}

	fprintf(stderr, "cp: unsupported file type: '%s'\n", source);

	return EXIT_FAILURE;
}

int main(int argc, char *argv[])
{
	int recursive = 0;
	int source_index = 1;

	if(argc > 1 && strcmp(argv[1], "-r") == 0)
	{
		recursive = 1;
		source_index = 2;
	}

	if(argc - source_index < 2)
	{
		fprintf(stderr, "cp: missing file operand\n");
		return EXIT_FAILURE;
	}

	if(argc - source_index > 2)
	{
		fprintf(stderr, "cp: too many arguments\n");
		return EXIT_FAILURE;
	}

	return copy_entry(argv[source_index], argv[source_index + 1], recursive);
}
