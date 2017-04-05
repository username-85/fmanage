#include "fm_util.h"
#include <stdio.h>
#include <sys/stat.h>

bool file_exist(char const *file)
{
	struct stat st;
	return (stat(file, &st) == 0);
}
