#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "ft_ping.h"

//　書式付き文字列を新しいメモリに確保して返す関数
char *ft_strf(const char *fmt, ...) {
	va_list args;
	char *str;
	int len;

	if (!fmt)
		return (NULL);
	va_start(args, fmt);
	// asprintfはmallocされた文字列ポインタを*strに格納し、長さを返す
	len = vasprintf(&str, fmt, args);
	va_end(args);

	if (len == -1)
		return (NULL);
	
	return (str);
}