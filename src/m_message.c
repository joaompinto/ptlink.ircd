/************************************************************************
 *   IRC - Internet Relay Chat, src/m_message.c
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
 *   $Id: m_message.c,v 1.7 2005/10/08 16:00:04 jpinto Exp $
 */
#include "m_commands.h"
#include "client.h"
#include "flud.h"
#include "ircd.h"
#include "numeric.h"
#include "s_serv.h"
#include "m_silence.h"
#include "send.h"
#include "svline.h" /* is_svlined() */
#include "msg.h"  /* ZZZ should be able to remove this */
#include "channel.h"
#include "irc_string.h"
#include "hash.h"
#include "class.h"
#include "s_conf.h"
#include "config.h"
#include "dconf_vars.h"

#include <string.h>

extern int      is_silenced(struct Client *, struct Client *);
extern int check_target_limit(struct Client *sptr, void *target, const char *name, int created, char *fmsg);
int space_count(char *message);
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
** m_message (used in m_private() and m_notice())
** the general function to deliver MSG's between users/channels
**
**      parv[0] = sender prefix
**      parv[1] = receiver list
**      parv[2] = message text
**
** massive cleanup
** rev argv 6/91
**
*/

static  int     m_message(struct Client *cptr,
                          struct Client *sptr,
                          int parc,
                          char *parv[],
                          int notice)
{
  struct Client       *acptr;
  struct Channel *chptr;
  char  *nick, *server, *cmd, *host;
  /* char *p; */
  int type=0, msgs=0;
  int cantsendtype = 0;
  char *reason = "";
  char *sfn, *efn; /* start/end of filename */
  char filename[255]; /* filename for DCC check */
  aConfItem *aconf;
  aConfItem* msgv;  
  char *stripedmsg;

  cmd = notice ? MSG_NOTICE : MSG_PRIVATE;

  if (parc < 2 || *parv[1] == '\0')
    {
      sendto_one(sptr, form_str(ERR_NORECIPIENT),
                 me.name, parv[0], cmd);
      return -1;
    }

  if (parc < 3 || *parv[2] == '\0')
    {
      sendto_one(sptr, form_str(ERR_NOTEXTTOSEND), me.name, parv[0]);
      return -1;
    }
	
  if (IsZombie(sptr) || IsStealth(sptr))
	return 0;
	
  if (MyConnect(sptr))
    {      
      stripedmsg = strip_control_codes_lower(parv[2]);
      if(!IsOper(sptr) && (msgv = is_msgvlined(stripedmsg)))
      	{
      	  sendto_one(sptr,":%s NOTICE %s :%s",
      	    me.name, parv[0], msgv->passwd);
          sendto_ops_imodes(IMODE_VLINES,"Blocked svlined message (from %s): %s",
            sptr->name, parv[2]);

	  /* If GLineOnSVline is a number higher than 0 and an user
	   * has triggered a SVline, add a temporary GLine (with time
	   * being equal GLineOnSVline time) to *@(user host). 
	   *
	   * Use wisely, as this can lead to big problems!
	   * -- openglx
	   */
	  if(GLineOnSVline > 0)
	  {
                 aconf = make_conf();
		 aconf->status = CONF_KILL;
		 
		 
		 DupString(aconf->host, cptr->realhost);
		 DupString(aconf->passwd, "Auto GLined for triggering a SVline");
		 DupString(aconf->user, "*");
		 DupString(aconf->name, me.name);
		 aconf->hold = CurrentTime + GLineOnSVline;
		 add_gline(aconf);
		 
                 sendto_serv_butone(NULL, ":%s GLINE %s@%s %lu %s :%s",
                                 me.name,
                                 aconf->user, aconf->host,
                                 aconf->hold - CurrentTime,
                                 aconf->name,
                                 aconf->passwd);
		 apply_gline(aconf->host, aconf->user, "Auto GLined for triggering a SVline");
	  }
 	  
      	  return 0;
      	}
#ifdef ANTI_SPAMBOT
#ifndef ANTI_SPAMBOT_WARN_ONLY
      /* if its a spambot, just ignore it */
      if(sptr->join_leave_count >= MAX_JOIN_LEAVE_COUNT)
        return 0;
#endif
#endif
#ifdef NO_DUPE_MULTI_MESSAGES
      if (strchr(parv[1],','))
        parv[1] = canonize(parv[1]);
#endif
    }


  /*
  ** channels are privmsg'd a lot more than other clients, moved up here
  ** plain old channel msg ?
  */
  while(msgs < MAX_MULTI_MESSAGES)
  {
     if(!msgs)
        nick = strtok(parv[1], ",");
     else
        nick = strtok(NULL, ",");

     if(!nick && msgs == 0)
        nick = parv[1];
     else if(!nick)
        break;

  if( IsChanPrefix(*nick)
      && (IsPerson(sptr) && (chptr = hash_find_channel(nick, NullChn))))
    {
      remove_autoaway(sptr);
    
#ifdef  IDLE_CHECK
      /* reset idle time for message only if target exists */
      if(MyClient(sptr) && sptr->user)
        sptr->user->last = CurrentTime;
#endif
#ifdef FLUD
      if(!notice)
        if(check_for_ctcp(parv[2]))
            check_for_flud(sptr, NULL, chptr, 1);
#endif /* FLUD */

      cantsendtype = can_send(sptr, chptr, parv[2]);
	  	  
      if (cantsendtype == 0)
        {
	  if (CheckTargetLimit && MyClient(sptr) &&
	     check_target_limit(sptr, chptr, chptr->chname, 0, parv[2]))              
	     return 0;
	     
          if (MyClient(sptr) && (chptr->mode.mode & MODE_NOCOLORS))
            parv[2] = strip_control_codes(parv[2]);

      	  sendto_channel_butone(cptr, sptr, chptr,
                              ":%s %s %s :%s",
                              parv[0], cmd, nick,
          	                  parv[2]);
		}
      else if (!notice) 
		{
		  switch(cantsendtype) 
			{
			
			  case MODE_MODERATED:
				reason = Moderated_Msg;
				break;
				
			  case MODE_NOPRIVMSGS:
				reason = NoExternal_Msg;
				break;				
				
			  case 	MODE_BAN:
			  	reason = Banned_Msg;
				break;
				
			  case MODE_NOSPAM:
				reason = NoSpam_Msg;
				break;
				
			  case MODE_NOFLOOD:
				reason = NoFlood_Msg;
				break;
			  
			  default : reason = NoCTCP_Msg;
			}
            
          if(cantsendtype != MODE_FLOODLIMIT)            
      	    sendto_one(sptr, form_str(ERR_CANNOTSENDTOCHAN),
                   me.name, parv[0], nick, reason);
		}
      msgs++;
      continue;
    }
      
  /*
  ** @# type of channel msg?
  */

  if(*nick == '@')
    type = MODE_CHANOP;
#ifdef HALFOP
  else if(*nick == '%')
    type = MODE_CHANOP|MODE_HALFOP;
#endif
  else if(*nick == '+')
#ifdef HALFOP
    type = MODE_CHANOP|MODE_VOICE|MODE_HALFOP;
#else    
    type = MODE_CHANOP|MODE_VOICE;
#endif    
    
  if(type)
    {
      /* Strip if using DALnet chanop/voice prefix. */
      if (*(nick+1) == '@' || *(nick+1) == '+')
        {
          nick++;
          *nick = '@';
#ifdef HALFOP
          type = MODE_CHANOP|MODE_VOICE|MODE_HALFOP;
#else
          type = MODE_CHANOP|MODE_VOICE;
#endif          
        }

      /* suggested by Mortiis */
      if(!*nick)        /* if its a '\0' dump it, there is no recipient */
        {
          sendto_one(sptr, form_str(ERR_NORECIPIENT),
                     me.name, parv[0], cmd);
          return -1;
        }

      /* At this point, nick+1 should be a channel name i.e. #foo or &foo
       * if the channel is found, fine, if not report an error
       */

      if ( (chptr = hash_find_channel(nick+1, NullChn)) )
        {
          remove_autoaway(sptr);
                        
#ifdef  IDLE_CHECK
          /* reset idle time for message only if target exists */
          if(MyClient(sptr) && sptr->user)
            sptr->user->last = CurrentTime;
#endif
#ifdef FLUD
          if(!notice)
            if(check_for_ctcp(parv[2]))
              check_for_flud(sptr, NULL, chptr, 1);
#endif /* FLUD */

          if (MyClient(cptr) && !IsService(sptr) && !is_chan_op(sptr,chptr))
            {
              if (!notice)
                {
                  sendto_one(sptr, form_str(ERR_CANNOTSENDTOCHAN),
                             me.name, parv[0], nick);
                }
	msgs++;
	continue;
            }
          else
            {
              sendto_channel_type(cptr,
                                  sptr,
                                  chptr,
                                  type,
                                  nick+1,
                                  cmd,
                                  parv[2]);
            }
        }
      else
        {
          sendto_one(sptr, form_str(ERR_NOSUCHNICK),
                     me.name, parv[0], nick);
	   msgs++;
	   continue;
        }
      return 0;
    }

  /* Lets trim mode prefixes */
#ifdef HALFOP
  while((*nick=='.')||(*nick=='@')||(*nick=='%')||(*nick=='+')) /* if a client has no halfop support strip it */
#else
  while((*nick=='.')||(*nick=='@')||(*nick=='+'))
#endif
    ++nick;
  /*
  ** nickname addressed?
  */
#ifdef WEBTV_SUPPORT
  if (!strcasecmp(nick, "irc") && MyClient(sptr))
    {
      parse(sptr, parv[2], (parv[2] + strlen(parv[2])));
      return 0;
    }
#endif
  
  if ((acptr = find_person(nick, NULL)))
    {
      remove_autoaway(sptr);
#ifdef  IDLE_CHECK
      /* reset idle time for message only if target exists */
      if(MyClient(sptr) && sptr->user)
        sptr->user->last = CurrentTime;
#endif

	  if(!IsService(sptr) && !IsIdentified(sptr) && (IsRegMsg(acptr)||secure_mode))
		return -1;

	  if( is_silenced(sptr, acptr) )/* source is silenced by target */
		return -1;
		
      if (MyClient(sptr) && /* bypassed target limit */
		CheckTargetLimit && check_target_limit(sptr, acptr, acptr->name,0, parv[2]))        
                return 0;		
		  
	  if(MyClient(sptr))
	  {	   
		if(!strncasecmp(acptr->name,"NickServ",8))
		  {
			if(ForceServicesAlias && (parv[2][0] != '\1'))
			  {
				sendto_one(cptr,":%s NOTICE %s :For this service please use /%s", 
				  me.name, sptr->name, acptr->name);
				return -1;
			  }
			else
			  {
				parv[1] = parv[2];
				return m_nickserv(cptr, acptr, 2, parv);
			  }
		  }		  
		  
		else if(!strncasecmp(acptr->name,"ChanServ", 8))
		  {
			if(ForceServicesAlias && (parv[2][0] != '\1'))
			  {
				sendto_one(cptr,":%s NOTICE %s :For this service please use /%s", 
				  me.name, sptr->name, acptr->name);
				return -1;
			  }
			else
			  {
				parv[1] = parv[2];
				return m_chanserv(cptr, acptr, 2, parv);
			  }
		  }
		  
		else if(!strncasecmp(acptr->name,"MemoServ", 8))
		  {
			if(ForceServicesAlias && (parv[2][0] != '\1'))
			  {
				sendto_one(cptr,":%s NOTICE %s :For this service please use /%s", 
				  me.name, sptr->name, acptr->name);
				return -1;
			  }
			else
			  {
				parv[1] = parv[2];
				return m_memoserv(cptr, acptr, 2, parv);
			  }
		  }

		else if(!strncasecmp(acptr->name,"NewsServ",8))
		  {
			if(ForceServicesAlias && (parv[2][0] != '\1'))
			  {
				sendto_one(cptr,":%s NOTICE %s :For this service please use /%s", 
				  me.name, sptr->name, acptr->name);
				return -1;
			  }
			else
			  {
				parv[1] = parv[2];
				return m_newsserv(cptr, acptr, 2, parv);
			  }
		  }		  

		else if(!strncasecmp(acptr->name,"OperServ", 8))
		  {
			if(ForceServicesAlias && (parv[2][0] != '\1'))
			  {
				sendto_one(cptr,":%s NOTICE %s :For this service please use /%s", 
				  me.name, sptr->name, acptr->name);
				return -1;
			  }
			else
			  {
				parv[1] = parv[2];
				return m_operserv(cptr, acptr, 2, parv);
			  }
		  }		  		  
		else if(!strncasecmp(acptr->name, "StatServ", 8)) 
		{
		    if(ForceServicesAlias && (parv[2][0] != '\1')) 
		    {
			sendto_one(cptr, ":%s NOTICE %s :For this service please use /%s",
			 me.name, sptr->name, acptr);
			return -1;
		    }
		    else 
		    {
			parv[1] = parv[2];
			return m_statserv(cptr, acptr, 2, parv);
		    }
		}
		else if(!strncasecmp(acptr->name, "BotServ", 7))
		{
		    if(ForceServicesAlias && (parv[2][0] != '\1'))
		    {
		      sendto_one(cptr, ":%s NOTICE %s :For this service please use /%s",
		        me.name, sptr->name, acptr->name);
			return -1;
		    }
		    else
		    {
			parv[1] = parv[2];
			return m_botserv(cptr, acptr, 2, parv);
		    }
		}
	  }

	  if (!notice && MyClient(sptr) && (parv[2][0] == '\1')
		  && (strlen(parv[2]) > 11)  &&
		  ((strncasecmp(&parv[2][1], "DCC SEND", 8) == 0) ||
		  (strncasecmp(&parv[2][1], "DCC RESUME", 10) == 0)) 
		  )
		{
		
			if(IsNoDCC(sptr))
			  {
				sendto_one(sptr,
             		":%s NOTICE %s :%s",
             		  me.name, sptr->name, NoDCCSend_Msg);
             	return -1;
			  }
			else
			  {  
				sfn = &parv[2][10];
				
				/* Credits here goto fabulous for informing me that "" are used for long filenames */
				if(*sfn=='\"')
				  {
				  	++sfn;		/* remove leading " from filename */
					efn=strchr(sfn+1,'\"');
					if(efn)
					  --efn;	/* remove trailing " from filename */
				  }
				else
				  efn=strchr(sfn+1,' ');
				
				if(efn==NULL)
				  efn=sfn+strlen(parv[2]);
				  
				if(efn-sfn>255)
				  efn = sfn + 255;
				  
				if(efn-sfn>0)
				  {
					strncpy(filename,sfn, efn-sfn);
					filename[efn-sfn]='\0';
#ifdef MIRC_DCC_BUG_CHECK
					if(space_count(sfn)>10)
					  {
					    sendto_ops_imodes(IMODE_VLINES,
					      "mIRC DCC bug exploit attempt from %s", sptr->name);
					    return -1;
					  }
#endif
					if((aconf=is_svlined(filename)))
	  				  {       
                        if(aconf->passwd)
                          sendto_one(sptr, ":%s NOTICE %s :You can't send the file, reason: %s",
             			    me.name, sptr->name, aconf->passwd);
                        else
                          sendto_one(sptr, ":%s NOTICE %s :%s", 
                            me.name, sptr->name, NoDCCSend_Msg);

			sendto_ops_imodes(IMODE_VLINES,
			  "DCC deny on \2%s!%s@%s\2 SVLined on file \2[\2%s\2]\2",
                	   sptr->name, sptr->username, sptr->realhost, 
			   filename);
							
 			  SetNoDCC(sptr);
					   
		  	  sendto_serv_butone(&me,":%s DCCDENY %s :SVLined on file %s",
  				me.name, parv[0], filename);
							
			return -1;
			 }
		  }
			  
	  }
    } 
	  
#ifdef FLUD
      if(!notice && MyConnect(acptr))
        if(check_for_ctcp(parv[2]))
          if(check_for_flud(sptr, acptr, NULL, 1))
            return 0;
#endif
#ifdef ANTI_DRONE_FLOOD
      if(MyConnect(acptr) && IsClient(sptr) && !IsService(sptr) && !IsAnOper(sptr) && DRONETIME)
        {
          if((acptr->first_received_message_time+DRONETIME) < CurrentTime)
            {
              acptr->received_number_of_privmsgs=1;
              acptr->first_received_message_time = CurrentTime;
              acptr->drone_noticed = 0;
            }
          else
            {
              if(acptr->received_number_of_privmsgs > DRONECOUNT)
                {
                  if(acptr->drone_noticed == 0) /* tiny FSM */
                    {
                      sendto_ops_imodes(IMODE_BOTS,
                             "Possible Drone Flooder %s [%s@%s] on %s target: %s",
                                     sptr->name, sptr->username,
                                     sptr->host,
                                     sptr->user->server, acptr->name);
                      acptr->drone_noticed = 1;
                    }
                  /* heuristic here, if target has been getting a lot
                   * of privmsgs from clients, and sendq is above halfway up
                   * its allowed sendq, then throw away the privmsg, otherwise
                   * let it through. This adds some protection, yet doesn't
                   * DOS the client.
                   * -Dianora
                   */
                  if(DBufLength(&acptr->sendQ) > (get_sendq(acptr)/2L))
                    {
                      if(acptr->drone_noticed == 1) /* tiny FSM */
                        {
                          sendto_ops_imodes(IMODE_BOTS,
                         "ANTI_DRONE_FLOOD SendQ protection activated for %s",
                                         acptr->name);

                          sendto_one(acptr,     
 ":%s NOTICE %s :*** Notice -- Server drone flood protection activated for %s",
                                     me.name, acptr->name, acptr->name);
                          acptr->drone_noticed = 2;
                        }
                    }

                  if(DBufLength(&acptr->sendQ) <= (get_sendq(acptr)/4L))
                    {
                      if(acptr->drone_noticed == 2)
                        {
                          sendto_one(acptr,     
                                     ":%s NOTICE %s :*** Notice -- Server drone flood protection de-activated for %s",
                                     me.name, acptr->name, acptr->name);
                          acptr->drone_noticed = 1;
                        }
                    }
                  if(acptr->drone_noticed > 1)
                    return 0;
                }
              else
                acptr->received_number_of_privmsgs++;
            }
        }
#endif
      if (!notice && MyConnect(sptr) &&
          acptr->user && acptr->user->away)
        sendto_one(sptr, form_str(RPL_AWAY), me.name,
                   parv[0], acptr->name,
                   acptr->user->away);
      sendto_prefix_one(acptr, sptr, ":%s %s %s :%s",
                        parv[0], cmd, nick, parv[2]);

#ifdef  IDLE_CHECK
      /* reset idle time for message only if its not to self */
      if (sptr != acptr)
        {
          if(sptr->user)
            sptr->user->last = CurrentTime;
        }
#endif
      msgs++;
      continue;
    }

  /* Everything below here should be reserved for opers 
   * as pointed out by Mortiis, user%host.name@server.name 
   * syntax could be used to flood without FLUD protection
   * its also a delightful way for non-opers to find users who
   * have changed nicks -Dianora
   *
   * Grrr it was pointed out to me that x@service is valid
   * for non-opers too, and wouldn't allow for flooding/stalking
   * -Dianora
   */

        
  /*
  ** the following two cases allow masks in NOTICEs
  ** (for OPERs only)
  **
  ** Armin, 8Jun90 (gruner@informatik.tu-muenchen.de)
  */
  if ((*nick == '$' || *nick == '#'))
    {

      if(!IsAnOper(sptr) && !IsService(sptr))
        {
          sendto_one(sptr, form_str(ERR_NOSUCHNICK),
                     me.name, parv[0], nick);
          return -1;
        }

#if 0
      if (!(s = (char *)strrchr(nick, '.')))
        {
          sendto_one(sptr, form_str(ERR_NOTOPLEVEL),
                     me.name, parv[0], nick);
          msgs++;
	  continue;
        }
      while (*++s)
        if (*s == '.' || *s == '*' || *s == '?')
          break;
      if (*s == '*' || *s == '?')
        {
          sendto_one(sptr, form_str(ERR_WILDTOPLEVEL),
                     me.name, parv[0], nick);
	  msgs++;
	  continue;
        }
#endif 
      sendto_match_butone(IsServer(cptr) ? cptr : NULL, 
                          sptr, nick + 1,
                          (*nick == '#') ? MATCH_HOST :
                          MATCH_SERVER,
                          ":%s %s %s :%s", parv[0],
                          cmd, nick, parv[2]);
      msgs++;
      continue;
    }
        
  /*
  ** user[%host]@server addressed?
  */
  if ((server = (char *)strchr(nick, '@')) &&
      (acptr = find_server(server + 1)))
    {
      int count = 0;

      /* Disable the user%host@server form for non-opers
       * -Dianora
       */

      if( (char *)strchr(nick,'%') && !IsAnOper(sptr))
        {
          sendto_one(sptr, form_str(ERR_NOSUCHNICK),
                     me.name, parv[0], nick);
 	  msgs++;
	  continue;
        }
        
      /*
      ** Not destined for a user on me :-(
      */
      if (!IsMe(acptr))
        {
          sendto_one(acptr,":%s %s %s :%s", parv[0],
                     cmd, nick, parv[2]);
          msgs++;
          continue;
        }

      *server = '\0';

      /* special case opers@server */
      if(!irccmp(nick,"opers") && IsAnOper(sptr))
        {
          sendto_realops("To opers: From %s: %s",sptr->name,parv[2]);
          msgs++;
          continue;
        }
        
      if ((host = (char *)strchr(nick, '%')))
        *host++ = '\0';

      /*
      ** Look for users which match the destination host
      ** (no host == wildcard) and if one and one only is
      ** found connected to me, deliver message!
      */
      acptr = find_userhost(nick, host, NULL, &count);
      if (server)
        *server = '@';
      if (host)
        *--host = '%';
      if (acptr)
        {
          if (count == 1)
            sendto_prefix_one(acptr, sptr,
                              ":%s %s %s :%s",
                              parv[0], cmd,
                              nick, parv[2]);
          else if (!notice)
            sendto_one(sptr,
                       form_str(ERR_TOOMANYTARGETS),
                       me.name, parv[0], nick, MAX_MULTI_MESSAGES);
        }
      if (acptr)
	{
	  msgs++;
	  continue;
	}
    }
  sendto_one(sptr, form_str(ERR_NOSUCHNICK), me.name,
             parv[0], nick);
   msgs++;
  }
  
  if (strtok(NULL, ",") && notice)
    sendto_one(sptr, ":%s NOTICE %s :Too many targets on NOTICE, for ONOTICE please use /notice @#channel message",
  	  me.name, parv[0]);

  return 0;
}

