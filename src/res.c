/*
 * src/res.c (C)opyright 1992 Darren Reed. All rights reserved.
 * This file may not be distributed without the author's permission in any
 * shape or form. The author takes no responsibility for any damage or loss
 * of property which results from the use of this software.
 *
 * $Id: res.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $
 *
 * July 1999 - Rewrote a bunch of stuff here. Change hostent builder code,
 *     added callbacks and reference counting of returned hostents.
 *     --Bleep (Thomas Helvey <tomh@inxpress.net>)
 */
#include "res.h"
#include "client.h"
#include "common.h"
#include "irc_string.h"
#include "ircd.h"
#include "numeric.h"
#include "restart.h"
#include "s_bsd.h"
#include "send.h"
#include "struct.h"
#include "s_debug.h"
#include "dconf_vars.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef USE_ADNS
#include <arpa/nameser.h>
#include <resolv.h>


#include <netdb.h>
#include <arpa/inet.h>

#ifdef DEBUGMODE
#include <errno.h>
#endif

#include <limits.h>
#if (CHAR_BIT != 8)
#error this code needs to be able to address individual octets 
#endif

#undef  DEBUG  /* because there is a lot of debug code in here :-) */

/*
 * Some systems do not define INADDR_NONE (255.255.255.255)
 * INADDR_NONE is actually a valid address, but it should never
 * be returned from any nameserver.
 * NOTE: The bit pattern for INADDR_NONE and INADDR_ANY (0.0.0.0) should be 
 * the same on all hosts so we shouldn't need to use htonl or ntohl to
 * compare or set the values.
 */ 
#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned int) 0xffffffff)
#endif

#define MAXPACKET       1024  /* rfc sez 512 but we expand names so ... */
#define RES_MAXALIASES  35    /* maximum aliases allowed */
#define RES_MAXADDRS    35    /* maximum addresses allowed */

/*
 * macros used to calulate offsets into fixed query buffer
 */
#define ALIAS_BLEN (size_t) ((RES_MAXALIASES + 1) * sizeof(char*))
#define ADDRS_BLEN (size_t) ((RES_MAXADDRS + 1) * sizeof(struct IN_ADDR*))

#define ADDRS_OFFSET  (size_t) (ALIAS_BLEN + ADDRS_BLEN)
#define ADDRS_DLEN    (size_t) (RES_MAXADDRS * sizeof(struct IN_ADDR))
#define NAMES_OFFSET  (size_t) (ADDRS_OFFSET + ADDRS_DLEN)
#define MAXGETHOSTLEN (size_t) (NAMES_OFFSET + MAXPACKET)

#define AR_TTL          600   /* minimum TTL in seconds for dns cache entries */

/*
 * the following values should be prime
 */
#define ARES_CACSIZE    307
#define MAXCACHED       281

/*
 * RFC 1104/1105 wasn't very helpful about what these fields
 * should be named, so for now, we'll just name them this way.
 * we probably should look at what named calls them or something.
 */
#define TYPE_SIZE       (size_t) 2
#define CLASS_SIZE      (size_t) 2
#define TTL_SIZE        (size_t) 4
#define RDLENGTH_SIZE   (size_t) 2
#define ANSWER_FIXED_SIZE (TYPE_SIZE + CLASS_SIZE + TTL_SIZE + RDLENGTH_SIZE)

/*
 * Building the Hostent
 * The Hostent struct is arranged like this:
 *          +-------------------------------+
 * Hostent: | struct hostent h              |
 *          |-------------------------------|
 *          | char *buf                     |
 *          +-------------------------------+
 *
 * allocated:
 *
 *          +-------------------------------+
 * buf:     | h_aliases pointer array       | Max size: ALIAS_BLEN;
 *          | NULL                          | contains `char *'s
 *          |-------------------------------|
 *          | h_addr_list pointer array     | Max size: ADDRS_BLEN;
 *          | NULL                          | contains `struct in_addr *'s
 *          |-------------------------------|
 *          | h_addr_list addresses         | Max size: ADDRS_DLEN;
 *          |                               | contains `struct in_addr's
 *          |-------------------------------|
 *          | storage for hostname strings  | Max size: ALIAS_DLEN;
 *          +-------------------------------+ contains `char's
 *
 *  For requests the size of the h_aliases, and h_addr_list pointer
 *  array sizes are set to MAXALISES and MAXADDRS respectively, and
 *  buf is a fixed size with enough space to hold the largest expected
 *  reply from a nameserver, see RFC 1034 and RFC 1035.
 *  For cached entries the sizes are dependent on the actual number
 *  of aliases and addresses. If new aliases and addresses are found
 *  for cached entries, the buffer is grown and the new entries are added.
 *  The hostent struct is filled in with the addresses of the entries in
 *  the Hostent buf as follows:
 *  h_name      - contains a pointer to the start of the hostname string area,
 *                or NULL if none is set.  The h_name is followed by the
 *                aliases, in the storage for hostname strings area.
 *  h_aliases   - contains a pointer to the start of h_aliases pointer array.
 *                This array contains pointers to the storage for hostname
 *                strings area and is terminated with a NULL.  The first alias
 *                is stored directly after the h_name.
 *  h_addr_list - contains a pointer to the start of h_addr_list pointer array.
 *                This array contains pointers to in_addr structures in the
 *                h_addr_list addresses area and is terminated with a NULL.
 *
 *  Filling the buffer this way allows for proper alignment of the h_addr_list
 *  addresses.
 *
 *  This arrangement allows us to alias a Hostent struct pointer as a
 *  real struct hostent* without lying. It also allows us to change the
 *  values contained in the cached entries and requests without changing
 *  the actual hostent pointer, which is saved in a client struct and can't
 *  be changed without blowing things up or a lot more fiddling around.
 *  It also allows for defered allocation of the fixed size buffers until
 *  they are really needed.
 *  Nov. 17, 1997 --Bleep
 */

typedef struct Hostent {
  struct hostent h;      /* the hostent struct we are passing around */
  char*          buf;    /* buffer for data pointed to from hostent */
} aHostent;

typedef struct reslist {
  int             id;
  int             sent;              /* number of requests sent */
  time_t          ttl;
  char            type;
  char            retries;           /* retry counter */
  char            sends;             /* number of sends (>1 means resent) */
  char            resend;            /* send flag. 0 == dont resend */
  time_t          sentat;
  time_t          timeout;
  struct IN_ADDR  addr;
  char*           name;
  struct reslist* next;
  struct DNSQuery query;             /* query callback for this request */
  aHostent        he;
} ResRQ;

typedef struct cache {
  struct cache*   hname_next;
  struct cache*   hnum_next;
  struct cache*   list_next;
  time_t          expireat;
  time_t          ttl;
  struct Hostent  he;
  struct DNSReply reply;
} aCache;

typedef struct cachetable {
  aCache* num_list;
  aCache* name_list;
} CacheTable;


int    ResolverFileDescriptor = -1;   /* GLOBAL - used in s_bsd.c */

static time_t nextDNSCheck    = 0;
static time_t nextCacheExpire = 1;

/*
 * Keep a spare file descriptor open. res_init calls fopen to read the
 * resolv.conf file. If ircd is hogging all the file descriptors below 256,
 * on systems with crippled FILE structures this will cause wierd bugs.
 * This is definitely needed for Solaris which uses an unsigned char to
 * hold the file descriptor.
 */ 
static int         spare_fd = -1;

