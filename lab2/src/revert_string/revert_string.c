#include "revert_string.h"
#include <stdlib.h>
#include <string.h>

void RevertString(char *str)
{
	if (str == NULL)
		return;

	char* end_ptr = str + (strlen(str) - 1);

	while (end_ptr > str)
	{
		char ch = *str;
		*str = *end_ptr;
		*end_ptr = ch;

		++str, --end_ptr;
	}
}

