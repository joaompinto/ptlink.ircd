/************************************************************************
 *   IRC - Internet Relay Chat, src/listener.c
 *   Copyright (C) 1999 Thomas Helvey <tomh@inxpress.net>
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
 *  $Id: listener.c,v 1.4 2005/08/27 16:23:50 jpinto Exp $
 */
#include "listener.h"
#include "s_log.h"
#include "client.h"
#include "irc_string.h"
#include "ircd.h"
#include "ircd_defs.h"
#include "numeric.h"
#include "s_bsd.h"
#include "s_conf.h"
#include "s_stats.h"
#include "send.h"
#include "struct.h"
#include "unicode.h"
#include "common.h"

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned int) 0xffffffff)
#endif

struct Listener* ListenerPollList = 0;

struct Listener* make_listener(int port, struct IN_ADDR addr)
{
  struct Listener* listener = 
    (struct Listener*) MyMalloc(sizeof(struct Listener));
  assert(0 != listener);

  memset(listener, 0, sizeof(struct Listener));

  listener->name        = me.name;
  listener->fd          = -1;
  listener->port        = port;
#ifdef IPV6
  memcpy(listener->addr.S_ADDR, addr.S_ADDR, sizeof(struct IN_ADDR));
#else
  listener->addr.s_addr = addr.s_addr;
#endif

#ifdef NULL_POINTER_NOT_ZERO
  listener->next = NULL;
  listener->conf = NULL;
#endif
  return listener;
}

void free_listener(struct Listener* listener)
{
  assert(0 != listener);
  MyFree(listener);
}

#define PORTNAMELEN 6  /* ":31337" */

/*
 * get_listener_name - return displayable listener name and port
 * returns "host.foo.org:6667" for a given listener
 */
const char* get_listener_name(const struct Listener* listener)
{
  static char buf[HOSTLEN + HOSTLEN + PORTNAMELEN + 4];
  assert(0 != listener);
  ircsprintf(buf, "%s[%s/%u]", 
             me.name, listener->name, listener->port);
  return buf;
}

/*
 * show_ports - send port listing to a client
 * inputs       - pointer to client to show ports to
 * output       - none
 * side effects - show ports
 */
void show_ports(struct Client* sptr)
{
  struct Listener* listener = 0;

  for (listener = ListenerPollList; listener; listener = listener->next)
    {
      sendto_one(sptr, form_str(RPL_STATSPLINE),
                 me.name,
                 sptr->name,
                 'P',
                 listener->port,
#ifdef HIDE_SERVERS_IPS
		 me.name,
#else		 
                 listener->name,
#endif		 
                 listener->ref_count,
                 (listener->active)?"active":"disabled");
    }
}
  
/*
 * inetport - create a listener socket in the AF_INET domain, 
 * bind it to the port given in 'port' and listen to it  
 * returns true (1) if successful false (0) on error.
 *
 * If the operating system has a define for SOMAXCONN, use it, otherwise
 *   use HYBRID_SOMAXCONN -Dianora
 */
#ifdef SOMAXCONN
#undef HYBRID_SOMAXCONN
#define HYBRID_SOMAXCONN SOMAXCONN
#endif

static int inetport(struct Listener* listener)
{
  struct SOCKADDR_IN port_sin;
  int                fd;
  int                opt = 1;

  /*
   * At first, open a new socket
   */
  fd = socket(AFINET, SOCK_STREAM, 0);

  if (-1 == fd) {
    report_error("opening listener socket %s:%s", 
                 get_listener_name(listener), errno);
    return 0;
  }
  else if ((HARD_FDLIMIT - 10) < fd) {
    report_error("no more connections left for listener %s:%s", 
                 get_listener_name(listener), errno);
    CLOSE(fd);
    return 0;
  }
  /* 
   * XXX - we don't want to do all this crap for a listener
   * set_sock_opts(listener);
   */ 
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*) &opt, sizeof(opt))) {
    report_error("setting SO_REUSEADDR for listener %s:%s", 
                 get_listener_name(listener), errno);
    CLOSE(fd);
    return 0;
  }

  /*
   * Bind a port to listen for new connections if port is non-null,
   * else assume it is already open and try get something from it.
   */
  memset(&port_sin, 0, sizeof(port_sin));
  port_sin.SIN_FAMILY = AFINET;
  port_sin.SIN_PORT   = htons(listener->port);
