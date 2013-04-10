/************************************************************************
 *   IRC - Internet Relay Chat, src/s_auth.c
 *   Copyright (C) 1992 Darren Reed
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   $Id: s_auth.c,v 1.4 2005/08/27 16:23:50 jpinto Exp $
 *
 * Changes:
 *   July 6, 1999 - Rewrote most of the code here. When a client connects
 *     to the server and passes initial socket validation checks, it
 *     is owned by this module (auth) which returns it to the rest of the
 *     server when dns and auth queries are finished. Until the client is
 *     released, the server does not know it exists and does not process
 *     any messages from it.
 *     --Bleep  Thomas Helvey <tomh@inxpress.net>
 */
#include "m_commands.h"
#include "s_auth.h"
#include "client.h"
#include "common.h"
#include "fdlist.h"              /* fdlist_add */
#include "irc_string.h"
#include "ircd.h"
#include "numeric.h"
#include "res.h"
#include "s_bsd.h"
#include "s_log.h"
#include "s_stats.h"
#include "send.h"
#include "struct.h"
#ifdef HAVE_SSL
#include "ssl.h"
#endif
#include "dconf_vars.h"

#include <netdb.h>               /* struct hostent */
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * a bit different approach
 * this replaces the original sendheader macros
 */
static struct {
  const char* message;
  size_t      length;
} HeaderMessages [] = {
  /* 123456789012345678901234567890123456789012345678901234567890 */
  { "NOTICE AUTH :*** Looking up your hostname...\r\n",    46 },
  { "NOTICE AUTH :*** Found your hostname\r\n",            38 },
  { "NOTICE AUTH :*** Found your hostname, cached\r\n",    46 },
  { "NOTICE AUTH :*** Couldn't look up your hostname\r\n", 49 },
  { "NOTICE AUTH :*** Checking Ident\r\n",                 33 },
  { "NOTICE AUTH :*** Got Ident response\r\n",             37 },
  { "NOTICE AUTH :*** No Ident response\r\n",              36 },
  { "NOTICE AUTH :*** Your forward and reverse DNS do not match, " \
    "ignoring hostname.\r\n",                              80 },
  { "NOTICE AUTH :*** Your hostname is too long, ignoring hostname\r\n", 63 },
  { "NOTICE AUTH :*** Hostname lookup disabled, using your numeric IP\r\n", 66 }  
};

typedef enum {
  REPORT_DO_DNS,
  REPORT_FIN_DNS,
  REPORT_FIN_DNSC,
  REPORT_FAIL_DNS,
  REPORT_DO_ID,
  REPORT_FIN_ID,
  REPORT_FAIL_ID,
  REPORT_IP_MISMATCH,
  REPORT_HOST_TOOLONG,
  REPORT_NO_DNS,
} ReportType;

#ifdef HAVE_SSL 
#define sendheader(c, r) \
   if(IsSecure((c)))\
    ircd_SSL_write((c), HeaderMessages[(r)].message, HeaderMessages[(r)].length);\
   else\
    send((c)->fd, HeaderMessages[(r)].message, HeaderMessages[(r)].length, 0)
#else
#define sendheader(c, r) \
   send((c)->fd, HeaderMessages[(r)].message, HeaderMessages[(r)].length, 0)
#endif

struct AuthRequest* AuthPollList = 0; /* GLOBAL - auth queries pending io */

static struct AuthRequest* AuthIncompleteList = 0;

/*
 * make_auth_request - allocate a new auth request
 */
static struct AuthRequest* make_auth_request(struct Client* client)
{
  /*
   * XXX - use blalloc here?
   */
  struct AuthRequest* request = 
               (struct AuthRequest*) MyMalloc(sizeof(struct AuthRequest));
  assert(0 != request);
  memset(request, 0, sizeof(struct AuthRequest));
  request->fd      = -1;
  request->client  = client;
  request->timeout = CurrentTime + CONNECTTIMEOUT;
  return request;
}

