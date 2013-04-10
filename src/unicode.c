/* unicode.c -- codepage convertion functionality;
 * Copyright (C) 2001-2002, ForestNet IRC Network
 * This is a free software; see file `license' for details.
 */

#ifndef lint
static char rcsid[] = "$Id: unicode.c,v 1.4 2005/08/27 16:23:50 jpinto Exp $";
#endif

#ifdef _WIN32
# include <io.h>
#else
# include <sys/file.h>
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

/* for fstat() */
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <string.h> /* strcat */
#include "irc_string.h" /* MyFree etc */
#include "unicode.h"
#include "common.h" /* strncmp for win32 */
#include "s_log.h" /* log( */
#include "irc_string.h" 
#include "dconf_vars.h" /* CodePagePath */
#define elementsof(x) (sizeof(x)/sizeof(x[0]))

/* available codepages */
struct codepage *cps[32];

/* buffer for conversion */
static char ucbuf[1024];

/* transliteration codepage index */
int codepage_translit = -1;

/* unresolved unicode character */
static const char uunknown[] = "?";


static int hex2int(char **p)
{
	int value;

	for (value = 0; IsXDigit(**p); (*p)++)
		value = value * 16 + (IsDigit(**p) ? **p - '0' : (ToLower(**p) - 'a' + 10));

	return value;
}


/* load a codepage from file */
static struct codepage *codepage_read(const char *filename)
{
	int f, len;
	char *raw, *p;
	struct codepage *cp;
	struct stat sb;

#ifdef _WIN32
	sprintf(ucbuf, "%s\\%s.cp", CodePagePath, filename);
#else
	sprintf(ucbuf, "%s/%s.cp", CodePagePath, filename);
#endif

	if ((f = open(ucbuf, O_RDONLY/*|O_TEXT*/)) == -1)
		return NULL;

	fstat(f, &sb);

	if (!(len = sb.st_size)) {
		close(f);
		return NULL;
	}

	raw = (char*)MyMalloc(len + 1);
	raw[read(f, (void*)raw, len)] = '\0';
	close(f);

	if (!(cp = (struct codepage *) MyMalloc(sizeof(*cp)))) {
		MyFree((void*)raw);
		return NULL;
	}

	memset((void*)cp, 0, sizeof(*cp));
	cp->bytes = sizeof(*cp);

	/* default name */
	
	strncpy(cp->name, filename, sizeof(cp->name));

	for (p = raw; *p; ) {
		/* aliased name */
		if (!strncasecmp(p, "name:", 5)) {
			strncpy(cp->alias, p + 5, sizeof(cp->alias));
			for (f = 0; cp->alias[f] != '\r' && cp->alias[f] != '\n' && f < sizeof(cp->alias); f++);
			cp->alias[f] = '\0';
		}

		/* transliteration table */
		else if (!strncasecmp(p, "u2s:", 4)) {
			/* skip the prefix */
			p += 4;

			/* lower boundary */
			cp->first = hex2int(&p) & 0xffff;

			/* higher boundary */
			if (*p == ',') {
				p++;
				cp->last = hex2int(&p) & 0xffff;
			}

			/* make sure we're at a comma and the boundaries are ok */
			if (*p == ',' && cp->last >= cp->first) {
				int len;
				char *dst;

				++p; /* skip the comma */

				/* allocate offset table */
				len = sizeof(short) * (cp->last - cp->first + 1);
				cp->u2i = (short*)MyMalloc(len);
				cp->bytes += len;

				/* calculate map length */
				for (dst = p, len = 0; *dst && *dst != '\r' && *dst != '\n'; ++dst, ++len);

				/* allocate memory for the map and copy it */
				cp->u2s = dst = (char*)MyMalloc(len + 1);
				cp->bytes += len;

				/* index the map, copying substrings */
				for (f = cp->first; f <= cp->last && *p && *p != '\r' && *p != '\n'; ++f) {
					/* empty sequence*/
					if (*p == ',') {
						cp->u2i[f - cp->first] = 0;
					}

					/* non-empty sequence */
					else {
						/* offset to this substring */
						cp->u2i[f - cp->first] = dst - cp->u2s;

						/* copy unescaped substring */
						for (; *p && *p != '\r' && *p != '\n' && *p != ','; ++dst) {
							/* copy one character */
							*dst = *p++;
							/* unescape */
							if (*dst == '\\' && *p && *p != '\r' && *p != '\n')
								*dst = *p++;
						}

						*dst++ = '\0';
					}

					++p;
				}

				/* `p' points to the EOL char now, move back a bit (see below) */
				--p;
			}
		}

		/* codepage-to-unicode table */
		else if (!strncasecmp(p, "c2u:", 4)) {
			for (p += 4, f = 0; f < 128; ++f) {
				cp->c2u[f] = hex2int(&p) & 0xffff;
				if (*p != ',')
					break;
				++p;
			}
		}

		/* unicode-to-codepage table */
		else if (!strncasecmp(p, "u2c:", 4)) {
			p += 4;
			cp->first = hex2int(&p) & 0xffff;
			if (*p == ',') {
				p++;
				cp->last = hex2int(&p) & 0xffff;
				if (*p == ',' && cp->last >= cp->first) {
					p++;
					len = cp->last - cp->first + 1;
					if (!(cp->u2c = (char*)MyMalloc(len))) {
						cp->first = 0;
						cp->last = 0;
					}
					else {
						cp->bytes += len;
						for (f = cp->first; f <= cp->last; f++) {
							cp->u2c[f - cp->first] = hex2int(&p);
							if (*p != ',')
								break;
							p++;
						}
						/* done */
					}
				}
			}
		}

		/* skip this line */
		for ( ; *p && *p != '\r' && *p != '\n'; ++p);
		for ( ; *p && (*p == '\r' || *p == '\n'); ++p);
	}

