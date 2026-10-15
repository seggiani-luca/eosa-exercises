#include "../../include/lib.h"

int atou(const char *s) {
	unsigned int n = 0;

	while (*s >= '0' && *s <= '9')
		n = n * 10 + (*s++ - '0');
	return n;
}

int atoi(const char *s) {
	if (*s == '-')
		return -atou(++s);
	else
		return atou(s);
}