/*
 * free_auth_request - cleanup auth request allocations
 */
void free_auth_request(struct AuthRequest* request)
{
  /*
   * XXX - use blfree here?
   */
  MyFree(request);
}

/*
 * unlink_auth_request - remove auth request from a list
 */
static void unlink_auth_request(struct AuthRequest* request,
                                struct AuthRequest** list)
{
  if (request->next)
    request->next->prev = request->prev;
  if (request->prev)
    request->prev->next = request->next;
  else
    *list = request->next;
}

/*
 * link_auth_request - add auth request to a list
 */
static void link_auth_request(struct AuthRequest* request,
                              struct AuthRequest** list)
{
  request->prev = 0;
  request->next = *list;
  if (*list)
    (*list)->prev = request;
  *list = request;
}

/*
 * release_auth_client - release auth client from auth system
 * this adds the client into the local client lists so it can be read by
 * the main io processing loop
 */
static void release_auth_client(struct Client* client)
{
  if (client->fd > highest_fd)
    highest_fd = client->fd;
  local[client->fd] = client;

  fdlist_add(client->fd, FDL_DEFAULT);
  add_client_to_list(client);
  
  SetAccess(client);
}



/*
 * auth_dns_callback - called when resolver query finishes
 * if the query resulted in a successful search, hp will contain
 * a non-null pointer, otherwise hp will be null.
 * set the client on it's way to a connection completion, regardless
 * of success of failure
 */

#ifndef USE_ADNS 
static void auth_dns_callback(void* vptr, struct DNSReply* reply)
{
  struct AuthRequest* auth = (struct AuthRequest*) vptr;

  ClearDNSPending(auth);
  if (reply)
    {
      struct hostent* hp = reply->hp;
#ifdef IPV6
      struct in6_addr temp;
      char name[HOSTLEN];
#endif
      
	int i;
      /*
       * Verify that the host to ip mapping is correct both ways and that
	* the ip#(s) for the socket is listed for the host.
	*/
      for (i = 0; hp->h_addr_list[i]; ++i)
      {
#ifdef IPV6
        inetntop(AFINET, hp->h_addr_list[i], name, HOSTLEN);
        inetpton(AFINET, name, &temp);
        if (!bcmp((char*)&temp, (char*)&auth->client->ip, sizeof(struct IN_ADDR)))
#else
        if (!bcmp(hp->h_addr_list[i], (char *)&auth->client->ip,
            sizeof(struct IN_ADDR)))
#endif
              break;
      }      
      if (!hp->h_addr_list[i])
	{
	  if(!HideConnectInfo)
            sendheader(auth->client, REPORT_IP_MISMATCH);
	}
      else
        {
          ++reply->ref_count;
	  auth->client->dns_reply = reply;
	  strncpy_irc(auth->client->host, hp->h_name, HOSTLEN);
	  if(!HideConnectInfo)
	    sendheader(auth->client, REPORT_FIN_DNS);
	}
    }
  else
    {
      /*
       * this should have already been done by s_bsd.c in add_connection
	*/
      strcpy(auth->client->host, auth->client->sockhost);
      if(!HideConnectInfo)
        sendheader(auth->client, REPORT_FAIL_DNS);
    }
    
  auth->client->host[HOSTLEN] = '\0';
  if (!IsDoingAuth(auth))
    {
        release_auth_client(auth->client);
	unlink_auth_request(auth, &AuthIncompleteList);
	free_auth_request(auth);
    }
}
 