static int         cachedCount = 0;
static CacheTable  hashtable[ARES_CACSIZE];
static aCache*     cacheTop = NULL;
static ResRQ*      requestListHead;     /* head of resolver request list */
static ResRQ*      requestListTail;     /* tail of resolver request list */


static void     add_request(ResRQ* request);
static void     rem_request(ResRQ* request);
static ResRQ*   make_request(const struct DNSQuery* query);
static void     rem_cache(aCache*);
static void     do_query_name(const struct DNSQuery* query, 
                              const char* name, 
                              ResRQ* request,
                              int type);
static void     do_query_number(const struct DNSQuery* query,
                                const struct IN_ADDR*, 
                                ResRQ* request);
static void     query_name(const char* name, 
                           int query_class, 
                           int query_type, 
                           ResRQ* request);
static int      send_res_msg(const char* buf, int len, int count);
static void     resend_query(ResRQ* request);
static int      proc_answer(ResRQ* request, HEADER* header, 
                                    char*, char *);
static aCache*  make_cache(ResRQ* request);
static aCache*  find_cache_name(const char* name);
static aCache*  find_cache_number(ResRQ* request, const char* addr);
static ResRQ*   find_id(int);
static int      hash_number(const unsigned char *);
static void     update_list(ResRQ *, aCache *);
static int      hash_name(const char* name);

static  struct cacheinfo {
  int  ca_adds;
  int  ca_dels;
  int  ca_expires;
  int  ca_lookups;
  int  ca_na_hits;
  int  ca_nu_hits;
  int  ca_updates;
} cainfo;

static  struct  resinfo {
  int  re_errors;
  int  re_nu_look;
  int  re_na_look;
  int  re_replies;
  int  re_requests;
  int  re_resends;
  int  re_sent;
  int  re_timeouts;
  int  re_shortttl;
  int  re_unkrep;
} reinfo;


/*
 * From bind 8.3, these aren't in earlier versions of bind
 *
 */
extern u_short  _getshort(const u_char *);
extern u_int    _getlong(const u_char *);
/*
 * int
 * res_isourserver(ina)
 *      looks up "ina" in _res.ns_addr_list[]
 * returns:
 *      0  : not found
 *      >0 : found
 * author:
 *      paul vixie, 29may94
 */
static int
res_ourserver(const struct __res_state* statp, const struct sockaddr_in *inp) 
{
  struct sockaddr_in ina;
  int ns;

  ina = *inp;
  for (ns = 0;  ns < statp->nscount;  ns++) {
    const struct sockaddr_in *srv = &statp->nsaddr_list[ns];

    if (srv->sin_family == ina.sin_family &&
         srv->sin_port == ina.sin_port &&
         (srv->sin_addr.s_addr == INADDR_ANY ||
          srv->sin_addr.s_addr == ina.sin_addr.s_addr))
             return (1);
  }
  return (0);
}

/*
 * start_resolver - do everything we need to read the resolv.conf file
 * and initialize the resolver file descriptor if needed
 */
static void start_resolver(void)
{
  char sparemsg[80];

  /*
   * close the spare file descriptor so res_init can read resolv.conf
   * successfully. Needed on Solaris
   */
  if (spare_fd > -1)
    CLOSE(spare_fd);

  res_init();      /* res_init always returns 0 */
  /*
   * make sure we have a valid file descriptor below 256 so we can
   * do this again. Needed on Solaris
   */
  spare_fd = open("/dev/null",O_RDONLY,0);
  if ((spare_fd < 0) || (spare_fd > 255)) {
    ircsprintf(sparemsg,"invalid spare_fd %d",spare_fd);
    restart(sparemsg);
  }

  if (!_res.nscount) {
    _res.nscount = 1;
    _res.nsaddr_list[0].sin_addr.s_addr = inet_addr("127.0.0.1");
  }
  _res.options |= RES_NOALIASES;
#ifdef DEBUG
  _res.options |= RES_DEBUG;
#endif
#ifdef IPV6
  _res.options |= RES_USE_INET6;
#endif  
  if (ResolverFileDescriptor < 0) {
    ResolverFileDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
    set_non_blocking(ResolverFileDescriptor);
  }
}

/*
 * init_resolver - initialize resolver and resolver library
 */
int init_resolver(void)
{

#ifdef  LRAND48
  srand48(CurrentTime);
#endif
  /*
   * XXX - we don't really need to do this, all of these are static
   */
  memset((void*) &cainfo,   0, sizeof(cainfo));
  memset((void*) hashtable, 0, sizeof(hashtable));
  memset((void*) &reinfo,   0, sizeof(reinfo));

  requestListHead = requestListTail = NULL;
  start_resolver();
  return ResolverFileDescriptor;
}

/*
 * restart_resolver - flush the cache, reread resolv.conf, reopen socket
 */
void restart_resolver(void)
{
  /* flush_cache();  flush the dns cache */
  start_resolver();
}

/*
 * add_local_domain - Add the domain to hostname, if it is missing
 * (as suggested by eps@TOASTER.SFSU.EDU)
 */
void add_local_domain(char* hname, int size)
{
  assert(0 != hname);
  /* 
   * try to fix up unqualified names 
   */
  if ((_res.options & RES_DEFNAMES) && !strchr(hname, '.')) {
#if 0
    /*
     * XXX - resolver should already be initialized
     */
    if (0 == (_res.options & RES_INIT))
      init_resolver();
#endif 
    if (_res.defdname[0]) {
      size_t len = strlen(hname);
      if ((strlen(_res.defdname) + len + 2) < size) {
        hname[len++] = '.';
        strcpy(hname + len, _res.defdname);
      }
    }
  }
}

/*
 * add_request - place a new request in the request list
 */
static void add_request(ResRQ* request)
{
  assert(0 != request);
  if (!requestListHead)
    requestListHead = requestListTail = request;
  else {
    requestListTail->next = request;
    requestListTail = request;
  }
  request->next = NULL;
  reinfo.re_requests++;
}

/*
 * rem_request - remove a request from the list. 
 * This must also free any memory that has been allocated for 
 * temporary storage of DNS results.
 */
static void rem_request(ResRQ* request)
{
  ResRQ** current;
  ResRQ*  prev = NULL;

  assert(0 != request);
  for (current = &requestListHead; *current; ) {
    if (*current == request) {
      *current = request->next;
      if (requestListTail == request)
        requestListTail = prev;
      break;
    } 
    prev    = *current;
    current = &(*current)->next;
  }
#ifdef  DEBUG
  Debug((DEBUG_DNS, "rem_request:Remove %#x at %#x %#x",
         old, *current, prev));
#endif
  MyFree(request->he.buf);
  MyFree(request->name);
  MyFree(request);
}

/*
 * make_request - Create a DNS request record for the server.
 */
