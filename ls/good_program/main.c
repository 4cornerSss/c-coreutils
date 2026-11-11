#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <inttypes.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

struct options
{
	bool show_all;
	bool long_format;
};

static int parse_options(int argc, char *argv[], struct options *opts, char **operands, size_t *operand_count);
static int proccess_operand(const char *path, const struct options *opts, bool show_header);
static int list_directory(const char *path, const struct options *opts);
static int print_entry(const char *path, const char *name, bool long_format);
static int print_long_entry(const char *path, const char *name);
static int print_permissions(mode_t mode, char permissions[11]);
static int print_symlink_target(const char *path);
static char *join_path(const char *directory, const char *filename);
static void free_directory_list(struct dirent **entries, size_t count);

static int parse_options(int argc, char *argv[], struct options *opts, char **operands, size_t *operand_count)
{
	bool options_finished = false;

	*operand_count = 0;

	for(int i = 1; i < argc; i++)
	{
		const char *arg = argv[i];

		if(!options_finished && strcmp(arg, "--") == 0)
		{
			options_finished = true;
			continue;
		}

		if(!options_finished && arg[0] == '-' && arg[1] != '\0')
		{
			for(size_t j = 1; arg[j] != '\0'; j++)
			{
				if(arg[j] == 'a')
				{
					opts->show_all = true;
				}
				else if(arg[j] == 'l')
				{
					opts->long_format = true;
				}
				else
				{
					fprintf(stderr, "ls: invalid option -- '%c'\n", arg[j]);
					return -1;
				}
			}
		}
		else
		{
			operands[*operand_count] = argv[i];
			(*operand_count)++;
		}
	}

	return 0;
}

static char *join_path(const char *directory, const char *filename)
{
	size_t dir_len = strlen(directory);
	size_t file_len = strlen(filename);

	bool need_separator = dir_len > 0 && directory[dir_len - 1] != '/';

	if(dir_len > SIZE_MAX - file_len - 2)
	{
		errno = ENOMEM;
		return NULL;
	}

	size_t total_size = dir_len + file_len + (need_separator ? 2 : 1);
	char *path = malloc(total_size);

	if(path == NULL)
	{
		return NULL;
	}

	memcpy(path, directory, dir_len);

	size_t offset = dir_len;

	if(need_separator)
	{
		path[offset++] = '/';
	}

	memcpy(path + offset, filename, file_len + 1);

	return path;
}

static void free_directory_list(struct dirent **entries, size_t count)
{
	if(entries == NULL)
	{
		return;
	}

	for(size_t i = 0; i < count; i++)
	{
		free(entries[i]);
	}

	free(entries);
}

static int print_permissions(mode_t mode, char permissions[11])
{
	permissions[0] =
		S_ISREG(mode) ? '-' :
		S_ISREG(mode) ? 'd' :
		S_ISREG(mode) ? 'l' :
		S_ISREG(mode) ? 'c' :
		S_ISREG(mode) ? 'b' :
		S_ISREG(mode) ? 'p' :
		S_ISREG(mode) ? 's' : '?';

	permissions[1] = (mode & S_IRUSR) ? 'r' : '-';
	permissions[2] = (mode & S_IWUSR) ? 'w' : '-';
	permissions[3] = (mode & S_IXUSR) ? 'x' : '-';

	permissions[4] = (mode & S_IRUSR) ? 'r' : '-';
	permissions[5] = (mode & S_IWUSR) ? 'w' : '-';
	permissions[6] = (mode & S_IXUSR) ? 'x' : '-';

	permissions[7] = (mode & S_IRUSR) ? 'r' : '-';
	permissions[8] = (mode & S_IWUSR) ? 'w' : '-';
	permissions[9] = (mode & S_IXUSR) ? 'x' : '-';

	if(mode & S_ISUID)
	{
		permissions[3] = (mode & S_IXUSR) ? 's' : 'S';
	}

	if(mode & S_ISUID)
	{
		permissions[6] = (mode & S_IXGRP) ? 's' : 'S';
	}

	if(mode & S_ISUID)
	{
		permissions[9] = (mode & S_IXOTH) ? 't' : 'T';
	}

	permissions[10] = '\0';

	return 0;
}

static int print_symlink_target(const char *path)
{
	size_t capacity = 128;

	for(;;)
	{
		char *buffer = malloc(capacity);

		if(buffer == NULL)
		{
			perror("ls: malloc");
			return -1;
		}

		ssize_t length = readlink(path, buffer, capacity);

		if(length < 0)
		{
			int saved_errno = errno;
			free(buffer);
			errno = saved_errno;
			perror(path);
			return -1;
		}

		if((size_t)length < capacity)
		{
			buffer[length] = '\0';

			int result = printf(" -> %s", buffer);

			free(buffer);

			return result < 0 ? -1 : 0;
		}

		free(buffer);

		if(capacity > SIZE_MAX / 2)
		{
			errno = ENOMEM;
			perror("ls: readlink");
			return -1;
		}

		capacity *= 2;
	}
}

