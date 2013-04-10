/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2000     *
 *                http://software.pt-link.net                     *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 $Id: m_watch.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $
 
 File: m_watch.c
 Desc: watch list handling routines
 Author: Lamego@PTlink.net  
*/

#include "m_commands.h"
#include "client.h"
#include "channel.h"
#include "hash.h"
#include "struct.h"
#include "ircd.h"
#include "numeric.h"
#include "s_serv.h"
#include "send.h"
#include "list.h"
#include "irc_string.h"
#include "ircd_defs.h"

#include <string.h>
static char buf[BUFSIZE];  

static void show_watch(struct Client *cptr, char *name, int rpl1, int rpl2);

/*
 * m_watch
 */
int   m_watch(struct Client *cptr, struct Client *sptr, int parc, char *parv[]) 
{
	struct Client  *acptr;
	char  *s, *p, *user;
	char def[2] = "l";
	
	if (parc < 2) 
	{
		/* Default to 'l' - list who's currently online */
		parc = 2;
		parv[1] = def;
	}
	
	for (p = NULL, s = strtoken(&p, parv[1], ", "); s; s = strtoken(&p, NULL, ", ")) 
	{
		if ((user = (char *)strchr(s, '!')))
		  *user++ = '\0'; /* Not used */
		
		/*
		 * Prefix of "+", they want to add a name to their WATCH
		 * list. 
		 */
		if (*s == '+') 
		{
			if (*(s+1)) 
			{
				if (sptr->watches >= MAXWATCH) 
				{
					sendto_one(sptr, form_str(ERR_TOOMANYWATCH),
								  me.name, cptr->name, s+1, MAXWATCH);					
					continue;
				}				
				add_to_watch_hash_table(s+1, sptr);
			}
			show_watch(sptr, s+1, RPL_NOWON, RPL_NOWOFF);
			continue;
		}
		
		/*
		 * Prefix of "-", coward wants to remove somebody from their
		 * WATCH list.  So do it. :-)
		 */
		if (*s == '-') 
		{
			del_from_watch_hash_table(s+1, sptr);
			show_watch(sptr, s+1, RPL_WATCHOFF, RPL_WATCHOFF);
			continue;
		}
					
		/*
		 * Fancy "C" or "c", they want to nuke their WATCH list and start
		 * over, so be it.
		 */
		if (*s == 'C' || *s == 'c') 
		{
			hash_del_watch_list(sptr);
			continue;
		}
		
		/*
		 * Now comes the fun stuff, "S" or "s" returns a status report of
		 * their WATCH list.  I imagine this could be CPU intensive if its
		 * done alot, perhaps an auto-lag on this?
		 */
		if (*s == 'S' || *s == 's') 
		{
			Link *lp;
			struct Watch *anptr;
			int  count = 0;
							
			/*
			 * Send a list of how many users they have on their WATCH list
			 * and how many WATCH lists they are on.
			 */
			anptr = hash_get_watch(sptr->name);
			if (anptr)
			  for (lp = anptr->watch, count = 1; (lp = lp->next); count++);
			sendto_one(sptr, form_str(RPL_WATCHSTAT), me.name, parv[0],
						  sptr->watches, count);
			
			/*
			 * Send a list of everybody in their WATCH list. Be careful
			 * not to buffer overflow.
			 */
			if ((lp = sptr->watch) == NULL) 
			{
				sendto_one(sptr, form_str(RPL_ENDOFWATCHLIST), me.name, parv[0],
							  *s);
				continue;
			}
			*buf = '\0';
			strcpy(buf, lp->value.wptr->nick);
			count = strlen(parv[0])+strlen(me.name)+10+strlen(buf);
			while ((lp = lp->next)) 
			{
				if (count+strlen(lp->value.wptr->nick)+1 > BUFSIZE - 2) 
				{
					sendto_one(sptr, form_str(RPL_WATCHLIST), me.name,
								  parv[0], buf);
					*buf = '\0';
					count = strlen(parv[0])+strlen(me.name)+10;
				}
				strcat(buf, " ");
				strcat(buf, lp->value.wptr->nick);
				count += (strlen(lp->value.wptr->nick)+1);
			}
			sendto_one(sptr, form_str(RPL_WATCHLIST), me.name, parv[0], buf);
			sendto_one(sptr, form_str(RPL_ENDOFWATCHLIST), me.name, parv[0], *s);
			continue;
		}
		
		/*
		 * Well that was fun, NOT.  Now they want a list of everybody in
		 * their WATCH list AND if they are online or offline? Sheesh,
		 * greedy arn't we?
		 */
		if (*s == 'L' || *s == 'l') 
		{
			Link *lp = sptr->watch;
			
			while (lp) 
			{
				if ((acptr = find_person(lp->value.wptr->nick, NULL)))
				  sendto_one(sptr, form_str(RPL_NOWON), me.name, parv[0],
								 acptr->name, acptr->username,
								 acptr->host, acptr->tsinfo);
				/*
				 * But actually, only show them offline if its a capital
				 * 'L' (full list wanted).
				 */
				else if (ToUpper(*s))
				  sendto_one(sptr, form_str(RPL_NOWOFF), me.name, parv[0],
								 lp->value.wptr->nick, "*", "*",
								 lp->value.wptr->lasttime);
				lp = lp->next;
			}
			
			sendto_one(sptr, form_str(RPL_ENDOFWATCHLIST), me.name, parv[0],  *s);
			continue;
		}
		/* Hmm.. unknown prefix character.. Ignore it. :-) */
	}
	
	return 0;
}

/*
 * RPL_NOWON   - Online at the moment (Succesfully added to WATCH-list)
 * RPL_NOWOFF  - Offline at the moement (Succesfully added to WATCH-list)
 * RPL_WATCHOFF   - Succesfully removed from WATCH-list.
 * ERR_TOOMANYWATCH - Take a guess :>  Too many WATCH entries.
 */
static void show_watch(struct Client *cptr, char *name, int rpl1, int rpl2)
{
        struct Client *acptr;
 
        if ((acptr = find_person(name, NULL)))
          sendto_one(cptr, form_str(rpl1), me.name, cptr->name,
                                         acptr->name, acptr->username,
                                         acptr->host, acptr->lasttime);
        else
          sendto_one(cptr, form_str(rpl2), me.name, cptr->name,
                                         name, "*", "*", 0);
}
