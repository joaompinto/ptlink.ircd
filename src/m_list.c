/************************************************************************
 *   IRC - Internet Relay Chat, src/m_list.c
 *   Copyright (C) 1990 Jarkko Oikarinen and
 *                      University of Oulu, Co Center
 *
 * $Id: m_list.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $ 
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
 */

 /* m_list imported from dal4.6.x to bras6.x(PTLink6.x) by fabulous */

#include "m_commands.h"
#include "client.h"
#include "common.h"
#include "flud.h"
#include "s_serv.h"       /* captab */
#include "s_user.h"
#include "send.h"
#include "struct.h"
#include "whowas.h"
#include "m_silence.h"
#include "m_svsinfo.h"	/* my_svsinfo_ts */
#include "dconf_vars.h"
#include "ircd_defs.h"
#include "hash.h"
#include "numeric.h"
#include "channel.h"
#include "ircd.h"
#include "irc_string.h"
#include "list.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

static int find_str_match_link(Link **lp, char *str);

/*
 * Look for a match in a list of strings. Go through the list, and run
 * match() on it. Side effect: if found, this link is moved to the top of
 * the list.
   */
static int find_str_match_link(Link **lp, char *str)
{
        Link    **head = lp;

	if (lp && *lp)
    {
                if (match((*lp)->value.cp, str))
			return 1;
		for (; (*lp)->next; *lp = (*lp)->next)
                        if (match((*lp)->next->value.cp, str))
        {
				Link *temp = (*lp)->next;
				*lp = (*lp)->next->next;
				temp->next = *head;
				*head = temp;
				return 1;
        }
		return 0;
    }
	return 0;
}


/*
 * send_list
 *
 * The function which sends
 * The function which sends the actual /list output back to the user.
 * Operates by stepping through the hashtable, sending the entries back if
 * they match the criteria.
 * cptr = Local client to send the output back to.
 * numsend = Number (roughly) of lines to send back. Once this number has
 * been exceeded, send_list will finish with the current hash bucket,
 * and record that number as the number to start next time send_list
 * is called for this user. So, this function will almost always send
 * back more lines than specified by numsend (though not by much,
 * assuming CHANNELHASHSIZE is was well picked). So be conservative
 * if altering numsend };> -Rak
 */
void send_list(aClient *cptr)
{
    struct Channel *chptr;
    static char nbuf[5 + MODEBUFLEN];
    static char modebuf[MODEBUFLEN];
    static char parabuf[MODEBUFLEN];
    int i, j;

#define l cptr->lopt /* lazy shortcut */
    if (!cptr->lopt) return;

    if (cptr->listprogress != -1) {
      for (i=cptr->listprogress; i<CH_MAX; i++) {
         int progress2 = cptr->listprogress2;
        for (j=0, chptr=(struct Channel*)(hash_get_channel_block(i).list);
             (chptr) && (j<hash_get_channel_block(i).links); chptr=chptr->hnextch, j++) {
          if (j<progress2) continue;  /* wind up to listprogress2 */
#ifdef REGLIST
              if (!IsAnOper (cptr) && !(chptr->mode.mode & MODE_REGISTERED))
              	  continue;
#endif			
              if (!IsOper (cptr) &&
                  (!cptr->user ||
                  (SecretChannel(chptr) && !IsMember(cptr, chptr))))
                  continue;
              if (!l->showall && ((chptr->users-chptr->iusers <= l->usermin) ||
                  ((l->usermax == -1)?0:(chptr->users-chptr->iusers >= l->usermax)) ||
                  ((chptr->channelts||1) <= l->chantimemin) ||
                  (chptr->topic_time < l->topictimemin) ||
                  (chptr->channelts >= l->chantimemax) ||
                  (chptr->topic_time > l->topictimemax)))
                  continue;
              /* For now, just extend to topics as well. Use patterns starting
               * with # to stick to searching channel names only. -Donwulff
               */
              if (l->nolist && 
                  (find_str_match_link(&(l->nolist), chptr->chname) ||
                   find_str_match_link(&(l->nolist), chptr->topic)))
                  continue;
              if (l->yeslist &&
                  (!find_str_match_link(&(l->yeslist), chptr->chname) &&
                   !find_str_match_link(&(l->yeslist), chptr->topic)))
                  continue;
		  
              if (IsOper(cptr))
			{
			  modebuf[0]='\0';
			  parabuf[0]='\0';
                  channel_modes(cptr, modebuf, parabuf, chptr, 1);
			  if(*modebuf)
                        ircsprintf(nbuf,"[%s %s] ", modebuf, (*parabuf) ? parabuf : "");
                  sendto_one(cptr, form_str(RPL_LIST), me.name, cptr->name, chptr->chname,
                             chptr->users-chptr->iusers, nbuf, chptr->topic);
                     } else
                  sendto_one(cptr, form_str(RPL_LIST), me.name, cptr->name,
                             ShowChannel(cptr, chptr) ? chptr->chname : "*",
                             chptr->users-chptr->iusers,
                             ShowChannel(cptr, chptr) ? chptr->topic : "", "");
              if (IsSendqPopped(cptr)) {
                /* we popped again! : P */
                cptr->listprogress=i;
                cptr->listprogress2=j;
                return;
              }
         } /* for buckets */
         cptr->listprogress2 = 0;
      } /* for hash */
    }    /* listprogress !- -1 */
    sendto_one(cptr, form_str(RPL_LISTEND), me.name, cptr->name);

    if (IsSendqPopped(cptr)) { /* popped with the RPL_LISTEND code. d0h */
      cptr->listprogress = -1;
      return;
    }
    ClearDoingList(cptr);   /* yupo, its over */
    free_link(l->yeslist);
    free_link(l->nolist);
    MyFree(l);
    l = NULL;

    return;
}
			

