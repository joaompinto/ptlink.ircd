/************************************************************************
 *   IRC - Internet Relay Chat, src/m_gline.c
 *   Copyright (C) 1990 Jarkko Oikarinen and
 *                      University of Oulu, Computing Center
 *
 *   See file AUTHORS in IRC package for additional names of
 *   the programmers. 
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
 *  $Id: m_ungline.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $
 */
#include "m_gline.h"
#include "channel.h"
#include "client.h"
#include "common.h"
#include "config.h"
#include "dline_conf.h"
#include "irc_string.h"
#include "ircd.h"
#include "m_kline.h"
#include "mtrie_conf.h"
#include "numeric.h"
#include "s_conf.h"
#include "s_misc.h"
#include "s_serv.h"
#include "scache.h"
#include "send.h"
#include "struct.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>


int     m_ungline(aClient *cptr,
                aClient *sptr,
                int parc,
                char *parv[]);


/* external variables */
extern ConfigFileEntryType ConfigFileEntry; /* defined in ircd.c */

/* internal variables */
extern  aConfItem *glines;

/* internal functions */
static int del_gline(char *, char *);


/*
 * m_ungline()
 *
 *		parv[0] = sender prefix
 *		parv[1] = user@host
 */
int     m_ungline(aClient *cptr,
                aClient *sptr,
                int parc,
                char *parv[])
{
  char *user, *host;            /* user and host of GLINE "victim" */
  char tempuser[USERLEN + 2];
  char temphost[HOSTLEN + 1];
  aConfItem *my_gline;

  if(MyClient(sptr)) /* allow remote opers to apply g lines */
    {

      /* Only globals can apply Glines */
      if (!IsOper(sptr))
        {
          sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, parv[0]);
          return 0;
        }

      if (!IsSetOperGline(sptr))
        {
          sendto_one(sptr,":%s NOTICE %s :You have no G flag",me.name,parv[0]);
          return 0;
        }

	}

  if ( parc < 2 )
    {
          sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
                     me.name, parv[0], "UNGLINE");
          return 0;
    }
	
  if(irccmp(parv[1],"ALL")==0)
	{
	
	  /* remove all glines */
	  my_gline=glines;
	  
	  while(my_gline)
		{
		  my_gline = my_gline->next;
		  free(glines);
		  glines = my_gline;
		}

	  if(MyClient(sptr))
		sendto_one(sptr, ":%s NOTICE %s :All G-Lines had been removed",
           me.name,
           parv[0]);

	  sendto_ops("%s!%s@%s on %s removed ALL G-Lines",
			sptr->name, sptr->username, sptr->host, sptr->user->server);
			
	  sendto_serv_butone(IsServer(cptr) ? cptr : NULL,
  						":%s UNGLINE ALL",
                         sptr->name);			
	  return 0;
	}
	
  if ( (host = strchr(parv[1], '@')) || *parv[1] == '*' )
    {
	    /* Explicit user@host mask given */
		
        if(host)                      /* Found user@host */
      	  {
        	user = parv[1];   /* here is user part */
        	*(host++) = '\0'; /* and now here is host */
      	  }
    	else
      	  {
        	user = "*";               /* no @ found, assume its *@somehost */
        	host = parv[1];
      	  }

    	if (!*host)           /* duh. no host found, assume its '*' host */
      	  host = "*";
  
    	strncpy_irc(tempuser, user, USERLEN + 1);     /* allow for '*' */
  		tempuser[USERLEN + 1] = '\0';
	    strncpy_irc(temphost, host, HOSTLEN);
  		temphost[HOSTLEN] = '\0';
	    user = tempuser;
  		host = temphost;
  	  }
  else
    {
	  sendto_one(sptr, ":%s NOTICE %s :Can't UnG-Line a nick, please use user@host",
           me.name,
           parv[0]);
    	return 0;
  	}
  if( !del_gline(user, host) )
	{
	  sendto_one(sptr, ":%s NOTICE %s :G-Line was not found",
           me.name,
           parv[0]);
		return 0;
	}
  else
	{
	
	  if(IsClient(sptr))
		{
		  if (!sptr->user || !sptr->user->server)
  			return 0;
			
	  	  sendto_ops("%s!%s@%s on %s removed gline for \2[\2%s@%s\2]\2",
			sptr->name, sptr->username, sptr->host, sptr->user->server,
			  user,host);
		}
	}

  sendto_serv_butone(IsServer(cptr) ? cptr : NULL,
  						":%s UNGLINE %s@%s",
                         sptr->name,
                         user,
                         host);
						 						 	
  return 0;
}


/* del_gline
 *
 * inputs       - user, host of gline to be deleted
 * output       - 0 if gline not found, -1 if found (deleted)
 * side effects - 
 *
 * deletes a gline
 * 
 * - Lamego
 */
int del_gline(char* user, char *host)
{
  aConfItem *kill_list_ptr;
  aConfItem *last_list_ptr;
  aConfItem *tmp_list_ptr;

  if(glines)
    {
      kill_list_ptr = last_list_ptr = glines;

      while(kill_list_ptr)
        {
		  if( (kill_list_ptr->user && (!user || !strcasecmp(kill_list_ptr->user,
                 user))) && (kill_list_ptr->host &&
                   (!host || !strcasecmp(kill_list_ptr->host,host))))
            {
              if(glines == kill_list_ptr)
                {
                  glines = last_list_ptr = tmp_list_ptr =
                    kill_list_ptr->next;
                }
              else
                {
                  /* its in the middle of the list, so link around it */
                  tmp_list_ptr = last_list_ptr->next = kill_list_ptr->next;
                }

              free_conf(kill_list_ptr);
              kill_list_ptr = tmp_list_ptr;
			  return -1; /* found and deleted */
            }
			
		  else 
		  
			{
			  last_list_ptr = kill_list_ptr;
              kill_list_ptr = kill_list_ptr->next;

			}
        }
    }
  return 0;	/* gline not found */
}
