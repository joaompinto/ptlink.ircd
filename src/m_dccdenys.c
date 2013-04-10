/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2005    *
 *                http://software.pt-link.net                    *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 *   $Id: 
  
 File: m_dccdenys.c
 Desc: list online DCC Denied people (+v guys)
 Author: openglx@StarByte.net

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
** m_dccdenys() by openglx
**
** m_ircops() By Claudio
** Rewritten by HAMLET
**   Lists online IRCOps
**
*/
int m_dccdenys(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Client *acptr;
  char buf[BUFSIZE];
  int dccdeny = 0;

  if (!IsPrivileged(cptr))
    {
      sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, parv[0]);
      return 0;
    }
	
  strcpy(buf, "========================================================================================");
  sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
  strcpy(buf, "\2Nick                           Hostname                           Server\2");
  sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
  strcpy(buf, "----------------------------------------------------------------------------------------");
  sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
  for (acptr = GlobalClientList; acptr; acptr = acptr->next)
	{
  	  if (!IsService(acptr) && !IsStealth(acptr) &&  IsNoDCC(acptr)) {
    
          if (!acptr->user) continue;

/* vlinks on /IRCops
* code by openglx
* idea to this by Midnight_Commander
*/
    if (acptr->user && acptr->user->vlink)
      {
        ircsprintf(buf, "\2%-29s\2  %s@%s %s", acptr->name ? acptr->name : "<unknown>",
                              acptr->username, acptr->realhost, acptr->user->vlink->name);
      }
    else
      {
      ircsprintf(buf, "\2%-29s\2  %-23s %s@%s", acptr->name ? acptr->name : "<unknown>",
                            acptr->username, acptr->realhost, acptr->user->server);
      }
      
/* end of the vlink support code */
                                                                                                 
          sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
          sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], "-");
          dccdeny++;
    	}
    }
 
  ircsprintf(buf, "Total: \2%d\2 DCC Den%s connected", dccdeny,
                        dccdeny > 1 ? "ies" : "y");
  sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
  strcpy(buf, "========================================================================================");
  sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
  sendto_one(sptr, form_str(RPL_ENDOFLISTS), me.name, parv[0], "DCCDENYS");
  return 0;
}

