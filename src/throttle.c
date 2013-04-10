/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2000     *
 *                http://software.pt-link.net                     *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 *   $Id: throttle.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $ 
 
  File: throttle.c
  Desc: host spoofing routines
  Author: Lamego@PTlink.net
*/
#include "common.h"
#include "s_log.h"
#include "irc_string.h"
#include "m_commands.h"
#include "ircd.h"
#include "send.h"
#include "client.h"
#include "numeric.h"
#include "dconf.h"
#include "dconf_vars.h"
#include "throttle.h"

#include <stdlib.h>
#include <string.h>

/* this is our hash table -- statically initialized to 0 */
static struct aThrottle *throttles[256];

void remove_clone_check(struct Client * cptr)
{
#ifdef IPV6
    unsigned char *p = (unsigned char *) &cptr->ip.S_ADDR;
#else
    unsigned char *p = (unsigned char *) &cptr->ip.s_addr;    
#endif

    struct aThrottle *ptr = throttles[(p[0] + p[1] + p[2] + p[3]) & 0xff];

#ifndef IPV6
    if (cptr->ip.s_addr == 0)
	return;
#endif
#ifdef IPV6
    while (ptr && (bcmp( (char*)cptr->ip.S_ADDR, (char*)ptr->ip.S_ADDR, sizeof(struct IN_ADDR) )!=0))
#else 
    while (ptr && (cptr->ip.s_addr !=  ptr->ip.s_addr))
#endif
	ptr = ptr->next;

    if (ptr && (ptr->count > 1))
	ptr->count--;
}

/*
 * check_clones
 *
 * This function counts the number of clients with the same IP number
 * as cptr that connected less than CHECK_CLONE_PERIOD seconds ago. 
 * return value
 * -1: reject connections
 * 0: permit connections
 *
 * Based on the original (ircu) from Seks, Record and Run
 * fixed up by nikb <nikb@dal.net>
 */

int check_clones(struct Client * cptr, const char *remote)
{
    struct aThrottle **tscn, *tptr;
#ifdef IPV6
    unsigned char *p = (unsigned char *) &cptr->ip.S_ADDR;
#else
    unsigned char *p = (unsigned char *) &cptr->ip.s_addr;
#endif
    unsigned char hval = (unsigned char) (p[0] ^ p[1] ^ p[2] ^ p[3]) & 0xFF;
    tscn = &throttles[hval];

#ifndef IPV6
    if (cptr->ip.s_addr == 0)
		return 0;
#endif
	/* scan the hash for this host and clean expired entries meanwhile */
#ifdef IPV6
   while (*tscn && (bcmp((char*)cptr->ip.S_ADDR, (char*)(*tscn)->ip.S_ADDR, sizeof(struct IN_ADDR))!=0))
#else	
    while (*tscn && ((cptr->ip.s_addr != (*tscn)->ip.s_addr)))
#endif    
	  {
		if ((*tscn)->last + CheckClonesPeriod < CurrentTime)
		  {
	  		tptr = *tscn;
/*
	  		if ((tptr->count == THROTTLE_SENTWARN) && ((*tscn)->last + CheckClonesDelay < CurrentTime))
			  {
				sendto_realops("Removing throttle from %s (%s)", tptr->connected, inetntoa((char *) &tptr->ip));
			  }
*/
			*tscn = tptr->next;				  
			free(tptr->connected);
	  		free(tptr);			  
		  } 
		else
		  tscn = &(*tscn)->next;
  	  }

    if ((tptr = *tscn))
	  {	/* There is a record for this host */		

		if (!(tptr->last + CheckClonesPeriod < CurrentTime)) 
		
		  {		/* not expired */
			tptr->last = CurrentTime;

	  		if (remote && !tptr->connected && (tptr->connected = (char *) MyMalloc(strlen(remote) + 1)))
			  strcpy(tptr->connected, remote);			

	  		if (tptr->count >= 0) 
			  {
				if (++tptr->count == CheckClonesLimit)
		  		tptr->count = THROTTLE_HITLIMIT;

				return 0;
	  		  }
			  
	  		if (tptr->count == THROTTLE_HITLIMIT) 
			  {
				sendto_realops("Throttling connections from %s (%s)", tptr->connected, cptr->realhost);
				tptr->count = THROTTLE_SENTWARN;

		  		return -1;		
	  		  }			  
		  } 
		else 				
		  {
			
			tptr->last = CurrentTime;
	  		if (tptr->count == THROTTLE_SENTWARN)
			  sendto_realops("Resetting throttle from %s (%s)", tptr->connected, inetntoa((char *) &tptr->ip));
	  
	  		tptr->count = 1;			

	  		if (remote && !tptr->connected && (tptr->connected = (char *) MyMalloc(strlen(remote) + 1)))
				strcpy(tptr->connected, remote);

	  		return 0;
		  }
  	  } 

    
  	else 
	  {	/* no record for the host, make one */
		tptr = (struct aThrottle *) MyMalloc(sizeof(struct aThrottle));
		tptr->ip = cptr->ip;
		tptr->next = throttles[hval];
		tptr->last = CurrentTime;
		tptr->count = 1;
		tptr->connected = NULL;

		if (remote && (tptr->connected = (char *) MyMalloc(strlen(remote) + 1)))
	      	strcpy(tptr->connected, remote);

		throttles[hval] = tptr;
		return 0;
    }

    /* we should never get here, but just in case */
    return 0;
}
