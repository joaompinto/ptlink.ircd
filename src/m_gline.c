/************************************************************************
 *   IRC - Internet Relay Chat, src/m_gline.c
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
 *  $Id: m_gline.c,v 1.4 2005/08/27 16:23:50 jpinto Exp $
 */
#include "m_gline.h"
#include "channel.h"
#include "hash.h"
#include "client.h"
#include "common.h"
#include "config.h"
#include "dline_conf.h"
#include "irc_string.h"
#include "ircd.h"
#include "m_kline.h"
#include "mtrie_conf.h"
#include "numeric.h"
#include "s_conf.h"
#include "s_misc.h"
#include "s_serv.h"
#include "scache.h"		
#include "s_bsd.h"  	/* for highest_fd */
#include "send.h"
#include "struct.h"
#include "dconf_vars.h"


#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>



/* external variables */
extern ConfigFileEntryType ConfigFileEntry; /* defined in ircd.c */

/* internal variables */
aConfItem *glines = (aConfItem *)NULL;

/* internal functions */

#if 0
static void log_gline_request(const char*,const char*,const char*,
                              const char* oper_server,
                              const char *,const char *,const char *);


static int majority_gline(aClient*, const char *,const char *, const char *, 
                          const char* serv_name,
                          const char *,const char *,const char *); 						  
#endif /* 0 */

/*
 * m_gline()
 *
 *		parv[0] = sender prefix
 *		parv[1] = nick | user@host
 *		parv[2] = time
 *		parv[3] = IsServer ? who : reason
 *		parv[4] = IsServer ? reason
 */
