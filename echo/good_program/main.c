#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int write_all(const char *data, size_t length)
{
	while(length > 0)
	{
		size_t written = fwrite(data, 1, length, stdout);

		if(written == 0)
		{
			if(ferror(stdout))
			{
				perror("echo");
			}
			else
			{
				fprintf(stderr, "echo: failed to write output\n");
			}

			return -1;
		}

		length -= written;
		data += written;
	}

	return 0;
}

static int write_escaped(const char *str)
{
	const char *start = str;
	const char *current = str;

	while(*current != '\0')
	{
		if(*current == '\\' && current[1] != '\0')
		{
			char replacement;

			switch(current[1])
			{
				case 'n':
					replacement = '\n';
					break;
				case 't':
					replacement = '\t';
					break;
				case '\\':
					replacement = '\\';
					break;
				default:
					current++;
					continue;
			}

			if(write_all(start, (size_t)(current - start)) != 0)
			{
				return -1;
			}

			if(write_all(&replacement, 1) != 0)
			{
				return -1;
			}

			current += 2;
			start = current;
		}
		else
		{
			current++;
		}
	}

	return write_all(start, (size_t)(current - start));
}

int main(int argc, char *argv[])
{
	int start_index = 1;
	int suppress_newline = 0;
	int interpret_escapes = 0;

	if(argc > 1)
	{
		if(strcmp(argv[1], "-n") == 0)
		{
			suppress_newline = 1;
			start_index = 2;
		}
		else if(strcmp(argv[1], "-e") == 0)
		{
			interpret_escapes = 1;
			start_index = 2;
		}
	}

	for(int i = start_index; i < argc; i++)
	{
		int result;

		if(interpret_escapes)
		{
			result = write_escaped(argv[i]);
		}
		else
		{
			result = write_all(argv[i], strlen(argv[i]));
		}

		if(result != 0)
		{
			return EXIT_FAILURE;
		}

		if(i < argc - 1)
		{
			if(write_all(" ", 1) != 0)
			{
				return EXIT_FAILURE;
			}
		}

		if(!suppress_newline)
		{
			if(write_all("\n", 1) != 0)
			{
				return EXIT_FAILURE;
			}
		}

		if(fflush(stdout) == EOF)
		{
			perror("echo");
			return EXIT_FAILURE;
		}

		return EXIT_SUCCESS;
	}

	return 0;
}
