#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>

int copy_file(char *sour, char *dest)
{
	int count_read;
	int count_write;
	char buffer[1024];

	FILE *file_exist = fopen(sour, "rb");

	if(file_exist == NULL)
	{
		perror("cp");
		return EXIT_FAILURE;
	}

	FILE *file_new = fopen(dest, "wb");

	if(file_new == NULL)
	{
		fclose(file_exist);
		perror("cp");
		return EXIT_FAILURE;
	}

	count_read = fread(buffer, sizeof(buffer[0]), sizeof(buffer), file_exist);

	while(count_read > 0)
	{
		count_write = fwrite(buffer, sizeof(buffer[0]), count_read, file_new);

		if(count_write == count_read)
		{
			count_read = fread(buffer, sizeof(buffer[0]), sizeof(buffer), file_exist);
		}
		else
		{
			fclose(file_exist);
			fclose(file_new);
			perror("error");
			return EXIT_FAILURE;
		}

		if(ferror(file_exist))
		{
			fclose(file_exist);
			fclose(file_new);
			perror("cp");
			return EXIT_FAILURE;
		}
		else if(feof(file_exist))
		{
			break;
		}

	}

	fclose(file_exist);
	fclose(file_new);

	return EXIT_SUCCESS;
}

int copy_recursive(char *sour, char *dest)
{
	int result;

	struct stat st_s;
	struct stat st_d;

	if(stat(sour, &st_s) != 0)
	{
		if(errno == ENOENT)
		{
			fprintf(stderr, "cp: cannot stat '%s': No such file or directory\n", sour);
			return 1;
		}
		else
		{
			perror("cp");
			return EXIT_FAILURE;
		}

	}
	else
	{
		if(stat(dest, &st_d) == 0)
		{
			if(st_s.st_dev == st_d.st_dev && st_s.st_ino == st_d.st_ino)
			{
				fprintf(stderr, "cp: '%s' and '%s' are the same file \n", sour, dest);
				return 1;
			}
		}
		else
		{
			if(errno == ENOENT)
			{
				if(S_ISDIR(st_s.st_mode))
				{
					mkdir(dest, 0775);
				}
				else
				{
					result = copy_file(sour, dest);

					if(result == EXIT_FAILURE)
					{
						return EXIT_FAILURE;
					}
				}
			}
			else
			{
				perror("cp");
				return EXIT_FAILURE;
			}
		}
	}

	if(S_ISDIR(st_s.st_mode))
	{
		char full_path_s[1024];
		char full_path_d[1024];

		DIR *dir = opendir(sour);

		if(dir != NULL)
		{
			struct dirent *entry;
			struct stat st_fp;

			while((entry = readdir(dir)) != NULL)
			{
				if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
				{				
					sprintf(full_path_s, "%s/%s", sour, entry->d_name);
					if(stat(full_path_s, &st_fp) == 0)
					{
						if(S_ISDIR(st_fp.st_mode))
						{
							sprintf(full_path_d, "%s/%s", dest, entry->d_name);

							if(mkdir(full_path_d, 0755) == 0)
							{
								result = copy_recursive(full_path_s, full_path_d);

								if(result == EXIT_FAILURE)
								{
									return EXIT_FAILURE;
								}
							}
							else if(errno == EEXIST)
							{
								result = copy_recursive(full_path_s, full_path_d);

								if(result == EXIT_FAILURE)
								{
									return EXIT_FAILURE;
								}
							}
							else
							{
								perror("cp");
								return EXIT_FAILURE;
							}
						}
						else
						{
							sprintf(full_path_d, "%s/%s", dest, entry->d_name);
							result = copy_file(full_path_s, full_path_d);

							if(result == EXIT_FAILURE)
							{
								return EXIT_FAILURE;
							}
						}
					}		
				}
			}

			closedir(dir);
		}
		else
		{
			perror("cp");
			return EXIT_FAILURE;
		}
	}
	else
	{
		result = copy_file(sour, dest);

		if(result == EXIT_FAILURE)
		{
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	int result;
	int start_index = 1;
	int source = start_index;
	int destination = start_index + 1;
	int recursive = 0;

	struct stat st_s;

	if(argc < 2)
	{
		fprintf(stderr, "cp: missing file operand\n");
		return 1;
	}

	if(argv[1][0] == '-')
	{
		if(strcmp(argv[1], "-r") == 0)
		{
			recursive = 1;
			start_index = 2;
			source = start_index;
			destination = start_index + 1;
		}
		else
		{
			fprintf(stderr, "cp: invalid option -- '%s'\n", argv[1]);
			return EXIT_FAILURE;
		}
	}

	if(argv[source] == NULL)
	{
		fprintf(stderr, "cp: missing file operand\n");
		return 1;
	}
	
	if(argv[destination] == NULL)
	{
		fprintf(stderr, "cp: missing destination file operand after '%s'\n", argv[source]);
		return 1;
	}

	if(stat(argv[source], &st_s) != 0)
	{
		perror("cp");
		return EXIT_FAILURE;
	}

	if(S_ISDIR(st_s.st_mode) && !recursive)
	{
		fprintf(stderr, "cp: -r not specified; omitting directory '%s'\n", argv[source]);
		return 1;
	}
	else
	{
		result = copy_recursive(argv[source], argv[destination]);
	}

	if(result == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	return 0;
}