#else 
static void auth_dns_callback(void* vptr, adns_answer* reply)
{
  struct AuthRequest* auth = (struct AuthRequest*) vptr;

  ClearDNSPending(auth);
  if (reply && (reply->status == adns_s_ok))
  {
      if(strlen(*reply->rrs.str) < HOSTLEN)
      {
        strcpy(auth->client->host, *reply->rrs.str);
        if(!HideConnectInfo)
          sendheader(auth->client, REPORT_FIN_DNS);
      } else {
        if(!HideConnectInfo)
          sendheader(auth->client, REPORT_HOST_TOOLONG);
      }
  } else
  {
     strcpy(auth->client->host, auth->client->sockhost);
     if(!HideConnectInfo)
       sendheader(auth->client, REPORT_FAIL_DNS);
  }
  MyFree(reply);
    
  auth->client->dns_query = NULL;
  auth->client->host[HOSTLEN] = '\0';
  if (!IsDoingAuth(auth))
    {
      release_auth_client(auth->client);
      unlink_auth_request(auth, &AuthIncompleteList);
      free_auth_request(auth);
    }
}
#endif

/*
 * authsenderr - handle auth send errors
 */
static void auth_error(struct AuthRequest* auth)
{
  ++ServerStats->is_abad;

  CLOSE(auth->fd);
  auth->fd = -1;

  ClearAuth(auth);
  if(!HideConnectInfo)
    sendheader(auth->client, REPORT_FAIL_ID);

  unlink_auth_request(auth, &AuthPollList);

  if (IsDNSPending(auth))
    link_auth_request(auth, &AuthIncompleteList);
  else
    {
      release_auth_client(auth->client);
      free_auth_request(auth);
    }
}

/*
 * start_auth_query - Flag the client to show that an attempt to 
 * contact the ident server on
 * the client's host.  The connect and subsequently the socket are all put
 * into 'non-blocking' mode.  Should the connect or any later phase of the
 * identifing process fail, it is aborted and the user is given a username
 * of "unknown".
 */
static int start_auth_query(struct AuthRequest* auth)
{
  struct SOCKADDR_IN sock;
  struct SOCKADDR_IN localaddr;
  socklen_t          locallen = sizeof(struct SOCKADDR_IN);
  int                fd;

  if ((fd = socket(AFINET, SOCK_STREAM, 0)) == -1)
    {
      /* er .. on the off chance we're having ident errors, we may leak a
       * server ip to a +d user. on the other hand, we should be spilling
       * such a large amount of ident errors on a busy server that the
       * server ip will be missed, and also there is no way that the person
       * seeing the notice would know that that ip was the server ..
       * we should change this sometime .... -gnp */
      report_error("creating auth stream socket %s:%s", 
		   get_client_name(auth->client, TRUE), errno);
      irclog(L_ERROR, "Unable to create auth socket for %s:%m",
	  get_client_name(auth->client, SHOW_IP));
      ++ServerStats->is_abad;
      return 0;
    }
  if ((MAXCONNECTIONS - 10) < fd)
    {
	  /* see my note on ip leakage above -gnp */
      sendto_realops("Can't allocate fd for auth on %s",
		     get_client_name(auth->client, TRUE));
      
      CLOSE(fd);
      return 0;
    }

  if(!HideConnectInfo)
    sendheader(auth->client, REPORT_DO_ID);
  if (!set_non_blocking(fd)) {
    report_error(NONB_ERROR_MSG, get_client_name(auth->client, SHOW_IP), errno);
    CLOSE(fd);
    return 0;
  }

  /* 
   * get the local address of the client and bind to that to
   * make the auth request.  This used to be done only for
   * ifdef VIRTTUAL_HOST, but needs to be done for all clients
   * since the ident request must originate from that same address--
   * and machines with multiple IP addresses are common now
   */
  memset(&localaddr, 0, locallen);
  getsockname(auth->client->fd, (struct sockaddr*) &localaddr, &locallen);
  localaddr.SIN_PORT = htons(0);

  if (bind(fd, (struct sockaddr*) &localaddr, sizeof(localaddr)))
    {
	  /* see my note above -gnp */
      report_error("binding auth stream socket %s:%s", 
		   get_client_name(auth->client, TRUE), errno);
      CLOSE(fd);
      return 0;
    }
  
#ifdef IPV6
  bcopy((const char*)auth->client->ip.S_ADDR, (char*)sock.SIN_ADDR.S_ADDR, sizeof(struct IN_ADDR));
#else
  sock.sin_addr.s_addr = auth->client->ip.s_addr;
#endif
  
  sock.SIN_PORT = htons(113);
  sock.SIN_FAMILY = AFINET;

  if (connect(fd, (struct sockaddr*) &sock, sizeof(sock)) == -1)
    {
      if (errno != EINPROGRESS)
	{
	  ServerStats->is_abad++;
	  /*
	   * No error report from this...
	   */
	  CLOSE(fd);
	  if(!HideConnectInfo)
	    sendheader(auth->client, REPORT_FAIL_ID);
	  return 0;
	}
    }

  auth->fd = fd;

  SetAuthConnect(auth);
  return 1;
}

