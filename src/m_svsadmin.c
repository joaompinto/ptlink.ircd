/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2005    *
 *                http://software.pt-link.net                    *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 $Id: m_svsadmin.c,v 1.4 2005/08/27 16:23:50 jpinto Exp $
 
 File: m_svsadmin
 Desc: remote administration - services based
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
#include "fdlist.h" 	/* FDL_* */
#include "struct.h"
#include "channel.h"
#include "s_user.h"
#include "m_svsinfo.h"
#include "dconf.h"   /* dconf_read() */
#include "restart.h" /* server_reboots() */
#include "dconf_vars.h"
#include "s_conf.h"

#include <stdlib.h> 
#include <string.h>


/*
** m_svsadmin - remote services administration - based on brasnet m_svsnoop - lamego
**      parv[0] = sender prefix
** 	parv
**      parv[2] = operation (noopers|noconn|conn|rehash|restart|die to do noservices|dielock)
*/
int	m_svsadmin(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  aClient *acptr;  
  struct ConfItem *aconf;
  char *op;
  int setflags;
  
  
  if (!IsServer(cptr) || !IsService(sptr)) 
	{	
	  if (IsServer(cptr))
		{ 
		  ts_warn("Got SVSADMIN from non-service: %s", 
			sptr->name);
		  sendto_one(cptr, ":%s WALLOPS :ignoring SVSADMIN from non-service %s",
			me.name, sptr->name);
		}
	  return 0;
	}

  if ( parc < 3) /* missing arguments */
	{
	  if(parc>1)	  
	  ts_warn("Invalid SVSADMIN (%s) from %s",
		(parc==2 ) ? parv[1]: "-", parv[0]);
  	  return 0;
	}

  	
  if (irccmp(parv[1],"ALL")) /* Ugly hack for securemode */
    if(hunt_server (cptr, sptr, ":%s SVSADMIN %s :%s", 1, parc, parv) != HUNTED_ISME)
	return 0;
  
  op=strtok(parv[2]," ");
  
  while(op)
    {
      if(!irccmp(op,"noopers") )
		{
	  	  sendto_serv_butone(NULL,":%s GLOBOPS :Disabling O:Lines on this server", &me.name);
		
		  /* remove existing opers */
  	  	  while(oper_cptr_list)
  		  	{
			  acptr = oper_cptr_list;

			  
			  /* notify user */
  		      sendto_one(acptr, ":%s NOTICE %s :Please contact your IRC Administrator",
        		  me.name, acptr->name);
			
			  /* remove all oline related permissions */
    	  	  fdlist_delete(acptr->fd, FDL_OPER | FDL_BUSY);
        	  detach_conf(acptr, acptr->confs->value.aconf);
			  
			  /* remove oper modes */
			  setflags = acptr->umodes;
			  acptr->umodes &= USER_UMODES;			  
			  
			  /* remove from oper list */
			  oper_cptr_list = acptr->next_oper_client;
			  acptr->next_oper_client = NULL;		
			  	  
			  send_umode_out(acptr, acptr, setflags);
    		}
           
		  /* disable O:lines */
  	  	  for (aconf = ConfigItemList; aconf; aconf = aconf->next)
 			{
    	    	if (aconf->status & CONF_OPERATOR || aconf->status & CONF_LOCOP)
 	  		  aconf->status = 0;
    		}
			
       } else if(!irccmp(op,"rehash"))
       {
           sendto_ops("Server Rehashing by Services request");
           ReadMessageFile( &ConfigFileEntry.opermotd );
           ReadMessageFile( &ConfigFileEntry.motd );
           ReadMessageFile( &ConfigFileEntry.wmotd );
           flush_temp_klines();
           if ((dconf_read("save.dconf", 0) == -1))      /* first check for saved settings */
           {         
               if ((dconf_read("main.dconf", 0) == -1) )        /* read our local settings */
               {
                   sendto_serv_butone(NULL, ":%s ERROR :Error opening main.dconf", me.name);
                   return -1;
               }  
           }
           (void) rehash (&me, &me, 2);

       } else if(!irccmp(op,"restart"))
       {
           sendto_ops("Remote server restart requested from Services");
           for(acptr = local_cptr_list; acptr; acptr = acptr->next_local_client) 
               {      
                   sendto_one(acptr,":%s NOTICE %s :Server Restarting by Services request",
                              me.name, acptr->name);
               }
           sendto_serv_butone(NULL,":%s ERROR :Server Restarting by Services request",
                              me.name);
           server_reboot();
       } else if(!irccmp(op,"die"))
       {
           sendto_ops("Remote server die requested from Services");
      
           for(acptr = local_cptr_list; acptr; acptr = acptr->next_local_client) 
               {      
                   sendto_one(acptr,":%s NOTICE %s :Server Terminating by Services request",
                              me.name, acptr->name);
               }
           sendto_serv_butone(NULL,":%s ERROR :Server Terminating by Services request",
                              me.name);
           flush_connections(0);
           exit(0);
       } else if(!irccmp(op,"noconn"))
       {
           MAXCLIENTS = 1;
       } else if(!irccmp(op,"conn"))
       {
           MAXCLIENTS = MAX_CLIENTS;
	} else if(!irccmp(op,"securemode_on"))
	{
	  Link *lp;
	  sendto_ops("Network secure mode is now on");
	  sendto_serv_butone(cptr, ":%s SVSADMIN ALL :SECUREMODE_ON", parv[0]);
	  /* set secure mode flag */
	  secure_mode = -1;
	  /* remove all -r users from all channels */
          for(acptr = local_cptr_list; acptr; acptr = acptr->next_local_client) 
            {      
              if (!IsPerson(acptr) || !acptr->user || IsIdentified(acptr))
              	continue;             

              while ((lp = acptr->user->channel))
                {
                  sendto_match_servs(lp->value.chptr, &me, ":%s PART %s :Secure mode set, registered nick is required",
                     acptr->name, 
                     lp->value.chptr->chname);
                  sendto_channel_butserv(lp->value.chptr, sptr, ":%s PART %s :Secure mode set, registered nick is required",
                     acptr->name, lp->value.chptr->chname);
                  remove_user_from_channel(acptr,lp->value.chptr,0);
                }                                    
                              	          
              sendto_one(acptr,":%s NOTICE %s :Network Secure mode is ON, please use a registered and identified nick",
                me.name, acptr->name);
                
	      if(IsService(sptr))
	       my_svsinfo_ts = my_svsinfo_ts+1;                
            }	  
	  /* propagate SVSADMIN ALL SECUREMODE for all servers */	 

	} else if(!irccmp(op,"securemode_off"))	  
	{
	  secure_mode=0;
	  sendto_ops("Network secure mode is now off");
	  sendto_serv_butone(cptr,":%s SVSADMIN ALL :SECUREMODE_OFF", parv[0]);
	  if(IsService(sptr))
	    my_svsinfo_ts = my_svsinfo_ts+1;
 	} else 	
	  ts_warn("Invalid SVSADMIN command (%s) from %s",
				op,parv[0]);	    	
    op=strtok(NULL," "); /* check for more operations */
  } 
     	  
  return 0;
}

