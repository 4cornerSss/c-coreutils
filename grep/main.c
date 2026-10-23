#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

char *exam_str(char *str_read, int size_read, const char *str_in)
{
	int size_in = strlen(str_in);

	if(size_read < size_in)
	{
		return NULL;
	}

	for(size_t i = 0; i < size_read; i++)
	{
		int num = 0;

		if(size_read - i >= size_in)
		{
			int status = 0;

			if(str_read[i] == str_in[0])
			{
				for(size_t k = 0; k < size_in; k++)
				{
					if(str_read[i + k] == str_in[k])
					{
						num += 1;
					}
					else
					{
						break;
					}
				}
			}

			if(num == size_in)
			{
				return str_read;
			}
		}
		else
		{
			return NULL;
		}

	}

	return NULL;
}

size_t processed_input(FILE *file, char *buffer, int size_sym, int size)
{
	size_t count_read = fread(buffer, size_sym, size, file);

	if(count_read == 0)
	{
		return 0;
	}

	return count_read;
}

int processed_output(FILE *file, const char *str, int size_sym, size_t count_read)
{
	size_t offset = 0;

	while(count_read != 0)
	{
		size_t count_write = fwrite(str + offset, size_sym, count_read, file);

		if(count_write == 0)
		{
			perror("grep");
			return 1;
		}
		
		offset += count_write;
		count_read -= count_write;
	}

	return 0;
}

int processed_in_out(FILE *file_in, FILE *file_out, const char *str_in)
{
	size_t count_in;

	char buffer[1024];

	int size_sym = sizeof(buffer[0]);
	int size = sizeof(buffer) / sizeof(buffer[0]);

	while((count_in = processed_input(file_in, buffer, size_sym, size)) != 0)
	{
		if(str_in != NULL)
		{
			char *buf;
			int num = 0;

			for(int i = 0; i < count_in; i++)
			{
				if(buffer[i] == '\n')
				{
					if((buf = exam_str(buffer + num, i - num, str_in)) != NULL)
					{
						int size_buf_sym = sizeof(buf[0]);
						int size_buf = i - num + 1;

						if(processed_output(file_out, buf, size_buf_sym, size_buf) != 0)
						{
							return 1;
						}
					}
					
					num = i + 1;
				}

			}
		}
		else
		{
			if(processed_output(file_out, buffer, size_sym, count_in) != 0)
			{
				return 1;
			}
		}
	}

	if(feof(file_in))
	{
		return 0;
	}
	else if(ferror(file_in))
	{
		perror("grep");
		return 1;
	}

	return 0;

}

int exam_file(const char *file_name)
{
	struct stat st;

	if(stat(file_name, &st) != 0)
	{
		if(errno == ENOENT)
		{
			fprintf(stderr, "grep: %s: No such file or directory", file_name);
			return 1;
		}
		else
		{
			perror("grep");
			return 1;
		}
	}

	return 0;
}

int processed_str_file(const char *str, const char *file_name)
{
	if(exam_file(file_name) != 0)
	{
		return 1;
	}

	FILE *file = fopen(file_name, "r");

	if(file == NULL)
	{
		perror("grep");
		return 1;
	}

	if(processed_in_out(file, stdout, str) != 0)
	{
		if(fclose(file) != 0)
		{
			perror("grep");
			return 1;
		}

		return 1;
	}

	if(fclose(file) != 0)
	{
		perror("grep");
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	char *option = NULL;
	char *paterns = NULL;
	char *file = NULL;

	int part_paterns = 1;
	int part_file = 2;

	if(argv[part_paterns] == NULL)
	{
		fprintf(stderr, "Usage: grep [OPTION]... PATTERNS [FILE]...\n");
		return 1;	
	}
	else
	{
		if(argv[part_paterns][0] == '-')
		{
			int part_option = 1;

			if(!strcmp(argv[part_option], "-v"))
			{
				option = argv[part_option];
				if((paterns = argv[part_paterns + 1]) == NULL)
				{
					fprintf(stderr, "grep: option requires an argument -- '%s'\n", argv[part_option]);
					fprintf(stderr, "Usage: grep [OPTION]... PATTERNS [FILE]...\n");
					return 1;
				}

				part_file += 1;
			}
			else
			{
				fprintf(stderr, "grep: invalid option -- '%s'\n", argv[part_option]);
				fprintf(stderr, "Usage: grep [OPTION]... PATTERNS [FILE]...\n");
				return 1;
			}
		}
		else
		{
			paterns = argv[part_paterns];
		}
	}

	if(argv[part_file] == NULL)
	{
		if(processed_in_out(stdin, stdout, NULL) != 0)
		{
			return 1;
		}
	}
	else
	{
		file = argv[part_file];

		if(processed_str_file(paterns, file) != 0)
		{
			return 1;
		}
	}


	return 0;
}