/*
 * GetValidIdent - parse ident query reply from identd server
 * 
 * Inputs        - pointer to ident buf
 * Output        - NULL if no valid ident found, otherwise pointer to name
 * Side effects        -
 */
static char* GetValidIdent(char *buf)
{
  int   remp = 0;
  int   locp = 0;
  char* colon1Ptr;
  char* colon2Ptr;
  char* colon3Ptr;
  char* commaPtr;
  char* remotePortString;

  /* All this to get rid of a sscanf() fun. */
  remotePortString = buf;
  
  colon1Ptr = strchr(remotePortString,':');
  if(!colon1Ptr)
    return 0;

  *colon1Ptr = '\0';
  colon1Ptr++;
  colon2Ptr = strchr(colon1Ptr,':');
  if(!colon2Ptr)
    return 0;

  *colon2Ptr = '\0';
  colon2Ptr++;
  commaPtr = strchr(remotePortString, ',');

  if(!commaPtr)
    return 0;

  *commaPtr = '\0';
  commaPtr++;

  remp = atoi(remotePortString);
  if(!remp)
    return 0;
              
  locp = atoi(commaPtr);
  if(!locp)
    return 0;

  /* look for USERID bordered by first pair of colons */
  if(!strstr(colon1Ptr, "USERID"))
    return 0;

  colon3Ptr = strchr(colon2Ptr,':');
  if(!colon3Ptr)
    return 0;
  
  *colon3Ptr = '\0';
  colon3Ptr++;
  return(colon3Ptr);
}

#ifdef IPV6

int ip6addrcmp (char *a, char *b)
{
int i;
for (i=0; i<sizeof(struct in6_addr); i++) if (a[i]!=b[i]) return 1;
return 0;
}

#endif

/*
 * start_auth - starts auth (identd) and dns queries for a client
 */
