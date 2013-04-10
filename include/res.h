/*
 * $Id: res.h,v 1.3 2005/08/27 16:23:49 jpinto Exp $ 
 * New res.h
 * Aaron Sethman <androsyn@ratbox.org>
 */

#ifndef _RES_H_INCLUDED
#define _RES_H_INCLUDED 1

#include "config.h"
#include "ircd_defs.h"
/* I hate this *blah* db */
#include "fileio.h"
#ifndef USE_ADNS
#include <sys/types.h>       /* time_t */
#else
#include "../adns/adns.h"
#endif

#ifndef USE_ADNS
struct Client;
struct hostent;

struct DNSReply {
  struct hostent* hp;        /* hostent struct  */
  int             ref_count; /* reference count */
};

struct DNSQuery {
  void* vptr;               /* pointer used by callback to identify request */
  void (*callback)(void* vptr, struct DNSReply* reply); /* callback to call */
};

extern int ResolverFileDescriptor;  /* GLOBAL - file descriptor (s_bsd.c) */

extern void get_res(void);
extern struct DNSReply* gethost_byname(const char* name,
                                       const struct DNSQuery* req);
extern struct DNSReply* gethost_byaddr(const char* name,
                                       const struct DNSQuery* req);
extern struct DNSReply* gethost_byname_type(const char* name,
					const struct DNSQuery* req,
					int type);
extern int		init_resolver(void);
extern void		restart_resolver(void);
extern time_t		timeout_resolver(time_t now);
extern void		delete_resolver_queries(const void* vptr);
extern unsigned long   cres_mem(struct Client* cptr);
extern void			clear_cache(void);

/*
 * add_local_domain - append local domain suffix to hostnames that
 * don't contain a dot '.'
 * name - string to append to
 * len  - total length of the buffer
 * name is modified only if there is enough space in the buffer to hold
 * the suffix
 */
extern void add_local_domain(char* name, int len);

#else
#define DNS_BLOCK_SIZE 64

struct DNSQuery {
	void *ptr;
	adns_query query;
	adns_answer answer;
	void (*callback)(void* vptr, adns_answer *reply);
};

void init_resolver(void);
void restart_resolver(void);
void timeout_adns (void);
void dns_writeable (int fd , void *ptr );
void dns_readable (int fd , void *ptr );
void dns_do_callbacks(void);
void dns_select (void);
void adns_gethost (const char *name, struct DNSQuery *req );
void adns_getaddr (struct in_addr *addr , struct DNSQuery *req );
void delete_adns_queries(struct DNSQuery *q);
void do_adns_io();
#endif /* USE_ADNS */
#endif
