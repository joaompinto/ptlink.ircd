/************************************************************************
 *   IRC - Internet Relay Chat, src/m_who.c
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
 *   $Id: m_whois.c,v 1.8 2005/10/16 15:01:33 jpinto Exp $
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

#include <string.h>

static char buf[BUFSIZE];

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
** m_whois
**      parv[0] = sender prefix
**      parv[1] = nickname masklist
*/
int     m_whois(struct Client *cptr,
                struct Client *sptr,
                int parc,
                char *parv[])
{
  static anUser UnknownUser =
  {
    NULL,       /* next */
    NULL,       /* channel */
    NULL,       /* invited */
	NULL,		/* silence */
    NULL,       /* away */
    0,          /* last */
    1,          /* refcount */
    0,          /* joined */
    "<Unknown>"         /* server */
  };
  static char rpl_oper[] = "an IRC Operator";
  static char rpl_locop[] = "an IRC Operator - Local IRC Operator";
  static char rpl_sadmin[] = "an IRC Operator - Services Administrator";
  static char rpl_admin[] = "an IRC Operator - Server Administrator";   
  static char rpl_tadmin[] = "an IRC Operator - Technical Administrator";
  static char rpl_nadmin[] = "an IRC Operator - Network Administrator"; 
  
  Link  *lp;
  anUser        *user;
  struct Client *acptr, *a2cptr;
  aChannel *chptr;
  char  *nick, *name;
  /* char  *tmp; */
  char  *p = NULL;
  int   found, len, mlen;
  static time_t last_used=0L;
  char *nick_match=NULL, *user_match=NULL, *host_match=NULL, *server_match=NULL;
  char *name_match=NULL;
  int found_mode;
  int hits = 0;
  char *mename = me.name;

  if(sptr->user && sptr->user->vlink)
    mename = sptr->user->vlink->name;
  
  if (parc < 2)
    {
      sendto_one(sptr, form_str(ERR_NONICKNAMEGIVEN),
                 mename, parv[0]);
      return 0;
    }

  if(parc > 2)
    {
      if (hunt_server(cptr,sptr,":%s WHOIS %s :%s", 1,parc,parv) !=
          HUNTED_ISME)
        return 0;
      parv[1] = parv[2];
    }

  if(!IsAnOper(sptr) && !MyConnect(sptr)) /* pace non local requests */
    {
      if((last_used + WHOIS_WAIT) > CurrentTime)
        {
          /* Unfortunately, returning anything to a non local
           * request =might= increase sendq to be usable in a split hack
           * Sorry gang ;-( - Dianora
           */
          return 0;
        }
      else
        {
          last_used = CurrentTime;
        }
    }

  /* Multiple whois from remote hosts, can be used
   * to flood a server off. One could argue that multiple whois on
   * local server could remain. Lets think about that, for now
   * removing it totally. 
   * -Dianora 
   */

  /*  for (tmp = parv[1]; (nick = strtoken(&p, tmp, ",")); tmp = NULL) */
  nick = parv[1];
  p = strchr(parv[1],',');
  if(p)
    *p = '\0';

    {
      int       invis, member, wilds;
      found = 0;
      (void)collapse(nick);
      wilds = (nick[0]=='$' || strchr(nick, '?') || strchr(nick, '*'));
      /*
      ** We're no longer allowing remote users to generate
      ** requests with wildcards.
      */
      if (wilds && !IsAnOper(sptr))
        {
          sendto_one(sptr, form_str(ERR_NOSUCHNICK),
                     mename, parv[0], nick);
          return 0;
        }
      /*        continue; */

      /* If the nick doesn't have any wild cards in it,
       * then just pick it up from the hash table
       * - Dianora 
       */

      if(!wilds)
        {
          acptr = hash_find_client(nick,(struct Client *)NULL);
          if(!acptr)
            {
              sendto_one(sptr, form_str(ERR_NOSUCHNICK),
                         mename, parv[0], nick);

              sendto_one(sptr, form_str(RPL_ENDOFWHOIS),
                         mename, parv[0], parv[1]);
						 
              return 0;
              /*              continue; */
            }
          if(IsStealth(acptr)) { 
	    sendto_one(sptr, form_str(ERR_NOSUCHNICK), mename,
	     parv[0], nick);
	     return 0; // Add by ^Stinger^ after the idea of Soldier (:
	  } 
	  if(!IsPerson(acptr))
            {
              sendto_one(sptr, form_str(RPL_ENDOFWHOIS),
                         mename, parv[0], parv[1]);
              return 0;
            }
            /*      continue; */

          user = acptr->user ? acptr->user : &UnknownUser;
          name = (!*acptr->name) ? "?" : acptr->name;
          invis = IsInvisible(acptr);
          member = (user->channel) ? 1 : 0;

          a2cptr = find_server(user->server);
          
          sendto_one(sptr, form_str(RPL_WHOISUSER), mename,
        	    parv[0], name, acptr->username, acptr->host, acptr->info);

	  if((IsOper(sptr) || (acptr == sptr)) && WhoisExtension)
	  {
		sendto_one(sptr, form_str(RPL_WHOISREALHOST), mename,
		    parv[0], name, acptr->realhost);
	  }

          mlen = strlen(mename) + strlen(parv[0]) + 6 +
            strlen(name);
			
          *buf = '\0';			
          if (IsSsl(acptr))
          {
              sendto_one(sptr, form_str(RPL_WHOISSECURE), mename, parv[0], parv[1]);
          }

		  if(((!IsPrivate(acptr) || IsOper(sptr)) || (acptr==sptr))
		  && !IsStealth(acptr))
          for (len = 0, *buf = '\0', lp = user->channel; lp;
               lp = lp->next)
            {
              chptr = lp->value.chptr;
              if (ShowChannel(sptr, chptr))
                {
                  if (len + strlen(chptr->chname)
                      > (size_t) BUFSIZE - 4 - mlen)
                    {
                      sendto_one(sptr,
                                 ":%s %d %s %s :%s",
                                 mename,
                                 RPL_WHOISCHANNELS,
                                 parv[0], name, buf);
                      *buf = '\0';
                      len = 0;
                    }

		  found_mode = user_channel_mode(acptr, chptr);
#ifdef HIDE_OPS
		  if(is_chan_op(sptr,chptr))
#endif
		    {
		      if(found_mode & CHFL_CHANOP)
			*(buf + len++) = '@';
#ifdef HALFOPS
                     else if (found_mode & CHFL_HALFOP)
                       *(buf + len++) = '%';
#endif
		      else if (found_mode & CHFL_VOICE)
			*(buf + len++) = '+';
		    }
                  if (len)
                    *(buf + len) = '\0';
                  (void)strcpy(buf + len, chptr->chname);
                  len += strlen(chptr->chname);
                  (void)strcat(buf + len, " ");
                  len++;
                }
            }
          if (buf[0] != '\0')
            sendto_one(sptr, form_str(RPL_WHOISCHANNELS),
                       mename, parv[0], name, buf);
          if(IsAnOper(sptr) || !HideServerOnWhois)
            {
#ifdef SERVERHIDE
            if (!(IsAnOper(sptr) || acptr == sptr))
              sendto_one(sptr, form_str(RPL_WHOISSERVER),
                       mename, parv[0], name, NetworkName,
                       NetworkDesc);
            else
#endif
            if(acptr->user && acptr->user->vlink)
              sendto_one(sptr, form_str(RPL_WHOISSERVER),
                     mename, parv[0], name, user->vlink->name,
                     user->vlink->passwd);
            else
              {
                if(!IsService(acptr) || IsAnOper(sptr) || !HideServicesServer)
                sendto_one(sptr, form_str(RPL_WHOISSERVER),
                     mename, parv[0], name, user->server,
                     a2cptr?a2cptr->info:"*Not On This Net*");
              }
	    } /* if(IsAnOper(sptr) || HideServerOnWhois) */
	  if (IsIdentified(acptr))
            sendto_one(sptr, form_str(RPL_WHOISIDENTIFIED),
                       mename, parv[0], name);

          if (IsHelper(acptr))
        	sendto_one(sptr, form_str(RPL_WHOISHELPOP),
                           mename, parv[0], name);
					   
          if(IsOper(sptr) && WhoisExtension)
	  {
	    sendto_one(sptr, form_str(RPL_WHOISMODE),
		mename, parv[0], name, get_mode_string(acptr));
	  }
	  
	  if (user->away)
            sendto_one(sptr, form_str(RPL_AWAY), mename,
                       parv[0], name, user->away);
	if(!IsHideOper(acptr) || IsOper(sptr))
	  {	
	    if (IsNetAdmin(acptr))
              sendto_one(sptr, form_str(RPL_WHOISOPERATOR),   
                      mename, parv[0], name, rpl_nadmin);
	    else if (IsTechAdmin(acptr))
              sendto_one(sptr, form_str(RPL_WHOISOPERATOR),   
                      mename, parv[0], name, rpl_tadmin);						   
            else if (IsSAdmin(acptr))
              sendto_one(sptr, form_str(RPL_WHOISOPERATOR),   
                      mename, parv[0], name, rpl_sadmin);						   
            else if (IsAdmin(acptr))
              sendto_one(sptr, form_str(RPL_WHOISOPERATOR),  
                      mename, parv[0], name, rpl_admin);
            else if (IsOper(acptr))
              sendto_one(sptr, form_str(RPL_WHOISOPERATOR), 
            	mename, parv[0], name, rpl_oper);
            else if (IsLocOp(acptr))
              sendto_one(sptr, form_str(RPL_WHOISOPERATOR),
            	mename, parv[0], name, rpl_locop);
	  }
#ifdef WHOIS_NOTICE
          if ((IsOper(acptr)) && ((acptr)->umodes & UMODE_SPY) &&
              (MyConnect(sptr)) && (IsPerson(sptr)) && (acptr != sptr) && !is_silenced(sptr, acptr))
            sendto_one(acptr,
                       ":%s NOTICE %s :*** Notice -- %s (%s@%s) is doing a /whois on you.",
                       me.name, acptr->name, parv[0], sptr->username,
                       sptr->realhost);
#endif /* #ifdef WHOIS_NOTICE */


          if ((acptr->user
#ifdef SERVERHIDE
              && IsAnOper(sptr)
#endif
              && MyConnect(acptr)))
            sendto_one(sptr, form_str(RPL_WHOISIDLE),
                       mename, parv[0], name,
                       CurrentTime - user->last,
                       acptr->firsttime);
					   
          sendto_one(sptr, form_str(RPL_ENDOFWHOIS), mename, parv[0], parv[1]);
          
          return 0;
          /*      continue; */
        }

      /* wild is true so here we go */
          if(nick[0]==':') /* real name match */
            {
              name_match =  &nick[1];
              nick_match = NULL;
            }
          else
	  if(nick[0]=='$') /* server name match */
	    {
	      server_match = &nick[1];
	      nick_match = NULL;
	    }
	  else
		{
		  host_match = strchr(nick,'@');
		  if(host_match)
			{
			  if(*host_match)
				*(host_match++) = '\0';						  
			  user_match=nick;		  		  
			  if(host_match=='\0')
				host_match="*";		  			
			  if(user_match=='\0')
			  user_match="*";		  
			}
		  else
			nick_match = nick;
		}
		
	  				
      for (acptr = GlobalClientList; acptr;
           acptr = acptr->next)
        {
          if (IsServer(acptr))
            continue;
          /*
           * I'm always last :-) and acptr->next == NULL!!
           */
          if (IsMe(acptr))
            break;
          /*
           * 'Rules' established for sending a WHOIS reply:
           *
           *
           * - if wildcards are being used dont send a reply if
           *   the querier isnt any common channels and the
           *   client in question is invisible and wildcards are
           *   in use (allow exact matches only);
           *
           * - only send replies about common or public channels
           *   the target user(s) are on;
           */

/* If its an unregistered client, ignore it, it can
   be "seen" on a /trace anyway  -Dianora */

          if(!IsRegistered(acptr))
            continue;

          user = acptr->user ? acptr->user : &UnknownUser;
          name = (!*acptr->name) ? "?" : acptr->name;
		  
		  if(  (server_match && !match(server_match, user->server))
		    || (nick_match && !match(nick, name)) 
			|| (host_match && !match(host_match, acptr->realhost)
			   && !match(host_match, acptr->host))
			|| (user_match && !match(user_match, acptr->username))
			|| (name_match &&  !match(name_match, acptr->info))
			)
        	  continue;
			  
		  ++hits;
			  
          a2cptr = find_server(user->server);
          
          sendto_one(sptr, form_str(RPL_WHOISUSER), mename,
                    parv[0], name,
                    acptr->username, 
					IsOper(sptr) ? acptr->realhost : acptr->host,					 
					acptr->info);
					
          found = 1;
          mlen = strlen(mename) + strlen(parv[0]) + 6 +
            strlen(name);
          for (len = 0, *buf = '\0', lp = user->channel; lp;
               lp = lp->next)
            {
              chptr = lp->value.chptr;
              if (ShowChannel(sptr, chptr))
                {
                  if (len + strlen(chptr->chname)
                      > (size_t) BUFSIZE - 4 - mlen)
                    {
                      sendto_one(sptr,
                                 ":%s %d %s %s :%s",
                                 mename,
                                 RPL_WHOISCHANNELS,
                                 parv[0], name, buf);
                      *buf = '\0';
                      len = 0;
                    }
		  found_mode = user_channel_mode(acptr, chptr);
#ifdef HIDE_OPS
                  if(is_chan_op(sptr,chptr))
#endif
		     {
		       if (found_mode & CHFL_CHANOP)
			 *(buf + len++) = '@';
#ifdef HALFOPS
                     else if (found_mode & CHFL_HALFOP)
                        *(buf + len++) = '%';
#endif                                            
		       else if (found_mode & CHFL_VOICE)
			 *(buf + len++) = '+';
		     }
                  if (len)
                    *(buf + len) = '\0';
                  (void)strcpy(buf + len, chptr->chname);
                  len += strlen(chptr->chname);
                  (void)strcat(buf + len, " ");
                  len++;
                }
            }
          if (buf[0] != '\0')
            sendto_one(sptr, form_str(RPL_WHOISCHANNELS),
                       mename, parv[0], name, buf);
         
#ifdef SERVERHIDE
          if (!(IsAnOper(sptr) || acptr == sptr))
            sendto_one(sptr, form_str(RPL_WHOISSERVER),
                       mename, parv[0], name, NetworkName,
                       NetworkDesc);
          else    
#endif
          sendto_one(sptr, form_str(RPL_WHOISSERVER),
                     mename, parv[0], name, user->server,
                     a2cptr?a2cptr->info:"*Not On This Net*");

          if (user->away)
            sendto_one(sptr, form_str(RPL_AWAY), mename,
                       parv[0], name, user->away);



  		  if (IsNetAdmin(acptr))
                sendto_one(sptr, form_str(RPL_WHOISOPERATOR),   
                           mename, parv[0], name, rpl_nadmin);
		  else if (IsTechAdmin(acptr))
                sendto_one(sptr, form_str(RPL_WHOISOPERATOR),   
                           mename, parv[0], name, rpl_tadmin);						   
          else if (IsSAdmin(acptr))
                sendto_one(sptr, form_str(RPL_WHOISOPERATOR),   
                           mename, parv[0], name, rpl_sadmin);						   
          else if (IsAdmin(acptr))
                sendto_one(sptr, form_str(RPL_WHOISOPERATOR),  
                           mename, parv[0], name, rpl_admin);
          else if (IsAnOper(acptr))
                sendto_one(sptr, form_str(RPL_WHOISOPERATOR), 
                           mename, parv[0], name, rpl_oper);


#ifdef WHOIS_NOTICE
          if ((MyOper(acptr)) && ((acptr)->umodes & UMODE_SPY) &&
              (MyConnect(sptr)) && (IsPerson(sptr)) && (acptr != sptr))
            sendto_one(acptr,
                       ":%s NOTICE %s :*** Notice -- %s (%s@%s) is doing a /whois on you.",
                       mename, acptr->name, parv[0], sptr->username,
                       sptr->realhost);
#endif /* #ifdef WHOIS_NOTICE */

          if ((acptr->user
#ifdef SERVERHIDE
              && IsAnOper(sptr) 
#endif                 
              && MyConnect(acptr)))
            sendto_one(sptr, form_str(RPL_WHOISIDLE),
                       mename, parv[0], name,
                       CurrentTime - user->last,
                       acptr->firsttime);

		if(hits>50)
		  {
  			sendto_one(sptr,":%s NOTICE %s :Aborting /whois output as flood prevention",
				mename, sptr->name);			  
			break;
		  }
        }
		
      if (!found)	  
        sendto_one(sptr, form_str(ERR_NOSUCHNICK),
                   mename, parv[0], nick);
	  else
		sendto_one(sptr,":%s NOTICE %s :This /whois matched \2%i\2 user(s)", 
			mename, sptr->name,hits);
      /*
      if (p)
        p[-1] = ',';
        */
    }

  sendto_one(sptr, form_str(RPL_ENDOFWHOIS), mename, parv[0], parv[1]);
  
  return 0;
}