static ResRQ* make_request(const struct DNSQuery* query)
{
  ResRQ* request;
  assert(0 != query);
  request = (ResRQ*) MyMalloc(sizeof(ResRQ));
  memset(request, 0, sizeof(ResRQ));

  request->sentat  = CurrentTime;
  if (ReverseLookup == 0)		/* No reverse lookup */
	{
	  request->retries = 0;
	  request->timeout = 0;
	}
  else if(ReverseLookup == 1)	/* Fast reverse lookup */
	{
  	  request->retries = 2;
	  request->timeout = 2;    /* start at 4 and exponential inc. */
	}
  else							/* Normal reverse lookup */
	{
	  request->retries = 3;
	  request->timeout = 4;    /* start at 2 and exponential inc. */  
	}
  request->resend  = 1;
#ifndef IPV6  
  request->addr.s_addr      = INADDR_NONE;
#endif
  request->he.h.h_addrtype  = AFINET;
  request->he.h.h_length    = sizeof(struct IN_ADDR);
  request->query.vptr       = query->vptr;
  request->query.callback   = query->callback;

#if defined(NULL_POINTER_NOT_ZERO)
  request->next             = NULL;
  request->he.buf           = NULL;
  request->he.h.h_name      = NULL;
  request->he.h.h_aliases   = NULL;
  request->he.h.h_addr_list = NULL;
#endif
  add_request(request);
  return request;
}

/*
 * timeout_query_list - Remove queries from the list which have been 
 * there too long without being resolved.
 */
static time_t timeout_query_list(time_t now)
{
  ResRQ*   request;
  ResRQ*   next_request = 0;
  time_t   next_time    = 0;
  time_t   timeout      = 0;

  Debug((DEBUG_DNS, "timeout_query_list at %s", myctime(now)));
  for (request = requestListHead; request; request = next_request) {
    next_request = request->next;
    timeout = request->sentat + request->timeout;
    if (now >= timeout) {
      if (--request->retries <= 0) {
#ifdef DEBUG
        Debug((DEBUG_ERROR, "timeout %x now %d cptr %x",
               request, now, request->cinfo.value.cptr));
#endif
        reinfo.re_timeouts++;
        (*request->query.callback)(request->query.vptr, 0);
        rem_request(request);
        continue;
      }
      else {
        request->sentat = now;
        request->timeout += request->timeout;
        resend_query(request);
#ifdef DEBUG
        Debug((DEBUG_INFO,"r %x now %d retry %d c %x",
               request, now, request->retries,
               request->cinfo.value.cptr));
#endif
      }
    }
    if (!next_time || timeout < next_time) {
      next_time = timeout;
    }
  }
  return (next_time > now) ? next_time : (now + AR_TTL);
}

/*
 * expire_cache - removes entries from the cache which are older 
 * than their expiry times. returns the time at which the server 
 * should next poll the cache.
 */
static time_t expire_cache(time_t now)
{
  aCache* cp;
  aCache* cp2;
  time_t  next = 0;

  for (cp = cacheTop; cp; cp = cp2) {
    cp2 = cp->list_next;
    if (cp->expireat < now) {
      cainfo.ca_expires++;
      rem_cache(cp);
    }
    else if (!next || next > cp->expireat)
      next = cp->expireat;
  }
  return (next > now) ? next : (now + AR_TTL);
}

/*
 * timeout_resolver - check request list and cache for expired entries
 */
time_t timeout_resolver(time_t now)
{
  if (nextDNSCheck < now)
    nextDNSCheck = timeout_query_list(now);
  if (nextCacheExpire < now)
    nextCacheExpire = expire_cache(now);
  return IRCD_MIN(nextDNSCheck, nextCacheExpire);
}


/*
 * delete_resolver_queries - cleanup outstanding queries 
 * for which there no longer exist clients or conf lines.
 */
void delete_resolver_queries(const void* vptr)
{
  ResRQ* request;
  ResRQ* next_request;

  for (request = requestListHead; request; request = next_request) {
    next_request = request->next;
    if (vptr == request->query.vptr)
      rem_request(request);
  }
}

/*
 * send_res_msg - sends msg to all nameservers found in the "_res" structure.
 * This should reflect /etc/resolv.conf. We will get responses
 * which arent needed but is easier than checking to see if nameserver
 * isnt present. Returns number of messages successfully sent to 
 * nameservers or -1 if no successful sends.
 */
static int send_res_msg(const char* msg, int len, int rcount)
{
  int i;
  int sent = 0;
  int max_queries = IRCD_MIN(_res.nscount, rcount);

  assert(0 != msg);
  /*
   * RES_PRIMARY option is not implemented
   * if (_res.options & RES_PRIMARY || 0 == max_queries)
   */
  if (0 == max_queries)
    max_queries = 1;

  for (i = 0; i < max_queries; i++) {
    /*
     * XXX - this should already have been done by res_init
     */
    _res.nsaddr_list[i].sin_family = AF_INET;
    if (sendto(ResolverFileDescriptor, msg, len, 0, 
               (struct sockaddr*) &(_res.nsaddr_list[i]),
               sizeof(struct sockaddr_in)) == len) {
      ++reinfo.re_sent;
      ++sent;
    }
    else
      Debug((DEBUG_ERROR,"s_r_m:sendto: %d on %d", 
             errno, ResolverFileDescriptor));
  }
  return sent;
}

/*
 * find_id - find a dns request id (id is determined by dn_mkquery)
 */
static ResRQ* find_id(int id)
{
  ResRQ* request;

  for (request = requestListHead; request; request = request->next) {
    if (request->id == id)
      return request;
  }
  return NULL;
}

/*
 * gethost_byname_type - get host address from name
 */

struct DNSReply* gethost_byname_type(const char* name, 
                               const struct DNSQuery* query,
                               int type)
{
  aCache* cp;
  assert(0 != name);

  ++reinfo.re_na_look;
  if ((cp = find_cache_name(name)))
    return &(cp->reply);

  do_query_name(query, name, NULL, type);
  nextDNSCheck = 1;
  return NULL;
}

/* Try to get a IPv6 address first, un;ess we're not using IPv6; in which case
 * just look up boring old v4 - stu
 */
struct DNSReply* gethost_byname(const char* name, const struct DNSQuery* query)
{
#ifdef IPV6
  return gethost_byname_type(name, query, T_AAAA);
#else
  return gethost_byname_type(name, query, T_A);
#endif
}
/*
 * gethost_byaddr - get host name from address
 */
struct DNSReply* gethost_byaddr(const char* addr,
                                const struct DNSQuery* query)
{
  aCache *cp;

  assert(0 != addr);

  ++reinfo.re_nu_look;
  if ((cp = find_cache_number(NULL, addr)))
    return &(cp->reply);

  do_query_number(query, (const struct IN_ADDR*)addr, NULL);
  nextDNSCheck = 1;
  return NULL;
}
/*
 * do_query_name - nameserver lookup name
 */
static void do_query_name(const struct DNSQuery* query, 
                          const char* name, ResRQ* request, int type)
{
  char  hname[HOSTLEN + 1];
  assert(0 != name);

  strncpy_irc(hname, name, HOSTLEN);
  hname[HOSTLEN] = '\0';
  add_local_domain(hname, HOSTLEN);

  if (!request) {
    request       = make_request(query);
    request->type = type;
    request->name = (char*) MyMalloc(strlen(hname) + 1);
    strcpy(request->name, hname);
  }
  query_name(hname, C_IN, type, request);
}

/*
 * do_query_number - Use this to do reverse IP# lookups.
 */
