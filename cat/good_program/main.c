#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define BUFFER_SIZE 8192

static void report_error(const char *name)
{
	if(errno != 0)
		fprintf(stderr, "cat: %s: %s\n", name, strerror(errno));
	else
		fprintf(stderr, "cat: %s: I/O error\n", name);
}

static int copy_stream(FILE *input, const char *input_name)
{
	unsigned char buffer[BUFFER_SIZE];

	for(;;)
	{
		size_t bytes_read = fread(buffer, 1, sizeof(buffer), input);

		if(bytes_read > 0)
		{
			size_t offset = 0;

			while(offset < bytes_read)
			{
				size_t bytes_written;

				errno = 0;

				bytes_written = fwrite(buffer + offset, 1, bytes_read - offset, stdout);
				if(bytes_written == 0)
				{
					report_error("stdout");
					return EXIT_FAILURE;
				}

				offset += bytes_written;

				if(ferror(stdout))
				{
					report_error("stdout");
					return EXIT_FAILURE;
				}
			}
		}

		if(ferror(input))
		{
			report_error(input_name);
			return EXIT_FAILURE;
		}

		if(feof(input))
			break;
	}

	return EXIT_SUCCESS;
}

static int process_file(const char *filename)
{
	FILE *input;
	int status;
	int close_status;

	input = fopen(filename, "rb");

	if(input == NULL)
	{
		report_error(filename);
		return EXIT_FAILURE;
	}

	status = copy_stream(input, filename);

	errno = 0;
	close_status = fclose(input);

	if(close_status == EOF)
	{
		report_error(filename);
		status = EXIT_FAILURE;
	}

	return status;
}

int main(int argc, char *argv[])
{
	int status = EXIT_SUCCESS;

	if(argc == 1)
	{
		status = copy_stream(stdin, "stdin");
	}
	else
	{
		for(int i = 0; i < argc; i++)
		{
			int result;

			if(strcmp(argv[i], "-") == 0)
				result = copy_stream(stdin, "stdin");
			else
				result = process_file(argv[i]);

			if(result != EXIT_SUCCESS)
				status = EXIT_FAILURE;
		}
	}

	errno = 0;

	if(fflush(stdout) == EOF)
	{
		report_error("stdout");
		status = EXIT_FAILURE;
	}

	return status;
}
