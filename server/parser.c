#define _GNU_SOURCE /* strchrnul */

#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "parser.h"

int is_word(char *word) {
	for (int i = 0; word[i] != '\0'; i++)
		if(!isalnum(word[i])) return 0;
	return 1;
}

int text_parser(char *buf, char *toks[3], int lens[3])
{

	int ntok;

	/* Separar tokens */
	{
		char *p = buf;
		ntok = 0;
		toks[ntok++] = p;
		while (ntok < 5 && (p = strchrnul(p, ' ')) && *p) {
			/* Longitud token anterior */
			lens[ntok-1] = p - toks[ntok-1];
			*(p++) = 0;
			/* Comienzo nuevo token */
			toks[ntok++] = p;
		}
		p = strchrnul(p, '\n');
		lens[ntok-1] = p - toks[ntok-1];
	}
	
	return ntok;
}


