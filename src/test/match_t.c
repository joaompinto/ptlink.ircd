/*
 * match_t.c - test driver for match used in ircd
 *
 * Thomas Helvey <tomh@inxpress.net>
 *
 * $Id: match_t.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $
 */
#include "irc_string.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

const char* const MATCH_FILE    = "data/match.data";
const char* const COLLAPSE_FILE = "data/collapse.data";

void pcollapse(char* buf)
{
  printf("%s\n", collapse(buf));
}

void test_collapse(FILE* file)
{
  char buf[BUFSIZ + 1];
  char* p;

  while (fgets(buf, BUFSIZ, file)) {
    if ((p = strchr(buf, '\n'))) 
      *p = '\0';
    pcollapse(buf);
  }
}

void test_match(FILE* file)
{
  char buf[BUFSIZ + 1];
  while (fgets(buf, BUFSIZ, file)) {
    char* p = buf;
    buf[strlen(buf) - 1] = '\0';
    while (*p && !IsSpace(*p))
      ++p;
    while (*p && IsSpace(*p))
      *p++ = '\0';
    if (!*p) continue;
    printf("%d: %s %s\n", match(p, buf), p, buf);
  }
  printf("\n");
}

int main(int argc, char** argv)
{
  FILE* file = fopen(COLLAPSE_FILE, "r");
  if (file) {
    test_collapse(file);
    fclose(file);
  }
  else
    printf("skipped collapse: %s %s\n", COLLAPSE_FILE, strerror(errno));
    
  file = fopen(MATCH_FILE, "r");
  if (file) {
    test_match(file);
    fclose(file);
  }
  else
    printf("skipped match: %s %s\n", MATCH_FILE, strerror(errno));
    
  return 0;
}