	cp->name[sizeof(cp->name) - 1] = '\0';

	MyFree((void*)raw);

	/* make sure codepage is usable */
	if (!cp->u2c && !cp->u2s && !cp->u2i) {
		MyFree((void*)cp);
		return NULL;
	}

	return cp;
}


/* release a codepage */
static void codepage_erase(int idx)
{
	if (cps[idx]->u2c)
		MyFree((void*)cps[idx]->u2c);

	if (cps[idx]->u2s)
		MyFree((void*)cps[idx]->u2s);

	if (cps[idx]->u2i)
		MyFree((void*)cps[idx]->u2i);

	MyFree((void*)cps[idx]);

	cps[idx] = NULL;
}


/* load a codepage from file */
int codepage_load(const char *filename)
{
	int i;
	struct codepage *cp;

	if (!filename || !*filename)
		return -1;

	/* check if the codepage is already loaded */
	for (i = 0; i < elementsof(cps); i++)
		if (cps[i] && (!strcasecmp(cps[i]->name, filename)
		|| !strcasecmp(cps[i]->alias, filename)))
			return i;

	/* find an empty slot */
	for (i = 0; i < elementsof(cps); i++)
		if (cps[i] == NULL)
			break;

	/* no room for data */
	if (i == elementsof(cps))
		return -1;

	if ((cp = codepage_read(filename)) == NULL)
		return -1;

	cps[i] = cp;

	/* find translit codeapge */
	if (codepage_translit == -1 && !strcasecmp(cp->name, "translit"))
		codepage_translit = i;

	cps[i] = cp;

	return i;
}


/* remove unused codepages */
void codepage_purge()
{
	int i;
	for (i = 0; i < elementsof(cps); i++) {
		if (cps[i] == NULL)
			continue;

		/* refresh codepages in use */
		if (cps[i]->refcount) {
			struct codepage *cp;

			/* failed to load, skip */
			if (!(cp = codepage_read(cps[i]->name)))
				continue;

			cp->refcount = cps[i]->refcount;

			/* replace the old one */
			codepage_erase(i);
			cps[i] = cp;

			continue;
		}

		/* unload unused codepages */
		codepage_erase(i);

		if (codepage_translit == i)
			codepage_translit = -1;
	}
}


/* find a specific codepage */
int codepage_find(const char *name)
{
	int idx;
	for (idx = 0; idx < elementsof(cps); idx++) {
		if (cps[idx] && (!strcasecmp(cps[idx]->name, name)
		|| !strcasecmp(cps[idx]->alias, name)))
			return idx;
	}
	return -1;
}


/* Transforms a given Unicode character code into an utf-8 sequence, returns
   a pointer to the first octet of the null-terminated sequence. */
static char * uc2utf(short uc)
{
	int i;
	static char utf8[5] = {0,0,0,0,0};

	/* 1 byte */
	if (uc < 0x80) {
		utf8[3] = (char)uc;
		return &utf8[3];
	}

	/* 2 bytes */
	else if (uc < 0x800) {
		utf8[2] = 0xc0 + ((uc / 64) & 0x1f);
		utf8[3] = 0x80 + (uc & 0x3f);
		return &utf8[2];
	}

	/* 3 bytes */
	else {
		utf8[1] = 0xe0 + ((uc / 4096) & 0x0f);
		utf8[2] = 0x80 + ((uc / 64) & 0x3f);
		utf8[3] = 0x80 + (uc & 0x3f);
		return &utf8[1];
	}
}


static short utf2uc(char **p)
{
	short uc;

	/* 1 byte (what's it doing here) */
	if ((**p & 0x80) == 0x00) {
		uc = **p;
		++(*p);
	}

	/* 2 bytes */
	else if ((**p & 0xe0) == 0xc0 && (*p)[1]) {
		uc = (*((*p)++) & 0x1f) << 6;
		uc |= *((*p)++) & 0x3f;
	}

	/* 3 bytes */
	else if ((**p & 0xf0) == 0xe0 && (*p)[1] && (*p)[2]) {
		uc = (*((*p)++) & 0x1f) << 12;
		uc |= (*((*p)++) & 0x03f) << 6;
		uc |= (*((*p)++) & 0x3f);
	}

	/* unknown length */
	else {
		uc = 0;
		for (++(*p); (**p & 0xc0) == 0x80; ++(*p));
	}

	return uc;
}


