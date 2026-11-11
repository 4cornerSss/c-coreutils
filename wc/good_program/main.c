#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 16384

typedef struct
{
	uintmax_t lines;
	uintmax_t words;
	uintmax_t bytes;
	bool in_word;
} WcCounters;

static int write_all(int fd, const void *buffer, size_t size)
{
	const unsigned char *data = buffer;
	size_t offset = 0;

	while(offset < size)
	{
		ssize_t written = write(fd, data + offset, size - offset);

		if(written < 0)
		{
			if(errno == EINTR)
				continue;

			perror("wc: write");
			return -1;
		}

		if(written == 0)
		{
			errno  = EIO;
			perror("wc: write");
			return -1;
		}

		offset += (size_t)written;
	}

	return 0;
}

static int update_counters(WcCounters *counters, const unsigned char *buffer, size_t size)
{
	for(size_t i = 0; i < size; i++)
	{
		if(counters->bytes == UINTMAX_MAX)
		{
			fprintf(stderr, "wc: byte counter overflow\n");
			return -1;
		}

		counters->bytes++;

		if(buffer[i] == '\n')
		{
			if(counters->lines == UINTMAX_MAX)
			{
				fprintf(stderr, "wc: line counter overflow\n");
				return -1;
			}

			counters->lines++;
		}

		if(isspace(buffer[i]))
		{
			if(counters->in_word)
			{
				if(counters->words == UINTMAX_MAX)
				{
					fprintf(stderr, "wc: word counter overflow\n");
					return -1;
				}

				counters->words++;
				counters->in_word = false;
			}
		}
		else
		{
			counters->in_word = true;
		}
	}

	return 0;
}

static int finish_counting(WcCounters *counters)
{
	if(counters->in_word)
	{
		if(counters->words == UINTMAX_MAX)
		{
			fprintf(stderr, "wc: word counter overflow\n");
			return -1;
		}

		counters->words++;
		counters->in_word = false;
	}

	return 0;
}

static int print_counts(int fd, const WcCounters *counters, const char *file_name)
{
	char output[128];

	int length = snprintf(output, sizeof(output), "%7" PRIuMAX " %7" PRIuMAX " %7" PRIuMAX, counters->lines, counters->words, counters->bytes);

	if(length < 0 || (size_t)length >= sizeof(output))
	{
		fprintf(stderr, "wc: formatting error\n");
		return -1;
	}

	if(write_all(fd, output, (size_t)length) != 0)
		return -1;

	if(file_name != NULL)
	{
		if(write_all(fd, " ", 1) != 0)
			return -1;

		size_t name_length = 0;

		while(file_name[name_length] != '\0')
			name_length++;

		if(write_all(fd, file_name, name_length) != 0)
			return -1;
	}

	if(write_all(fd, "\n", 1) != 0)
		return -1;

	return 0;
}

static int process_stream(int input_fd, const char *file_name)
{
	unsigned char buffer[BUFFER_SIZE];

	WcCounters counters = {0};

	while(true)
	{
		ssize_t bytes_read = read(input_fd, buffer, sizeof(buffer));

		if(bytes_read < 0)
		{
			if(errno == EINTR)
				continue;

			perror("wc: read");
			return -1;
		}

		if(bytes_read == 0)
			break;

		if(update_counters(&counters, buffer, (size_t)bytes_read) != 0)
			return -1;
	}

	if(finish_counting(&counters) != 0)
		return -1;

	if(print_counts(STDOUT_FILENO, &counters, file_name) != 0)
		return -1;

	return 0;
}

static int process_file(const char *file_name)
{
	int fd = open(file_name, O_RDONLY);

	if(fd < 0)
	{
		perror(file_name);
		return -1;
	}

	struct stat file_info;

	if(fstat(fd, &file_info) != 0)
	{
		perror("wc: fstat");
		close(fd);
		return -1;
	}

	if(S_ISDIR(file_info.st_mode))
	{
		fprintf(stderr, "wc: %s: Is a directory\n", file_name);
		close(fd);
		return -1;
	}

	int result = process_stream(fd, file_name);

	if(close(fd) != 0)
	{
		perror("wc: close");
		result = -1;
	}

	return result;
}

int main(int argc, char *argv[])
{
	if(argc == 1)
		return process_stream(STDOUT_FILENO, NULL) == 0 ? 0 : 1;

	int exit_status = 0;

	for(int i = 1; i < argc; i++)
	{
		if(process_file(argv[i]) != 0)
			exit_status = 1;
	}

	return exit_status;
}
