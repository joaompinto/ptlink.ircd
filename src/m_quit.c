/************************************************************************
 *   IRC - Internet Relay Chat, src/m_quit.c
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
 *   $Id: m_quit.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $
 */ 
#include "m_commands.h"
#include "client.h"
#include "ircd.h"
#include "sprintf_irc.h"
#include "numeric.h"
#include "s_serv.h"
#include "send.h"
#include "channel.h"	/* spam_words */
#include "svline.h" 	/* is_svlined() */
#include "irc_string.h"	/* strip_control_codes_lower() */
#include "dconf_vars.h"


#include <string.h>		/* for strlen() */
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
** m_quit
**      parv[0] = sender prefix
**      parv[1] = comment
*/
int     m_quit(struct Client *cptr,
               struct Client *sptr,
               int parc,
               char *parv[])
{
  char *comment = (parc > 1 && parv[1]) ? parv[1] : cptr->name;
  static char comment2[TOPICLEN];
  char *stripedmsg;
  
  sptr->flags |= FLAGS_NORMALEX;
  
  if (strlen(comment) > (size_t) TOPICLEN)
    comment[TOPICLEN] = '\0';
	
  if (MyClient(sptr))
	{	
	  if (NoQuitMsg)
		comment = NoQuitMsg;
	  else if(ZombieQuitMsg && IsZombie(sptr))
		comment = ZombieQuitMsg; // We don't want zombie's to 
					 // use a normal quit (:
	  else
		{	  
		  if( AntiSpamExitMsg &&
    		(sptr->firsttime + AntiSpamExitTime) > CurrentTime)
  			  comment = AntiSpamExitMsg;
		  else if(NoColorsQuitMsg && comment && (strchr(comment, 3) || strchr(comment, 27)))
			  comment = NoColorsQuitMsg;
		  else if(NoSpamExitMsg && comment && is_spam(comment))
			  comment = NoSpamExitMsg;			
		  else if(QuitPrefix)
			{
                           stripedmsg = strip_control_codes_lower(comment);
      		           if(!IsOper(sptr) && is_msgvlined(stripedmsg))
        		     {
				comment = NoSpamExitMsg;
        		     }
			
			  if (comment && strlen(comment) > (size_t) TOPICLEN - strlen(QuitPrefix))
  				comment[TOPICLEN - strlen(QuitPrefix)] = '\0';		

				ircsprintf(comment2,"%s%s", QuitPrefix, (comment) ? comment : "");
				comment = comment2;				
		  	}
		}
	}
	
  return IsServer(sptr) ? 0 : exit_client(cptr, sptr, sptr, comment);
}
