#include <errno.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define BYTES_PER_LINE 16
#define READ_BUFFER_SIZE 4096
#define OUTPUT_BUFFER_SIZE 128

static int write_all(int fd, const char *buffer, size_t size)
{
	size_t offset = 0;

	while(offset < size)
	{
		ssize_t written = write(fd, buffer + offset, size - offset);

		if(written < 0)
		{
			if(errno == EINTR)
				continue;

			return -1;
		}

		if(written == 0)
		{
			errno = EIO;
			return -1;
		}

		offset += (size_t)written;
	}
	
	return 0;
}

static ssize_t read_retry(int fd, unsigned char *buffer, size_t size)
{
	ssize_t result;

	do
	{
		result = read(fd, buffer, size);
	}
	while(result < 0 && errno == EINTR);

	return result;
}

static int format_line(const unsigned char *bytes,
		size_t count,
		uintmax_t offset,
		char *output,
		size_t output_size)
{
	int result;
	size_t used = 0;

	result = snprintf(output, output_size, "07" PRIxMAX " ", offset);

	if(result < 0 || (size_t)result >= output_size)
		return -1;

	used = (size_t)result;

	for(size_t i = 0; i < count; i++)
	{
		result = snprintf(output + used, output_size - used, "%02x ", (unsigned int)bytes[i]);

	if(result < 0 || (size_t)result >= output_size - used)
		return -1;

	used += (size_t)result;
	}

	if(used >= output_size)
		return -1;

	output[used++] = '\n';

	return (int)used;
}

static int format_final_offset(uintmax_t offset,
				char *output,
				size_t output_size)
{
	int result = snprintf(output, output_size,
				"%07" PRIxMAX " \n", offset);
	if(result < 0 || (size_t)result >= output_size)
		return -1;
	
	return result;
}

static int process_input(int input, int output)
{
	unsigned char read_buffer[READ_BUFFER_SIZE];
	unsigned char line_bytes[BYTES_PER_LINE];

	char output_buffer[OUTPUT_BUFFER_SIZE];

	size_t line_count = 0;
	uintmax_t file_offset = 0;

	while (1)
	{
		ssize_t bytes_read = read_retry(input,
						read_buffer,
						sizeof(read_buffer));

		if (bytes_read < 0)
		{
		    perror("hexdump: read");
		    return -1;
		}

		if (bytes_read == 0)
		    break;

		for (ssize_t i = 0; i < bytes_read; i++)
		{
		    line_bytes[line_count++] = read_buffer[i];

		    if (line_count == BYTES_PER_LINE)
		    {
			int output_size = format_line(line_bytes,
						      line_count,
						      file_offset,
						      output_buffer,
						      sizeof(output_buffer));

			if (output_size < 0)
			{
				fprintf(stderr, "hexdump: formatting error\n");
				return -1;
			}

			if (write_all(output, output_buffer,
				      (size_t)output_size) < 0)
			{
				perror("hexdump: write");
				return -1;
			}

			file_offset += line_count;
			line_count = 0;
		    }
		}
	}

	if (line_count > 0)
	{
	int output_size = format_line(line_bytes,
				      line_count,
				      file_offset,
				      output_buffer,
				      sizeof(output_buffer));

	if (output_size < 0)
	{
		fprintf(stderr, "hexdump: formatting error\n");
		return -1;
	}

	if (write_all(output, output_buffer,
		      (size_t)output_size) < 0)
	{
		perror("hexdump: write");
		return -1;
	}

	file_offset += line_count;
	}

	int final_size = format_final_offset(file_offset,
					 output_buffer,
					 sizeof(output_buffer));

	if (final_size < 0)
	{
		fprintf(stderr, "hexdump: formatting error\n");
		return -1;
	}

	if (write_all(output, output_buffer, (size_t)final_size) < 0)
	{
		perror("hexdump: write");
		return -1;
	}

	return 0;
}

static int open_file(const char *filename)
{
	int fd = open(filename, O_RDONLY);

	if(fd < 0)
	{
		perror(filename);
		return -1;
	}

	int status = process_input(fd, STDOUT_FILENO);

	if(close(fd) < 0)
	{
		perror("hexdump: close");

		if(status == 0)
			status = -1;
	}

	return status;
}

int main(int argc, char *argv[])
{
	if(argc > 2)
	{
		fprintf(stderr, "Usage: %s [file]\n", argv[0]);
		return 1;
	}

	if(argc == 1)
	{
		return process_input(STDIN_FILENO, STDOUT_FILENO) < 0;
	}

	return open_file(argv[1]) < 0;
}