int     m_gline(aClient *cptr,
                aClient *sptr,
                int parc,
                char *parv[])
{
  char *oper_name = NULL;              /* nick of oper requesting GLINE */
  char *oper_username = NULL;          /* username of oper requesting GLINE */
  char *oper_host = NULL;              /* hostname of oper requesting GLINE */
  const char* oper_server = NULL;      /* server of oper requesting GLINE */
  char *user, *host;            /* user and host of GLINE "victim" */
  char *reason;                 /* reason for "victims" demise */
  char *who;					/* who added the gline */
  char *p;
  register char tmpch;
  register int nonwild;
  char buffer[512];
  const char *current_date;
  char tempuser[USERLEN + 2];
  char temphost[HOSTLEN + 1];
  char *ch, *timestr;
  unsigned long secs = 0;
  aConfItem *aconf;
  aConfItem *curr_gl;
  int targets;
  char *target;
  aClient *acptr;
  struct Channel *chptr;
  Link *lp;
  
  if(MyClient(sptr)) /* allow remote opers to apply g lines */
    {

      /* Only globals can apply Glines */
      if (!IsOper(sptr))
        {
          sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, parv[0]);
          return 0;
        }

      if (!IsSetOperGline(sptr))
        {
          sendto_one(sptr,":%s NOTICE %s :You have no G flag",me.name,parv[0]);
          return 0;
        }

    }
    
  if(MyClient(sptr))
    {
      if(parc==2 && DefaultGLineReason && DefaultGLineTime)
        {
          secs = DefaultGLineTime;
          parv[3] = DefaultGLineReason;
          parc = 4;
        }
      else if(parc==3 && DefaultGLineReason)
        {
          if(strcasecmp(parv[1],"list")==0)
            {
              report_glines(sptr, parv[2]);
              return 0;
            }
          parv[3] = DefaultGLineReason;
          parc = 4;
        }
    }
    
  if ( parc < 4 )
    {
        sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
                   me.name, parv[0], "GLINE");
        return 0;
    }

  /* check for reason */
  if(IsServer(cptr) && parc<5)
	reason = "No reason";
  else
	reason = IsServer(cptr) ? parv[4] : parv[3];

  if(secs==0)
    {
      /* calculate time in seconds */
      timestr = parv[2];  
      ch=&timestr[strlen(timestr)-1];
      switch(*ch) 
	  {
  	    case 'm': secs = atol(timestr)*60;break;
        case 'h': secs = atol(timestr)*3600;break;
        case 'd': secs = atol(timestr)*3600*24;break;
        default : secs = atol(timestr);break;
      } 	
    }
  who = IsServer(cptr) ? parv[3]: sptr->name;

  if(IsClient(sptr))
    {
      if (sptr->user && sptr->user->server)
        oper_server = sptr->user->server;
      else
        return 0;
	  
	  oper_name     = sptr->name;
	  oper_username = sptr->username;
	  oper_host     = sptr->realhost;
    };

  if( MyClient(sptr) )
    {
	  current_date = smalldate((time_t) 0);
	  ircsprintf(buffer, "%s (%s)", reason, current_date);
	} else
	  ircsprintf(buffer, "%s", reason);
  
  host = strchr(parv[1], '@');
  
  /* Explicit user@host mask given */
  if(host)                      /* Found user@host */
    {
      user = parv[1];   /* here is user part */
      *(host++) = '\0'; /* and now here is host */
    }
  else /* it can be a host,channel or nick */
    {
      target = parv[1];

      if(*target=='#') /* its a channel gline  */
        {
            
          chptr = hash_find_channel(target, NULL);
          if (!chptr)
            {
              sendto_one(sptr, form_str(ERR_NOSUCHCHANNEL),
                 me.name, parv[0], target);
              return(0);
            }
            
          if(IsClient(sptr))
          sendto_ops("Channel GLINE on %s from %s (%s@%s)",
            target, oper_name, oper_username, oper_host);
            
	      sendto_serv_butone(NULL,":%s GLOBOPS :Channel GLINE on %s from %s (%s@%s)",
		    me.name,
            target, oper_name, oper_username, oper_host);
          
          /* first add and propagate glines */  
          for (lp = chptr->members; lp; lp = lp->next)
            {
              acptr = lp->value.cptr;
              
              if(IsStealth(acptr) || IsAnOper(acptr))
                  continue;
	
	      user = "*";
              host = acptr->realhost;

              if (find_is_glined(host, user, NULL))
                continue;
                                
              aconf = make_conf();
              aconf->status = CONF_KILL;
  	      
              DupString(aconf->host, host);      
              DupString(aconf->passwd, buffer);
              DupString(aconf->user, user);
              DupString(aconf->name, who);
              aconf->hold = CurrentTime + secs;  
              add_gline(aconf);
              sendto_serv_butone(NULL,
  						":%s GLINE %s@%s %lu %s :%s",
                         sptr->name,
                         user,
                         host,
						 secs,
						 who,
                         reason);
	          sendto_ops("%s!%s@%s on %s added gline for \2[\2%s@%s\2]\2 \2[\2%s\2]\2 expiring on %s",
		        oper_name,
  		        oper_username,
  		        oper_host,
  		        oper_server,
  		        user,
  		        host,
  		        reason,
		        smalldate(aconf->hold));              
            }
            
  
            targets = 0;
            curr_gl = glines;
            
            /* now apply the added glines */              
            while(curr_gl)
              {
                targets += apply_gline(curr_gl->host, curr_gl->user, reason);
                curr_gl = curr_gl->next;
              }
              
            if(targets)
		      sendto_one(sptr,":%s NOTICE %s :This gline matched \2%i\2 local user(s)",
			    me.name, sptr->name, targets);
	        else
  		      sendto_one(sptr,":%s NOTICE %s :No local users matching this gline",
			    me.name, sptr->name);	  

            return 0;
        }
      else if(strchr(target,'.')) /* its a host */
        {
          user = "*";               /* no @ found, assume its *@somehost */
          host = target;          
        }
      else /* its a nick */
        {
          if((acptr = find_person(target, NULL)))
            {
              if(IsAnOper(acptr))
                return 0;
              user = "*";
              host = acptr->realhost;
            }
          else
            {
              sendto_one(sptr, form_str(ERR_NOSUCHNICK),
                me.name, parv[0], target);
              return 0;
            }
        }
    }      
    
  if (!*host)           /* duh. no host found, assume its '*' host */
    host = "*";

  strncpy_irc(tempuser, user, USERLEN + 1);     /* allow for '*' */
  tempuser[USERLEN + 1] = '\0';
  strncpy_irc(temphost, host, HOSTLEN);
  temphost[HOSTLEN] = '\0';
  user = tempuser;
  host = temphost;
					
	 /*
	  * Now we must check the user and host to make sure there
  	  * are at least NONWILDCHARS non-wildcard characters in
  	  * them, otherwise assume they are attempting to gline
      * *@* or some variant of that. This code will also catch
      * people attempting to gline *@*.tld, as long as NONWILDCHARS
      * is greater than 3. In that case, there are only 3 non-wild
      * characters (tld), so if NONWILDCHARS is 4, the gline will
      * be disallowed.
	  * -wnder
  	  */

	nonwild = 0;
	p = user;
	while ((tmpch = *p++))
	  {
		if (!IsKWildChar(tmpch))
  	  	  {
	      /*
  	       * If we find enough non-wild characters, we can
    	   * break - no point in searching further.
	       */
  			if (++nonwild >= NONWILDCHARS)
      		break;
  		  }
	  }

	if (nonwild < NONWILDCHARS)
	  {
  	   /*
    	* The user portion did not contain enough non-wild
  	    * characters, try the host.
    	*/
  		p = host;
  		while ((tmpch = *p++))
  		  {
    	    if (!IsKWildChar(tmpch))
      		  if (++nonwild >= NONWILDCHARS)
        		break;
  		  }
	  }
	  
	if (nonwild < NONWILDCHARS)
	  {
	  	  /*
		   * Not enough non-wild characters were found, assume
    	   * they are trying to gline *@*.
    	   */
  		if (MyClient(sptr))
    	  sendto_one(sptr,
      		":%s NOTICE %s :Please include at least %d non-wildcard characters with the user@host",
      		me.name,
      		parv[0],
      		NONWILDCHARS);
  		return 0;
	  }		
	
  if (find_is_glined(host, user, NULL)) {
	if(MyClient(sptr))
	    sendto_one(sptr,
		   ":%s NOTICE %s :*** Cannot add G:Line - Match already exists!", 
		   me.name, sptr->name);
	return 0;
  }
  
  aconf = make_conf();
  aconf->status = CONF_KILL;
  
	  
  DupString(aconf->host, host);      
  DupString(aconf->passwd, buffer);
  DupString(aconf->user, user);
  DupString(aconf->name, who);
  aconf->hold = CurrentTime + secs;
  
  add_gline(aconf);

  if(IsClient(sptr)) 
	{
	  
	  sendto_ops("%s!%s@%s on %s added gline for \2[\2%s@%s\2]\2 \2[\2%s\2]\2 expiring on %s",
		oper_name,
  		oper_username,
  		oper_host,
  		oper_server,
  		user,
  		host,
  		reason,
		smalldate(aconf->hold));  	
	  
	}
  targets = apply_gline(host, user, reason);
  
  if (MyClient(sptr))
	{
	  if(targets)
		sendto_one(sptr,":%s NOTICE %s :This gline matched \2%i\2 user(s)",
			me.name, sptr->name, targets);
	  else
  		sendto_one(sptr,":%s NOTICE %s :No local users matching this gline",
			me.name, sptr->name);	  
	}
	
  sendto_serv_butone(IsServer(cptr) ? cptr : NULL,
  						":%s GLINE %s@%s %lu %s :%s",
                         sptr->name,
                         user,
                         host,
						 secs,
						 who,
                         reason);

  return 0;
}

