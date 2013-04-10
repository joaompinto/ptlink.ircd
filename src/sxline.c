/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2001     *
 *                http://software.pt-link.net                     *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
   
 File: sxline.c
 Desc: services xlines (real name xlines)
 Author: Lamego

 * $Id: sxline.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $  
*/

#include "m_commands.h"
#include "client.h"
#include "s_user.h"
#include "s_misc.h"
#include "common.h"
#include "ircd.h"
#include "numeric.h"
#include "s_serv.h"
#include "irc_string.h"
#include "send.h"
#include "s_conf.h"
#include "struct.h"
#include "sxline.h"
#include "dconf_vars.h"

#include <stdlib.h> 
#include <string.h>

/* sxlines storage */
aConfItem *sxlines = (aConfItem *)NULL;

/*
 * m_sxline() - add info ban line
 *
 *	parv[0] = sender prefix
 *	parv[1] =  info banned mask
 */
int	m_sxline(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
  {
    aConfItem *aconf;
    char *reason = NULL;
    char *mask;
    int len;

	if (!IsService(sptr) && !IsServer(cptr))
	  {
		sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, parv[0]);
		return 0;
	  }	  
		
	if(parc<3)
	  {
		sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
      	  me.name, parv[0], "SXLINE");
        return 0;
	  }
      
    len=atoi(parv[1]);
    mask = parv[2];
    
    if ((strlen(mask) > len) && (mask[len])==':')
      {
        mask[len] = '\0';
        reason = mask+len+1;
      } 
    else
      { /* Bogus */
        return 0;
      }
    
    if (!find_sxline(mask)) /* sxline does not exist */
	  {

		aconf = make_conf();
		DupString(aconf->name, mask);
		DupString(aconf->passwd, reason);
	    aconf->next = sxlines;
		sxlines = aconf;		

        sendto_serv_butone(cptr, ":%s SXLINE %d :%s:%s", sptr->name, len,
                       aconf->name,aconf->passwd);
	  }

	return 0;
  }

/*
 * m_unsxline() - remove sxline
 *
 *	parv[0] = sender prefix
 *	parv[1] = sxlined name
 */
int	m_unsxline(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
  {
	aConfItem *sxl = sxlines;
	aConfItem *last_sxl = NULL;
		
	if (!IsService(sptr) && !IsServer(cptr))
	  {
		sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, parv[0]);
		return 0;
	  }	  
		
	if(parc<2)
	  {
		sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
      	  me.name, parv[0], "UNSXLINE");
        return 0;
	  }		  
	
	while(sxl && irccmp(sxl->name, parv[1]))
	  {
		last_sxl = sxl;
		sxl = sxl->next;
	  }
	  
    if (sxl) /* if sxline does exist */
	  {
		if(last_sxl) /* this is not the first sxline */
			last_sxl->next=sxl->next; /* adjust list link -> */
		  else
			sxlines = sxl->next;
			
		free_conf(sxl);
		
	    sendto_serv_butone(cptr, ":%s UNSXLINE :%s",
			parv[0], parv[1]);
	  }
	  
	return 0;
  }
  

/*
 * find_sxline  - checks if the host matchs any entry on the sqline list
 * inputs       - lookup host
 * output       - return sxline entry if found or NULL if not found;
 * side effects - heading sxline will be checked and deleted if expired
 */
struct ConfItem* find_sxline(char *name)
  {
	aConfItem *sxl;

	/* First lets check if we have sxlines */
	if(!sxlines)
	  return NULL;
	  	  
	sxl = sxlines;

    /* search for host */	
	while(sxl && irccmp(sxl->name, name))
	  sxl=sxl->next;
	  
    return sxl;	
		
  }

/*
 * is_sxlined  - checks if the host matchs any entry on the sqline list
 * inputs       - lookup name
 * output       - return sxline entry if found or NULL if not found;
 * side effects - heading sxline will be checked and deleted if expired
 */
struct ConfItem* is_sxlined(char *name)
  {
	aConfItem *sxl;

	/* First lets check if we have sxlines */
	if(!sxlines)
	  return NULL;
	  	  
	sxl = sxlines;

    /* search for host */	
	while(sxl && !match(sxl->name, name))
	  sxl=sxl->next;
	  
    return sxl;	
		
  }
  
  
/* send_all_sxlines
 *
 * inputs       - pointer to aClient
 * output       - none
 * Side effects - sends all sxlines to the specified server
 *
 */
void send_all_sxlines(struct Client *acptr)
  {
	struct ConfItem *sxl = sxlines;
	
	while(sxl)
	  {	  	  	  
        sendto_one(acptr, ":%s SXLINE %d :%s:%s",
              me.name, strlen(sxl->name), sxl->name, sxl->passwd ? sxl->passwd : "");
		sxl = sxl->next;
	  }
  }
  
/*
 * report_sxlines
 *
 * inputs       - pointer to client to report to
 * output       - none
 * side effects - all SG lines are listed to client 
 */
void report_sxlines(struct Client *sptr)
{
  struct ConfItem *aconf;
  char *host;
  char *user;
  char *pass;
  char *name;
  int port;
  
  aconf = sxlines;
  
  while(aconf)
	{
  	  get_printable_conf(aconf, &name, &host, &pass, &user, &port);
          
      sendto_one(sptr, form_str(RPL_STATSQLINE),
                   me.name, sptr->name, name, pass, "", "");
	  aconf=aconf->next;	  
    }
}

/*
 * clear_sxlines
 *
 * inputs       - none
 * output       - none
 * side effects - clear the sxline list
 */
void clear_sxlines(void)
  {
  
    aConfItem *aconf;
	aConfItem *sxl=sxlines;
  
	while(sxl)
	  {
		aconf = sxl->next;
		free_conf(sxl);
		sxl = aconf;
		}
			  
	sxlines = NULL;
	
  }