static void do_query_number(const struct DNSQuery* query, 
			    const struct IN_ADDR* addr,
                            ResRQ* request)
{
  char  ipbuf[128];
  const unsigned char* cp;

#ifdef IPV6
  cp = (const u_char *)addr->s6_addr;
  if (cp[0]==0 && cp[1]==0 && cp[2]==0 && cp[3]==0 && cp[4]==0 &&
      cp[5]==0 && cp[6]==0 && cp[7]==0 && cp[8]==0 && cp[9]==0 &&
      ((cp[10]==0 && cp[11]==0) || (cp[10]==0xff && cp[11]==0xff)))
    {
      (void)sprintf(ipbuf, "%u.%u.%u.%u.in-addr.arpa.",
                (u_int)(cp[15]), (u_int)(cp[14]),
                (u_int)(cp[13]), (u_int)(cp[12]));
    } 
     else
    {
          (void)sprintf(ipbuf,
 "%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.ip6.int.",
         (u_int)(cp[15]&0xf), (u_int)(cp[15]>>4),
         (u_int)(cp[14]&0xf), (u_int)(cp[14]>>4),
         (u_int)(cp[13]&0xf), (u_int)(cp[13]>>4),
         (u_int)(cp[12]&0xf), (u_int)(cp[12]>>4),
         (u_int)(cp[11]&0xf), (u_int)(cp[11]>>4),
         (u_int)(cp[10]&0xf), (u_int)(cp[10]>>4),
         (u_int)(cp[9]&0xf), (u_int)(cp[9]>>4),
         (u_int)(cp[8]&0xf), (u_int)(cp[8]>>4),
         (u_int)(cp[7]&0xf), (u_int)(cp[7]>>4),
         (u_int)(cp[6]&0xf), (u_int)(cp[6]>>4),
         (u_int)(cp[5]&0xf), (u_int)(cp[5]>>4),
         (u_int)(cp[4]&0xf), (u_int)(cp[4]>>4),
         (u_int)(cp[3]&0xf), (u_int)(cp[3]>>4),
         (u_int)(cp[2]&0xf), (u_int)(cp[2]>>4),
         (u_int)(cp[1]&0xf), (u_int)(cp[1]>>4),
         (u_int)(cp[0]&0xf), (u_int)(cp[0]>>4));
         }
#else
    cp = (const u_char *)&addr->s_addr;
    (void)sprintf(ipbuf, "%u.%u.%u.%u.in-addr.arpa.",
              (u_int)(cp[3]), (u_int)(cp[2]),
              (u_int)(cp[1]), (u_int)(cp[0]));
#endif
   
  assert(0 != addr);   
  if (!request) {
    request              = make_request(query);
    request->type        = T_PTR;
 #ifdef IPV6
        bcopy((const char *)addr->s6_addr,
            (char *)&request->addr.s6_addr, sizeof(struct in6_addr));
#else
        bcopy((const char *)&addr->s_addr,
            (char *)&request->addr.s_addr, sizeof(struct in_addr));
#endif
  }  
  query_name(ipbuf, C_IN, T_PTR, request);
}

/*
 * query_name - generate a query based on class, type and name.
 */
static void query_name(const char* name, int query_class,
                       int type, ResRQ* request)
{
  char buf[MAXPACKET];
  int  request_len = 0;

  assert(0 != name);
  assert(0 != request);

  memset(buf, 0, sizeof(buf));
  if ((request_len = res_mkquery(QUERY, name, query_class, type, 
                                 NULL, 0, NULL, (u_char *) buf, sizeof(buf))) > 0) {
    HEADER* header = (HEADER*) buf;
#ifndef LRAND48
    int            k = 0;
    struct timeval tv;
#endif
    /*
     * generate a unique id
     * NOTE: we don't have to worry about converting this to and from
     * network byte order, the nameserver does not interpret this value
     * and returns it unchanged
     */
#ifdef LRAND48
    do {
      header->id = (header->id + lrand48()) & 0xffff;
    } while (find_id(header->id));
#else
    gettimeofday(&tv, NULL);
    do {
      header->id = (header->id + k + tv.tv_usec) & 0xffff;
      k++;
    } while (find_id(header->id));
#endif /* LRAND48 */
    request->id = header->id;
    ++request->sends;

    request->sent += send_res_msg(buf, request_len, request->sends);
  }
}

static void resend_query(ResRQ* request)
{
  assert(0 != request);

  if (request->resend == 0)
    return;
  ++reinfo.re_resends;
  switch(request->type) {
  case T_PTR:
    do_query_number(NULL, &request->addr, request);
    break;
#ifdef IPV6
  case T_AAAA:
#endif
  case T_A:
    do_query_name(NULL, request->name, request, request->type);
    break;
  default:
    break;
  }
}

/*
 * proc_answer - process name server reply
 * build a hostent struct in the passed request
 */