/*
 * m_list
 *	parv[0] = sender prefix
 *	parv[1,2,3...] = Channels or list options.
 */
int	m_list(aClient *cptr, aClient *sptr, int parc, char *parv[])
{
    struct Channel *chptr;
    char   *name, *p = NULL;
    LOpts  *lopt;
    short  int  showall = 0;
    Link   *yeslist = NULL, *nolist = NULL, *listptr;
    short  usermin = 0, usermax = -1;
    time_t chantimemin = 0, topictimemin = 0;
    time_t chantimemax, topictimemax;

    static char *usage[] = {
	"   Usage: /raw LIST options (on mirc) or /quote LIST options (ircII)",
	"",
	"If you don't include any options, the default is to send you the",
	"entire unfiltered list of channels. Below are the options you can",
	"use, and what channels LIST will return when you use them.",
	">number  List channels with more than <number> people.",
	"<number  List channels with less than <number> people.",
	"C>number List channels created between now and <number> minutes ago.",
	"C<number List channels created earlier than <number> minutes ago.",
	"T>number List channels whose topics are older than <number> minutes",
	"         (Ie, they have not changed in the last <number> minutes.",
	"T<number List channels whose topics are not older than <number> minutes.",
	"*mask*   List channels that match *mask*",
	"!*mask*  List channels that do not match *mask*",
	NULL
    };
    static time_t last_used=0L;

   if(!IsAnOper(sptr))
     {
       if(((last_used + PACE_WAIT) > CurrentTime) || (LIFESUX))
         {
           sendto_one(sptr,form_str(RPL_LOAD2HI),me.name,parv[0]);
            return 0;
          }
       else
         last_used = CurrentTime;
        }

   /* throw away non local list requests that do get here -Dianora */
   if(!MyConnect(sptr))
    return 0;
    
    /*
     * I'm making the assumption it won't take over a day to transmit
     * the list... -Rak
     */
    chantimemax = topictimemax = CurrentTime + 86400;
  
  
    if ((parc == 2) && (!strcasecmp(parv[1], "?"))) {
	char **ptr = usage;
			 
	for (; *ptr; ptr++)
            sendto_one(sptr, ":%s 334 %s :%s", me.name,
		       cptr->name, *ptr);
	return 0;
    }
		  
    /*
     * A list is already in process, for now we just interrupt the
     * current listing, perhaps later we can allow stacked ones...
     *  -Donwulff (Not that it's hard or anything, but I don't see
     *             much use for it, beyond flooding)
     */
					
    if(cptr->lopt) {
        free_link(cptr->lopt->yeslist);
        free_link(cptr->lopt->nolist);
	MyFree(cptr->lopt);
	cptr->lopt=NULL;
        sendto_one(sptr, form_str(RPL_LISTEND), me.name, cptr->name);
        ClearSendqPop(sptr);
	/* Interrupted list, penalize 10 seconds */
	if(!IsPrivileged(sptr))
	    sptr->since+=10;
	
	return 0;
			}	

    sendto_one(sptr, form_str(RPL_LISTSTART), me.name, cptr->name);
					 
    /* LIST with no arguements */
    if (parc < 2 || BadPtr(parv[1])) {
	lopt = (LOpts *)MyMalloc(sizeof(LOpts));
	    if (!lopt)
            return 0;

	/*
	 * Changed to default to ignoring channels with only
	 * 1 person on, to decrease floods... -Donwulff
	 */
	bzero(lopt, sizeof(LOpts)); /* To be sure! */
        lopt->yeslist=lopt->nolist=(Link *)NULL;
	lopt->next = (LOpts *)lopt->yeslist;	
	lopt->usermin = RestrictedChans ? 0 : 1; /* Default */
	lopt->usermax = -1;
        lopt->chantimemax = lopt->topictimemax = CurrentTime + 86400;
	cptr->lopt = lopt;
        if (IsSendable(cptr)) {
            SetDoingList(sptr);     /* only set if its a full list */
            ClearSendqPop(sptr);    /* just to make sure */
            cptr->listprogress = 0;
            cptr->listprogress2 = 0;
            send_list(cptr);
          }
	return 0;
        }
      

    /*
     * General idea: We don't need parv[0], since we can get that
     * information from cptr.name. So, let's parse each element of
     * parv[], setting pointer parv to the element being parsed.
     */
    while (--parc) {
	parv += 1;
	if (BadPtr(parv)) /* Sanity check! */
	    continue;

	name = strtoken(&p, *parv, ",");

	while (name) {
	  switch (*name) {
	    case '>':
		showall = 1;
		usermin = strtol(++name, (char **) 0, 10);
		break;

	    case '<':
		showall = 1;
		usermax = strtol(++name, (char **) 0, 10);
		break;

	    case 't':
	    case 'T':
		showall = 1;
		switch (*++name) {
		    case '>':
                        topictimemax = CurrentTime - 60 *
				       strtol(++name, (char **) 0, 10);
			break;

		    case '<':
                        topictimemin = CurrentTime - 60 *
				       strtol(++name, (char **) 0, 10);
			break;

		    case '\0':
			topictimemin = 1;
			break;

		    default:
                        sendto_one(sptr, ":%s 521 %s :Bad list syntax, type /quote list ? or /raw list ?",
				   me.name, cptr->name);
                        free_link(yeslist);
                        free_link(nolist);
                        sendto_one(sptr, form_str(RPL_LISTEND),
				   me.name, cptr->name);
			return 0;
      }
		break;

		case 'c':
		case 'C':
		    showall = 1;
		    switch (*++name) {
			case '>':
                            chantimemin = CurrentTime - 60 *
					  strtol(++name, (char **) 0, 10);
			    break;

			case '<':
                            chantimemax = CurrentTime - 60 *
					  strtol(++name, (char **) 0, 10);
			    break;

			default:
                            sendto_one(sptr, ":%s 521 %s :Bad list syntax, type /quote list ? or /raw list ?",
                                      me.name, sptr->name);
                            free_link(yeslist);
                            free_link(nolist);
                            sendto_one(sptr, form_str(RPL_LISTEND),
				       me.name, cptr->name);
        return 0;
      }
		    break;

		default: /* A channel or channel mask */

		    /*
		     * new syntax: !channelmask will tell ircd to ignore
		     * any channels matching that mask, and then
		     * channelmask will tell ircd to send us a list of
		     * channels only masking channelmask. Note: Specifying
		     * a channel without wildcards will return that
		     * channel even if any of the !channelmask masks
		     * matches it.
		     */

		    if (*name == '!') {
			showall = 1;
			listptr = make_link();
			listptr->next = nolist;
			DupString(listptr->value.cp, name+1);
			nolist = listptr;
		    }
		    else if (strchr(name, '*') || strchr(name, '?')) {
			showall = 1;
			listptr = make_link();
			listptr->next = yeslist;
			DupString(listptr->value.cp, name);
			yeslist = listptr;
		    }
		    else {
                        chptr = hash_find_channel(name, NullChn);
			if (chptr && ShowChannel(sptr, chptr))
                            sendto_one(sptr, form_str(RPL_LIST),
				       me.name, cptr->name,
				       ShowChannel(sptr,chptr) ? name : "*",
                                       chptr->users-chptr->iusers,
                                       chptr->topic, "");
		    }
	  } /* switch (*name) */
	name = strtoken(&p, NULL, ",");
	} /* while(name) */
    } /* while(--parc) */

    if (!showall || (chantimemin > CurrentTime) ||
         (topictimemin > CurrentTime)) {
        free_link(yeslist);
        free_link(nolist);
        sendto_one(sptr, form_str(RPL_LISTEND), me.name, cptr->name);
      return 0;
    }   

    lopt = (LOpts *)MyMalloc(sizeof(LOpts));

    lopt->showall = 0;
    lopt->next = NULL;
    lopt->yeslist = yeslist;
    lopt->nolist = nolist;
    lopt->usermin = usermin;
    lopt->usermax = usermax;
    lopt->currenttime = CurrentTime;
    lopt->chantimemin = chantimemin;
    lopt->chantimemax = chantimemax;
    lopt->topictimemin = topictimemin;
    lopt->topictimemax = topictimemax;

    cptr->lopt = lopt;

    cptr->listprogress = 0;
    cptr->listprogress2 = 0;

    ClearSendqPop(cptr);
    SetDoingList(cptr);
    send_list(cptr);

  return 0;
}
