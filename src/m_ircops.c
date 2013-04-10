/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2005    *
 *                http://software.pt-link.net                    *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 *   $Id: m_ircops.c,v 1.5 2005/08/27 16:23:50 jpinto Exp $
  
 File: m_ircops.c
 Desc: list online ircops
 Author: Lamego@PTlink.net

 Vlink support to /IRCops added by openglx (openglx@BrasNerd.com.br)  
*/

#include "m_commands.h"
#include "client.h"
#include "channel.h"
#include "hash.h"
#include "struct.h"
#include "ircd.h"
#include "numeric.h"
#include "s_serv.h"
#include "send.h"
#include "m_silence.h"
#include "list.h"
#include "irc_string.h"
#include "s_conf.h"
#include "dconf_vars.h"

#include <string.h>     /* for strchr() */

/*
** m_ircops() By Claudio
** Rewritten by HAMLET
**   Lists online IRCOps
**
*/
int m_ircops(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Client *acptr;
  char *status;
  char buf[BUFSIZE];
  int locals = 0, globals = 0;

  if (!IRCopsForAll && !IsPrivileged(cptr))
    {
      sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, parv[0]);
      return 0;
    }
	
  strcpy(buf, "========================================================================================");
  sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
  strcpy(buf, "\2Nick                           Status                           Server\2");
  sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
  strcpy(buf, "----------------------------------------------------------------------------------------");
  sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
  for (acptr = GlobalClientList; acptr; acptr = acptr->next)
	{
  	  if (!IsService(acptr) && !IsStealth(acptr) &&  IsAnOper(acptr)
  	     && (!IsHideOper(acptr) || IsAnOper(sptr)) ) {
    
          if (!acptr->user) continue;

	  if (IsTechAdmin(acptr)) 
		status = "Technical Administrator";
	  else if (IsNetAdmin(acptr)) 
		status = "Network Administrator";
	  else if (IsSAdmin(acptr)) 
		status = "Services Administrator";
	  else if (IsAdmin(acptr)) 
		status = "Server Administrator";		
	  else if(IsOper(acptr))
		status = "Global IRC Operator"; 
	  else
		status = "Local IRC Operator";  	
	  

/* vlinks on /IRCops
* code by openglx
* idea to this by Midnight_Commander
*/
    if (acptr->user && acptr->user->vlink)
      {
        ircsprintf(buf, "\2%-29s\2  %-23s  %-6s  %s", acptr->name ? acptr->name : "<unknown>", status,
                              acptr->user->away ? "(AWAY)" : "", acptr->user->vlink->name);
      }
    else
      {
      ircsprintf(buf, "\2%-29s\2  %-23s  %-6s  %s", acptr->name ? acptr->name : "<unknown>", status,
                            acptr->user->away ? "(AWAY)" : "", acptr->user->server);
      }
      
/* end of the vlink support code */
                                                                                                 
          sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
          sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], "-");
          if (IsOper(acptr)) globals++;
          else locals++;
    	}
    }
 
  ircsprintf(buf, "Total: \2%d\2 IRCOp%s connected - \2%d\2 Globa%s, \2%d\2 Loca%s", globals+locals,
                        (globals+locals) > 1 ? "s" : "",
                        globals, globals > 1 ? "ls" : "l", locals, locals > 1 ? "ls" : "l");
  sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
  strcpy(buf, "========================================================================================");
  sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
  sendto_one(sptr, form_str(RPL_ENDOFLISTS), me.name, parv[0], "IRCOPS");
  return 0;
}

