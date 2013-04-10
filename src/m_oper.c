/************************************************************************
 *   IRC - Internet Relay Chat, src/m_oper.c
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
 *   $Id: m_oper.c,v 1.7 2005/08/27 16:23:50 jpinto Exp $
 */

#include "m_commands.h"
#include "client.h"
#include "fdlist.h"
#include "irc_string.h"
#include "ircd.h"
#include "numeric.h"
#include "s_conf.h"
#include "s_log.h"
#include "s_user.h"
#include "send.h"
#include "struct.h"
#include "spoof.h"
#include "dconf_vars.h"
#include "crypt.h"


#include <fcntl.h>
#include <unistd.h>
#include <string.h> /* strncpy */

#ifndef OWN_CRYPT
#if defined(CRYPT_OPER_PASSWORD)
#ifdef HAVE_CRYPT_H
#include <crypt.h>
extern        char *crypt();
#endif
#endif
#endif

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
** m_oper
**      parv[0] = sender prefix
**      parv[1] = oper name
**      parv[2] = oper password
*/
int m_oper(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  struct ConfItem *aconf;
  char  *name, *password, *encr;
  static int oper_fail_count = 0;
  struct Client *acptr;  
  char *operprivs;
  static char buf[BUFSIZE];
  char newmask[HOSTLEN+USERLEN+2];

  name = parc > 1 ? parv[1] : (char *)NULL;
  password = parc > 2 ? parv[2] : (char *)NULL;

  if(OnlyRegisteredOper && !IsIdentified(cptr) && 
	(acptr = find_server(ServicesServer)))
	{
		sendto_one(sptr, ":%s NOTICE %s :You need a registered and identified nick to get oper!", me.name, parv[0]);
		return 0;
	}

  if(IsZombie(cptr) || oper_fail_count > 3)
	{
		  
	  if(MyClient(cptr))
		SetZombie(cptr);
				
  	  sendto_one(sptr, form_str(ERR_NOOPERHOST), me.name, parv[0]);
	  
	  if(oper_fail_count > 3)
	    oper_fail_count = 0;
	  
	  return 0;
	}
	
  if (!IsServer(cptr) && ( EmptyString(name) || EmptyString(password)))
    {
      sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
                 me.name, parv[0], "OPER");
      return 0;
    }
        
  /* if message arrived from server, trust it, and set to oper */
  
  if ((IsServer(cptr) || IsMe(cptr)) && !IsOper(sptr))
    {
      sptr->flags |= UMODE_OPER;
      Count.oper++;
      sendto_serv_butone(cptr, ":%s MODE %s :+o", parv[0], parv[0]);
      if (IsMe(cptr))
        sendto_one(sptr, form_str(RPL_YOUREOPER),
                   me.name, parv[0]);
      return 0;
    }
  else if (IsAnOper(sptr))
    {
      if (MyConnect(sptr))
        {
          sendto_one(sptr, form_str(RPL_YOUREOPER),
                     me.name, parv[0]);
          SendMessageFile(sptr, &ConfigFileEntry.opermotd);
          if(sptr->user) /* reset idle time */
            {
              sptr->user->last = CurrentTime;
              remove_autoaway(sptr);
            }
        }
      return 0;
    }
  if (!(aconf = find_conf_exact(name, sptr->username, sptr->realhost,
                                CONF_OPS)) &&
      !(aconf = find_conf_exact(name, sptr->username, sptr->host,
                                CONF_OPS)) &&
      !(aconf = find_conf_exact(name, sptr->username,
                                inetntoa((char *)&cptr->ip), CONF_OPS)))
    {
      sendto_one(sptr, form_str(ERR_NOOPERHOST), me.name, parv[0]);
#if defined(FAILED_OPER_NOTICE) && defined(SHOW_FAILED_OPER_ID)
#ifdef SHOW_FAILED_OPER_PASSWD
      sendto_realops("Failed OPER attempt [%s(%s)] - identity mismatch: %s [%s@%s]",
        name, password, sptr->name, sptr->username, sptr->realhost);
#else
      sendto_realops("Failed OPER attempt [%s] - host mismatch by %s (%s@%s)",
        name, parv[0], sptr->username, sptr->realhost);
    irclog(L_NOTICE, "Failed OPER attempt [%s] - host mismatch by %s (%s@%s)",
                     name, parv[0], sptr->username, sptr->realhost);
#endif /* SHOW_FAILED_OPER_PASSWD */
#endif /* FAILED_OPER_NOTICE && SHOW_FAILED_OPER_ID */
	  if(oper_fail_count<10) 
		oper_fail_count++;
		
      return 0;
    }
#ifdef CRYPT_OPER_PASSWORD
  /* use first two chars of the password they send in as salt */

  /* passwd may be NULL pointer. Head it off at the pass... */
  if (password && *aconf->passwd)
#ifdef OWN_CRYPT
    encr = crypt_des(password, aconf->passwd);
#else
    encr = crypt(password, aconf->passwd);
#endif
  else
    encr = "";
#else
  encr = password;
