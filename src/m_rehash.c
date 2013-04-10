/************************************************************************
 *   IRC - Internet Relay Chat, src/m_rehash.c
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
 *   $Id: m_rehash.c,v 1.4 2005/08/27 16:23:50 jpinto Exp $
 */
#include "m_commands.h"
#include "client.h"
#include "common.h"
#include "irc_string.h"
#include "ircd.h"
#include "list.h"
#include "m_gline.h"
#include "numeric.h"
#include "res.h"
#include "s_conf.h"
#include "s_log.h"
#include "help.h"
#include "dconf_vars.h"
#include "dconf.h"
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
 * m_rehash - REHASH message handler
 * parv[1]  - type of rehash (optional)
 */
int m_rehash(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  int errors;
  int found = NO;
  
  if (!MyClient(sptr) || !IsAnOper(sptr))
    {
      sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, parv[0]);
      return 0;
    }
            	    
  if ( !IsOperRehash(sptr) )
    {
      sendto_one(sptr,":%s NOTICE %s :You have no H flag", me.name, parv[0]);
      return 0;
    }

  if (parc > 1) {
	  if (irccmp(parv[1],"HELPSYS") == 0)
        {
          sendto_one(sptr, form_str(RPL_REHASHING), me.name, parv[0], "HELPSYS");
          sendto_ops("%s is rehashing HELPSYS", parv[0]);

          /* Reread help system */
          help_free(&user_help);
          help_free(&oper_help);
          help_free(&admin_help);
          
          help_load(UserHelpFile , &user_help);	
          help_load(OperHelpFile , &oper_help);	
          help_load(AdminHelpFile , &admin_help);	

	  	  found = YES;
        }
      else if (irccmp(parv[1],"DCONF") == 0)
		{
	      sendto_one(sptr, form_str(RPL_REHASHING), me.name, parv[0], "DCONF");

          sendto_ops("%s is rehashing DCONF", parv[0]);
		  if( (errors = dconf_read("main.dconf", 2)) )
			{
      		  sendto_one(sptr, ":%s NOTICE %s :*** (%i) errors found",
          		  me.name,parv[0], errors);		  
			}				 
	  	  found = YES;
		}		
#ifndef NORES
	  else if (irccmp(parv[1],"DNS") == 0)
        {
          sendto_one(sptr, form_str(RPL_REHASHING), me.name, parv[0], "DNS");
		  
          sendto_ops("%s is rehashing DNS", parv[0]);
				 
          restart_resolver();   /* re-read /etc/resolv.conf AGAIN?
                                   and close/re-open res socket */

          found = YES;
        }
#endif
      else if(irccmp(parv[1],"TKLINES") == 0)
        {
		  /* see the note for glines and such .. -gnp */
		  if (!SetOperK(sptr))
		    {
			  sendto_one(sptr,":%s NOTICE %s :You have no K flag",me.name,parv[0]);
			  return 0;
		    }
          sendto_one(sptr, form_str(RPL_REHASHING), me.name, parv[0], "temp klines");
          flush_temp_klines();
          sendto_ops("%s is clearing temp klines", parv[0]);
          found = YES;
        }
      else if(irccmp(parv[1],"MOTD") == 0)
        {
          sendto_ops("%s is forcing re-reading of MOTD file",parv[0]);
          ReadMessageFile( &ConfigFileEntry.motd );
          ReadMessageFile( &ConfigFileEntry.wmotd );		  
          found = YES;
        }
      else if(irccmp(parv[1],"OMOTD") == 0)
        {
          sendto_ops("%s is forcing re-reading of OPER MOTD file",parv[0]);
          ReadMessageFile( &ConfigFileEntry.opermotd );
          found = YES;
        }
#if TOFIX        
      else if(irccmp(parv[1],"HELP") == 0)
        {
          sendto_ops("%s is forcing re-reading of oper help file",parv[0]);
          ReadMessageFile( &ConfigFileEntry.helpfile );
          found = YES;
        }
#endif
      else if(irccmp(parv[1],"DUMP") == 0)
        {
          sendto_ops("%s is dumping conf file",parv[0]);
          rehash_dump(sptr);
          found = YES;
        }
      else if(irccmp(parv[1],"DLINES") == 0)
        {
          sendto_one(sptr, form_str(RPL_REHASHING), me.name, parv[0], "kline.conf");
					 
          /* this does a full rehash right now, so report it as such */
          sendto_ops("%s is rehashing dlines from server config file", parv[0]);
					 
          irclog(L_NOTICE, "REHASH From %s", get_client_name(sptr, HIDE_IP));
          dline_in_progress = 1;
          return rehash(cptr, sptr, 0);
        }
      if(found)
        {
          irclog(L_NOTICE, "REHASH %s From %s", parv[1], get_client_name(sptr, HIDE_IP));
          return 0;
        }
      else
        {
#define OUT "rehash one of :DCONF TKLINES HELPSYS MOTD OMOTD DUMP DLINES"
          sendto_one(sptr, ":%s NOTICE %s :%s", me.name, sptr->name, OUT);
          return(0);
        }
    }
  else
    {
      sendto_one(sptr, form_str(RPL_REHASHING), me.name, parv[0],
                 "ircd.conf");
				 
      sendto_ops("%s is rehashing server config file", parv[0]);
      irclog(L_NOTICE, "REHASH From %s", get_client_name(sptr, SHOW_IP));
      return rehash(cptr, sptr, 0);
    }
  return 0; /* shouldn't ever get here */
}

