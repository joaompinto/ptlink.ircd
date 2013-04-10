/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2000     *
 *                http://software.pt-link.net                     *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 $Id: s_services.c,v 1.6 2005/11/03 16:56:43 waxweazle Exp $
 
 File: s_services.c
 Desc: services aliases functions
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
#include "s_user.h"
#include "dconf_vars.h"
#include "s_conf.h"

#include <stdlib.h> 
#include <string.h>

int exceed_services_usage(struct Client *sptr);

/*
 * check_services_usage - checks for excessive services usage
 * inputs       - source client
 * output       - return -1 if usage exceeded
 * side effects - none
 */
int exceed_services_usage(struct Client *sptr)
  {
	  
	if(IsAnOper(sptr))
	  return 0;
  		
	if( (sptr->last_services_use + ServicesInterval) < CurrentTime)
  	  sptr->number_of_services_use = 0;
	  
    sptr->last_services_use  = CurrentTime;
    sptr->number_of_services_use++;	
	
	if (sptr->number_of_services_use > ServicesUseCount)		
	  {
		sendto_one(sptr,
      	  ":%s NOTICE %s :*** Notice -- Excessive services usage, wait %d seconds before using services again.",
          me.name,
          sptr->name,
          ServicesInterval);
		return YES;
	  }
	else 
	  return NO;
  }
  
/*
 * m_functions execute protocol messages on this server:
 *
 *      cptr    is always NON-NULL, pointing to a *LOCAL* client
 *              structure (with an open socket connected!). This
 *              identifies the physical socket where the message
 *              originated (or which caused the m_function to be
 *              executed--some m_functions may call others...).
 *
 *      sptr    is the source of the message, defined by the
 *              prefix part of the message if present. If not
 *              or prefix not found, then sptr==cptr.
 *
 *              (!IsServer(cptr)) => (cptr == sptr), because
 *              prefixes are taken *only* from servers...
 *
 *              (IsServer(cptr))
 *                      (sptr == cptr) => the message didn't
 *                      have the prefix.
 *
 *                      (sptr != cptr && IsServer(sptr) means
 *                      the prefix specified servername. (?)
 *
 *                      (sptr != cptr && !IsServer(sptr) means
 *                      that message originated from a remote
 *                      user (not local).
 *
 *              combining
 *
 *              (!IsServer(sptr)) means that, sptr can safely
 *              taken as defining the target structure of the
 *              message in this server.
 *
 *      *Always* true (if 'parse' and others are working correct):
 *
 *      1)      sptr->from == cptr  (note: cptr->from == cptr)
 *
 *      2)      MyConnect(sptr) <=> sptr == cptr (e.g. sptr
 *              *cannot* be a local connection, unless it's
 *              actually cptr!). [MyConnect(x) should probably
 *              be defined as (x == x->from) --msa ]
 *
 *      parc    number of variable parameter strings (if zero,
 *              parv is allowed to be NULL)
 *
 *      parv    a NULL terminated list of parameter pointers,
 *
 *                      parv[0], sender (prefix string), if not present
 *                              this points to an empty string.
 *                      parv[1]...parv[parc-1]
 *                              pointers to additional parameters
 *                      parv[parc] == NULL, *always*
 *
 *              note:   it is guaranteed that parv[0]..parv[parc-1] are all
 *                      non-NULL pointers.
 */


