#include <stdio.h>
#include <string.h>
#include <stdlib.h>

size_t output(const char *sym, size_t offset, size_t size)
{
	return fwrite(sym + offset, sizeof(sym[0]), size, stdout);
}

int processed_output(const char *name, size_t size)
{
	size_t count_write;
	size_t offset = 0;
	size_t count = size;

	while(count != 0)
	{
		count_write = output(name, offset, count);

		if(count_write == 0)
		{
			if(ferror(stdout))
			{
				perror("echo");
				return EXIT_FAILURE;
			}
			else
			{
				return 1;
			}
		}

		offset += count_write;
		count -= count_write;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	const char *newline = "\n";
	const char *space = " ";
	int start_index = 1;
	int escape = 0;
	int result;

	if(argc == 1)
	{
		result = processed_output(newline, strlen(newline));

		if(result != 0)
		{
			return 1;
		}

		return 0;
	}

	if(!strcmp(argv[1], "-n"))
	{
		start_index = 2;
		newline = "";

	}

	if(!strcmp(argv[1], "-e"))
	{
		start_index = 2;
		escape = 1;
	}

	for(int i = start_index; i < argc; i++)
	{
		size_t size_str = strlen(argv[i]);

		if(escape)
		{
			const char *arg;

			for(size_t j = 0; j < size_str; j++)
			{
				if(argv[i][j] == '\\' && argv[i][j + 1])
				{
					if(argv[i][j + 1] == 'n')
					{
						arg = "\n";
						j += 1;
					}
					else if(argv[i][j + 1] == 't')
					{
						arg = "\t";
						j += 1;
					}
					else if(argv[i][j + 1] == '\\')
					{
						arg = "\\";
						j += 1;
					}
					else
					{
						arg = "\\";
					}

						result = processed_output(arg, strlen(arg));

						if(result != 0)
						{
							return 1;
						}
				}
				else
				{
					result = processed_output(argv[i] + j, 1);

					if(result != 0)
					{
						return 1;
					}
				}
			}

			if(i < (argc - 1))
			{
				result = processed_output(space, strlen(space));

				if(result != 0)
				{
					return 1;
				}
			}
		}
		else
		{
			result = processed_output(argv[i], size_str);

			if(result != 0)
			{
				return 1;
			}

			if(i < (argc - 1))
			{
				result = processed_output(space, strlen(space));

				if(result != 0)
				{
					return 1;
				}
			}
		}

	}

	result = processed_output(newline, strlen(newline));

	if(result != 0)
	{
		return 1;
	}

	return 0;
}