static int proc_answer(ResRQ* request, HEADER* header,
                       char* buf, char* eob)
{
  char   hostbuf[HOSTLEN+1+100]; /* working buffer */
  unsigned char*  current;              /* current position in buf */
  char** alias;                /* alias list */
  char** addr;                 /* address list */
  char*  name;                 /* pointer to name string */
  char*  address;              /* pointer to address */
  char*  endp;                 /* end of our buffer */
  int    query_class;          /* answer class */
  int    type;                 /* answer type */
  unsigned int    rd_length;            /* record data length */
  int    answer_count = 0;     /* answer counter */
  int    n;                    /* temp count */
  int    addr_count  = 0;      /* number of addresses in hostent */
  int    alias_count = 0;      /* number of aliases in hostent */
  struct hostent* hp;          /* hostent getting filled */

  assert(0 != request);
  assert(0 != header);
  assert(0 != buf);
  assert(0 != eob);
  
  current = (unsigned char *) buf + sizeof(HEADER);
  hp = &(request->he.h);
  /*
   * lazy allocation of request->he.buf, we don't allocate a buffer
   * unless there is something to put in it.
   */
  if (!request->he.buf) {
    request->he.buf = (char*) MyMalloc(MAXGETHOSTLEN + 1);
    request->he.buf[MAXGETHOSTLEN] = '\0';
    /*
     * array of alias list pointers starts at beginning of buf
     */
    hp->h_aliases = (char**) request->he.buf;
    hp->h_aliases[0] = NULL;
    /*
     * array of address list pointers starts after alias list pointers
     * the actual addresses follow the the address list pointers
     */ 
    hp->h_addr_list = (char**)(request->he.buf + ALIAS_BLEN);
    /*
     * don't copy the host address to the beginning of h_addr_list
     */
    hp->h_addr_list[0] = NULL;
  }
  endp = request->he.buf + MAXGETHOSTLEN;
  /*
   * find the end of the address list
   */
  addr = hp->h_addr_list;
  while (*addr) {
    ++addr;
    ++addr_count;
  }
  /*
   * make address point to first available address slot
   */
      address = request->he.buf + ADDRS_OFFSET +
                     (sizeof(struct IN_ADDR) * addr_count);

  /*
   * find the end of the alias list
   */
  alias = hp->h_aliases;
  while (*alias) {
    ++alias;
    ++alias_count;
  }
  /*
   * make name point to first available space in request->buf
   */
  if (alias_count > 0) {
    name = hp->h_aliases[alias_count - 1];
    name += (strlen(name) + 1);
  }
  else if (hp->h_name)
    name = hp->h_name + strlen(hp->h_name) + 1;
  else
    name = request->he.buf + ADDRS_OFFSET + ADDRS_DLEN;
 
  /*
   * skip past queries
   */ 
  for (; header->qdcount > 0; --header->qdcount) {
    if ((n = dn_skipname(current, (u_char *) eob)) < 0)
      break;
    current += (size_t) n + QFIXEDSZ;
  }
  /*
   * process each answer sent to us blech.
   */
  while (header->ancount-- > 0 && (char *) current < eob && name < endp) {
    n = dn_expand((u_char *) buf, (u_char *) eob, current, hostbuf, sizeof(hostbuf));
    if (n < 0) {
      /*
       * broken message
       */
      return 0;
    }
    else if (n == 0) {
      /*
       * no more answers left
       */
      return answer_count;
    }
    hostbuf[HOSTLEN] = '\0';
    /* 
     * With Address arithmetic you have to be very anal
     * this code was not working on alpha due to that
     * (spotted by rodder/jailbird/dianora)
     */
    current += (size_t) n;

    if (!(((char *)current + ANSWER_FIXED_SIZE) < eob))
      break;

    type = _getshort(current);
    current += TYPE_SIZE;

    query_class = _getshort(current);
    current += CLASS_SIZE;

    request->ttl = _getlong(current);
    current += TTL_SIZE;

    rd_length = _getshort(current);
    current += RDLENGTH_SIZE;

    /* 
     * Wait to set request->type until we verify this structure 
     */
    /* add_local_domain(hostbuf, HOSTLEN); */

    switch(type) {
#ifdef IPV6
      case T_AAAA:
#endif
      case T_A:
      /*
       * check for invalid rd_length or too many addresses
       */
      if (rd_length != ((type==T_AAAA) ? sizeof(struct in6_addr) :
          sizeof(struct in_addr)))
        return answer_count;
      if (++addr_count < RES_MAXADDRS) {
        if (answer_count == 1)
          hp->h_addrtype = (query_class == C_IN) ?  AF_INET : AF_UNSPEC;


#ifdef IPV6
        if (type == T_AAAA)
          bcopy((const char*)current, address, rd_length);
        else {
          struct in6_addr tmp;
          char nam[HOSTLEN];
          bcopy(address, (char*)tmp.s6_addr, sizeof(struct in6_addr));
          inetntop(AFINET, &tmp, nam, HOSTLEN);
          
          /* ugly hack */
          memset(tmp.s6_addr, 0, 10);
          tmp.s6_addr[10] = tmp.s6_addr[11] = 0xff;
          memcpy(tmp.s6_addr+12, current, 4);
          bcopy((const char*)tmp.s6_addr, address, sizeof(struct in6_addr));
        }
#else
        bcopy(current, address, rd_length);
#endif

        *addr++ = address;
        *addr = 0;
        address += sizeof(struct IN_ADDR);
        if (!hp->h_name) {
          strcpy(name, hostbuf);
          hp->h_name = name;
          name += strlen(name) + 1;
        }
        Debug((DEBUG_INFO,"got ip # %s for %s", 
              inetntoa((char*) hp->h_addr_list[addr_count - 1]), hostbuf));
      }
      current += rd_length;
      ++answer_count;
      break;
    case T_PTR:
      n = dn_expand((u_char *) buf, (u_char *) eob, current, hostbuf, sizeof(hostbuf));
      if (n < 0) {
        /*
         * broken message
         */
        return 0;
      }
      else if (n == 0) {
        /*
         * no more answers left
         */
        return answer_count;
      }
      /*
       * This comment is based on analysis by Shadowfax, Wohali and johan, 
       * not me.  (Dianora) I am only commenting it.
       *
       * dn_expand is guaranteed to not return more than sizeof(hostbuf)
       * but do all implementations of dn_expand also guarantee
       * buffer is terminated with null byte? Lets not take chances.
       *  -Dianora
       */
      hostbuf[HOSTLEN] = '\0';
      current += (size_t) n;

      Debug((DEBUG_INFO,"got host %s",hostbuf));
      /*
       * copy the returned hostname into the host name
       * ignore duplicate ptr records
       */
      if (!hp->h_name) {
        strcpy(name, hostbuf);
        hp->h_name = name;
        name += strlen(name) + 1;
      }
      ++answer_count;
      break;
    case T_CNAME:
      Debug((DEBUG_INFO,"got cname %s", hostbuf));
      if (++alias_count < RES_MAXALIASES) {
        strncpy_irc(name, hostbuf, endp - name);
        *alias++ = name;
        *alias   = 0;
        name += strlen(name) + 1;
      }
      current += rd_length;
      ++answer_count;
      break;
    default :
#ifdef DEBUG
      Debug((DEBUG_INFO,"proc_answer: type:%d for:%s", type, hostbuf));
#endif
      break;
    }
  }
  return answer_count;
}

/*
 * get_res - read a dns reply from the nameserver and process it.
 */
void get_res(void)
{
  char               buf[sizeof(HEADER) + MAXPACKET];
  HEADER*            header;
  ResRQ*             request = NULL;
  aCache*            cp = NULL;
  int                rc;
  int                answer_count;
  int 	             type;
  int         len = sizeof(struct sockaddr_in);
  struct sockaddr_in sin;
  struct IN_ADDR     mapped;

  rc = recvfrom(ResolverFileDescriptor, buf, sizeof(buf), 0, 
                (struct sockaddr*) &sin, &len);
  if (rc <= sizeof(HEADER))
    return;
  /*
   * convert DNS reply reader from Network byte order to CPU byte order.
   */
  header = (HEADER*) buf;
  /* header->id = ntohs(header->id); */
  header->ancount = ntohs(header->ancount);
  header->qdcount = ntohs(header->qdcount);
  header->nscount = ntohs(header->nscount);
  header->arcount = ntohs(header->arcount);
#ifdef  DEBUG
  Debug((DEBUG_NOTICE, "get_res:id = %d rcode = %d ancount = %d",
         header->id, header->rcode, header->ancount));
#endif
  ++reinfo.re_replies;
  /*
   * response for an id which we have already received an answer for
   * just ignore this response.
   */
  if (0 == (request = find_id(header->id)))
    return;
  /*
   * check against possibly fake replies
   */
  if (!res_ourserver(&_res, &sin)) {
    ++reinfo.re_unkrep;
    return;
  }

  if ((header->rcode != NOERROR) || (header->ancount == 0)) {
    ++reinfo.re_errors;
    if (SERVFAIL == header->rcode)
      resend_query(request);
	else if(request->type == T_AAAA)
	{
      request->type = T_A;
      resend_query(request);
	}
    else {
      /*
       * If a bad error was returned, we stop here and dont send
       * send any more (no retries granted).
       */
      Debug((DEBUG_DNS, "Fatal DNS error: %d", header->rcode));
      (*request->query.callback)(request->query.vptr, 0);
      rem_request(request);
    } 
    return;
  }
  /*
   * If this fails there was an error decoding the received packet, 
   * try it again and hope it works the next time.
   */
  answer_count = proc_answer(request, header, buf, buf + rc);
#ifdef DEBUG
  Debug((DEBUG_INFO,"get_res:Proc answer = %d",a));
#endif
  if (answer_count) {
    if (request->type == T_PTR) {
      struct DNSReply* reply = NULL;
      if (0 == request->he.h.h_name) {
        /*
         * got a PTR response with no name, something bogus is happening
         * don't bother trying again, the client address doesn't resolve
         */
        (*request->query.callback)(request->query.vptr, reply);
        rem_request(request);
        return;
      }
      
      Debug((DEBUG_DNS, "relookup %s <-> %s",
             request->he.h.h_name, inetntoa((char*) &request->he.h.h_addr)));
      /*
       * Lookup the 'authoritive' name that we were given for the
       * ip#.  By using this call rather than regenerating the
       * type we automatically gain the use of the cache with no
       * extra kludges.
       */
#ifdef  IPV6
      bcopy((const char*)request->addr.S_ADDR, (char*)&mapped.S_ADDR, sizeof(struct IN_ADDR));
      if (IN6_IS_ADDR_V4MAPPED(&mapped))
      {
        type = T_A;
      }
      else
      {
        type = T_AAAA;
      }
#else
      type = T_A;
#endif

      reply = gethost_byname_type(request->he.h.h_name, &request->query, type);
      if (0 == reply) {
        /*
         * If name wasn't found, a request has been queued and it will
         * be the last one queued.  This is rather nasty way to keep
         * a host alias with the query. -avalon
         */
        MyFree(requestListTail->he.buf);
        requestListTail->he.buf = request->he.buf;
        request->he.buf = 0;
        memcpy(&requestListTail->he.h, &request->he.h, sizeof(struct hostent));
      }
      else
        (*request->query.callback)(request->query.vptr, reply);
      rem_request(request);
    }
    else {
      /*
       * got a name and address response, client resolved
       */
      cp = make_cache(request);

      /* if cp is NULL then don't try to use it...&cp->reply will be 0x2c
       * in this case btw.
       * make_cache can return NULL if hp->h_name and hp->h_addr_list[0]
       * are NULL -Dianora
       */

      if(cp)
        {        
          (*request->query.callback)(request->query.vptr, &cp->reply);
        }
      else
        {
          (*request->query.callback)(request->query.vptr, 0 );
        }

#ifdef  DEBUG
      Debug((DEBUG_INFO,"get_res:cp=%#x request=%#x (made)",cp,request));
#endif
      rem_request(request);
    }
  }
  else if (!request->sent) {
    /*
     * XXX - we got a response for a query we didn't send with a valid id?
     * this should never happen, bail here and leave the client unresolved
     */
    (*request->query.callback)(request->query.vptr, 0);
    rem_request(request);
  }
}


