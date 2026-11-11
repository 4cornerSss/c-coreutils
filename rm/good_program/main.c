#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define PROGRAM_NAME "rm"

enum
{
	FLAG_FORCE = 1,
	FLAG_RECURSIVE = 2
};

static int remove_entry(int parent_fd, const char *name, int flags);

static int report_error(const char *name, int flags, int ignore_missing)
{
	int saved_errno = errno;

	if(!(ignore_missing && (flags & FLAG_FORCE) && saved_errno == ENOENT))
	{
		errno = saved_errno;
		perror(name);
	}

	errno = saved_errno;

	if(ignore_missing && (flags & FLAG_FORCE) && saved_errno == ENOENT)
	{
		return 0;
	}

	return -1;
}

static int remove_directory(int parent_fd, const char *name, int flags)
{
	int fd = openat(parent_fd, name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);

	if(fd == -1)
	{
		return report_error(name, flags, 1);
	}

	DIR *dir = fdopendir(fd);

	if(dir == NULL)
	{
		int saved_errno = errno;
		close(fd);
		errno = saved_errno;
		return report_error(name, flags, 0);
	}

	int status = 0;
	struct dirent *entry;

	for(;;)
	{
		errno = 0;
		entry = readdir(dir);

		if(entry == NULL)
		{
			if(errno != 0)
			{
				status = report_error(name, flags, 0);
			}
			break;
		}

		if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
		{
			continue;
		}

		if(remove_entry(dirfd(dir), entry->d_name, flags) != 0)
		{
			status = -1;
		}
	}

	if(closedir(dir) == -1)
	{
		report_error(name, flags, 0);
		status = -1;
	}

	if(unlinkat(parent_fd, name, AT_REMOVEDIR) == -1)
	{
		report_error(name, flags, 0);
		status = -1;
	}

	return status;
}

static int remove_entry(int parent_fd, const char *name, int flags)
{
	struct stat sb;

	if(fstatat(parent_fd, name, &sb, AT_SYMLINK_NOFOLLOW) == -1)
	{
		return report_error(name, flags, 1);
	}

	if(S_ISDIR(sb.st_mode))
	{
		if(!(flags & FLAG_RECURSIVE))
		{
			fprintf(stderr, "%s: cannot remove '%s': is a directory\n", PROGRAM_NAME, name);
			return -1;
		}

		return remove_directory(parent_fd, name, flags);
	}

	if(unlinkat(parent_fd, name, 0) == -1)
	{
		return report_error(name, flags, 0);
	}

	return 0;
}

static int parse_options(int argc, char *argv[], int *flags)
{
	int i = 1;

	while(i < argc)
	{
		if(strcmp(argv[i], "--") == 0)
		{
			return i + 1;
		}

		if(argv[i][0] != '-' || argv[i][1] == '\0')
		{
			break;
		}

		for(const char *option = argv[i] + 1; *option; option++)
		{
			switch(*option)
			{
				case 'f':
					*flags |= FLAG_FORCE;
					break;
				case 'r':
				case 'R':
					*flags |= FLAG_RECURSIVE;
					break;
				default:
					fprintf(stderr, "%s: invalid option -- '%c'\n", PROGRAM_NAME, *option);
					return -1;
			}
		}

		i++;
	}

	return i;
}

int main(int argc, char *argv[])
{
	int flags = 0;
	int first_operand = parse_options(argc, argv, &flags);

	if(first_operand == -1)
	{
		return EXIT_FAILURE;
	}

	if(first_operand == argc)
	{
		if(!(flags & FLAG_FORCE))
		{
			fprintf(stderr, "%s: missing operand\n", PROGRAM_NAME);
			return EXIT_FAILURE;
		}

		return EXIT_SUCCESS;
	}

	int status = EXIT_SUCCESS;

	for(int i = first_operand; i < argc; i++)
	{
		if(remove_entry(AT_FDCWD, argv[i], flags) != 0)
		{
			status = EXIT_FAILURE;
		}
	}

	return status;
}
