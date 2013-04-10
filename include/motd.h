/*
 * motd.h
 *
 * $Id: motd.h,v 1.3 2005/08/27 16:23:49 jpinto Exp $
 */
#ifndef INCLUDED_motd_h
#define INCLUDED_motd_h
#ifndef INCLUDED_ircd_defs_h
#include "ircd_defs.h"    /* MAX_DATE_STRING */
#endif
#ifndef INCLUDED_limits_h
#include <limits.h>       /* PATH_MAX */
#define INCLUDED_limits_h
#endif

#define MESSAGELINELEN 89       

#ifndef PATH_MAX    
#define PATH_MAX    4096
#endif

typedef enum {
  USER_MOTD,
  OPER_MOTD,
  WEB_MOTD
} MotdType;

struct MessageFileLine
{
  char                    line[MESSAGELINELEN + 1];
  struct MessageFileLine* next;
};

typedef struct MessageFileLine MessageFileLine;

struct MessageFile
{
  char             fileName[PATH_MAX + 1];
  MotdType         motdType;
  MessageFileLine* contentsOfFile;
  char             lastChangedDate[MAX_DATE_STRING + 1];
};

typedef struct MessageFile MessageFile;

typedef struct Tline ATline;
struct Tline
{
  char *host;
  MessageFile motd;  
  ATline *next;
} Tline;


struct Client;

void InitMessageFile(MotdType, char *, struct MessageFile *);
int SendMessageFile(struct Client *, struct MessageFile *);
int ReadMessageFile(MessageFile *);
MessageFile *FindMotd(const char* host);
extern ATline * tlines;
int m_motd(struct Client *cptr, struct Client *sptr, int parc, char *parv[]);

#endif /* INCLUDED_motd_h */
