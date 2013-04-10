/************************************************************************
 *   IRC - Internet Relay Chat, src/m_svinfo.c
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
 *   $Id: m_svinfo.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $
 */
#include "m_commands.h"
#include "client.h"
#include "common.h"     /* TRUE bleah */
#include "irc_string.h"
#include "ircd.h"
#include "numeric.h"
#include "send.h"
#include "struct.h"
#include "s_conf.h"
#include "dconf_vars.h"

#include <assert.h>
#include <time.h>
#include <stdlib.h>

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
 * m_svinfo - SVINFO message handler
 *      parv[0] = sender prefix
 *      parv[1] = TS_CURRENT for the server
 *      parv[2] = TS_MIN for the server
 *      parv[3] = clock ts 
 */
int m_svinfo(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  aConfItem *aconf;
  time_t remote_ts = 0;
  struct Client *acptr;
  
  if (MyConnect(sptr) && IsUnknown(sptr))
    return exit_client(sptr, sptr, sptr, "Need SERVER before SVINFO");

  if (!IsServer(sptr) || !MyConnect(sptr) || parc < 3)
    return 0;
    
  if (TS_CURRENT < atoi(parv[2]) || atoi(parv[1]) < TS_MIN)
    {
      /*
       * a server with the wrong TS version connected; since we're
       * TS_ONLY we can't fall back to the non-TS protocol so
       * we drop the link  -orabidoo
       */
#ifdef HIDE_SERVERS_IPS
      sendto_realops("Link %s dropped, incompatible TS protocol version (%s,%s)",
                 get_client_name(sptr,MASK_IP), parv[1], parv[2]);
#else
      sendto_realops("Link %s dropped, incompatible TS protocol version (%s,%s)",
                 get_client_name(sptr, TRUE), parv[1], parv[2]);
#endif     
      return exit_client(sptr, sptr, sptr, "Incompatible TS version");
    }

  if(parc>3)
    {
      remote_ts = atol(parv[3]);
      if (UseIRCNTP && (remote_ts > 0)) 
        {
          if(IsService(cptr) || 
          ((aconf = find_conf_host(cptr->confs, sptr->name, CONF_HUB)) 
          	&& !(acptr = find_server(ServicesServer))))
            {
              ircntp_offset = remote_ts - time(NULL);
              sendto_realops("Adjusting IRCNTP offset to %d",
                ircntp_offset);
            }
        }
    }
    
  return 0;
}

