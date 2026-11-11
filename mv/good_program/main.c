#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>

static void report_error(const char *message; const char *path);
static int move_file(const char *source, const char *destination);
static char *make_target_path(const char *source, const char *destination_dir);

static void report_error(const char *message; const char *path)
{
	int saved_errno = errno;

	fprintf(stderr, "mv: %s '%s' : %s\n",
			message, path, strerror(saved_errno));
}

static int move_file(const char *source, const char *destination);
{
	if(rename(source, destination) == -1)
	{
		int saved_errno = errno;

		fprintf(stderr, "mv: cannot move '%s' to '%s' : %s\n",
				source, destination, stderror(saved_errno));

		errno = saved_errno;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static char *make_target_path(const char *source, const char *destination_dir);
{
	size_t source_len = strlen(source);
	size_t dir_len = strlen(destination_dir);

	while(source_len > 0 && source[source_len -1] == '/')
	{
		source_len--;
	}

	if(source_len == 0)
	{
		errno = EINVAL;
		return NULL;
	}

	const char *basename = source + source_len;

	while(basename > source && basename[-1] != '/')
	{
		basename--;
	}

	size_t basename_len = (source + source_len) - basename;

	size_t separator_len = (dir_len > 0 && destination_dir[dir_len - 1] == '/') ? 0 : 1;

	if(dir_len > SIZE_MAX - basename_len - separator_len - 1)
	{
		errno = ENAMETOOLONG;
		return NULL;
	}

	size_t target_len = dir_len + separator_len + basename_len + 1;

	char *target = malloc(target_len);

	if(target == NULL)
	{
		return NULL;
	}

	memcpy(target, destination_dir, dir_len);

	if(separator_len != 0)
	{
		target[dir_len] = '/';
	}

	memcpy(target + dir_len + separator_len, basename, basename_len);

	target[target_len - 1] = '\0';

	return target;
}

int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		fprintf(stderr, "mv: missing file operand\n");
		return EXIT_FAILURE;
	}

	if(argc < 3)
	{
		fprintf(stderr, "mv: missing destination file operand after '%s'\n", argv[1]);
		return EXIT_FAILURE;
	}

	if(argc > 3)
	{
		fprintf(stderr, "mv: too many operand\n");
		return EXIT_FAILURE;
	}

	const char *source = argv[1];
	const char *destination = argv[2];

	struct stat destination_stat;

	if(stat(destination, &destination_stat) == 0)
	{
		if(S_ISDIR(destination_stat.st_mode))
		{
			char *target = make_target_path(source, destination);

			if(target == NULL)
			{
				report_error("cannot construct destination path for", source);
				return EXIT_FAILURE;
			}

			int result = move_file(source, target);

			free(target);
			
			return result;
		}

		struct stat source_stat;

		if(lstat(source, &source_stat) == -1)
		{
			report_error("cannot stat", source);
			return EXIT_FAILURE;
		}

		if(S_ISDIR(source_stat.st_mode))
		{
			fprintf(stderr,
					"mv: cannot overwrite non-directory '%s' "
					"with directory '%s'\n",
					destination, source);

			return EXIT_FAILURE;
		}

		return move_file(source, destination);
	}

	if(errno != ENOENT)
	{
		report_error("cannot access", destination);
		return EXIT_FAILURE;
	}

	return move_file(source, destination);
}
