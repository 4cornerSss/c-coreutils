#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024

int process_file(FILE *file)
{
	char buffer[BUFFER_SIZE];
	size_t count_read;

	while((count_read = fread(buffer, sizeof(buffer[0]), sizeof(buffer), file)) != 0)
	{
		size_t count_write;
		size_t offset = 0;
		size_t count = count_read;

		while(count != 0)
		{
			count_write = fwrite(buffer + offset, sizeof(buffer[0]), count, stdout);

			if(ferror(stdout))
			{
				perror("cat");
				return EXIT_FAILURE;
			}

			if(count_write == 0)
			{
				return EXIT_FAILURE;
			}

			offset += count_write;
			count -= count_write;

		}

	}	

	if(ferror(file))
	{
		perror("cat");
		return EXIT_FAILURE;
	}

	return 0;
}

int output_content_file(char *name)
{
	int result;
	int result_close;

	FILE *file = fopen(name, "r");
	
	if(file == NULL)
	{
		perror("cat");
		return EXIT_FAILURE;
	}

	result = process_file(file);
	result_close = fclose(file);

	if(result != 0 || result_close != 0)
	{
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int status = 0;
	int result = 0;

	if(argc == 1)
	{
		result = process_file(stdin);

		if(result != 0)
		{
			return 1;
		}
	}

	for(int i = 1; i < argc; i++)
	{
		result = output_content_file(argv[i]);

		if(result != 0)
		{
			status = 1;
		}
	}

	return status;
}