#ifdef __CYGWIN__
  port_sin.sin_addr   = listener->addr;
  
  if (INADDR_ANY != listener->addr.S_ADDR) {
    strncpy_irc(listener->vhost, inetntoa((char *)&listener->addr), HOSTLEN);
    listener->name = listener->vhost;
  }
  
#else
#ifdef IPV6
  bcopy((const char*)listener->addr.S_ADDR, (char*)port_sin.SIN_ADDR.S_ADDR, sizeof(struct IN_ADDR));
  if ( bcmp((char*)listener->addr.S_ADDR, &INADDRANY, sizeof(struct IN_ADDR)) == 0 ) 
#else
  port_sin.sin_addr   = listener->addr;
  if (INADDRANY != listener->addr.s_addr) 
#endif      
  {
 	struct addrinfo *ans;
	int ret;
        char port[5];
	char tmp[HOSTLEN];
      	/*
     	 * XXX - blocking call to getaddrinfo
         */
	sprintf( port, "%d", listener->port);
#ifdef IPV6
	inetntop(AFINET, &listener->addr, tmp, HOSTLEN);
#else
	inet_ntop(AF_INET, &listener->addr, tmp, HOSTLEN);
#endif
	ret = getaddrinfo(tmp, port, NULL, &ans );
	if( ret == 0  && ans->ai_canonname)
	  strncpy_irc(listener->vhost, ans->ai_canonname, HOSTLEN);
    }

#endif
                 
  if (bind(fd, (struct sockaddr*) &port_sin, sizeof(port_sin))) {
    report_error("binding listener socket %s:%s", 
                 get_listener_name(listener), errno);
    CLOSE(fd);
    return 0;
  }

  if (listen(fd, HYBRID_SOMAXCONN)) {
    report_error("listen failed for %s:%s", 
                 get_listener_name(listener), errno);
    CLOSE(fd);
    return 0;
  }

  /*
   * XXX - this should always work, performance will suck if it doesn't
   */
  if (!set_non_blocking(fd))
    report_error(NONB_ERROR_MSG, get_listener_name(listener), errno);

  listener->fd = fd;

  return 1;
}

static struct Listener* find_listener(int port, struct IN_ADDR addr)
{
  struct Listener* listener;
  for (listener = ListenerPollList; listener; listener = listener->next) 
#ifdef IPV6
    if(port == listener->port && bcmp((const char*)addr.S_ADDR, (const char*)listener->addr.S_ADDR, sizeof(struct IN_ADDR)) ==0)
#else
    if(port == listener->port && addr.s_addr == listener->addr.s_addr)
#endif
      return listener;
  return 0;
}
 
  
/*
 * add_listener- create a new listener 
 * port - the port number to listen on
 * vhost_ip - if non-null must contain a valid IP address string in the format "255.255.255.255"
 * options - listener options
 * cp - listener default code page
 */
void add_listener(int port, const char* vhost_ip, char* options, char* cp)
{
  struct Listener* listener;
  struct IN_ADDR   vaddr;

  /*
   * if no port in conf line, don't bother
   */
  if (0 == port)
    return;

#ifdef IPV6
  vaddr = INADDRANY;
#else
   vaddr.s_addr = INADDRANY;
#endif

  if (vhost_ip) {
#ifdef IPV6
     if(inetpton(AFINET, vhost_ip, &vaddr) == 0)
       return;
#else
      vaddr.s_addr = inet_addr(vhost_ip);
      if (INADDR_NONE == vaddr.s_addr)
        return;
#endif
  }

  if ((listener = find_listener(port, vaddr))) {
    listener->active = 1;
    return;
  }
  
  if(options && *options)
    irclog(L_NOTICE,"Adding listener for %s, port %d Options:%s",
  	vhost_ip ? vhost_ip : "ANY", port, options );
  else
    irclog(L_NOTICE,"Adding listener for %s, port %d",
  	vhost_ip ? vhost_ip : "ANY", port);
  	
  listener = make_listener(port, vaddr);
  if(options)
  {
    while(*options)
      {
        switch(*options)
          {
            case 's' : 
#ifdef HAVE_SSL            
            if(no_ssl)
              irclog(L_WARN,"ignoring port with 's' option!");
            else
              listener->options |= LST_SSL; 
#else
              irclog(L_WARN,"ircd compiled without SSL support, ignoring port with 's' option!");
#endif              
              break;
            case 'S' : listener->options |= LST_SERVER; break;
            case 'W' : listener->options |= LST_WEBCHAT; break;
            case 'J' : listener->options |= LST_JAVACR; break;
            case 'n' : listener->options |= LST_NOSPOOF; break;
          }
        ++options;          
      }
  }
  
  if(cp && cp[0])
    {
       listener->codepage = codepage_find(cp); 
       if(listener->codepage==-1)
         irclog(L_WARN,"Codepage %s defined on P:line was not loaded!", cp);
    }
  else
     listener->codepage=-1;
     
  if (inetport(listener)) {
    listener->active = 1;
    listener->next   = ListenerPollList;
    ListenerPollList = listener; 
  }
  else
    free_listener(listener);
}

