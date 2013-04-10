/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2005    *
 *                    http://software.pt-link.net                *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 
 File: m_svsguest.c
 Desc: services nick change (for a guest generated nick)
 Author: Lamego
 
 * $Id: m_svsguest.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#include "s_conf.h"
#include "dconf_vars.h"
#include "whowas.h" /* we need add_history( */



void change_nick(aClient *acptr, char *newnick);

/*
** m_svsguest()
**   parv[0] = sender
**   parv[1] = target nick
**   parv[2] = guest prefix
**   parv[3] = max guest number (9999)
**
*/
int m_svsguest(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  int randnum;
  int maxnum;
  int scount;	/* search count */
  char guestnick[NICKLEN];
  struct Client* acptr;

  /* Check if received from services */
  if(!IsServer(cptr) || !IsService(sptr)) 
    {	
      if (IsServer(cptr))
        { 
	  ts_warn("Got SVSGUEST from non-service: %s", 
	    sptr->name);
	  sendto_one(cptr, ":%s WALLOPS :ignoring SVSGUEST from non-service %s",
	    me.name, sptr->name);
	}
      return 0;
    }

  if( parc < 4 ) /* Check for arguments count */
    {	  
      ts_warn("Invalid SVSGUEST (%s) from %s",
	(parc==2 ) ? parv[1]: "-", parv[0]);
      return 0;
    }
  
  
  
  if ((acptr = find_person(parv[1], NULL)) && MyClient(acptr)) /* person found connected here */
    {
      maxnum = atoi(parv[3]);
      randnum = 1+ (random() % (maxnum+1));
      snprintf(guestnick, NICKLEN, "%s%d", parv[2], randnum);
      scount = 0;
                                                                                
      while((scount++<maxnum+1) && 
        find_client(guestnick, (aClient *)NULL))
        {
          randnum = 1+ (random() % (maxnum+1));
          snprintf(guestnick, NICKLEN, "%s%d", parv[2], randnum);
        }
	
      if(scount<maxnum+1) /* check if we reached max guests count */
        {
           change_nick(acptr, guestnick);
	} 
	else
	  exit_client(acptr, acptr, &me, "Maximum guests count reached!!!");
    }
  else if (acptr) /* nick was found but is not our client */	
    {
      if ( (acptr->from != cptr)) /* this should never happen */
        sendto_one(acptr, 
	  ":%s SVSGUEST %s %s %s", parv[0], parv[1], parv[2], parv[3]);	  
    }
  return 0;
}

void change_nick(aClient *acptr, char *newnick)
{
  char     nick[NICKLEN + 2];
    
  strncpy_irc(nick, newnick, NICKLEN);
  nick[NICKLEN] = '\0';

  ClearIdentified(acptr);	/* maybe not needed, but safe -Lamego */

  /* send change to common channels */
  sendto_common_channels(acptr, ":%s NICK :%s", acptr->name, nick);
  if (acptr->user)
    {
      acptr->tsinfo = CurrentTime;
      add_history(acptr,1);
      /* propagate to all servers */
      sendto_serv_butone(NULL, ":%s NICK %s :%lu",
      acptr->name, nick, acptr->tsinfo);
    }          
		  	
  /*
   **  Finally set new nick name.
  */
  if (acptr->name[0]) 
    {
      del_from_client_hash_table(acptr->name, acptr);
      hash_check_watch(acptr, RPL_LOGOFF);     
    }
		
  strcpy(acptr->name, nick);
  add_to_client_hash_table(nick, acptr);
  hash_check_watch(acptr, RPL_LOGON);
	  
  /* Lets apply nick change delay (if needed) */
  acptr->services_nick_change++;
  if(acptr->services_nick_change>1)
    {
      acptr->number_of_nick_changes = MAX_NICK_CHANGES+1;
      acptr->last_nick_change = CurrentTime;
      acptr->services_nick_change = 0;
    }
}
