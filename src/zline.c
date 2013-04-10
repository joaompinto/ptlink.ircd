/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2001     *
 *                http://software.pt-link.net                     *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 
 File: zline.c
 Desc: zap lines
 Author: Lamego@PTlink.net
  
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
#include "zline.h"
#include "dconf_vars.h"


#include <stdlib.h> 
#include <string.h>

/* zlines storage */
aConfItem *zlines = (aConfItem *)NULL;

/*
 * m_zline() - add zap line
 *
 *	parv[0] = sender prefix
 *	parv[1] = banned host
 *	parv[2] = time
 *	parv[3] = reason 
 */
int	m_zline(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
  {
    aConfItem *aconf;
	int secs;
	char* timestr;
	char* ch;
		
	if (!IsPrivileged(cptr)) 
	  {
		sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, parv[0]);
		return 0;
	  }	  
		
	if(parc<3)
	  {
		sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
      	  me.name, parv[0], "ZLINE");
        return 0;
	  }

    if(!valid_hostname(parv[1]))
  	  {
          sendto_one(sptr,":%s NOTICE %s :*** Notice -- Invalid hostname for zline", 
                     me.name, sptr->name );		
		return 0;
      }
	  		  
    if (!find_zline(parv[1])) /* zline does not exist */
	  {

		timestr= parv[2];  
		ch=&timestr[strlen(timestr)-1];
		switch(*ch) 
		  {
  			case 'm': secs = atol(timestr)*60;break;
    		case 'h': secs = atol(timestr)*3600;break;
    		case 'd': secs = atol(timestr)*3600*24;break;
    		default : secs = atol(timestr);break;
  		  } 		
	  
		aconf = make_conf();
		DupString(aconf->name, parv[1]);
		
		aconf->hold = CurrentTime+secs;

		if(parc>3 && *parv[3]) 
		  DupString(aconf->passwd, parv[3]);
		else
		  aconf->passwd = NULL;

	    aconf->next = zlines;
		zlines = aconf;		

		if(IsPerson(sptr))
		  {
			sendto_ops("%s!%s@%s on %s added zline for \2[\2%s\2]\2 \2[\2%s\2]\2 expiring on %s",
			  sptr->name,
			  sptr->username,
			  sptr->realhost,
			  sptr->user->server,
			  aconf->name,
			  aconf->passwd ? aconf->passwd : "no reason",
  			  smalldate(aconf->hold));
		  }
		  
	    sendto_serv_butone(cptr, ":%s ZLINE %s %lu :%s",
			parv[0], parv[1], secs, parc>3 ? parv[3] : "");
			
	  }
	else
	  {
		if(MyClient(sptr))
	  	  sendto_one(sptr,
			 ":%s NOTICE %s :*** Cannot add Z:Line - Host already exists!", 
		   me.name, sptr->name);		
	  }	  
	return 0;
  }

/*
 * m_unzline() - remove zap line
 *
 *	parv[0] = sender prefix
 *	parv[1] = zapped host
 */
int	m_unzline(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
  {
	aConfItem *zl = zlines;
	aConfItem *last_zl = NULL;
		
	if (!IsPrivileged(cptr)) 
	  {
		sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, parv[0]);
		return 0;
	  }	  
		
	if(parc<2)
	  {
		sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
      	  me.name, parv[0], "UNZLINE");
        return 0;
	  }		  
	
	while(zl && irccmp(zl->name, parv[1]))
	  {
		last_zl = zl;
		zl = zl->next;
	  }
	  
    if (zl) /* if zline does exist */
	  {
		if(last_zl) /* this is not the first zline */
			last_zl->next=zl->next; /* adjust list link -> */
		  else
			zlines = zl->next;
			
		free_conf(zl);
		
		if(IsClient(sptr))
		  sendto_ops("%s!%s@%s on %s removed zline for \2[\2%s\2]\2",
			  sptr->name, sptr->username, sptr->host, sptr->user->server,
			  parv[1]);
				
	    sendto_serv_butone(cptr, ":%s UNZLINE %s",
			parv[0], parv[1]);			
	  }
	else
	  if(MyClient(sptr))
		sendto_one(sptr, ":%s NOTICE %s :Z-Line was not found",
           me.name,
           parv[0]);
	  
	return 0;
  }
  

/*
 * find_zline  - checks if the host matchs any entry on the sqline list
 * inputs       - lookup host
 * output       - return zline entry if found or NULL if not found;
 * side effects - heading zline will be checked and deleted if expired
 */
struct ConfItem* find_zline(char *host)
  {
	aConfItem *zl;
	aConfItem *aux_zl = NULL;

	/* First lets check if we have zlines */
	if(!zlines)
	  return NULL;
	  	  
	zl = zlines;

    /* search for host */	
	while(zl)
	  {
  		if(CurrentTime > zl->hold)
		  {

			sendto_ops("Z:line for %s has expired", zl->name);
		  
			if(aux_zl) /* this is not the first zline*/
			  {
				aux_zl->next = zl->next;
				free_conf(zl);
				zl = aux_zl->next;	
			  }
			else
			  {
				zlines = zl->next;
				free_conf(zl);
				zl = zlines;
			  }
			  
			continue;
			
		  }
		else
		  if( irccmp(zl->name, host) == 0 )
			return zl;
		  
		aux_zl=zl;
		zl=zl->next;
		
	  }	
	  
    return NULL;	/* just to avoid unreachable warning */
		
  }
  
/* send_all_zlines
 *
 * inputs       - pointer to aClient
 * output       - none
 * Side effects - sends all zlines to the specified server
 *
 */
void send_all_zlines(struct Client *acptr)
  {
	struct ConfItem *zl = zlines;
	
	while(zl)
	  {	  
		if(zl->hold-CurrentTime > 0)
	  	  sendto_one(acptr, ":%s ZLINE %s %d :%s",
			  me.name, zl->name,
			  zl->hold-CurrentTime,
			  zl->passwd ? zl->passwd : "");	
			  
		zl = zl->next;		
	  }
  }
  
/*
 * report_zlines
 *
 * inputs       - pointer to client to report to
 * output       - none
 * side effects - all Z lines are listed to client 
 */
void report_zlines(struct Client *sptr)
{
  struct ConfItem *aconf;
  
  (void) find_zline("");	/* check for expiring zlines */
  aconf = zlines;
  
  while(aconf)
	{
      sendto_one(sptr, ":%s 226 %s Z %s For: %d secs Reason: %s",
           me.name, sptr->name, aconf->name,
           aconf->hold - CurrentTime, aconf->passwd ? aconf->passwd : "");
				   
	  aconf=aconf->next;	  
    }
}