/*
 * apply_gline
 * 
 * inputs       - gline host and user
 * output       - number of local glined users
 * side effects - all users matching the gline mask will be disconnected
 *
 */
 
int apply_gline(char *host, char *user, char *reason)
  {
	register int i;
	register struct Client* acptr;
	int gcount = 0;
	for (i = 0; i <= highest_fd; i++)
  	  {

    	if (!(acptr = local[i]) || IsServer(acptr))
      	  continue;               /* and go examine next fd/cptr */

		if(user && !match(user, acptr->username))
		  continue;			  
		  
		if(host && !match(host,acptr->host) && !match(host,acptr->realhost) 
		  && !match(host,inetntoa((char *) &acptr->ip)))
		  continue;				  

		if (IsElined(acptr) || IsExemptGline(acptr))	  
		  {
      		sendto_realops("G-line exemption for %s",
          		get_client_name(acptr, FALSE));
			continue;
		  }
		  

		if(gcount<20)
    	  sendto_realops("G-line active for %s",
			  get_client_name(acptr, FALSE));		
		else if(gcount==20)
		    sendto_realops("Aborting gline target oper notice as flood prevention");
			
		sendto_one(acptr, form_str(ERR_YOUREBANNEDCREEP),
                  		  me.name, acptr->name, reason );

				  
		exit_client(acptr, acptr, &me, 
		  GLineOthersReason ? GLineOthersReason : reason);				
		  
		++gcount;
		
	  }
	  
	  return gcount;
  }