/*
 * dup_hostent - Duplicate a hostent struct, allocate only enough memory for
 * the data we're putting in it.
 */
static void dup_hostent(aHostent* new_hp, struct hostent* hp)
{
  char*  p;
  char** ap;
  char** pp;
  int    alias_count = 0;
  int    addr_count = 0;
  size_t bytes_needed = 0;

  assert(0 != new_hp);
  assert(0 != hp);

  /* how much buffer do we need? */
  bytes_needed += (strlen(hp->h_name) + 1);

  pp = hp->h_aliases;
  while (*pp) {
    bytes_needed += (strlen(*pp++) + 1 + sizeof(void*));
    ++alias_count;
  }
  pp = hp->h_addr_list;
  while (*pp++) {
    bytes_needed += (hp->h_length + sizeof(void*));
    ++addr_count;
  }
  /* Reserve space for 2 nulls to terminate h_aliases and h_addr_list */
  bytes_needed += (2 * sizeof(void*));

  /* Allocate memory */
  new_hp->buf = (char*) MyMalloc(bytes_needed);

  new_hp->h.h_addrtype = hp->h_addrtype;
  new_hp->h.h_length = hp->h_length;

  /* first write the address list */
  pp = hp->h_addr_list;
  ap = new_hp->h.h_addr_list =
      (char**)(new_hp->buf + ((alias_count + 1) * sizeof(void*)));
  p = (char*)ap + ((addr_count + 1) * sizeof(void*));
  while (*pp)
  {
    *ap++ = p;
    memcpy(p, *pp++, hp->h_length);
    p += hp->h_length;
  }
  *ap = 0;
  /* next write the name */
  new_hp->h.h_name = p;
  strcpy(p, hp->h_name);
  p += (strlen(p) + 1);

  /* last write the alias list */
  pp = hp->h_aliases;
  ap = new_hp->h.h_aliases = (char**) new_hp->buf;
  while (*pp) {
    *ap++ = p;
    strcpy(p, *pp++);
    p += (strlen(p) + 1);
  }
  *ap = 0;
}

/*
 * update_hostent - Add records to a Hostent struct in place.
 */
static int update_hostent(aHostent* hp, char** addr, char** alias)
{
  char*  p;
  char** ap;
  char** pp;
  int    alias_count = 0;
  int    addr_count = 0;
  char*  buf = NULL;
  size_t bytes_needed = 0;

  if (!hp || !hp->buf)
    return -1;

  /* how much buffer do we need? */
  bytes_needed = strlen(hp->h.h_name) + 1;
  pp = hp->h.h_aliases;
  while (*pp) {
    bytes_needed += (strlen(*pp++) + 1 + sizeof(void*));
    ++alias_count;
  }
  if (alias) {
    pp = alias;
    while (*pp) {
      bytes_needed += (strlen(*pp++) + 1 + sizeof(void*));
      ++alias_count;
    }
  }
  pp = hp->h.h_addr_list;
  while (*pp++) {
    bytes_needed += (hp->h.h_length + sizeof(void*));
    ++addr_count;
  }
  if (addr) {
    pp = addr;
    while (*pp++) {
      bytes_needed += (hp->h.h_length + sizeof(void*));
      ++addr_count;
    }
  }
  /* Reserve space for 2 nulls to terminate h_aliases and h_addr_list */
  bytes_needed += 2 * sizeof(void*);

  /* Allocate memory */
  buf = (char*) MyMalloc(bytes_needed);

  /* first write the address list */
  pp = hp->h.h_addr_list;
  ap = hp->h.h_addr_list =
      (char**)(buf + ((alias_count + 1) * sizeof(void*)));
  p = (char*)ap + ((addr_count + 1) * sizeof(void*));
  while (*pp) {
    memcpy(p, *pp++, hp->h.h_length);
    *ap++ = p;
    p += hp->h.h_length;
  }
  if (addr) {
    while (*addr) {
      memcpy(p, *addr++, hp->h.h_length);
      *ap++ = p;
      p += hp->h.h_length;
    }
  }
  *ap = 0;

  /* next write the name */
  strcpy(p, hp->h.h_name);
  hp->h.h_name = p;
  p += (strlen(p) + 1);

  /* last write the alias list */
  pp = hp->h.h_aliases;
  ap = hp->h.h_aliases = (char**) buf;
  while (*pp) {
    strcpy(p, *pp++);
    *ap++ = p;
    p += (strlen(p) + 1);
  }
  if (alias) {
    while (*alias) {
      strcpy(p, *alias++);
      *ap++ = p;
      p += (strlen(p) + 1);
    }
  }
  *ap = 0;
  /* release the old buffer */
  p = hp->buf;
  hp->buf = buf;
  MyFree(p);
  return 0;
}

/*
 * hash_number - IP address hash function
 */
static int hash_number(const unsigned char* ip)
{
  /* could use loop but slower */
  unsigned int hashv = (unsigned int) *ip++;
  hashv += hashv + (unsigned int) *ip++;
  hashv += hashv + (unsigned int) *ip++;
  hashv += hashv + (unsigned int) *ip;
  hashv %= ARES_CACSIZE;
  return (hashv);
}

/*
 * hash_name - hostname hash function
 */
static int hash_name(const char* name)
{
  unsigned int hashv = 0;

  for (; *name && *name != '.'; name++)
    hashv += *name;
  hashv %= ARES_CACSIZE;
  return (hashv);
}

/*
 * add_to_cache - Add a new cache item to the queue and hash table.
 */
