/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2005    *
 *                http://software.pt-link.net                    *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 *   $Id:
  
 File: m_helpers.c
 Desc: list online helpers (+h guys)
 Author: openglx@StarByte.net

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
** m_helpers() by openglx
**
** m_ircops() By Claudio
** Rewritten by HAMLET
**   Lists online IRCOps
**
*/
int m_helpers(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Client *acptr;
  char buf[BUFSIZE];
  int helpers = 0;

  strcpy(buf, "========================================================================================");
  sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
  strcpy(buf, "\2Nick                           \2");
  sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
  strcpy(buf, "----------------------------------------------------------------------------------------");
  sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
  for (acptr = GlobalClientList; acptr; acptr = acptr->next)
	{
  	  if (!IsService(acptr) && !IsStealth(acptr) && IsHelper(acptr)) {
    
          if (!acptr->user) continue;
	  
        ircsprintf(buf, "\2%-29s\2  %-6s", acptr->name ? acptr->name : "<unknown>", 
                              acptr->user->away ? "(AWAY)" : "");
                                                                                                 
          sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
          sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], "-");
          helpers++;
    	}
    }
 
  ircsprintf(buf, "Total: \2%d\2 Helper%s connected", helpers,
                        helpers > 1 ? "s" : "");
  sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
  strcpy(buf, "========================================================================================");
  sendto_one(sptr, form_str(RPL_LISTS), me.name, parv[0], buf);
  sendto_one(sptr, form_str(RPL_ENDOFLISTS), me.name, parv[0], "HELPERS");
  return 0;
}

