/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2005    *
 *                http://software.pt-link.net                    *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 
 File: m_svsinfo.c
 Desc: services info
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
#include "sxline.h"
#include "svline.h"
#include "sqline.h"
#include "vlinks.h"
#include "m_svsinfo.h"

#include <stdlib.h> 
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

time_t my_svsinfo_ts = 0;
char my_svsinfo_ts_S[32];

/*
** m_svsinfo - services data integrity
**      parv[0] = sender prefix
**      parv[1] = ts
**	parv[2] = max users
**	parv[3] = secure mode (0 = off, -1 =on) 
*/
int	m_svsinfo(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
  {
	time_t	remote_ts;
	
	if (parc<3 || !IsServer(sptr))
	  return 0;
	  
	remote_ts = atol(parv[1]);  
	
	if(IsService(sptr) || (my_svsinfo_ts < remote_ts)) /* we need to clear all sxlines */
	  {
			  
		sendto_ops("Clearing current services data on %s synchronization",
			IsService(sptr) ? "services" : "server");
		  
		clear_sxlines();
		clear_svlines();
		clear_sqlines();
   	clear_vlinks();
		
		my_svsinfo_ts = remote_ts;	/* update our ts */
		Count.max_tot = atol(parv[2]); /* update our max seen clients */
		if(parc>3)
		  secure_mode = atoi(parv[3]);
		sendto_serv_butone(cptr, ":%s SVSINFO %s %s %i",
		  parv[0], parv[1], parv[2], secure_mode);
		ircsprintf(my_svsinfo_ts_S, "%d" , my_svsinfo_ts);
	  }
	return 0;
  }
