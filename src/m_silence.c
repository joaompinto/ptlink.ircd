/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2000     *
 *                http://software.pt-link.net                     *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 $Id: m_silence.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $
 
 File: m_silence.c
 Desc: silence handling routines
 Author: Lamego@PTlink.net  
*/

#include "m_silence.h"
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
#include "channel.h"		/* to include pretty_mask() */
#include "irc_string.h"
#include "ircd_defs.h"

#include <stdlib.h>
#include <string.h>
int del_silence(aClient *sptr, char *mask);
int is_silenced( struct Client *,  struct Client *);
extern char         *pretty_mask(char *);
//extern int      is_silenced( struct Client *,  struct Client *);

/* is_silenced - Returns 1 if a sptr is silenced by acptr */
int is_silenced(aClient *sptr, aClient *acptr) {
	 Link *lp;
	 anUser *user;
	 char sender[HOSTLEN+NICKLEN+USERLEN+5];
	 if (!(acptr->user)||!(lp=acptr->user->silence)||!(user=sptr->user))
		 return 0;
	 ircsprintf(sender,"%s!%s@%s",sptr->name,sptr->username,sptr->host);
	 for (;lp;lp=lp->next) {
			if (match(lp->value.cp, sender)) {
				 if (!MyConnect(sptr)) {
						sendto_one(sptr->from, ":%s SILENCE %s :%s",acptr->name,
											 sptr->name, lp->value.cp);
						lp->flags=1; 
				 }
				 return 1;
			}
	 }
	 return 0;
}

int del_silence(aClient *sptr, char *mask) {
	Link **lp, *tmp;
	for (lp=&(sptr->user->silence);*lp;lp=&((*lp)->next))
	  if (strcmp(mask, (*lp)->value.cp)==0) {
		  tmp = *lp;
		  *lp = tmp->next;
		  free(tmp->value.cp);
		  free_link(tmp);
		  return 0;
	  }
	return 1;
}

static int add_silence(aClient *sptr,char *mask) {
	Link *lp;
	int cnt=0, len=0;
	for (lp=sptr->user->silence;lp;lp=lp->next) {
		len += strlen(lp->value.cp);
		if (MyClient(sptr)) {
		  if ((len > MAXSILELENGTH) || (++cnt >= MAXSILES)) {
			  sendto_one(sptr, form_str(ERR_SILELISTFULL), me.name, sptr->name, mask);
			  return -1;
		  } else {
			  if (match(lp->value.cp, mask))
				 return -1;
		  }
                }
		else if (!strcmp(lp->value.cp, mask))
		  return -1;
	}
	lp = make_link();
	lp->next = sptr->user->silence;
	lp->value.cp = (char *)MyMalloc(strlen(mask)+1);
	(void)strcpy(lp->value.cp, mask);
	sptr->user->silence = lp;
	return 0;
}

/* m_silence
** 		parv[0] = sender prefix
** From local client:
** 		parv[1] = mask (NULL sends the list)
** From remote client:
** 		parv[1] = nick that must be silenced
** 		parv[2] = mask
*/
int     m_silence(struct Client *cptr,
                struct Client *sptr,
                int parc,
                char *parv[]) 
{
  Link *lp;
  struct Client *acptr=NULL;
  char c, *cp;
  if (MyClient(sptr)) 
	{
	  acptr = sptr;
	  
	  if (parc < 2 || *parv[1]=='\0' || (acptr = find_person(parv[1], NULL))) 
		{
		  if (!(acptr->user)) return 0;
		  for (lp = acptr->user->silence; lp; lp = lp->next)
			  sendto_one(sptr, form_str(RPL_SILELIST), me.name,
							 sptr->name, acptr->name, lp->value.cp);
		  sendto_one(sptr, form_str(RPL_ENDOFSILELIST), me.name, acptr->name);
		  return 0;
		}
		
	  cp = parv[1];
	  c = *cp;
	  
	  if (c=='-' || c=='+') 
		cp++;		
	  else if (!(strchr(cp, '@') || strchr(cp, '.') ||
					  strchr(cp, '!') || strchr(cp, '*'))) 
		{
		  sendto_one(sptr, form_str(ERR_NOSUCHNICK), me.name, parv[0], parv[1]);
		  return 0;
		}
	  else c = '+';
	  
	  cp = pretty_mask(cp);
		if ((c=='-' && !del_silence(sptr,cp)) ||
			 (c!='-' && !add_silence(sptr,cp))) {
			sendto_prefix_one(sptr, sptr, ":%s SILENCE %c%s", parv[0], c, cp);
			if (c=='-')
			  sendto_serv_butone(NULL, ":%s SILENCE * -%s", sptr->name, cp);
		}
		
	} 
  else if (parc < 3 || *parv[2]=='\0') 
	{
	  sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS), me.name, parv[0], "SILENCE");
	  return -1;
	} 
  else if ((c = *parv[2])=='-' || (acptr = find_person(parv[1], NULL))) 
	{
	  if (c=='-') 
		{
		  if (!del_silence(sptr,parv[2]+1))
			  sendto_serv_butone(cptr, ":%s SILENCE %s :%s",
										parv[0], parv[1], parv[2]);
		} 
	  else 
		{
		  (void)add_silence(sptr,parv[2]);
		  if (!MyClient(acptr))
		    sendto_one(acptr, ":%s SILENCE %s :%s",
				 parv[0], parv[1], parv[2]);
		} 
	} 
  else 
	{
	  sendto_one(sptr, form_str(ERR_NOSUCHNICK), me.name, parv[0], parv[1]);
	  return 0;
	}
  return 0;
}
