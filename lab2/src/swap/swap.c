#include "swap.h"

void Swap(char *left, char *right)
{
	char str = *left;

	*left = *right;
	*right = str;
}