/*
** m_private
**      parv[0] = sender prefix
**      parv[1] = receiver list
**      parv[2] = message text
*/

int     m_private(struct Client *cptr,
                  struct Client *sptr,
                  int parc,
                  char *parv[])
{
  return m_message(cptr, sptr, parc, parv, 0);
}

/*
** m_notice
**      parv[0] = sender prefix
**      parv[1] = receiver list
**      parv[2] = notice text
*/

int     m_notice(struct Client *cptr,
                 struct Client *sptr,
                 int parc,
                 char *parv[])
{
  return m_message(cptr, sptr, parc, parv, 1);
}

/*
 * check_target_limit
 *
 * sptr must be a local client !
 *
 * Returns 'true' (1) when too many targets are addressed.
 * Returns 'false' (0) when it's ok to send to this target.
 */
int check_target_limit(struct Client *sptr, void *target, const char *name,
    int created, char *fmsg)    
{
  register unsigned char *p;
  register unsigned int tmp = ((size_t)target & 0xffff00) >> 8;
  unsigned char hash = (tmp * tmp) >> 12;
  
  if (sptr->targets[0] == hash) /* Same target as last time ? */
    return 0;
        
  for (p = sptr->targets; p < &sptr->targets[MAXTARGETS - 1];)
    if (*++p == hash)
    {
      memmove(&sptr->targets[1], &sptr->targets[0], p - sptr->targets);
      sptr->targets[0] = hash;
      return 0;
    }

  /* New target */
  if (!created)   
  {
    if (CurrentTime < sptr->nexttarget)
    {
      if (sptr->nexttarget - CurrentTime < TARGET_DELAY + 8)	/* No server flooding */
        {
       	  sptr->nexttarget += 2;
          
          if (CheckSpamOnTarget && (strchr(fmsg,'#') || is_spam(fmsg))) 
            {
              SetZombie(sptr);
#if 0             
              if (!find_zline(sptr->realhost)) 
                {
                  char tmpbuf[BUFSIZE+NICKLEN+7+NICKLEN];
                  ircsprintf(tmpbuf, "%s->%s: %s\15", sptr->name, name, fmsg);
                  aconf = make_conf();
                  DupString(aconf->name, sptr->realhost);
		
                  aconf->hold = CurrentTime+1200;
                  DupString(aconf->passwd, tmpbuf);

                  aconf->next = zlines;
                  zlines = aconf;  
                  sendto_serv_butone(NULL, ":%s ZLINE %s %d :%s",
                    me.name, sptr->realhost, 1200, tmpbuf);
                  sendto_realops_flags(UMODE_DEBUG, "Added spam Z:line, %s",
                    tmpbuf);
                }
#endif 
              } else                          
       		    sendto_one(sptr, form_str(ERR_TARGETTOOFAST),                
       		        me.name, sptr->name, name, sptr->nexttarget - CurrentTime);
	    sptr->since = CurrentTime + 10; /* die flooder  -Lamego */
            sendto_ops_imodes(IMODE_TARGET, "Target limit triggered for %s", sptr->name);
      }
	    return 1;
    }
    else
    {   
      sptr->nexttarget += TARGET_DELAY;
      if (sptr->nexttarget < CurrentTime - (TARGET_DELAY * (MAXTARGETS - 1)))
          sptr->nexttarget = CurrentTime - (TARGET_DELAY * (MAXTARGETS - 1));
    }
  }  
     
  memmove(&sptr->targets[1], &sptr->targets[0], MAXTARGETS - 1);
  sptr->targets[0] = hash;
  return 0;
}

/* Count spaces on message for mIRC bug detection */
int space_count(char *message)
{
  char *c = message;
  int i = 0;
  c = message;
  while(*c)
    {
      if(*c==' ')
        ++i;
      ++c;
    }
  return i;
}