/*
 * flush_glines
 * 
 * inputs       - NONE
 * output       - NONE
 * side effects -
 *
 * Get rid of all placed G lines, hopefully to be replaced by gline.log
 * placed k-lines
 */
void flush_glines()
{
  aConfItem *kill_list_ptr;

  if((kill_list_ptr = glines))
    {
      while(kill_list_ptr)
        {
          glines = kill_list_ptr->next;
          free_conf(kill_list_ptr);
          kill_list_ptr = glines;
        }
    }
  glines = (aConfItem *)NULL;
}

/* find_gkill
 *
 * inputs       - aClient pointer to a Client struct
 * output       - aConfItem pointer if a gline was found for this client
 * side effects - none
 */

aConfItem *find_gkill(aClient* cptr, char* username)
{
  aConfItem *found;
  
  if (IsElined(cptr)||IsExemptGline(cptr))
	return (aConfItem *) NULL;

  if(*username=='~')
    ++username;
    
  found = find_is_glined(cptr->realhost, username, inetntoa((char *) &cptr->ip));
  
  if(!found)
	found = find_is_glined(cptr->host, username, 0);  

  return found;  
}

/*
 * find_is_glined
 * inputs       - hostname
 *              - username
 * output       - pointer to aConfItem if user@host glined
 * side effects -
 *  WARNING, no sanity checking on length of name,host etc.
 * thats expected to be done by caller.... *sigh* -Dianora
 */

struct ConfItem* find_is_glined(const char* host, const char* username, const char* ip)
{
  aConfItem *kill_list_ptr;     /* used for the link list only */
  aConfItem *last_list_ptr;
  aConfItem *tmp_list_ptr;

  /* gline handling... exactly like temporary klines 
   * I expect this list to be very tiny. (crosses fingers) so CPU
   * time in this, should be minimum.
   * -Dianora
  */

  if(glines)
    {
      kill_list_ptr = last_list_ptr = glines;

      while(kill_list_ptr)
        {
          if(kill_list_ptr->hold <= CurrentTime)  /* a gline has expired */
            {
              if(glines == kill_list_ptr)
                {
                  /* Its pointing to first one in link list*/
                  /* so, bypass this one, remember bad things can happen
                     if you try to use an already freed pointer.. */

                  glines = last_list_ptr = tmp_list_ptr =
                    kill_list_ptr->next;
                }
              else
                {
                  /* its in the middle of the list, so link around it */
                  tmp_list_ptr = last_list_ptr->next = kill_list_ptr->next;
                }

              free_conf(kill_list_ptr);
              kill_list_ptr = tmp_list_ptr;
            }
          else
            {
			  if(!username || (kill_list_ptr->user && match(kill_list_ptr->user, username)))
				{
				  if(ip && kill_list_ptr->host && match(kill_list_ptr->host,ip)) /* we have an ip match */	
					return(kill_list_ptr);

          		  if(!host || (kill_list_ptr->host && match(kill_list_ptr->host,host)))				  
            		return(kill_list_ptr);
				}
            	
			  last_list_ptr = kill_list_ptr;
          	  kill_list_ptr = kill_list_ptr->next;
            }
        }
    }