void start_auth(struct Client* client)
{
#ifndef USE_ADNS  
  struct DNSQuery     query;
#ifdef IPV6
  struct hostent *ip6direct = NULL, *ip6reverse = NULL;
  char ip6name[HOSTLEN + 1], ip6addr[sizeof(struct in6_addr)];	/* copies to avoid bugs on systems
								where libc is using only one hostent */
  int ip6counter = 0;
#endif
#endif
  struct AuthRequest* auth = 0;

  assert(0 != client);
  if(ReverseLookup || CheckIdentd)
    auth = make_auth_request(client);
#ifndef USE_ADNS  
  query.vptr     = auth;
  query.callback = auth_dns_callback;
#endif

if(ReverseLookup)
  {

#ifdef USE_ADNS    
  client->dns_query = MyMalloc(sizeof(struct DNSQuery));
  client->dns_query->ptr     = auth;
  client->dns_query->callback = auth_dns_callback;
#endif 
  if(!HideConnectInfo)
    sendheader(client, REPORT_DO_DNS);

#ifndef USE_ADNS
#ifdef IPV6
/* We bypass the normal procedure here as it's full of bugs */
  ip6reverse = gethostbyaddr((char*)&client->ip,sizeof(client->ip),AF_INET6);
  if (ip6reverse) {
    strncpy(ip6name, ip6reverse->h_name, HOSTLEN);	/* We avoid use of strlcpy as it's not 
    							   available on all platforms */
    ip6name[HOSTLEN]=0;
    for (ip6counter=0; ip6counter<sizeof(struct in6_addr); ip6counter++) ((char*)ip6addr)[ip6counter]=ip6reverse->h_addr_list[0][ip6counter];
    
    ip6direct = gethostbyname2(ip6name, AF_INET6);
    if (ip6direct) {
      for (ip6counter=0;ip6direct->h_addr_list[ip6counter];ip6counter++)
	if(!ip6addrcmp(ip6direct->h_addr_list[ip6counter],ip6addr)) {
          strncpy(client->realhost, ip6name, HOSTLEN);
          strncpy(client->host, ip6name, HOSTLEN);
	  client->host[HOSTLEN]=0;
	  client->realhost[HOSTLEN]=0;
	  ip6counter = -1;	/* reuse this to know that we found reverse */
          break;
        }
      
      if ((ip6counter > -1) && (ip6direct->h_addr_list[ip6counter] == NULL))
        ip6counter = -2;		/* Direct <-> Reverse mismatch */
     }	/* if(ip6direct) */
  }	/* if(ip6reverse) */
  
  if (!HideConnectInfo)
    switch(ip6counter) {
      case -1:	/* Found your hostname */
        sendheader(client, REPORT_FIN_DNS);
        break;
      case -2:	/* Direct - Reverse mismatch */
        sendheader(client, REPORT_IP_MISMATCH);
        break;
      default:	/* Couldn't find your hostname */
        sendheader(client, REPORT_FAIL_DNS);
      }
#else
  client->dns_reply = gethost_byaddr((const char*) &client->ip, &query);
  if (client->dns_reply)
    {
      ++client->dns_reply->ref_count;
      strncpy_irc(client->host, client->dns_reply->hp->h_name, HOSTLEN);
      if(!HideConnectInfo)
        sendheader(client, REPORT_FIN_DNSC);
    }
  else
    SetDNSPending(auth);
#endif
#else
    adns_getaddr(&client->ip, client->dns_query);
    SetDNSPending(auth);
#endif    

  } else /* no ReverseLookup */
  {
    if(!HideConnectInfo)
      sendheader(client, REPORT_NO_DNS);
  }
  if(CheckIdentd || ReverseLookup)
  { 
    if (CheckIdentd && start_auth_query(auth))
      link_auth_request(auth, &AuthPollList);
    else if (IsDNSPending(auth))
      link_auth_request(auth, &AuthIncompleteList);
    else
      {
        free_auth_request(auth);
        release_auth_client(client);
      }
  } else 
    release_auth_client(client);
}

/*
 * timeout_auth_queries - timeout resolver and identd requests
 * allow clients through if requests failed
 */
void timeout_auth_queries(time_t now)
{
  struct AuthRequest* auth;
  struct AuthRequest* auth_next = 0;

  for (auth = AuthPollList; auth; auth = auth_next)
    {
      auth_next = auth->next;
      if (auth->timeout < CurrentTime)
	{
	  if (-1 < auth->fd)
	    CLOSE(auth->fd);
          if(!HideConnectInfo)
	    sendheader(auth->client, REPORT_FAIL_ID);
	  if (IsDNSPending(auth))
	    {
#ifndef USE_ADNS
	      delete_resolver_queries(auth);	    	    
#else
	      delete_adns_queries(auth->client->dns_query);
	      auth->client->dns_query->query = NULL;
#endif
	      if(!HideConnectInfo)
	        sendheader(auth->client, REPORT_FAIL_DNS);
	    }
	  irclog(L_INFO, "DNS/AUTH timeout %s",
	      get_client_name(auth->client, SHOW_IP));

	  auth->client->since = now;
	  release_auth_client(auth->client);
	  unlink_auth_request(auth, &AuthPollList);
	  free_auth_request(auth);
	}
    }
  for (auth = AuthIncompleteList; auth; auth = auth_next)
    {
      auth_next = auth->next;
      if (auth->timeout < CurrentTime)
	    {
#ifndef USE_ADNS
    	  delete_resolver_queries(auth);	    	    
#else
	  delete_adns_queries(auth->client->dns_query);
	  auth->client->dns_query->query = NULL;
#endif    
	  if(!HideConnectInfo)
	    sendheader(auth->client, REPORT_FAIL_DNS);
	  irclog(L_INFO, "DNS timeout %s", get_client_name(auth->client, SHOW_IP));
	  auth->client->since = now;
	  release_auth_client(auth->client);
	  unlink_auth_request(auth, &AuthIncompleteList);
	  free_auth_request(auth);
	  }
    }
}