static aCache* add_to_cache(aCache* ocp)
{
  aCache  *cp = NULL;
  int  hashv;

#ifdef DEBUG
  Debug((DEBUG_INFO, "add_to_cache:ocp %#x he %#x name %#x addrl %#x 0 %#x",
         ocp, &ocp->he, ocp->he.h.h_name, ocp->he.h.h_addr_list,
         ocp->he.h.h_addr_list[0]));
#endif
  ocp->list_next = cacheTop;
  cacheTop = ocp;

  /*
   * Make sure non-bind resolvers don't blow up (Thanks to Yves)
   */
  if (!ocp) return NULL;
  if (!(ocp->he.h.h_name)) return NULL;
  if (!(ocp->he.h.h_addr)) return NULL;
  
  hashv = hash_name(ocp->he.h.h_name);

  ocp->hname_next = hashtable[hashv].name_list;
  hashtable[hashv].name_list = ocp;

  hashv = hash_number((unsigned char *)ocp->he.h.h_addr);

  ocp->hnum_next = hashtable[hashv].num_list;
  hashtable[hashv].num_list = ocp;

#ifdef  DEBUG
  Debug((DEBUG_INFO, "add_to_cache:added %s[%08x] cache %#x.",
         ocp->he.h.h_name, ocp->he.h.h_addr_list[0], ocp));
  Debug((DEBUG_INFO,
         "add_to_cache:h1 %d h2 %x lnext %#x namnext %#x numnext %#x",
         hash_name(ocp->he.h.h_name), hashv, ocp->list_next,
         ocp->hname_next, ocp->hnum_next));
#endif

  /*
   * LRU deletion of excessive cache entries.
   */
  if (++cachedCount > MAXCACHED) {
    for (cp = cacheTop; cp->list_next; cp = cp->list_next)
      ;
    rem_cache(cp);
  }
  ++cainfo.ca_adds;
  return ocp;
}

/*
 * update_list - does not alter the cache structure passed. It is assumed that
 * it already contains the correct expire time, if it is a new entry. Old
 * entries have the expirey time updated.
*/
static void update_list(ResRQ* request, aCache* cachep)
{
  aCache** cpp;
  aCache*  cp = cachep;
  char*    s;
  char*    t;
  int      i;
  int      j;
  char**   ap;
  char*    addrs[RES_MAXADDRS + 1];
  char*    aliases[RES_MAXALIASES + 1];

  /*
   * search for the new cache item in the cache list by hostname.
   * If found, move the entry to the top of the list and return.
   */
  ++cainfo.ca_updates;

  for (cpp = &cacheTop; *cpp; cpp = &((*cpp)->list_next)) {
    if (cp == *cpp)
      break;
  }
  if (!*cpp)
    return;
  *cpp = cp->list_next;
  cp->list_next = cacheTop;
  cacheTop = cp;
  if (!request)
    return;

#ifdef  DEBUG
  Debug((DEBUG_DEBUG,"u_l:cp %#x na %#x al %#x ad %#x",
         cp, cp->he.h.h_name, cp->he.h.h_aliases, cp->he.h.h_addr));
  Debug((DEBUG_DEBUG,"u_l:request %#x h_n %#x", 
         request, request->he.h.h_name));
#endif
  /*
   * Compare the cache entry against the new record.  Add any
   * previously missing names for this entry.
   */
  *aliases = 0;
  ap = aliases;
  for (i = 0, s = request->he.h.h_name; s; s = request->he.h.h_aliases[i++]) {
    for (j = 0, t = cp->he.h.h_name; t; t = cp->he.h.h_aliases[j++]) {
      if (0 == irccmp(t, s))
        break;
    }
    if (!t) {
      *ap++ = s;
      *ap = 0;
    }
  }
  /*
   * Do the same again for IP#'s.
   */
  *addrs = 0;
  ap = addrs;
  for (i = 0; (s = request->he.h.h_addr_list[i]); i++) {
    for (j = 0; (t = cp->he.h.h_addr_list[j]); j++) {
      if (!memcmp(t, s, sizeof(struct IN_ADDR)))
        break;
    }
    if (!t) {
      *ap++ = s;
      *ap = 0;
    }
  }
  if (*addrs || *aliases)
    update_hostent(&cp->he, addrs, aliases);
}

/*
 * find_cache_name - find name in nameserver cache
 */
static aCache* find_cache_name(const char* name)
{
  aCache* cp;
  char*   s;
  int     hashv;
  int     i;

  assert(0 != name);
  hashv = hash_name(name);

  cp = hashtable[hashv].name_list;
#ifdef  DEBUG
  Debug((DEBUG_DNS,"find_cache_name:find %s : hashv = %d",name,hashv));
#endif

  for (; cp; cp = cp->hname_next) {
    for (i = 0, s = cp->he.h.h_name; s; s = cp->he.h.h_aliases[i++]) {
      if (irccmp(s, name) == 0) {
        ++cainfo.ca_na_hits;
        update_list(NULL, cp);
        return cp;
      }
    }
  }

  for (cp = cacheTop; cp; cp = cp->list_next) {
    /*
     * if no aliases or the hash value matches, we've already
     * done this entry and all possiblilities concerning it.
     */
    if (!*cp->he.h.h_aliases)
      continue;
    if (!cp->he.h.h_name)   /* don't trust anything -Dianora */
      continue;
    if (hashv == hash_name(cp->he.h.h_name))
      continue;
    for (i = 0, s = cp->he.h.h_aliases[i]; s && i < RES_MAXALIASES; i++) {
      if (!irccmp(name, s)) {
        cainfo.ca_na_hits++;
        update_list(NULL, cp);
        return cp;
      }
    }
  }
  return NULL;
}

/*
 * find_cache_number - find a cache entry by ip# and update its expire time
 */
static aCache* find_cache_number(ResRQ* request, const char* addr)
{
  aCache* cp;
  int     hashv;
  int     i;
#ifdef  DEBUG
  struct IN_ADDR* ip = (struct IN_ADDR*) addr;
#endif

  assert(0 != addr);
  hashv = hash_number((unsigned char*) addr);
  cp = hashtable[hashv].num_list;
#ifdef DEBUG
  Debug((DEBUG_DNS,"find_cache_number:find %s[%08x]: hashv = %d",
         inetntoa(addr), ntohl(ip->s_addr), hashv));
#endif

  for (; cp; cp = cp->hnum_next) {
    for (i = 0; cp->he.h.h_addr_list[i]; i++) {
      if (!memcmp(cp->he.h.h_addr_list[i], addr, sizeof(struct IN_ADDR))) {
        cainfo.ca_nu_hits++;
        update_list(NULL, cp);
        return cp;
      }
    }
  }
  for (cp = cacheTop; cp; cp = cp->list_next) {
    /*
     * single address entry...would have been done by hashed
     * search above...
     */
    if (!cp->he.h.h_addr_list[1])
      continue;
    /*
     * if the first IP# has the same hashnumber as the IP# we
     * are looking for, its been done already.
     */
    if (hashv == hash_number((unsigned char*)cp->he.h.h_addr_list[0]))
      continue;
    for (i = 1; cp->he.h.h_addr_list[i]; i++) {
      if (!memcmp(cp->he.h.h_addr_list[i], addr, sizeof(struct IN_ADDR))) {
        cainfo.ca_nu_hits++;
        update_list(NULL, cp);
        return cp;
      }
    }
  }
  return NULL;
}

