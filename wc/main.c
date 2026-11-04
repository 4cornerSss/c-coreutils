#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>

#define size_buffer_bytes 1024
#define size_buffer_format 1024

ssize_t read_bytes(const int input_name, char *buffer, size_t size_buffer)
{
	ssize_t count_read_bytes = read(input_name, buffer, size_buffer);

	if(count_read_bytes < 0)
	{
		fprintf(stderr, "wc: Error reading bytes\n");
		return -1;
	}

	return count_read_bytes;
}

int write_bytes(const int output_name, char *buffer, ssize_t count_read_bytes)
{
	ssize_t count_bytes = count_read_bytes;
	size_t offset = 0;

	while(count_bytes != 0)
	{
		ssize_t count_write_bytes = write(output_name, buffer + offset, count_bytes);

		if(count_write_bytes <= 0)
		{
			fprintf(stderr, "wc: Error writing bytes\n");
			return 1;
		}

		count_bytes -= count_write_bytes;
		offset += count_write_bytes;
	}

	return 0;
}

int newline()
{
	char *newline = "\n";

	if(write_bytes(STDOUT_FILENO, newline, 1) != 0)
	{
		return 1;
	}
	
	return 0;
}

int processed_count_word(char *buffer_bytes, size_t count_read_bytes, int count)
{
	int word = 0;

	for(size_t i = 0; i < count_read_bytes; i++)
	{
		if(buffer_bytes[i] == ' ' || buffer_bytes[i] == '\n')
		{
			if(word)
			{
				count += 1;
				word = 0;
			}
		}
		else
		{
			word = 1;
		}

	}

	return count;
}

int processed_count_string(char *buffer_bytes, size_t count_read_bytes, int count)
{
	for(size_t i = 0; i < count_read_bytes; i++)
	{
		if(buffer_bytes[i] == '\n')
		{
			count += 1;
		}

	}

	return count;
}

int format_output_counts(int output_name, int strings, int words, int symbols, char *file_name)
{
	char buffer[size_buffer_format];
	size_t size_buffer = sizeof(buffer);

	int written = snprintf(buffer, size_buffer, "%5d\t%5d\t%5d\t%s", strings, words, symbols, file_name);

	if(written < 0)
	{
		perror("wc");
		return 1;
	}
	else if(written == 0)
	{
		return 0;
	}

	int count_write_bytes = write_bytes(output_name, buffer, strlen(buffer));

	if(count_write_bytes != 0)
	{
		return 1;
	}

	if(newline() != 0)
	{
		return 1;
	}

	return 0;
}

int processed_read_write_bytes(const int input_name, const int output_name, char *file_name)
{
	ssize_t count_read_bytes;
	int count_write_bytes;

	char buffer[size_buffer_bytes];
	size_t size_buffer = sizeof(buffer) / sizeof(*buffer);

	int count_string = 0;
	int count_word = 0;
	int count_symbol = 0;

	while((count_read_bytes = read_bytes(input_name, buffer, size_buffer)) != 0)
	{
		if(count_read_bytes < 0)
		{
			return 1;
		}

		count_string = processed_count_string(buffer, count_read_bytes, count_string);
		count_word = processed_count_word(buffer, count_read_bytes, count_word);
		count_symbol += count_read_bytes;
	}

	if(format_output_counts(output_name, count_string, count_word, count_symbol, file_name) != 0)
	{
		return 1;
	}

	return 0;
}

int processed_file(char *file_name)
{
	int fd_file = open(file_name, O_RDONLY);

	if(fd_file < 0)
	{
		perror("wc");
		return 1;
	}

	if(processed_read_write_bytes(fd_file, STDOUT_FILENO, file_name) != 0)
	{
		return 1;
	}

	close(fd_file);

	return 0;
}

int processed_exam_file(char *file_name)
{
	struct stat st_file;

	if(stat(file_name, &st_file) != 0)
	{
		if(errno == ENOENT)
		{
			fprintf(stderr, "wc: File does not exist\n");
			return 1;
		}
		else if(errno == EACCES)
		{
			fprintf(stderr, "wc: Permission denied\n");
			return 1;
		}
		else
		{
			perror("wc");
			return 1;
		}
	}
	else
	{
		if(S_ISDIR(st_file.st_mode))
		{
			fprintf(stderr, "wc: '%s' is a directory\n", file_name);
			return 1;
		}
		else
		{
			if(processed_file(file_name) != 0)
			{
				return 1;
			}
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	if(argc == 1)
	{
		if(processed_read_write_bytes(STDIN_FILENO, STDOUT_FILENO, "") != 0)
		{
			return 1;
		}
	}

	for(size_t i = 1; i < argc; i++)
	{
		if(processed_exam_file(argv[i]) != 0)
		{
			return 1;
		}
	}

	return 0;
}