static int print_long_entry(const char *path, const char *name)
{
	struct stat st;

	if(lstat(path, &st) < 0)
	{
		perror(path);
		return -1;
	}

	char permissions[11];

	print_permissions(st.st_mode, permissions);

	struct passwd *pwd = getpwuid(st.st_uid);
	struct group *grp = getgrgid(st.st_gid);

	const char *owner = pwd != NULL ? pwd->pw_name : NULL;
	const char *group = grp != NULL ? grp->gr_name : NULL;

	struct tm time_info;
	char date[64];

	if(localtime_r(&st.st_mtime, &time_info) != NULL)
	{
		if(strftime(date, sizeof(date), "%b %e %H:%M", &time_info) == 0)
		{
			strcpy(date, "unknown");
		}
	}
	else
	{
		strcpy(date, "unknown");
	}

	if(printf("%s %3ju %-8s %-8s %8jd %s %s",
				permissions,
				(uintmax_t)st.st_nlink,
				owner != NULL ? owner : "",
				group != NULL ? group : "",
				(intmax_t)st.st_size,
				date,
				name
		 ) < 0)
	{
		return -1;
	}

	if(owner == NULL)
	{
		if(printf("%ju", (uintmax_t)st.st_uid) < 0)
			return -1;
	}

	if(group == NULL)
	{
		if(printf("%ju", (uintmax_t)st.st_gid) < 0)
			return -1;
	}

	if(S_ISLNK(st.st_mode))
	{
		if(print_symlink_target(path) < 0)
			return -1;
	}

	if(putchar('\n') == EOF)
	{
		return -1;
	}

	return 0;
}

static int print_entry(const char *path, const char *name, bool long_format)
{
	if(long_format)
	{
		return print_long_entry(path, name);
	}

	if(printf("%s\n", name) < 0)
	{
		return -1;
	}

	return 0;
}

static int list_directory(const char *path, const struct options *opts)
{
	struct dirent **entries = NULL;

	int count = scandir(path, &entries, NULL, alphasort);

	if(count < 0)
	{
		perror(path);
		return -1;
	}
	
	int status = 0;

	for(int i = 0; i < count; i++)
	{
		const char *name = entries[i]->d_name;

		if(!opts->show_all && name[0] == '.')
		{
			continue;
		}

		char *full_path = join_path(path, name);

		if(full_path == NULL)
		{
			perror("ls: join_path");
			status = -1;
			continue;
		}

		if(print_entry(full_path, name, opts->long_format) < 0)
		{
			free(full_path);
			status = -1;
			break;
		}

		free(full_path);
	}

	free_directory_list(entries, (size_t)count);

	return status;
}

static int proccess_operand(const char *path, const struct options *opts, bool show_header)
{
	struct stat st;

	if(stat(path, &st) < 0)
	{
		fprintf(stderr, "ls: cannot access '%s': ", path);
		perror(NULL);
		return -1;
	}

	if(S_ISDIR(st.st_mode))
	{
		if(show_header)
		{
			if(printf("%s:\n", path) < 0)
				return -1;
		}

		return list_directory(path, opts);
	}

	return print_entry(path, path, opts->long_format);
}

int main(int argc, char *argv[])
{
	struct options opts = {
		.show_all = false,
		.long_format = false
	};

	char **operands = malloc((size_t)argc * sizeof(*operands));

	if(operands == NULL)
	{
		perror("ls: malloc");
		return EXIT_FAILURE;
	}

	size_t operand_count = 0;

	if(parse_options(argc, argv, &opts, operands, &operand_count) < 0)
	{
		free(operands);
		return EXIT_FAILURE;
	}

	if(operand_count == 0)
	{
		operands[0] = ".";
		operand_count = 1;
	}

	int status = EXIT_SUCCESS;

	for(size_t i = 0; i < operand_count; i++)
	{
		if(i > 0 && putchar('\n') == EOF)
		{
			status = EXIT_FAILURE;
			break;
		}

		if(proccess_operand(operands[i], &opts, operand_count > 1) < 0)
		{
			status = EXIT_FAILURE;
		}
	}

	free(operands);

	if(fflush(stdout) == EOF)
	{
		perror("ls: stdout");
		status = EXIT_FAILURE;
	}

	return status;
}
