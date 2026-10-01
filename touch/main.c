#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int start_index = 1;
	if(argc < 2)
	{
		fprintf(stderr, "touch: missing file operand\n");
		return 1;
	}	

	for(int i = start_index; i < argc; i++)
	{
		struct stat buffer;

		if(stat(argv[i], &buffer) != 0)
		{
			if(errno == ENOENT)
			{
				FILE *file = fopen(argv[i], "w");
				if(file == NULL)
				{
					perror("Error");
					return EXIT_FAILURE;
				}
				else
				{
					fclose(file);
				}
			}
			else
			{
				perror("Error");
				return EXIT_FAILURE;
			}
		}
		else
		{
			struct timespec times[2];

			times[0].tv_sec = 0;
			times[0].tv_nsec = UTIME_OMIT;
			times[1].tv_sec = 0;
			times[1].tv_nsec = UTIME_NOW;

			if(utimensat(AT_FDCWD, argv[i], times, 0) != 0)
			{
				perror("Error");
				return EXIT_FAILURE;
			}
		}

	}	
	return 0;
}
