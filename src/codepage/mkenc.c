/* mkenc.c -- codepage conversion tables creation utility;
 * Copyright (C) 2001-2002, ForestNet IRC Network
 * This is a free software; see file `license' for details.
 */

#ifndef lint
static char cvsid[] = "$Id: mkenc.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $";
#endif

#ifndef _WIN32
# include <sys/file.h>
#else
# include <io.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
# define strcasecmp stricmp
#endif

#define isspace(c) ((c == '\t') || (c == ' '))

static const char *malloc_err = "Memory allocation error\n";
static const char *usage_msg =
"Usage: mkenc [OPTION|FILE]...\n"
"\n"
"Options:\n"
"  -n   next files are treated as normal (ascii-to-unicode) maps\n"
"  -t   next files are treated as translit (unicode-to-string) maps\n"
;


int hex2int(char *from, unsigned int *to)
{
	if (from[0] != '0' || (from[1] != 'x' && from[1] != 'X'))
		return 0;

	for (from += 2, *to = 0; isxdigit(*from); from++)
		*to = *to * 16 + (isdigit(*from)
			? *from - '0' : tolower(*from) - 'a' + 10);

	return 1;
}


static void make_one(const char *fname, int translit)
{
	FILE *f;
	struct stat sb;
	unsigned short a2u[256];
	int val1, val2, uc_min = 65535, uc_max = 0;
	char *p, *u2a = NULL, **u2s = NULL, tmpbuf[1024];

	if (!(f = fopen(fname, "r")))
		return;

	memset(a2u, 0, sizeof(a2u));

	if (!(u2a = malloc(sizeof(char) * 65536))) {
		fclose(f);
		return;
	}
	memset(u2a, 0, sizeof(char) * 65536);

	if (translit) {
		if (!(u2s = malloc(sizeof(char*) * 65536))) {
			free(u2a);
			fclose(f);
			return;
		}
		memset(u2s, 0, sizeof(char*) * 65536);
	}

	while (fgets(tmpbuf, sizeof(tmpbuf), f)) {
		if (p = strchr(tmpbuf, '#'))
			*p = '\0';

		for (p = tmpbuf; isspace(*p); ++p);

		if (!hex2int(p, &val1))
			continue;

		for (; *p && !isspace(*p); ++p);
		for (; isspace(*p); ++p);

		if (translit) {
			if (!strlen(p) || val1 >= 65536)
				continue;

			if (val1 < uc_min)
				uc_min = val1;
			if (val1 > uc_max)
				uc_max = val1;

			/* don't leak memory with strdup() */
			if (!u2s[val1]) {
				u2s[val1] = strdup(p);
				if (p = strchr(u2s[val1], '\r'))
					*p = '\0';
				if (p = strchr(u2s[val1], '\n'))
					*p = '\0';
			}
		}
		else {
			if (!hex2int(p, &val2) || val1 >= 256 || val1 < 0x80)
				continue;

			if (val2 < uc_min)
				uc_min = val2;
			if (val2 > uc_max)
				uc_max = val2;

			if (val1 < 256)
				a2u[val1] = (unsigned short)(val2 & 0xffff);

			if (val2 < 65536)
				u2a[val2] = (unsigned char)(val1 & 0xff);
		}
	}

	fclose(f);

	strcpy(tmpbuf, fname);
	if (p = strrchr(tmpbuf, '.'))
		*p = '\0';
        strcat(tmpbuf, ".cp");
	if (f = fopen(tmpbuf, "w")) {
		if (translit) {
			fprintf(f, "u2s:%x,%x,", uc_min, uc_max);
			for (val1 = uc_min; val1 <= uc_max; ++val1) {
				p = u2s[val1] ? u2s[val1] : "";
				if (strchr(p, ',')) {
					char *p1 = tmpbuf;
					for (; *p; *p1++ = *p++)
						if (*p == ',')
							*p1++ = '\\';
					*p1 = '\0';
					p = tmpbuf;
				}

				fprintf(f, "%s%s", p,
					val1 == uc_max ? "" : ",");
			}
			fprintf(f, "\n");
		}

		else {
			/* ascii-to-unicode part */
			fprintf(f, "c2u:");
			for (val1 = 0x80; val1 < 0x100; ++val1) {
				if (a2u[val1])
					fprintf(f, "%x", a2u[val1] & 0xffff);
				if (val1 != 255)
					fprintf(f, ",");
			}
			fprintf(f, "\n");

			/* unicode-2-ascii part */
			fprintf(f, "u2c:%x,%x,", uc_min, uc_max);
			for (val1 = uc_min; val1 <= uc_max; ++val1) {
				if (u2a[val1])
					fprintf(f, "%x", u2a[val1] & 0xff);
				if (val1 != uc_max)
					fprintf(f, ",");
			}
			fprintf(f, "\n");
		}

		fclose(f);
	}
	else fprintf(stderr, "Failed to write to %s.\n", tmpbuf);

	free(u2a);

	if (translit) {
		for (val1 = 0; val1 < 65536; ++val1)
			if (u2s[val1])
				free(u2s[val1]);
		free(u2s);
	}
}


int main(int argc, char *argv[])
{
	int translit = 0;

	if (argc < 2) {
		printf("%s", usage_msg);
		exit(1);
	}

	/* skip my name */
	--argc, ++argv;

	for (; argc; --argc, ++argv) {
		if (!strcasecmp(*argv, "-t")) {
			translit = 1;
			continue;
		}
		else if (!strcasecmp(*argv, "-n")) {
			translit = 0;
			continue;
		}
		make_one(*argv, translit);
	}

	return 0;
}
