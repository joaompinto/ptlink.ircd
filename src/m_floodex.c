/************************************************************************
 *   IRC - Internet Relay Chat, src/m_floodex.c
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
 *   $Id: m_floodex.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $
 */
#include "m_commands.h"
#include "client.h"
#include "ircd.h"
#include "numeric.h"
#include "s_conf.h"
#include "s_serv.h"
#include "send.h"

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
 * m_floodex - FLOODEX command handler
 *      parv[0] = sender prefix
 *      parv[1] = target nick
 */
int m_floodex(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
	struct Client *acptr; /* the target user. */

	/* trust servers and opers. */
	if( !IsServer(cptr) && !IsOper(cptr) )
	{
		sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, parv[0]);
		return 0;
	}
	if( parc < 2 || *parv[1] == '\0' )
	{
		sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS), me.name, parv[0], "FLOODEX");
		return 0;
	}

	if( !(acptr = find_person(parv[1], 0)) )
    {
		if( !IsServer(cptr) )
			sendto_one(sptr, form_str(ERR_NOSUCHNICK), me.name, parv[0], parv[1]);
		return 0;
	}

	if( IsFloodEx(acptr) )
	{
		ClearFloodEx(acptr);
		sendto_ops("%s (%s@%s) removed flood exemption on %s (%s@%s).",
			sptr->name, sptr->username, sptr->realhost, acptr->name, acptr->username, acptr->realhost);

		if( MyClient(acptr) )
		{
			sendto_one(acptr, ":%s MODE %s :-f", acptr->name, acptr->name);
			sendto_one(acptr, ":%s NOTICE %s :You are no longer exempted from flood checks.", me.name, acptr->name);
		}
	}
	else
	{
		SetFloodEx(acptr);
		sendto_ops("%s (%s@%s) set flood exemption on %s (%s@%s).",
			sptr->name, sptr->username, sptr->realhost, acptr->name, acptr->username, acptr->realhost);

		if( MyClient(acptr) )
		{
			sendto_one(acptr, ":%s MODE %s :+f", acptr->name, acptr->name);
			sendto_one(acptr, ":%s NOTICE %s :You are now exempted from flood checks.", me.name, acptr->name);
		}
	}

	sendto_serv_butone(cptr, ":%s FLOODEX %s", parv[0], acptr->name);

	return 0;
}

