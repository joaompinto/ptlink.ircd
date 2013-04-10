/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2000     *
 *                http://software.pt-link.net                     *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 *   $Id:
 
 File: sqline.c
 Desc: services Q:lines
 Author: Lamego@PTlink.net
  
*/

#include "m_commands.h"
#include "client.h"
#include "common.h"
#include "ircd.h"
#include "numeric.h"
#include "s_serv.h"
#include "irc_string.h"
#include "send.h"
#include "s_conf.h"
#include "struct.h"
#include "sqline.h"
#include "dconf_vars.h"


#include <stdlib.h> 
#include <string.h>

/* sqlines storage */
aConfItem *sqlines = (aConfItem *)NULL;

/*
 * m_sqline() - add services based sqline
 *
 *	parv[0] = sender prefix
 *	parv[1] = banned nick (masks are allowed)
 *	parv[2] = reason 
 */
int	m_sqline(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
  {
    aConfItem *aconf;
	
	if (!IsServer(sptr) && !IsService(sptr)) 
	  {	
		if (IsServer(cptr))
		  { 
			ts_warn("Got SQLINE from non-service/server: %s", 
			  sptr->name);
			sendto_one(cptr, ":%s WALLOPS :ignoring SQLINE from non-service/server %s",
			  me.name, sptr->name);
		}		
		return 0;
	  }
	  
	if(parc<2)
	  {
		sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
      	  me.name, parv[0], "SQLINE");
        return 0;
	  }
	  
    if (!find_sqline(parv[1])) /* if sqline dost not exist */
	  {
		aconf = make_conf();
		DupString(aconf->name, parv[1]);
		
		if(parc>2 && *parv[2]) 
		  DupString(aconf->passwd, parv[2]);
		else
		  aconf->passwd = NULL;

	    aconf->next = sqlines;
	    sqlines = aconf;
	
	    sendto_serv_butone(cptr, ":%s SQLINE %s :%s",
			parv[0], parv[1], parc >2 ? parv[2] : "");
			
	  }
	  
	return 0;
  }

/*
 * m_unsqline() - remove services based sqline
 *
 *	parv[0] = sender prefix
 *	parv[1] = banned nick (masks are allowed)
 */
int	m_unsqline(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
  {
	aConfItem *sql = sqlines;
	aConfItem *last_sql;
		
	if (!IsServer(sptr) && !IsService(sptr)) 
	  {	
		if (IsServer(cptr))
		  { 
			ts_warn("Got UNSQLINE from non-service/server: %s", 
			  sptr->name);
			sendto_one(cptr, ":%s WALLOPS :ignoring UNSQLINE from non-service/server %s",
			  me.name, sptr->name);
		}		
		return 0;
	  }

	if(parc<2)
	  return 0;
	
	last_sql = NULL;
	
	while(sql && irccmp(sql->name, parv[1]))
	  {
		last_sql = sql;
		sql = sql->next;
	  }
	  
    if (sql) /* if sqline does exist */
	  {
		if(last_sql) /* this is not the first sqline */
			last_sql->next=sql->next; /* adjust list link -> */
		  else
			sqlines = sql->next;
			
		free_conf(sql);
	  }

    /* always propagate UNSQLINE */
    sendto_serv_butone(cptr, ":%s UNSQLINE %s",
			parv[0], parv[1]);			
	  
	return 0;
  }
  

/*
 * find_sqline  - checks if nick/mask matchs any entry on the sqline list
 * inputs       - lookup nick/mask
 * output       - return gline entry if found or NULL if not found;
 * side effects - none
 */
struct ConfItem* find_sqline(char *nick)
  {
	aConfItem *sql = sqlines;
	
	while(sql && !match(sql->name, nick))
	  sql = sql->next;
	  	  
	return sql;	
  }
  
/* send_all_sqlines
 *
 * inputs       - pointer to aClient
 * output       - none
 * Side effects - sends all sqlines to the specified server
 *
 */
void send_all_sqlines(struct Client *acptr)
  {
	aConfItem *sql = sqlines;
	
	while(sql)
	  {
	    sendto_one(acptr, ":%s SQLINE %s :%s",
			me.name, sql->name, sql->passwd ? sql->passwd : "");	
		sql = sql->next;		
	  }
  }
  
/*
 * report_sqlines
 *
 * inputs       - pointer to client to report to
 * output       - none
 * side effects - all Q lines are listed to client 
 */
void report_sqlines(struct Client *sptr)
{
  struct ConfItem *aconf = sqlines;
  char *host;
  char *user;
  char *pass;
  char *name;
  int port;

  while(aconf)
	{
  	  get_printable_conf(aconf, &name, &host, &pass, &user, &port);
          
      sendto_one(sptr, form_str(RPL_STATSQLINE),
                   me.name, sptr->name, name, pass, "", "");
	  aconf=aconf->next;	  
    }
}


/*
 * clear_sqlines
 *
 * inputs       - none
 * output       - none
 * side effects - clear the sqlines list
 */
void clear_sqlines()
  {
  
    aConfItem *aconf;
	aConfItem *sql=sqlines;
  
	while(sql)
	  {
		aconf = sql->next;
		free_conf(sql);
		sql = aconf;
		}
			  
	sqlines = NULL;
	
  }
