#include <stdio.h>
#include <limits.h>
#include <unistd.h>

int main()
{
	char cwd[PATH_MAX];

	if((getcwd(cwd, sizeof(cwd))) == NULL)
	{
		perror("pwd");
		return 1;
	}
	
	printf("%s\n", cwd);
	return 0;
}
