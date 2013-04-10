/************************************************************************
 *   IRC - Internet Relay Chat, src/m_links.c
 *   Copyright (C) 1990 Jarkko Oikarinen and
 *                      University of Oulu, Computing Center
 *
 *   See file AUTHORS in IRC package for additional names of
 *   the programmers. 
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   $Id: m_links.c,v 1.4 2005/08/27 16:23:50 jpinto Exp $
 */
#include "m_commands.h"
#include "client.h"
#include "irc_string.h"
#include "ircd.h"
#include "numeric.h"
#include "s_serv.h"
#include "s_conf.h"
#include "send.h"
#include "vlinks.h"
#include "dconf_vars.h" /* for HideServicesServer */

#include <assert.h>
#include <string.h>

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
 * m_links - LINKS message handler
 *      parv[0] = sender prefix
 *      parv[1] = servername mask
 * or
 *      parv[0] = sender prefix
 *      parv[1] = server to query 
 *      parv[2] = servername mask
 */
int m_links(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  const char*    mask = "";
  struct Client* acptr;
  char           clean_mask[2 * HOSTLEN + 4];
  char*          p;
  char*          s;
  int            bogus_server = 0;
  static time_t  last_used = 0L;
  char  *mename = me.name;
  
  if(sptr->user && sptr->user->vlink)
    mename = sptr->user->vlink->name;

  if (parc > 2)
    {
      if (hunt_server(cptr, sptr, ":%s LINKS %s :%s", 1, parc, parv)
          != HUNTED_ISME)
        return 0;
      mask = parv[2];
    }
  else if (parc == 2)
    mask = parv[1];

  assert(0 != mask);

  if(!IsAnOper(sptr))
    {
      /* reject non local requests */
      if(!MyClient(sptr))
        return 0;

      if((last_used + PACE_WAIT) > CurrentTime)
        {
          /* safe enough to give this on a local connect only */
          sendto_one(sptr,form_str(RPL_LOAD2HI),mename,parv[0]);
          return 0;
        }
      else
        {
          last_used = CurrentTime;
        }
    }

  /*
   * *sigh* Before the kiddies find this new and exciting way of 
   * annoying opers, lets clean up what is sent to all opers
   * -Dianora
   */

  s = (char *) mask;
  while (*s)
    {
      if (!IsServChar(*s)) {
        bogus_server = 1;
        break;
      }
      s++;
    }

  if (bogus_server)
    {
      sendto_ops_imodes(IMODE_SPY,
                         "BOGUS LINKS '%s' requested by %s (%s@%s) [%s]",
                         clean_string(clean_mask, (const unsigned char *) mask, 2 * HOSTLEN),
                         sptr->name, sptr->username,
                         sptr->host, sptr->user->server);
      return 0;
    }

  if (*mask)       /* only necessary if there is a mask */
    mask = collapse(clean_string(clean_mask, (const unsigned char *) mask, 2 * HOSTLEN));

  if (MyConnect(sptr))
    sendto_ops_imodes(IMODE_SPY,
                       "LINKS '%s' requested by %s (%s@%s) [%s]",
                       mask, sptr->name, sptr->username,
                       sptr->host, sptr->user->server);
  
  /* DisableLinksForUsers */
  if (DisableLinksForUsers && !IsAnOper(sptr))
  {
  	sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, parv[0]);
  	return 0;
  }

  for (acptr = GlobalClientList; acptr; acptr = acptr->next) 
    {
      if (!IsServer(acptr) && !IsMe(acptr))
        continue;
      if (*mask && !match(mask, acptr->name))
        continue;
      if(IsAnOper(sptr))
         sendto_one(sptr, form_str(RPL_LINKS),
                    mename, parv[0], acptr->name, acptr->serv->up,
                    acptr->hopcount, (acptr->info[0] ? acptr->info :
                                      "(Unknown Location)"));
      else
        {
          if(acptr->info[0])
            {
              /* kludge, you didn't see this nor am I going to admit
               * that I coded this.
               */
              p = strchr(acptr->info,']');
              if(p)
                p += 2; /* skip the nasty [IP] part */
              else
                p = acptr->info;
            }
          else
            p = "(Unknown Location)";

#ifdef SERVERHIDE
          sendto_one(sptr, form_str(RPL_LINKS),
                    mename, parv[0], acptr->name, mename,
                     0, p);
#else
          if ( !IsService(acptr) || IsAnOper(sptr) || !HideServicesServer)
          sendto_one(sptr, form_str(RPL_LINKS),
                    mename, parv[0], acptr->name, acptr->serv->up,
                    acptr->hopcount, p);
#endif
        }

    }
  dump_vlinks(parv[0], sptr);
  
  sendto_one(sptr, form_str(RPL_ENDOFLINKS), mename, parv[0],
             EmptyString(mask) ? "*" : mask);
  return 0;
}