void mark_listeners_closing(void)
{
  struct Listener* listener;
  for (listener = ListenerPollList; listener; listener = listener->next)
    listener->active = 0;
}

/*
 * close_listener - close a single listener
 */
void close_listener(struct Listener* listener)
{
  assert(0 != listener);
  /*
   * remove from listener list
   */
  if (listener == ListenerPollList)
    ListenerPollList = listener->next;
  else {
    struct Listener* prev = ListenerPollList;
    for ( ; prev; prev = prev->next) {
      if (listener == prev->next) {
        prev->next = listener->next;
        break; 
      }
    }
  }
  if (-1 < listener->fd)
    CLOSE(listener->fd);
  free_listener(listener);
}
 
/*
 * close_listeners - close and free all listeners that are not being used
 */
void close_listeners()
{
  struct Listener* listener;
  struct Listener* listener_next = 0;
  /*
   * close all 'extra' listening ports we have
   */
  for (listener = ListenerPollList; listener; listener = listener_next) {
    listener_next = listener->next;
    if (0 == listener->active && 0 == listener->ref_count)
      close_listener(listener);
  }
}

void accept_connection(struct Listener* listener)
{
  static time_t      last_oper_notice = 0;
  struct SOCKADDR_IN addr;
  socklen_t          addrlen = sizeof(struct SOCKADDR_IN);
  int                fd;

  assert(0 != listener);

  listener->last_accept = CurrentTime;
  /*
   * There may be many reasons for error return, but
   * in otherwise correctly working environment the
   * probable cause is running out of file descriptors
   * (EMFILE, ENFILE or others?). The man pages for
   * accept don't seem to list these as possible,
   * although it's obvious that it may happen here.
   * Thus no specific errors are tested at this
   * point, just assume that connections cannot
   * be accepted until some old is closed first.
   */
  if (-1 == (fd = accept(listener->fd, (struct sockaddr*) &addr, &addrlen))) {
    if (EAGAIN == errno)
       return;
    /*
     * slow down the whining to opers bit
     */
    if((last_oper_notice + 20) <= CurrentTime) {
      report_error("Error accepting connection %s:%s", 
                 listener->name, errno);
      last_oper_notice = CurrentTime;
    }
    return;
  }
  /*
   * check for connection limit
   */
  if ((MAXCONNECTIONS - 10) < fd) {
    ++ServerStats->is_ref;
    /* 
     * slow down the whining to opers bit 
     */
    if((last_oper_notice + 20) <= CurrentTime) {
      sendto_realops("All connections in use. (%s)", 
                     get_listener_name(listener));
      last_oper_notice = CurrentTime;
    }
    send(fd, "ERROR :All connections in use\r\n", 32, 0);
    CLOSE(fd);
    return;
  }
  /*
   * check to see if listener is shutting down
   */
  if (!listener->active) {
    ++ServerStats->is_ref;
    send(fd, "ERROR :Use another port\r\n", 25, 0);
    CLOSE(fd);
    return;
  }
  /*
   * check conf for ip address access
   */
  if (!conf_connect_allowed(addr.SIN_ADDR)) {
    ServerStats->is_ref++;
#ifdef REPORT_DLINE_TO_USER
     send(fd, "NOTICE DLINE :*** You have been D-lined\r\n", 41, 0);
#endif
    CLOSE(fd);
    return;
  }
  ServerStats->is_ac++;
  nextping = CurrentTime;
  add_connection(listener, fd);
}

