/******************************************************************************/
/* Internet Relay Chat daemon, see `license' for further information          */
/* (c) 2001, forestnet.org                                                    */
/******************************************************************************/
/*
 *   $Id: 
 */
 
#ifndef __unicode_h_included
#define __unicode_h_included

struct codepage {
	char name[32];      /* codepage name, e.g., microsoft-cp1251 */
	char alias[32];     /* aliased name */

	short c2u[128];     /* codepage-to-unicode map */

	short first, last;  /* map ranges */

	char *u2c;          /* unicode-to-codepage map */

	char *u2s;          /* unicode-to-us_ascii_sequence map, see below */
	short *u2i;         /* indexes, see below */

	int refcount;
	unsigned int bytes;
};

/* in case of a transliteration table (unicode-to-us_ascii_seq conversion)
   u2c should be NULL, and u2s points to a set of null-terminated strings
   that represent unicode characters from `first' to `last' respectively.

   in this case, c2u is filled with question marks (`?'), and u2i points to
   the indexes, so that the address of the string for a particular unicode
   character is calculated as: cp->u2s[cp->u2i[code - cp->first]];

   indexes for unresolved characters should be 0x0000;
*/

extern int codepage_load(const char *);
extern void codepage_purge();
extern char *codepage_to_unicode(char *, int);
extern char *unicode_to_codepage(char *, int);
extern int codepage_find(const char *);
extern char *codepage_list();
extern struct codepage *cps[32];
extern void LoadCodePages(void);
#endif /* !defined(__unicode_h_included) */
