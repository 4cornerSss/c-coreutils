#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

static bool contains(const char *line,
		size_t line_length,
		const char *pattern,
		size_t pattern_length);

static int write_line(FILE *output,
		const char *label,
		const char *line,
		size_t line_length);

static int process_stream(FILE *input,
		FILE *output,
		const char *pattern,
		size_t pattern_length,
		bool invert,
		const char *label,
		bool *matched);

static int process_file(const char *filename,
		const char *pattern,
		size_t pattern_length,
		bool invert,
		const char *label,
		bool *matched);

static int write_line(FILE *output,
		const char *label,
		const char *line,
		size_t line_length)
{
	if(label != NULL)
	{
		if(fprintf(output, "%s:", label) < 0)
		{
			return -1;
		}
	}

	if(fwrite(line, 1, line_length, output) != line_length)
	{
		return -1;
	}

	return 0;
}

static bool contains(const char *line,
		size_t line_length,
		const char *pattern,
		size_t pattern_length)
{
	if(pattern_length == 0)
	{
		return true;
	}

	if(pattern_length > line_length)
	{
		return false;
	}

	for(size_t i = 0; i <= line_length - pattern_length; i++)
	{
		if(memcmp(line + i, pattern, pattern_length) == 0)
		{
			return true;
		}
	}

	return false;
}

static int process_stream(FILE *input,
		FILE *output,
		const char *pattern,
		size_t pattern_length,
		bool invert,
		const char *label,
		bool *matched)
{
	char *line = NULL;
	size_t copacity = 0;
	ssize_t line_length;
	int result = 0;

	while(true)
	{
		errno = 0;
		line_length = getline(&line, &copacity, input);

		if(line_length == -1)
		{
			if(ferror(input) || errno != 0)
			{
				int error_code = errno != 0 ? errno : EIO;

				if(label != NULL)
				{
					fprintf(stderr, "grep: %s: %s\n",
							label, strerror(error_code));
				}
				else
				{
					fprintf(stderr, "grep: standard input: %s\n", strerror(error_code));
				}

				result = -1;
			}

			break;
		}

		bool found = contains(line, (size_t)line_length, pattern, pattern_length);

		if(found != invert)
		{
			*matched = true;
			
			if(write_line(output, label, line,
						(size_t)line_length) != 0)
			{
				perror("grep: stdout");
				result = -1;
				break;
			}
		}
	}

	free(line);

	return result;
}

static int process_file(const char *filename,
		const char *pattern,
		size_t pattern_length,
		bool invert,
		const char *label,
		bool *matched)
{
	FILE *input = fopen(filename, "r");

	if(input == NULL)
	{
		fprintf(stderr, "grep: %s: %s\n", filename, strerror(errno));
		return -1;
	}

	int result = process_stream(input, stdout, pattern, pattern_length,
			invert, label, matched);

	if(fclose(input) != 0)
	{
		fprintf(stderr, "grep: %s: %s\n", filename, strerror(errno));
		result = -1;
	}

	return result;
}

int main(int argc, char *argv[])
{
	bool invert = false;
	bool matched = false;

	int arg_index = 1;

	if(arg_index < argc && strcmp(argv[arg_index], "-v") == 0)
	{
		invert = true;
		arg_index++;
	}

	if(arg_index < argc && strcmp(argv[arg_index], "--") == 0)
	{
		arg_index++;
	}

	if(arg_index >= argc)
	{
		fprintf(stderr, "Usage: grep [-v] PATTERN [FILE...]\n");
		return 2;
	}

	const char *pattern = argv[arg_index];
	size_t pattern_length = strlen(pattern);

	arg_index++;

	int file_count = argc - arg_index;
	bool has_error = false;

	if(file_count == 0)
	{
		if(process_stream(stdin,
				stdout,
				pattern, pattern_length,
				invert, NULL, &matched) != 0)
		{
			has_error = true;
		}
	}
	else
	{
		bool show_filename = file_count > 1;

		for(int i = arg_index; i < argc; i++)
		{
			const char *filename = argv[i];
			const char *label = show_filename ? filename : NULL;

			int result;

			if(strcmp(filename, "-") == 0)
			{
				result = process_stream(stdin,
						stdout,
						pattern,
						pattern_length,
						invert, label, &matched);
			}
			else
			{
				result = process_file(filename,
						pattern, pattern_length,
						invert, label, &matched);
			}

			if(result != 0)
			{
				has_error = true;
			}
		}
	}

	if(fflush(stdout) == EOF)
	{
		perror("grep: stdout");
		has_error = true;
	}

	if(has_error)
	{
		return 2;
	}

	return matched ? 0 : 1;
}
