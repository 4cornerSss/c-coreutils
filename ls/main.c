#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

int processed_output(const char *name, size_t size)
{
	size_t count_write;
	size_t offset = 0;
	size_t count = size;

	while(count > 0)
	{
		count_write = fwrite(name + offset, sizeof(name[0]), count, stdout);

		if(count_write == 0)
		{
			if(ferror(stdout))
			{
				perror("ls");
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

int newline_output()
{
	char *newline = "\n";

	if(processed_output(newline, strlen(newline)) != 0)
	{
		return 1;
	}

	return 0;
}

int space_output()
{
	char *space = " ";

	if(processed_output(space, strlen(space)) != 0)
	{
		return 1;
	}

	return 0;
}

int tab_output()
{
	char *tab = "\t";

	if(processed_output(tab, strlen(tab)) != 0)
	{
		return 1;
	}

	return 0;
}

int headline(const char *name)
{
	char *colons = ":";

	if((processed_output(name, strlen(name))) == 0)
	{
		if((processed_output(colons, strlen(colons))) == 0)
		{
			if((newline_output()) != 0)
			{
				return 1;
			}
		}
		else
		{
			return 1;
		}
	}
	else
	{
		return 1;
	}

	return 0;
}

int close_list(struct dirent **name, int start, int size)
{
	for(int i = start; i < size; i++)
	{
		free(name[i]);
	}

	free(name);

	return 0;
}

int processed_rights(mode_t right, const char *file)
{
	int exam_func;

	char str[1024];
	char *string;
	char new_str[10] = "---------";

	int num = 0;

	snprintf(str, sizeof(str), "%o", right);

	for(int i = 0; i < strlen(str); i++)
	{
		if(str[i] == '0')
		{
			string = "---";
		}
		else if(str[i] == '1')
		{
			string = "--x";
		}
		else if(str[i] == '2')
		{
			string = "-w-";
		}
		else if(str[i] == '3')
		{
			string = "-wx";
		}
		else if(str[i] == '4')
		{
			string = "r--";
		}
		else if(str[i] == '5')
		{
			string = "r-x";
		}
		else if(str[i] == '6')
		{
			string = "rw-";
		}
		else if(str[i] == '7')
		{
			string = "rwx";
		}

		for(int j = 0; j < 3; j++)
		{
			new_str[j + (num * 3)] = string[j];
		}
		
		num += 1;
	}

	new_str[9] = '\0';

	exam_func = processed_output(file, strlen(file));	

	if(exam_func != 0)
	{
		return 1;
	}

	exam_func = processed_output(new_str, strlen(new_str));	

	if(exam_func != 0)
	{
		return 1;
	}

	exam_func = tab_output();

	if(exam_func != 0)
	{
		return 1;
	}

	return 0;
}

int processed_size(long int size)
{
	int exam_func;

	char str[1024];

	snprintf(str, sizeof(str), "%ld", size);

	exam_func = processed_output(str, strlen(str));	

	if(exam_func != 0)
	{
		return 1;
	}

	exam_func = tab_output();

	if(exam_func != 0)
	{
		return 1;
	}

	return 0;
}

int processed_user(mode_t uid, mode_t gid)
{
	int exam_func;

	struct passwd *pwd = getpwuid(uid);

	if(pwd == NULL)
	{
		perror("ls");
		return 1;
	}

	struct group *grp = getgrgid(gid);

	if(grp == NULL)
	{
		perror("ls");
		return 1;
	}

	exam_func = processed_output(pwd->pw_name, strlen(pwd->pw_name));	

	if(exam_func != 0)
	{
		return 1;
	}

	exam_func = tab_output();

	if(exam_func != 0)
	{
		return 1;
	}

	exam_func = processed_output(grp->gr_name, strlen(grp->gr_name));	

	if(exam_func != 0)
	{
		return 1;
	}

	exam_func = tab_output();

	if(exam_func != 0)
	{
		return 1;
	}

	return 0;
}

int processed_date(const char *date)
{
	int exam_func;

	exam_func = processed_output(date, strlen(date));	

	if(exam_func != 0)
	{
		return 1;
	}

	exam_func = tab_output();

	if(exam_func != 0)
	{
		return 1;
	}

	return 0;
}

int processed_full_info(const char *name)
{
	int exam_func;

	struct stat st;

	if(lstat(name, &st) == 0)
	{
		if(S_ISREG(st.st_mode))
		{
			exam_func = processed_rights(st.st_mode & 0777, "-");

			if(exam_func != 0)
			{
				return 1;
			}

		}
		else if(S_ISLNK(st.st_mode))
		{
			exam_func = processed_rights(st.st_mode & 0777, "l");

			if(exam_func != 0)
			{
				return 1;
			}
		}
		else if(S_ISDIR(st.st_mode))
		{
			exam_func = processed_rights(st.st_mode & 0777, "d");

			if(exam_func != 0)
			{
				return 1;
			}

		}
		else
		{
			return 1;
		}
	}
	else
	{
		if(errno == ENOENT)
		{
			return 1;
		}
		else
		{
			return 1;
		}
	}

	exam_func = processed_user(st.st_uid, st.st_gid);

	if(exam_func != 0)
	{
		return 1;
	}

	exam_func = processed_size(st.st_size);

	if(exam_func != 0)
	{
		return 1;
	}

	exam_func = processed_date(ctime(&st.st_mtime));

	if(exam_func != 0)
	{
		return 1;
	}

	return 0;
}

int processed_directory(const char *name, const int vision, const int full_info)
{
	int exam_func;

	struct dirent **namelist;
	
	int count_elms = scandir(name, &namelist, NULL, alphasort);
	int count_file = 0;
	int comp_count = 0;

	if(count_elms < 0)
	{
		perror("ls");

		return 1;
	}
	if(count_elms == 2 && !vision)
	{
		close_list(namelist, 0, count_elms);

		return 0;
	}

	for(int i = 0; i < count_elms; i++)
	{
		if((namelist[i]->d_name)[0] != '.' || vision)
		{
			count_file += 1;
		}
	}


	for(int i = 0; i < count_elms; i++)
	{
		if((namelist[i]->d_name)[0] != '.' || vision)
		{
			comp_count += 1;

			if(full_info)
			{
				char full_path[PATH_MAX];
				
				snprintf(full_path, sizeof(full_path), "%s/%s", name, namelist[i]->d_name);

				exam_func = processed_full_info(full_path);

				if(exam_func != 0)
				{
					close_list(namelist, i, count_elms);

					return 1;
				}
			}

			exam_func = processed_output(namelist[i]->d_name, strlen(namelist[i]->d_name));
			
			if(exam_func != 0)
			{
				close_list(namelist, i, count_elms);

				return 1;
			}

			if(comp_count < count_file)
			{
				if(!full_info)
				{
					exam_func = space_output();
					
					if(exam_func != 0)
					{
						close_list(namelist, i, count_elms);

						return 1;
					}
				}
				else
				{
					exam_func = newline_output();
					
					if(exam_func != 0)
					{
						close_list(namelist, i, count_elms);

						return 1;
					}
				}
			}
		}

		free(namelist[i]);
	}

	free(namelist);


	return 0;
}

int processed_name_files(const char *name, const int vision, const int head, const int full_info)
{
	int exam_func;

	struct stat st_n;

	if(stat(name, &st_n) == 0)
	{
		if(S_ISDIR(st_n.st_mode))
		{
			if(head)
			{
				exam_func = headline(name);

				if(exam_func != 0)
				{
					return 1;
				}
			}

			exam_func = processed_directory(name, vision, full_info);

			if(exam_func != 0)
			{
				return 1;
			}

		}
		else
		{
			if(full_info)
			{
				exam_func = processed_full_info(name);

				if(exam_func != 0)
				{
					return 1;
				}
			}

			exam_func = processed_output(name, strlen(name));

			if(exam_func != 0)
			{
				return 1;
			}

			if(full_info)
			{
				exam_func = newline_output();

				if(exam_func != 0)
				{
					return 1;
				}
			}
			else
			{
				exam_func = space_output();

				if(exam_func != 0)
				{
					return 1;
				}
			}

			return 0;
		}
	}
	else
	{
		if(errno == ENOENT)
		{
			fprintf(stderr, "ls: cannot access '%s': No such file or directory\n", name);
			return 1;
		}
		else
		{
			perror("ls");
			return EXIT_FAILURE;
		}
	}


	return 0;
}


int main(int argc, char *argv[])
{
	int status = 0;
	int exam_func = 0;

	int head = 0;
	int start_index = 0;
	int vision = 0;
	int full_info = 0;

	if(argc == 1)
	{
		exam_func = processed_name_files(".", vision, head, full_info);
		
		if(exam_func != 0)
		{
			status = exam_func;
		}

		exam_func = newline_output();
		
		if(exam_func != 0)
		{
			status = exam_func;;
		}

		return status;
	}

	char *arguments[argc - 1];

	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
		{
			if(!strcmp(argv[i], "-a"))
			{
				vision = 1;
			}
			else if(!strcmp(argv[i], "-l"))
			{
				full_info = 1;
			}
			else
			{
				fprintf(stderr, "ls: invalid option -- '%s'\n", argv[i]);

				return 1;
			}
		}
		else
		{
			arguments[start_index] = argv[i];

			start_index += 1;
		}
	}

	if(start_index == 0)
	{
		exam_func = processed_name_files(".", vision, head, full_info);
		
		if(exam_func != 0)
		{
			status = exam_func;
		}

		exam_func = newline_output();
		
		if(exam_func != 0)
		{
			status = exam_func;;
		}

		return status;
	}
	else if(start_index == 1)
	{
		exam_func = processed_name_files(arguments[0], vision, head, full_info);
		
		if(exam_func != 0)
		{
			status = exam_func;
		}

		exam_func = newline_output();
		
		if(exam_func != 0)
		{
			status = exam_func;;
		}

		return status;
	}
	else
	{
		head = 1;

		for(int i = 0; i < start_index; i++)
		{

			exam_func = processed_name_files(arguments[i], vision, head, full_info);

			if(exam_func != 0)
			{
				status = exam_func;
			}
		}

		exam_func = newline_output();
		
		if(exam_func != 0)
		{
			status = exam_func;;
		}
	}


	return status;
}
