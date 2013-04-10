/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2005    *
 *                http://software.pt-link.net                    *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 *   $Id: m_setname.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $
 
 File: m_setname.c
 Desc: allow users to change it's real name after being connected
 Author: openglx@StarByte.net
*/

#include <string.h>
  
#include "m_commands.h"
#include "client.h"
#include "ircd.h"
#include "numeric.h"
#include "s_log.h"
#include "s_serv.h"
#include "send.h"
#include "whowas.h"
#include "irc_string.h"
#include "spoof.h"
#include "s_user.h"
#include "crypt.h"
#include "hash.h"
#include "s_conf.h"
#include "dconf_vars.h"

/* m_setname - for SETNAME command
 * allow users to change it's real name
 *  -- openglx, on a boring 25/12/2003
 */
int m_setname(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{

  struct Client *acptr = sptr;
  char *newname = parv[1];
  char *mename = me.name;
  
  if(acptr->user && acptr->user->vlink)
    mename = acptr->user->vlink->name;

  if (!AllowSetNameToEveryone && !IsPrivileged(sptr))
    {
      sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, parv[0]);
      return 0;
    }
  
  if (parc<2 || EmptyString(parv[1]))
    {
      if(MyClient(sptr))
        sendto_one(sptr, ":%s NOTICE %s :*** Syntax: /SETNAME <real name>", mename, parv[0]);
      return 0;
    }
  
  if (strlen(newname) > REALLEN) /* just to be sure */
    {
      if(MyClient(sptr))
        sendto_one(sptr, ":%s NOTICE %s :*** SETNAME: select a real name under %d chars", 
                   mename, acptr->name, REALLEN);
      return 0;
    }   
  
  strcpy(acptr->info, newname);
  
  sendto_serv_butone(cptr, ":%s SETNAME :%s", parv[0], newname);
  
  if (MyClient(acptr)) 
     sendto_one(acptr, ":%s NOTICE %s :*** New real name set: \2%s\2", mename, acptr->name,
                acptr->info);

  return 0; /* I wonder why shouldn't this be 1 */
                              
}