/* Conversion of a string from the specified codepage to unicode (utf-8).
   Conversion is performed within the same string, returns a pointer to the
   source buffer, utf-8 encoded and null-terminated.  The resulting string
   can't be longer than 1024 bytes. */
char * codepage_to_unicode(char *src, int cpidx)
{
	char *dst, *tmp, *sptr, c;
	struct codepage *cp;
	short uc;

	if (cpidx < 0 || cpidx >= elementsof(cps))
		return src;

	cp = cps[cpidx];

	for (sptr = src, dst = ucbuf; (c = *sptr) && dst < (ucbuf + sizeof(ucbuf) - 1); ++sptr) {
		/* us_ascii char, no conversion */
		if ((c & 0x80) == 0x00) {
			*dst++ = c;
			continue;
		}

		/* an unicode character */
		if ((uc = cp->c2u[c & 0x7f]) > 0x7f) {
			for (tmp = uc2utf(uc); *tmp; )
				*dst++ = *tmp++;
			continue;
		}

		/* an unicode character resolved to an ascii one */
		*dst++ = (char) (uc ? uc : '?');
	}

	*dst = '\0';
	return ucbuf;
}


/* Converts a string from utf-8 stream to the specified codepage. */
char * unicode_to_codepage(char *source, int cpidx)
{
	int changes = 0;
	struct codepage *cp, *tl = NULL;
	char *dst = ucbuf, *src = source, hdr, *p, *end;
	short uc, count, idx;

	/* transparent table */
	if (cpidx < 0 || cpidx >= elementsof(cps) || !(cp = cps[cpidx]) || (!cp->u2c && !cp->u2i))
		return source;

	/* transliteration codepage */
	if (codepage_translit >= 0 && codepage_translit < elementsof(cps))
		tl = cps[codepage_translit];

	/* single byte table */
	if (cp->u2c) {
		while (*src && dst < ucbuf + sizeof(ucbuf)) {
			char c;

			if ((unsigned char)*src < 0x80) {
				*dst++ = *src++;
				continue;
			}

			changes++;

			uc = utf2uc(&src);

			/* character within the current codepage */
			if (uc >= cp->first && uc <= cp->last && (c = cp->u2c[uc - cp->first])) {
				*dst++ = c;
			}

			/* we have a transliteration page installed, try it */
			else if (tl && tl != cp && uc >= tl->first && uc <= tl->last) {
				/* single byte transliteration (ever happens?) */
				if (tl->u2c)
					*dst++ = (char) tl->u2c[uc - tl->first];

				/* multibyte transliteration (usual case) */
				else {
					short idx;
					if (idx = tl->u2i[uc - tl->first]) {
						*dst = '\0';
						strcat(dst, &tl->u2s[idx]);
						for (; *dst; ++dst);
					}
					else *dst++ = '?';
				}
			}

			/* no luck */
			else *dst++ = '?';
		}
	}

	/* multibyte table */
	else {
		while (*src) {
			short idx;

			if ((unsigned char)*src < 0x80) {
				*dst++ = *src++;
				continue;
			}

			++changes;

			uc = utf2uc(&src);

			/* a char within our codepage */
			if (uc >= cp->first && uc <= cp->last && (idx = cp->u2i[uc - cp->first])) {
				*dst = '\0';
				strcat(dst, &cp->u2s[idx]);
				for (; *dst; ++dst);
			}

			/* transliterate */
			else if (tl && cp != tl && uc >= tl->first && uc <= tl->last) {
				/* single byte transliteration (ever happens?) */
				if (tl->u2c)
					*dst++ = (char) tl->u2c[uc - tl->first];

				/* multibyte transliteration (usual case) */
				else {
					short idx;
					if (idx = tl->u2i[uc - tl->first]) {
						*dst = '\0';
						strcat(dst, &tl->u2s[idx]);
						for (; *dst; ++dst);
					}
					else *dst++ = '?';
				}
			}

			/* unresolved character */
			else *dst++ = '?';
		}
	}

	if (!changes)
		return source;

	*dst = '\0';
	ucbuf[sizeof(ucbuf)-1] = '\0';
	return ucbuf;
}


char * codepage_list()
{
	int idx;

	strcpy(ucbuf, "utf8");

	for (idx = 0; idx < elementsof(cps); idx++) {
		if (cps[idx] == NULL)
			continue;
		strcat(ucbuf, ",");
		strcat(ucbuf, cps[idx]->name);
	}

	ucbuf[sizeof(ucbuf)-1] = '\0';
	return ucbuf;
}

/*
 * LoadCodePages
 * Load codepage files from CodePagePath
 */
void LoadCodePages(void)
{
  char fn[32];
  char *c = CodePages;
  char *p;
  int  i;
  
  if(c==NULL)
    return;
  
  irclog(L_NOTICE,"Loading codepages %s", 
  	CodePages);
  while(*c!='\0')
    {
      i=0;
      while((*c!='\0') && (*c!=','))
        fn[i++]=*(c++); /* needs a buffer overflow check someday -Lamego */
      fn[i]='\0';
      if(*c!='\0') 
        ++c;
      codepage_load(fn);      
    }
}