static aCache* make_cache(ResRQ* request)
{
  aCache* cp;
  int     i;
  struct hostent* hp;
  assert(0 != request);

  hp = &request->he.h;
  /*
   * shouldn't happen but it just might...
   */
  if (!hp->h_name || !hp->h_addr_list[0])
    return NULL;
  /*
   * Make cache entry.  First check to see if the cache already exists
   * and if so, return a pointer to it.
   */
  for (i = 0; hp->h_addr_list[i]; ++i) {
    if ((cp = find_cache_number(request, hp->h_addr_list[i])))
      return cp;
  }
  /*
   * a matching entry wasnt found in the cache so go and make one up.
   */ 
  cp = (aCache*) MyMalloc(sizeof(aCache));
  memset(cp, 0, sizeof(aCache));
  dup_hostent(&cp->he, hp);
  cp->reply.hp = &cp->he.h;

  if (request->ttl < 600) {
    ++reinfo.re_shortttl;
    cp->ttl = 600;
  }
  else
    cp->ttl = request->ttl;
  cp->expireat = CurrentTime + cp->ttl;
#ifdef DEBUG
  Debug((DEBUG_INFO,"make_cache:made cache %#x", cp));
#endif
  return add_to_cache(cp);
}

/*
 * rem_cache - delete a cache entry from the cache structures 
 * and lists and return all memory used for the cache back to the memory pool.
 */
static void rem_cache(aCache* ocp)
{
  aCache**        cp;
  int             hashv;
  struct hostent* hp;
  assert(0 != ocp);

  if (0 < ocp->reply.ref_count) {
    ocp->expireat = CurrentTime + AR_TTL;
    return;
  }
  hp = &ocp->he.h;

#ifdef  DEBUG
  Debug((DEBUG_DNS, "rem_cache: ocp %#x hp %#x l_n %#x aliases %#x",
         ocp, hp, ocp->list_next, hp->h_aliases));
#endif
  /*
   * remove cache entry from linked list
   */
  for (cp = &cacheTop; *cp; cp = &((*cp)->list_next)) {
    if (*cp == ocp) {
      *cp = ocp->list_next;
      break;
    }
  }
  /*
   * XXX - memory leak!!!?
   * remove cache entry from hashed name lists
   */
  assert(0 != hp->h_name);
  if (hp->h_name == 0)
    return;
  hashv = hash_name(hp->h_name);

#ifdef  DEBUG
  Debug((DEBUG_DEBUG,"rem_cache: h_name %s hashv %d next %#x first %#x",
        hp->h_name, hashv, ocp->hname_next, hashtable[hashv].name_list));
#endif
  for (cp = &hashtable[hashv].name_list; *cp; cp = &((*cp)->hname_next)) {
    if (*cp == ocp) {
      *cp = ocp->hname_next;
      break;
    }
  }
  /*
   * remove cache entry from hashed number list
   */
  hashv = hash_number((unsigned char *)hp->h_addr);
  /*
   * XXX - memory leak!!!?
   */
  assert(-1 < hashv);
  if (hashv < 0)
    return;
#ifdef  DEBUG
  Debug((DEBUG_DEBUG,"rem_cache: h_addr %s hashv %d next %#x first %#x",
         inetntoa(hp->h_addr), hashv, ocp->hnum_next,
         hashtable[hashv].num_list));
#endif
  for (cp = &hashtable[hashv].num_list; *cp; cp = &((*cp)->hnum_next)) {
    if (*cp == ocp) {
      *cp = ocp->hnum_next;
      break;
    }
  }
  /*
   * free memory used to hold the various host names and the array
   * of alias pointers.
   */
  MyFree(ocp->he.buf);
  MyFree(ocp);
  --cachedCount;
  ++cainfo.ca_dels;
}

/*
 * m_dns - dns status query
 */
int m_dns(aClient *cptr, aClient *sptr, int parc, char *parv[])
{
  aCache* cp;
  int     i;
  struct hostent* hp;

  if (!IsAnOper(sptr))
    return 0;

  if (parv[1] && *parv[1] == 'l') {
    for(cp = cacheTop; cp; cp = cp->list_next) {
      hp = &cp->he.h;
      sendto_one(sptr, "NOTICE %s :Ex %d ttl %d host %s(%s)",
                 parv[0], cp->expireat - CurrentTime, cp->ttl,
                 hp->h_name, inetntoa(hp->h_addr));
      for (i = 0; hp->h_aliases[i]; i++)
        sendto_one(sptr,"NOTICE %s : %s = %s (CN)",
                   parv[0], hp->h_name, hp->h_aliases[i]);
      for (i = 1; hp->h_addr_list[i]; i++)
        sendto_one(sptr,"NOTICE %s : %s = %s (IP)",
                   parv[0], hp->h_name, inetntoa(hp->h_addr_list[i]));
    }
    return 0;
  }
  if (parv[1] && *parv[1] == 'd') {
    sendto_one(sptr, "NOTICE %s :ResolverFileDescriptor = %d", parv[0], ResolverFileDescriptor);
    return 0;
  }
  sendto_one(sptr,"NOTICE %s :Ca %d Cd %d Ce %d Cl %d Ch %d:%d Cu %d",
             sptr->name,
             cainfo.ca_adds, cainfo.ca_dels, cainfo.ca_expires,
             cainfo.ca_lookups, cainfo.ca_na_hits, cainfo.ca_nu_hits, 
             cainfo.ca_updates);
  
  sendto_one(sptr,"NOTICE %s :Re %d Rl %d/%d Rp %d Rq %d",
             sptr->name, reinfo.re_errors, reinfo.re_nu_look,
             reinfo.re_na_look, reinfo.re_replies, reinfo.re_requests);
  sendto_one(sptr,"NOTICE %s :Ru %d Rsh %d Rs %d(%d) Rt %d", sptr->name,
             reinfo.re_unkrep, reinfo.re_shortttl, reinfo.re_sent,
             reinfo.re_resends, reinfo.re_timeouts);
  return 0;
}

unsigned long cres_mem(aClient *sptr)
{
  aCache*          c = cacheTop;
  struct  hostent* h;
  int              i;
  unsigned long    nm = 0;
  unsigned long    im = 0;
  unsigned long    sm = 0;
  unsigned long    ts = 0;

  for ( ;c ; c = c->list_next) {
    sm += sizeof(*c);
    h = &c->he.h;
    for (i = 0; h->h_addr_list[i]; i++) {
      im += sizeof(char *);
      im += sizeof(struct IN_ADDR);
    }
    im += sizeof(char *);
    for (i = 0; h->h_aliases[i]; i++) {
      nm += sizeof(char *);
      nm += strlen(h->h_aliases[i]);
    }
    nm += i - 1;
    nm += sizeof(char *);
    if (h->h_name)
      nm += strlen(h->h_name);
  }
  ts = ARES_CACSIZE * sizeof(CacheTable);
  sendto_one(sptr, ":%s %d %s :RES table %d",
             me.name, RPL_STATSDEBUG, sptr->name, ts);
  sendto_one(sptr, ":%s %d %s :Structs %d IP storage %d Name storage %d",
             me.name, RPL_STATSDEBUG, sptr->name, sm, im, nm);
  return ts + sm + im + nm;
}

/*
 * clear_cache - removes all entries from the cache
 */
void clear_cache(void)
{
  aCache* cp;
  aCache* cp2;

  for (cp = cacheTop; cp; cp = cp2) {
    cp2 = cp->list_next;
      cainfo.ca_expires++;
      rem_cache(cp);
  }
  return;
}
#endif