#endif  /* CRYPT_OPER_PASSWORD */

  if ((aconf->status & CONF_OPS) &&
      0 == strcmp(encr, aconf->passwd) && !attach_conf(sptr, aconf))
    {
      int old = (sptr->umodes & ALL_UMODES);

      if(sptr->user) /* reset idle time */
        {
          sptr->user->last = CurrentTime;
          remove_autoaway(sptr);
        }
      if (aconf->status == CONF_LOCOP)      
        SetLocOp(sptr);
      else
        SetOper(sptr); 

      if((int)aconf->hold)
        {
          sptr->umodes |= ((int)aconf->hold & ALL_UMODES); 
          sendto_one(sptr, ":%s NOTICE %s :*** Oper flags set from conf",
            me.name,parv[0]);
        }
      else
        {
           if (aconf->status == CONF_LOCOP)
             sptr->umodes |= LOCOP_UMODES;
           else
             sptr->umodes |= OPER_UMODES;
        }
        
      if(aconf->ip)
        sptr->imodes |= aconf->ip;
      else
        sptr->imodes = OPER_DEFAULT_IMODES;
        
      sendto_one(sptr, ":%s NOTICE %s :*** Information modes set from conf: %s",
        me.name, parv[0], imode_change(0,  sptr->imodes));

      Count.oper++;
      oper_fail_count = 0;	/* reset failed oper count */

      SetElined(cptr);
      SetExemptGline(cptr);
      
      /* LINKLIST */  
      /* add to oper link list -Dianora */
      cptr->next_oper_client = oper_cptr_list;
      oper_cptr_list = cptr;

      if(cptr->confs)
        {
          struct ConfItem *paconf;
          paconf = cptr->confs->value.aconf;
          operprivs = oper_privs_as_string(cptr,paconf->port);          
	  if( IsConfAdmin(aconf) )
		SetAdmin(cptr);
        }
      else
        operprivs = "";

      fdlist_add(sptr->fd, FDL_OPER | FDL_BUSY);
	  if( IsAdmin(sptr) )
	      	sendto_ops("%s (%s@%s) is now a Server Administrator", parv[0],
                 sptr->username, sptr->realhost);
	  else if(IsTechAdmin(sptr))
		sendto_ops("%s (%s@%s) is now a Technical Administrator", parv[0],
		 sptr->username, sptr->realhost);
	  else if(IsNetAdmin(sptr))
		sendto_ops("%s (%s@%s) is now a Network Administrator", parv[0],
		 sptr->username, sptr->realhost);
	  else
    	sendto_ops("%s (%s@%s) is now an operator (%c)", parv[0],
                 sptr->username, sptr->realhost,
                 IsOper(sptr) ? 'O' : 'o');
      send_umode_out(cptr, sptr, old);
      sendto_one(sptr, form_str(RPL_YOUREOPER), me.name, parv[0]);
      sendto_one(sptr, ":%s NOTICE %s :*** Oper privs are %s",me.name,parv[0],
                 operprivs);

      SendMessageFile(sptr, &ConfigFileEntry.opermotd);


#if !defined(CRYPT_OPER_PASSWORD) && (defined(FNAME_OPERLOG) || defined(SYSLOG_OPER))
        encr = "";
#endif
        irclog(L_TRACE, "%s %s by %s!%s@%s", IsAdmin(sptr) ? "ADMIN" : "OPER",
            name, parv[0], sptr->username, sptr->realhost);

						
#ifdef FNAME_OPERLOG
        {
          int     logfile;

          /*
           * This conditional makes the logfile active only after
           * it's been created - thus logging can be turned off by
           * removing the file.
           *
           */

          if (IsPerson(sptr) &&
              (logfile = open(FNAME_OPERLOG, O_WRONLY|O_APPEND)) != -1)
            {
              /* (void)alarm(0); */
              ircsprintf(buf, "%s %s (%s) by (%s!%s@%s)\n", 
                               myctime(CurrentTime), name, 
							   IsAdmin(sptr) ?  "ADMIN" : "OPER",
                               parv[0], sptr->username,
                               sptr->realhost);
              write(logfile, buf, strlen(buf));
              close(logfile);
            }
        }
#endif
	  newmask[0]='\0';

	  if (IsNetAdmin(cptr) && NetAdminMask)
		strncpy(newmask, NetAdminMask, HOSTLEN+USERLEN+2);	  
	  else if (IsTechAdmin(cptr) && TechAdminMask)
		strncpy(newmask, TechAdminMask, HOSTLEN+USERLEN+2);
	  else if (IsAdmin(cptr) && AdminMask)
		strncpy(newmask, AdminMask, HOSTLEN+USERLEN+2);
	  else if (IsOper(cptr) && OperMask)
		strncpy(newmask, OperMask, HOSTLEN+USERLEN+2);
	  else if (IsLocOp(cptr) && LocopMask)
		strncpy(newmask, LocopMask, HOSTLEN+USERLEN+2);
			
	  if(OperCanUseNewMask && newmask[0])
		{
		  parv[1] = newmask;
		  return m_newmask(cptr,sptr, 2, parv);
		}

    }
  else
    {
      detach_conf(sptr, aconf);
      sendto_one(sptr,form_str(ERR_PASSWDMISMATCH),me.name, parv[0]);
#ifdef FAILED_OPER_NOTICE
#ifdef SHOW_FAILED_OPER_PASSWD
      sendto_realops("Failed OPER attempt [%s(%s)] - passwd mismatch: %s [%s@%s]",
        name, password, sptr->name, sptr->username, sptr->realhost);
#else
      sendto_realops("Failed OPER attempt by %s (%s@%s)",
                     parv[0], sptr->username, sptr->realhost);
    irclog(L_NOTICE, "Failed OPER attempt by %s (%s@%s)",
                     parv[0], sptr->username, sptr->realhost);
					 
	  if(oper_fail_count<10) 
		oper_fail_count++;					 
#endif /* SHOW_FAILED_OPER_PASSWD */
#endif
    }
  return 0;
}
