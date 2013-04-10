/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2002     *
 * http://www.ptlink.net/Coders/ - coders@PTlink.net             *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 
 File: vlinks.c
 Desc: virtual links
 Author: Lamego@PTlink.net
 Based on Brasnet vlinks from fabulous@t7ds.com.br
*/

#include "client.h"
#include "common.h"
#include "ircd.h"
#include "numeric.h"
#include "s_serv.h"
#include "irc_string.h"
#include "send.h"
#include "s_conf.h"
#include "struct.h"
#include "s_user.h"
#include "vlinks.h"


#include <stdlib.h> 
#include <string.h>


/* svlinks storage 
   name = server name
   passwd = description
*/
aConfItem *vlinks = (aConfItem *)NULL;


/*
 * m_vlink () - add/delete virtual link
 *
 *	parv[0] = sender prefix
 *	parv[1] = [-]virtual server name ('-' as prefix means delete)
 *	parv[2] = virtual server description (only used when adding)
 */
int	m_vlink(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
  {
    aConfItem *vl, *last_vl = NULL;
    
	int is_del = 0; /* delete flag */
	if (!IsServer(sptr) && !IsService(sptr)) 
	  {	
		if (IsServer(cptr))
		  { 
			ts_warn("Got VLINK from non-service/server: %s", 
			  sptr->name);
			sendto_one(cptr, ":%s WALLOPS :ignoring VLINK from non-service/server %s",
			  me.name, sptr->name);
		  }		
		return 0;
	  }
     
    if(parc>1 && parv[1][0]=='-')
        is_del = 1;
    
	if(parc<2 || (!is_del && parc<3))
	  {
		sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
      	  me.name, parv[0], "VLINK");
        return 0;
	  }

    if(is_del) /* delete vlink */
      {
        vl = vlinks;
        while(vl && irccmp(vl->name, &parv[1][1]))
		  {
            last_vl = vl;
		    vl = vl->next;
	      }
          
        if (vl) /* if vlink does exist */
	      {
            struct Client *acptr;
            /* lets check if any user pointing to this vlink */
            for (acptr = GlobalClientList; acptr; acptr = acptr->next)
              {
                if(acptr->user && acptr->user->vlink==vl)
                  acptr->user->vlink = NULL;
              }
		    if(last_vl) /* this is not the first sqline */
			  last_vl->next=vl->next; /* adjust list link -> */
		    else
			  vlinks = vl->next;			
		    free_conf(vl);
          }
          
	    sendto_serv_butone(cptr, ":%s VLINK %s",
			parv[0], parv[1]);			
      }
    else /* add vlink */
      {
        if(!valid_hostname(parv[1]))
          {
          	ts_warn("Got invalid VLINK hostname: %s from %s", 
			  parv[1], sptr->name);
            return 0;
          }
        vl = find_vlink(parv[1]);
        if(!vl)
          {
            vl = make_conf();
	        DupString(vl->name, parv[1]);
            DupString(vl->passwd, parv[2]);
            vl->next = vlinks;
            vlinks = vl;
          }
        sendto_serv_butone(cptr, ":%s VLINK %s :%s",
		  parv[0], parv[1], parv[2]);
      }
    return 1;
  }

/*
 * find_vlink  - checks if nick/mask matchs any entry on the vlink list
 * inputs       - lookup nick/mask
 * output       - return gline entry if found or NULL if not found;
 * side effects - none
 */
struct ConfItem* find_vlink(char *nick)
  {
	aConfItem *vl = vlinks;
	
	while(vl && !match(vl->name, nick))
	  vl = vl->next;
	  	  
	return vl;	
  }
  
/* send_all_vlinks
 *
 * inputs       - pointer to aClient
 * output       - none
 * Side effects - sends all vlinks to the specified server
 *
 */
void send_all_vlinks(struct Client *acptr)
  {
	aConfItem *vl = vlinks;
	
	while(vl)
	  {
	    sendto_one(acptr, ":%s VLINK %s :%s",
			me.name, vl->name, vl->passwd);	
		vl = vl->next;		
	  }
  }
  
/*
 * report_vlinks
 *
 * inputs       - pointer to client to report to
 * output       - none
 * side effects - all Q lines are listed to client 
 */
void report_vlinks(struct Client *sptr)
{
  struct ConfItem *aconf = vlinks;
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
 * clear_vlinks
 *
 * inputs       - none
 * output       - none
 * side effects - clear the vlinks list
 */
void clear_vlinks()
  {
  
    aConfItem *aconf;    
    aConfItem *vl=vlinks;
    aClient *acptr;
        
    /* clear all vlink bindings */
    for (acptr = GlobalClientList; acptr; acptr = acptr->next)
      {
        if(acptr->user && acptr->user->vlink)
          acptr->user->vlink = NULL;
      }      
	while(vl)
	  {
		aconf = vl->next;
		free_conf(vl);
		vl = aconf;
	  }
			  
	vlinks = NULL;	
  }

/*
 * dump_vlinks
 *
 * inputs       - none
 * output       - none
 * side effects - send vlinks list with LINKS format
 */
void dump_vlinks(char* dest, struct Client *acptr)
  {
  	aConfItem *vl=vlinks;
    char *mename = me.name;

    if(acptr->user && acptr->user->vlink)
      mename = acptr->user->vlink->name;
    
    while(vl)
      {
        sendto_one(acptr, form_str(RPL_LINKS),
            mename, dest, vl->name, mename,
            0, vl->passwd);    
        vl=vl->next;
      }
  }