/*
 * send_auth_query - send the ident server a query giving "theirport , ourport"
 * The write is only attempted *once* so it is deemed to be a fail if the
 * entire write doesn't write all the data given.  This shouldnt be a
 * problem since the socket should have a write buffer far greater than
 * this message to store it in should problems arise. -avalon
 */
void send_auth_query(struct AuthRequest* auth)
{
  struct SOCKADDR_IN us;
  struct SOCKADDR_IN them;
  char            authbuf[32];
  socklen_t       ulen = sizeof(struct sockaddr);
  socklen_t       tlen = sizeof(struct sockaddr);

  if (getsockname(auth->client->fd, (struct sockaddr *)&us,   &ulen) ||
      getpeername(auth->client->fd, (struct sockaddr *)&them, &tlen))
    {
      
      irclog(L_INFO, "auth get{sock,peer}name error for %s:%m",
	  get_client_name(auth->client, SHOW_IP));
      auth_error(auth);
      return;
    }
  ircsprintf(authbuf, "%u , %u\r\n",
             (unsigned int) ntohs(them.SIN_PORT),
             (unsigned int) ntohs(us.SIN_PORT));

  if (send(auth->fd, authbuf, strlen(authbuf), 0) == -1)
    {
      if (EAGAIN == errno)
	return;
      auth_error(auth);
      return;
    }
  ClearAuthConnect(auth);
  SetAuthPending(auth);
}


/*
 * read_auth_reply - read the reply (if any) from the ident server 
 * we connected to.
 * We only give it one shot, if the reply isn't good the first time
 * fail the authentication entirely. --Bleep
 */
#define AUTH_BUFSIZ 128

void read_auth_reply(struct AuthRequest* auth)
{
  char* s=(char *)NULL;
  char* t=(char *)NULL;
  int   len;
  int   count;
  char  buf[AUTH_BUFSIZ + 1]; /* buffer to read auth reply into */

  len = recv(auth->fd, buf, AUTH_BUFSIZ, 0);
  
  if (len > 0)
    {
      buf[len] = '\0';

      if( (s = GetValidIdent(buf)) )
	{
	  t = auth->client->username;
	  for (count = USERLEN; *s && count; s++)
	    {
	      if(*s == '@')
		{
		  break;
		}
	      if ( !IsSpace(*s) && *s != ':' )
		{
		  *t++ = *s;
		  count--;
		}
	    }
	  *t = '\0';
	}
    }

  if ((len < 0) && (EAGAIN == errno))
    return;

  CLOSE(auth->fd);
  auth->fd = -1;
  ClearAuth(auth);
  
  if (!s)
    {
      ++ServerStats->is_abad;
      strcpy(auth->client->username, "unknown");
    }
  else
    {
      if(!HideConnectInfo)
        sendheader(auth->client, REPORT_FIN_ID);
      ++ServerStats->is_asuc;
      SetGotId(auth->client);
    }
  unlink_auth_request(auth, &AuthPollList);

  if (IsDNSPending(auth))
    link_auth_request(auth, &AuthIncompleteList);
  else
    {
      release_auth_client(auth->client);
      free_auth_request(auth);
    }
}

