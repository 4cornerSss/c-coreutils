#include <stdio.h>

#include <unistd.h>
#include <fcntl.h>

ssize_t read_bytes(int input, unsigned char *buffer, size_t size_buffer)
{
	ssize_t count = read(input, buffer, size_buffer);

	if(count < 0)
	{
		fprintf(stderr, "hexdump: Error reading bytes\n");
		return -1;
	}

	return count;
}

ssize_t write_bytes(int output, char *buffer, size_t count_read, size_t offset)
{
	ssize_t count_write_remain = 0;
	ssize_t count_write = write(output, buffer + offset, count_read);

	if(count_write <= 0)
	{
		fprintf(stderr, "hexdump: Error writing bytes\n");
		return -1;
	}

	if(count_write < count_read)
	{
		if((count_write_remain = write_bytes(output, buffer, count_read - count_write, offset + count_write)) < 0)
		{
			return -1;
		}
	}

	return count_write + count_write_remain;
}

int space_out()
{
	char *space = " ";

	if(write_bytes(STDOUT_FILENO, space, 1, 0) <= 0)
	{
		return 1;
	}

	return 0;
}

int newline_out()
{
	char *newline = "\n";

	if(write_bytes(STDOUT_FILENO, newline, 1, 0) <= 0)
	{
		return 1;
	}

	return 0;
}

char *formatting_bytes(unsigned char byte, char *buf, size_t size)
{
	snprintf(buf, size, "%02x", byte);
	return buf;
}

char *bias_bytes(char *buf, size_t size, int bias)
{
	snprintf(buf, size, "%07x", bias);
	return buf;
}

int processed_output_bytes(int input, int output)
{
	ssize_t count_read;
	ssize_t count_write;

	size_t count_write_bytes = 16;

	unsigned char main_buffer[1024];
	size_t size_main_buffer = sizeof(main_buffer) / sizeof(main_buffer[0]);

	int count_str = 0;

	char buffer[3];
	size_t size_buffer = sizeof(buffer) / sizeof(buffer[0]);

	char bias_buf[8];
	size_t size_bias_buf = sizeof(bias_buf) / sizeof(bias_buf[0]);

	while((count_read = read_bytes(input, main_buffer, size_main_buffer)) != 0)
	{
		if(count_read < 0)
		{
			return 1;
		}

		size_t remain = count_read;
		size_t offset = 0;
		
		while(remain > 0)
		{
			bias_bytes(bias_buf, size_bias_buf, count_str);

			count_write = write_bytes(output, bias_buf, size_bias_buf - 1, 0);

			if(count_write <= 0)
			{
				return 1;
			}
			
			if(space_out() != 0)
			{
				return 1;
			}

			if(remain >= count_write_bytes)
			{
				for(size_t i = 0; i < count_write_bytes; i++)
				{
					formatting_bytes(main_buffer[offset], buffer, size_buffer);

					count_write = write_bytes(output, buffer, size_buffer - 1, 0);

					if(count_write <= 0)
					{
						return 1;
					}
					
					if(space_out() != 0)
					{
						return 1;
					}

					offset += 1;
				}

				remain -= count_write_bytes;

				if(newline_out() != 0)
				{
					return 1;
				}

				count_str += 16;
			}
			else
			{
				for(size_t i = 0; i < remain; i++)
				{
					formatting_bytes(main_buffer[offset], buffer, size_buffer);

					count_write = write_bytes(output, buffer, size_buffer - 1, 0);

					if(count_write <= 0)
					{
						return 1;
					}

					if(space_out() != 0)
					{
						return 1;
					}

					offset += 1;
				}

				remain = 0;

				if(newline_out() != 0)
				{
					return 1;
				}

				count_str += offset;
			}
		}
	}

	bias_bytes(bias_buf, size_bias_buf, count_str);

	count_write = write_bytes(output, bias_buf, size_bias_buf - 1, 0);

	if(count_write <= 0)
	{
		return 1;
	}
	
	if(space_out() != 0)
	{
		return 1;
	}

	if(newline_out() != 0)
	{
		return 1;
	}

	return 0;
}

int open_file(const char *file)
{
	int fd;

	if((fd = open(file, O_RDONLY)) == -1)
	{
		return 1;
	}

	if(processed_output_bytes(fd, STDOUT_FILENO) != 0)
	{
		return 1;
	}

	close(fd);

	return 0;
}

int main(int argc, char *argv[])
{
	int status_out;

	if(argc == 1)
	{
		status_out = processed_output_bytes(STDIN_FILENO, STDOUT_FILENO);

		if(status_out != 0)
		{
			return 1;
		}

		return 0;
	}

	int start_index = 1;

	if(argv[start_index] != NULL)
	{
		if(open_file(argv[start_index]) != 0)
		{
			return 1;
		}

		return 0;
	}

	return 0;
}