  return((aConfItem *)NULL);
}

/* report_glines
 *
 * inputs       - aClient pointer, match mask
 * output       - NONE
 * side effects - 
 *
 * report pending glines, and placed glines.
 * 
 * - Dianora              
 */
void report_glines(aClient *sptr, char* mask)
{
  aConfItem *kill_list_ptr;
  aConfItem *last_list_ptr;
  aConfItem *tmp_list_ptr;
  char *host;
  char *name;
  char reason[512];
  char  s[HOSTLEN + USERLEN+3];
  
  if(glines)
    {
      kill_list_ptr = last_list_ptr = glines;

      while(kill_list_ptr)
        {
          if(kill_list_ptr->hold <= CurrentTime)   /* gline has expired */
            {
              if(glines == kill_list_ptr)
                {
                  glines = last_list_ptr = tmp_list_ptr =
                  kill_list_ptr->next;
                }
              else
                {
                  /* its in the middle of the list, so link around it */
                  tmp_list_ptr = last_list_ptr->next = kill_list_ptr->next;
                }
              free_conf(kill_list_ptr);
              kill_list_ptr = tmp_list_ptr;
            }
          else
            {
            

              
              if(kill_list_ptr->host)
                host = kill_list_ptr->host;
              else
                host = "*";

              if(kill_list_ptr->user)
                name = kill_list_ptr->user;
              else
                name = "*";
                
			  ircsprintf(s,"%s@%s", name,host);
              
              if(mask && !match(mask, s))
                {
                  last_list_ptr = kill_list_ptr;
                  kill_list_ptr = kill_list_ptr->next;
                  continue;
                }
                
              if(kill_list_ptr->passwd)	
				{
				  if(strlen(kill_list_ptr->passwd) > 200)	/* just some overflow prevention */
					kill_list_ptr->passwd[200]='0';
					
				  ircsprintf(reason,"\2[\2%s\2]\2 from \2%s\2 expiring on \2%s\2",
					  kill_list_ptr->passwd, kill_list_ptr->name, smalldate(kill_list_ptr->hold));
				}
              else
                strncpy_irc(reason, "No Reason", sizeof(reason));
				

              sendto_one(sptr,form_str(RPL_STATSKLINE), me.name,
                         sptr->name, 'G' , s, "*", reason);

              last_list_ptr = kill_list_ptr;
              kill_list_ptr = kill_list_ptr->next;
            }
        }
    }
}



/* add_gline
 *
 * inputs       - pointer to aConfItem
 * output       - none
 * Side effects - links in given aConfItem into gline link list
 *
 * Identical to add_temp_kline code really.
 *
 * -Dianora
 */

void add_gline(aConfItem *aconf)
{
  aconf->next = glines;
  glines = aconf;
}

/* send_all_glines
 *
 * inputs       - pointer to aClient
 * output       - none
 * Side effects - sends all glines to the specified server
 *
 */
void send_all_glines(struct Client *acptr)
{
  aConfItem *aconf = glines;
  
  while(aconf)
	{
	  sendto_one(acptr, ":%s GLINE %s@%s %lu %s :%s",
	    me.name,
	    aconf->user, aconf->host,
	    aconf->hold - CurrentTime,
	    aconf->name,
	    aconf->passwd);
	  aconf = aconf->next;
	}
}
