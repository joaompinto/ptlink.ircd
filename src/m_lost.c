/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2005    *
 *                http://software.pt-link.net                    *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 *   $Id: m_lost.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $
  
 File: m_lost.c
 Desc: list lost (not on any channel) users
 Author: Lamego@PTlink.net

*/

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
#include "m_silence.h"
#include "list.h"
#include "irc_string.h"
#include "s_conf.h"
#include "dconf_vars.h"


/*
** m_lost() 
** parv[0] = sender
**
*/
int m_lost(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Client *acptr;
  char buf[BUFSIZE];
  int lost = 0;

  if (!IsPrivileged(cptr))
    {
      sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, parv[0]);
      return 0;
    }
	
  strcpy(buf, "========================================================================================");
  sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
  strcpy(buf, "\2User\2");
  sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
  strcpy(buf, "----------------------------------------------------------------------------------------");
  sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
  for (acptr = GlobalClientList; acptr; acptr = acptr->next)
    {
      if (IsService(acptr) || IsStealth(acptr) ||  IsAnOper(acptr) ||
         IsIdentified(acptr))
        continue;
        
      if (!acptr->user || (acptr->user->channel != NULL)) 
        continue;

      ircsprintf(buf, "\2%-29s\2  %-23s %s", acptr->name ? acptr->name : "<unknown>",
        acptr->user->away ? "(AWAY)" : "", acptr->user->server);
                                                                                                 
      sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
      
      lost++;
    }
 
  strcpy(buf, "========================================================================================");
  sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
  ircsprintf(buf, "Total: \2%d\2 Lost Users", lost);
  sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
  sendto_one(sptr, form_str(RPL_ENDOFLISTS), me.name, parv[0], "LOST.");
  return 0;
}