/*
** m_nickserv - Ported to PTlink6 with flood protection - Lamego
**      parv[0] = sender prefix
**      parv[1] = nickserv command
*/
int	m_nickserv(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
  {
    struct Client *acptr;
	
	if ( IsZombie(sptr) || exceed_services_usage(sptr) == YES )	/* check for services flood */
	  return -1;
	  
	if (parc < 2 || *parv[1] == '\0') 
	  {
        sendto_one(sptr, form_str(ERR_NOTEXTTOSEND), me.name, parv[0]);
        return -1;
  	  }
		  
    if (services_on && (acptr = find_server(ServicesServer)))
  	  sendto_one(acptr, ":%s PRIVMSG NickServ@%s :%s", 
  	  	parv[0], ServicesServer, parv[1]);
    else
      sendto_one(sptr, form_str(ERR_SERVICESDOWN), me.name,
	  		parv[0], "NickServ");					  
	return 0;					  
  }

/*
** m_chanserv - Ported to PTlink6 with flood protection - Lamego
**      parv[0] = sender prefix
**      parv[1] = chanserv command
*/
int 	m_chanserv(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
  {
    struct Client *acptr;
	
	if ( IsZombie(sptr) || exceed_services_usage(sptr) == YES )	/* check for services flood */
	  return -1;
	  
	if (parc < 2 || *parv[1] == '\0') 
	  {
        sendto_one(sptr, form_str(ERR_NOTEXTTOSEND), me.name, parv[0]);
        return -1;
  	  }
	  
    if (services_on && (acptr = find_server(ServicesServer)))
  	  sendto_one(acptr, ":%s PRIVMSG ChanServ@%s :%s", 
  	  	parv[0], ServicesServer, parv[1]);
    else
      sendto_one(sptr, form_str(ERR_SERVICESDOWN), me.name,
	  		parv[0], "ChanServ");					  
	return 0;					  
  }

/*
** m_statserv - Ported to PTlink6 with flood protection -Lamego
**      parv[0] = sender prefix
**      parv[1] = chanserv command
** Put in for future use of statserv - ^Stinger^
*/
int m_statserv(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
    struct Client *acptr;
    
    if (IsZombie(sptr) || exceed_services_usage(sptr) == YES)	/* check for services flood */
	  return -1;
	  
    if (parc < 2 || *parv[1] == '\0') {
        sendto_one(sptr, form_str(ERR_NOTEXTTOSEND), me.name, parv[0]);
        return -1;
    }
	  
    if (services_on && (acptr = find_server(ServicesServer)))
  	  sendto_one(acptr, ":%s PRIVMSG StatServ@%s :%s", 
  	  	parv[0], ServicesServer, parv[1]);
    else
      sendto_one(sptr, form_str(ERR_SERVICESDOWN), me.name, parv[0], "StatServ");					  
      return 0;					  
}


/*
** m_memoserv - Ported to PTlink6 with flood protection - Lamego
**      parv[0] = sender prefix
**      parv[1] = memoserv command
*/
int	m_memoserv(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
  {
    struct Client *acptr;
	
	if ( IsZombie(sptr) || exceed_services_usage(sptr) == YES )	/* check for services flood */
	  return -1;
	  
	if (parc < 2 || *parv[1] == '\0') 
	  {
        sendto_one(sptr, form_str(ERR_NOTEXTTOSEND), me.name, parv[0]);
        return -1;
  	  }
    if (services_on && (acptr = find_server(ServicesServer)))
  	  sendto_one(acptr, ":%s PRIVMSG MemoServ@%s :%s", 
  	  	parv[0], ServicesServer, parv[1]);			
    else
        sendto_one(sptr, form_str(ERR_SERVICESDOWN), me.name,
	  		parv[0], "MemoServ");					  
	return 0;					  
  }

/*
** m_newsserv - Ported to PTlink6 with flood protection - Lamego
**      parv[0] = sender prefix
**      parv[1] = newsserv command
*/
int	m_newsserv(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
  {
    struct Client *acptr;
	
	if ( IsZombie(sptr) || exceed_services_usage(sptr) == YES )	/* check for services flood */
	  return -1;
	  
	if (parc < 2 || *parv[1] == '\0') 
	  {
        sendto_one(sptr, form_str(ERR_NOTEXTTOSEND), me.name, parv[0]);
        return -1;
  	  }

    if (services_on && (acptr = find_server(ServicesServer)))
  	  sendto_one(acptr, ":%s PRIVMSG NewsServ@%s :%s", 
  	  	parv[0], ServicesServer, parv[1]);
    else
        sendto_one(sptr, form_str(ERR_SERVICESDOWN), me.name,
	  		parv[0], "NewsServ");					  
	return 0;					  
  }

/*
** m_operserv - Ported to PTlink6 with flood protection - Lamego
**      parv[0] = sender prefix
**      parv[1] = operserv command
*/
int	m_operserv(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
  {
    struct Client *acptr;

  if (!IsPrivileged(cptr))
    {    
      sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, parv[0]);
      return 0;
    }    

	if ( IsZombie(sptr) || exceed_services_usage(sptr) == YES )	/* check for services flood */
	  return -1;
	  
	if (parc < 2 || *parv[1] == '\0') 
	  {
        sendto_one(sptr, form_str(ERR_NOTEXTTOSEND), me.name, parv[0]);
        return -1;
  	  }
	  
    if (services_on && (acptr = find_server(ServicesServer)))
  	  sendto_one(acptr, ":%s PRIVMSG OperServ@%s :%s", 
  	  	parv[0], ServicesServer, parv[1]);
    else
        sendto_one(sptr, form_str(ERR_SERVICESDOWN), me.name,
	  		parv[0], "OperServ");					  
	return 0;					  
  }

/*
** m_botserv - Ported to PTlink6 with flood protection - Lamego
**      parv[0] = sender prefix
**      parv[1] = botserv command
*/
int m_botserv(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
  {
    struct Client *acptr;
	
	if ( IsZombie(sptr) || exceed_services_usage(sptr) == YES )	/* check for services flood */
	  return -1;
	  
	if (parc < 2 || *parv[1] == '\0') 
	  {
        sendto_one(sptr, form_str(ERR_NOTEXTTOSEND), me.name, parv[0]);
        return -1;
  	  }
		  
    if (services_on && (acptr = find_server(ServicesServer)))
  	  sendto_one(acptr, ":%s PRIVMSG BotServ@%s :%s", 
  	  	parv[0], ServicesServer, parv[1]);
    else
      sendto_one(sptr, form_str(ERR_SERVICESDOWN), me.name,
	  		parv[0], "BotServ");					  
	return 0;					  
  }
/*
** m_helpserv - Ported to PTlink6 with flood protection - ^Stinger^
** Although it is not really used, it is a part of the services (:
**      parv[0] = sender prefix
**      parv[1] = helpserv command
*/
int m_helpserv(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
    struct Client *acptr;
    
    if ( IsZombie(sptr) || exceed_services_usage(sptr) == YES )     /* check for services flood */ 
        return -1;
	    
    if (parc < 2 || *parv[1] == '\0')
    {
	sendto_one(sptr, form_str(ERR_NOTEXTTOSEND), me.name, parv[0]);
	return -1;
    }
							
    if (services_on && (acptr = find_server(ServicesServer)))
	sendto_one(acptr, ":%s PRIVMSG HelpServ@%s :%s", 
	 parv[0], ServicesServer, parv[1]);
    else
	sendto_one(sptr, form_str(ERR_SERVICESDOWN), me.name,
    	    parv[0], "HelpServ");
        return 0;
}																
/*
** m_identify - Automatic NickServ/ChanServ direction for the identify command
**      parv[0] = sender prefix
**      parv[1] = nick/chan
**      parv[2] = password
*/ 
int m_identify(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{

    struct Client *acptr;
	
	if ( IsZombie(sptr) || exceed_services_usage(sptr) == YES )	/* check for services flood */
	  return -1;
 
    if (parc < 2 || *parv[1] == '\0') 
	  {
        sendto_one(sptr, form_str(ERR_NOTEXTTOSEND), me.name, parv[0]);
        return -1;
  	  }
	  
    if (*parv[1]) 
	  {
            if (services_on && (acptr = find_server(ServicesServer)))
                sendto_one(acptr, ":%s PRIVMSG NickServ@%s :IDENTIFY %s", 
                	parv[0], ServicesServer, parv[1]);
            else
                sendto_one(sptr, form_str(ERR_SERVICESDOWN), me.name,
                           parv[0], "NickServ");
  	  }
	  
    return 0;
}      		


/*
** m_sanotice - send notice to sadmins - Lamego
**      parv[0] = sender prefix
**      parv[1] = message
*/  
int	m_sanotice(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
  {
	char *message = parc > 1 ? parv[1] : NULL;

    if (BadPtr(message))
  	  {
    	if (MyClient(sptr))
    	  sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
      		me.name, parv[0], "SANOTICE");
    	return 0;
    }

    if ( !IsService(sptr) && !IsSAdmin(sptr) )
  	  {
    	if(MyClient(sptr)) 
		  sendto_one(sptr, form_str(ERR_NOPRIVILEGES), 
      		me.name, parv[0]);
    	return 0;
	  } 
	  
  	if (strlen(message) > TOPICLEN)
  	  message[TOPICLEN] = (char) 0;
	
	sendto_ops_flags(UMODE_SADMIN,"SANOTICE from %s: %s", parv[0], message);
	
	sendto_serv_butone(IsServer(cptr) ? cptr : NULL, ":%s SANOTICE :%s",
  	  parv[0], message);
					     
   return 0;
}

/*
** m_svsjoin - Ported to PTlink6 with TS check -Lamego
**      parv[0] = sender prefix
**      parv[1] = nick 
**      parv[2] = TS 	(optional, validated if numeric)
**      parv[3] = channels list
*/
int	m_svsjoin(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  struct Client *acptr;
  time_t ts = 0;

  if (!IsServer(cptr) || !IsService(sptr)) 
	{	
	  if (IsServer(cptr))
		{ 
		  ts_warn("Got SVSJOIN from non-service: %s", 
			sptr->name);
		  sendto_one(cptr, ":%s WALLOPS :ignoring SVSJOIN from non-service %s",
			me.name, sptr->name);
		}
	  return 0;
	}


  if (parc < 3) /* missing arguments */
  {
    ts_warn("Invalid SVSJOIN (%s) from %s",
      (parc==2) ? parv[1]: "-", parv[0]);
     return 0;
  }
	
  if ( parc < 4 || !IsDigit(*parv[2]) )
	{
	  parv[3] = parv[2];
	  parv[2] = "0";
	  ++parc;
	}
	


  if ((acptr = find_person(parv[1], NULL)) && MyClient(acptr)) /* person found connected here */
	{
	
	  ts = atol(parv[2]);
	  if (ts && (ts != acptr->tsinfo))	/* This is not the person the mode was sent for */
  		{
		  sendto_ops_imodes(IMODE_DEBUG, 
	  		"Unmatched SVSPART tsinfo for %s", parv[1]); 
		  return 0;						/* just ignore it */
		}	    

	  parv[2]=parv[3];
 	  (void) m_join(acptr, acptr, 2, &parv[1]);
	}	
  else if (acptr && (acptr != cptr)) /* nick was found but is not our client */
	{
  	  if ( (acptr->from != cptr)) /* this should never happen */
		sendto_one(acptr, ":%s SVSJOIN %s %s :%s",
		  parv[0], parv[1], parv[2], parv[3] );
	}	  		
  return 0;
}

/*
** m_svspart - based on svsjoin - Lamego
**      parv[0] = sender prefix
**      parv[1] = nick 
**      parv[2] = TS 	(optional, validated if numeric)
**      parv[3] = channels list
**      parv[4] = reason
*/
int	m_svspart(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  struct Client *acptr;
  time_t ts = 0;

  if (!IsServer(cptr) || !IsService(sptr)) 
	{	
	  if (IsServer(cptr))
		{ 
		  ts_warn("Got SVSPART from non-service: %s", 
			sptr->name);
		  sendto_one(cptr, ":%s WALLOPS :ignoring SVSPART from non-service %s",
			me.name, sptr->name);
		}
	  return 0;
	}

  if (parc < 3) /* missing arguments */
  {	
    ts_warn("Invalid SVSPART (%s) from %s",
      (parc==2 ) ? parv[1]: "-", parv[0]);
    return 0;
  }
	
  if ( parc < 4 || !IsDigit(*parv[2]) )
	{
      if(parc>3) /* we can have a reason now -Lamego */
        parv[4]=parv[3];
        
	  parv[3] = parv[2];
	  parv[2] = "0";
	  ++parc;
	}
	


  if ((acptr = find_person(parv[1], NULL)) && MyClient(acptr)) /* person found connected here */
	{
	
	  ts = atol(parv[2]);
	  if (ts && (ts != acptr->tsinfo))	/* This is not the person the mode was sent for */
  		{
		  sendto_ops_imodes(IMODE_DEBUG, 
	  		"Unmatched SVSPART tsinfo for %s", parv[1]); 
		  return 0;						/* just ignore it */
		}	    

      if(parc>4)
        sendto_one(acptr,":%s NOTICE %s :Forced part from %s, reason: %s",
          parv[0], parv[1], parv[3], parv[4]);

	  parv[2]=parv[3];
        
 	  (void) m_part(acptr, acptr, 2, &parv[1]);
	}	
  else if (acptr && (acptr != cptr)) /* nick was found but is not our client */
	{
  	  if ( (acptr->from != cptr)) /* this should never happen */
        {
		  sendto_one(acptr, ":%s SVSPART %s %s %s :%s",
			parv[0], parv[1], parv[2], parv[3], 
            parc>4 ? parv[4] : "");
        }
	}	
  return 0;
}
  
/* The syntax was copied from Unreal IRCd to make other services coder's life 
easier */
/* 0000263: chghost command for remote hostname change */

/*
** m_chghost
**      parv[0] = sender prefix
**      parv[1] = nick 
**      parv[2] = hostname
*/
int	m_chghost(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  struct Client *acptr;
  time_t ts = 0;
  char *mename = me.name;
      
  if(!IsServer(cptr) || !IsService(sptr)) /* just ignore if no privilege */
    return 0;
  
  if ((parc < 3) || !*parv[2]) /* invalid parameters */
    return 0;
    
  if ((strlen(parv[2]) > HOSTLEN) || !valid_hostname(parv[2])) /* invalid */
    return 0;
    
  if ((acptr = find_person(parv[1], NULL))) /* person found */
  {
    if(acptr->user && acptr->user->vlink)
      mename = acptr->user->vlink->name;      
    strcpy(acptr->host, parv[2]);
    sendto_serv_butone(cptr, ":%s CHGHOST %s %s", parv[0], parv[1], parv[2]);
    if (MyClient(acptr)) /* if its a local user */
      sendto_one(acptr, ":%s NOTICE %s :*** New hostname: \2%s\2", 
        mename, acptr->name, acptr->host);    
  }  
}  
