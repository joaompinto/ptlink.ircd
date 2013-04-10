/************************************************************************
 *   IRC - Internet Relay Chat, src/m_stats.c
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
 *  $Id: m_stats.c,v 1.6 2005/08/27 16:23:50 jpinto Exp $
 */
#include "m_commands.h"  /* m_pass prototype */
#include "channel.h"
#include "class.h"       /* report_classes */
#include "client.h"      /* Client */
#include "common.h"      /* TRUE/FALSE */
#include "dline_conf.h"  /* report_dlines */
#include "irc_string.h"  /* strncpy_irc */
#include "ircd.h"        /* me */
#include "listener.h"    /* show_ports */
#include "msg.h"         /* Message */
#include "mtrie_conf.h"  /* report_mtrie_conf_links */
#include "m_gline.h"     /* report_glines */
#include "numeric.h"     /* ERR_xxx */
#include "scache.h"      /* list_scache */
#include "send.h"        /* sendto_one */
#include "s_bsd.h"       /* highest_fd */
#include "s_conf.h"      /* ConfItem, report_configured_links */
#include "s_debug.h"     /* send_usage */
#include "s_misc.h"      /* serv_info */
#include "s_serv.h"      /* hunt_server, show_servers */
#include "s_stats.h"     /* tstats */
#include "s_user.h"      /* show_opers */
#include "sxline.h"		 /* report_sxlines */
#include "sqline.h"		 /* report_sqlines */
#include "svline.h"		 /* report_svlines */
#include "zline.h"		 /* report_zlines */
#include "vlinks.h"		 /* report_vlinks */

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

/* for domain stats */
char* domainlist[1024];
int domainhits[1024];

/*
 * m_stats - STATS message handler
 *      parv[0] = sender prefix
 *      parv[1] = statistics selector (defaults to Message frequency)
 *      parv[2] = server name (current server defaulted, if omitted)
 *
 *      Currently supported are:
 *              M = Message frequency (the old stat behaviour)
 *              L = Local Link statistics
 *              C = Report C and N configuration lines
 *
 *
 * m_stats/stats_conf
 *    Report N/C-configuration lines from this server. This could
 *    report other configuration lines too, but converting the
 *    status back to "char" is a bit akward--not worth the code
 *    it needs...
 *
 *    Note:   The info is reported in the order the server uses
 *            it--not reversed as in ircd.conf!
 */
static const char* Lformat = ":%s %d %s %s %u %u %u %u %u :%u %u %s";

int m_stats(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  struct Message* mptr;
  struct Client*  acptr;
  char            l_stat = parc > 1 ? parv[1][0] : '\0';
  int             i;
  int             doall = 0;
  int             wilds = 0;
  int             ignore_request = 0;
  int             valid_stats = 0;
  char*           name;
  char *mename = me.name;
  
  if(sptr->user && sptr->user->vlink)
    mename = sptr->user->vlink->name;

  if(!IsClient(sptr))
    {
      return 0;
    }

  if(!IsAnOper(sptr))
    {
      sendto_one(sptr, form_str(ERR_NOPRIVILEGES), mename, parv[0]);
	  return -1;
    }

  if (hunt_server(cptr,sptr,":%s STATS %s :%s",2,parc,parv)!=HUNTED_ISME)
    return 0;

  if (parc > 2)
    {
      name = parv[2];
      if (!irccmp(name, mename))
        doall = 2;
      else if (match(name, mename))
        doall = 1;
      if (strchr(name, '*') || strchr(name, '?'))
        wilds = 1;
    }
  else
    name = mename;

  switch (l_stat)
    {
    case 'L' : case 'l' :
      /*
       * send info about connections which match, or all if the
       * mask matches mename.  Only restrictions are on those who
       * are invisible not being visible to 'foreigners' who use
       * a wild card based search to list it.
       */
      for (i = 0; i <= highest_fd; i++)
        {
          if (!(acptr = local[i]))
            continue;

			
          if (!doall && wilds && !match(name, acptr->name))
            continue;
          if (!(doall || wilds) && irccmp(name, acptr->name))
            continue;

          /* I've added a sanity test to the "CurrentTime - acptr->since"
           * occasionally, acptr->since is larger than CurrentTime.
           * The code in parse.c "randomly" increases the "since",
           * which means acptr->since is larger then CurrentTime at times,
           * this gives us very high odd number.. 
           * So, I am going to return 0 for ->since if this happens.
           * - Dianora
           */
          /* trust opers not on this server */
          /* if(IsAnOper(sptr)) */

          /* Don't trust opers not on this server */
#ifdef HIDE_SERVERS_IPS
          if(MyClient(sptr) && IsAnOper(sptr) && !IsServer(acptr) &&
	    !IsConnecting(acptr) && !IsHandshake(acptr)) 
#else
          if(MyClient(sptr) && IsAnOper(sptr))
#endif
            {
              sendto_one(sptr, Lformat, mename,
                     RPL_STATSLINKINFO, parv[0],
                     get_client_name(acptr, TRUE),
                     (int)DBufLength(&acptr->sendQ),
                     (int)acptr->sendM, (int)acptr->sendK,
                     (int)acptr->receiveM, (int)acptr->receiveK,
                     CurrentTime - acptr->firsttime,
                     (CurrentTime > acptr->since) ? (CurrentTime - acptr->since):0,
                     IsServer(acptr) ? show_capabilities(acptr) : "-");
              }
            else
              {
                if(IsServer(acptr))
                  sendto_one(sptr, Lformat, mename,
                     RPL_STATSLINKINFO, parv[0],
                     get_client_name(acptr, HIDEME),
                     (int)DBufLength(&acptr->sendQ),
                     (int)acptr->sendM, (int)acptr->sendK,
                     (int)acptr->receiveM, (int)acptr->receiveK,
                     CurrentTime - acptr->firsttime,
                     (CurrentTime > acptr->since) ? (CurrentTime - acptr->since):0,
                     IsServer(acptr) ? show_capabilities(acptr) : "-");
                 else
                  sendto_one(sptr, Lformat, mename,
                     RPL_STATSLINKINFO, parv[0],
                     (IsUpper(lstat)) ?
                     get_client_name(acptr, TRUE) :
                     get_client_name(acptr, FALSE),
                     (int)DBufLength(&acptr->sendQ),
                     (int)acptr->sendM, (int)acptr->sendK,
                     (int)acptr->receiveM, (int)acptr->receiveK,
                     CurrentTime - acptr->firsttime,
                     (CurrentTime > acptr->since) ? (CurrentTime - acptr->since):0,
                     IsServer(acptr) ? show_capabilities(acptr) : "-");
              }
        }
      valid_stats++;
      break;
    case 'C' : case 'c' :
      report_configured_links(sptr, CONF_CONNECT_SERVER|CONF_NOCONNECT_SERVER);
      valid_stats++;
      break;

    case 'B' : case 'b' :
      sendto_one(sptr,":%s NOTICE %s :Use stats I instead", mename, parv[0]);
      break;

    case 'D':
      report_dlines(sptr);
      valid_stats++;
      break;
      
    case 'd':
      report_domain_stats(sptr);
      valid_stats++;
      break;

    case 'E' : case 'e' :
      sendto_one(sptr,":%s NOTICE %s :Use stats I instead", mename, parv[0]);
      break;

    case 'F' : case 'f' :
      report_channel_stats(sptr);
      break;

    case 'g' :
      report_sxlines(sptr);
      valid_stats++;
      break;


    case 'G' :
      report_glines(sptr, NULL);
      valid_stats++;
      break;

    case 'H' : case 'h' :
      report_configured_links(sptr, CONF_HUB|CONF_LEAF);
      valid_stats++;
      break;

    case 'i' :
      report_vlinks(sptr);
      valid_stats++;
      break;

    case 'I' :
      report_mtrie_conf_links(sptr, CONF_CLIENT);
      valid_stats++;
      break;

    case 'k' :
      report_temp_klines(sptr);
      valid_stats++;
      break;

    case 'K' :
      if(parc > 3)
        report_matching_host_klines(sptr,parv[3]);
      else
        report_mtrie_conf_links(sptr, CONF_KILL);
      valid_stats++;
      break;

    case 'M' : case 'm' :
      for (mptr = msgtab; mptr->cmd; mptr++)
          sendto_one(sptr, form_str(RPL_STATSCOMMANDS),
                     mename, parv[0], mptr->cmd,
                     mptr->count, mptr->bytes);
      valid_stats++;
      break;

    case 'o' : case 'O' :
      report_configured_links(sptr, CONF_OPS);
      valid_stats++;
      break;

    case 'P' :
      show_ports(sptr);
      valid_stats++;
      break;

    case 'p' :
      show_opers(sptr);
      valid_stats++;
      break;

    case 'Q' :
      report_qlines(sptr);
      valid_stats++;
      break;

    case 'q' :
      report_sqlines(sptr);
      valid_stats++;
      break;	  

    case 'R' : case 'r' :
      send_usage(sptr,parv[0]);
      valid_stats++;
      break;

    case 'S' :
      list_scache(cptr,sptr,parc,parv);
      valid_stats++;
      break;

    case 'T' : case 't' :
      tstats(sptr, parv[0]);
      valid_stats++;
      break;

    case 'U' :
      report_specials(sptr,CONF_ULINE,RPL_STATSULINE);
      valid_stats++;
      break;

    case 'u' :
      {
        time_t now;
        
        now = CurrentTime - me.since;
        sendto_one(sptr, form_str(RPL_STATSUPTIME), mename, parv[0],
                   now/86400, (now/3600)%24, (now/60)%60, now%60);
        sendto_one(sptr, form_str(RPL_STATSCONN), mename, parv[0],
                   MaxConnectionCount, MaxClientCount,
                   Count.totalrestartcount);
        valid_stats++;
        break;
      }

    case 'V' :
      show_servers(sptr);
      valid_stats++;
      break;

    case 'v' :
      report_svlines(sptr);
      valid_stats++;
      break;
    case 'x' :
        report_specials(sptr,CONF_XLINE,RPL_STATSXLINE);
        valid_stats++;
      break;
	  
    case 'X' :        
	report_configured_links(sptr, CONF_MISSING);
        valid_stats++;
      break;;
	  

    case 'Y' : case 'y' :
        report_classes(sptr);
        valid_stats++;
      break;
	  
    case 'Z' :
      report_zlines(sptr);
      valid_stats++;
      break;

	case 'z' :
      count_memory(sptr, parv[0]);
      valid_stats++;
      break;

    case '?':
      serv_info(sptr, parv[0]);
      valid_stats++;
      break;

    default :
      l_stat = '*';
      break;
    }
  sendto_one(sptr, form_str(RPL_ENDOFSTATS), mename, parv[0], l_stat);

  /* personally, I don't see why opers need to see stats requests
   * at all. They are just "noise" to an oper, and users can't do
   * any damage with stats requests now anyway. So, why show them?
   * -Dianora
   */

  if (valid_stats)
    {
      if ( (l_stat == 'L') || (l_stat == 'l') )
        {
          sendto_ops_imodes(IMODE_SPY,
                "STATS %c requested by %s (%s@%s) [%s] on %s%s",
                l_stat,
                sptr->name,
                sptr->username,
                sptr->host,
                sptr->user->server,
                parc > 2 ? parv[2] : "<no recipient>",
                ignore_request > 0 ? " [request ignored]" : "\0"
                );
        }
      else
        {
          sendto_ops_imodes(IMODE_SPY,
                "STATS %c requested by %s (%s@%s) [%s]%s",
                l_stat,
                sptr->name,
                sptr->username,
                sptr->host,
                sptr->user->server,
                ignore_request > 0 ? " [request ignored]" : "\0"
                );
        }
    }

  return 0;
}

int report_channel_stats(struct Client *sptr)
{
  struct Channel *chptr;
  int total_opless = 0;
  int total = 0;
  int opped;

  for(chptr = channel; chptr; chptr = chptr->nextch)
  {
    struct SLink *m;
    opped = 0;
    
    for (m = chptr->members; m; m = m->next)
    {
      if(m->flags & CHFL_CHANOP)
      {
        opped++;
	break;
      }
    }

    if(!opped)
      total_opless++;

    total++;
  }

  sendto_one(sptr, ":%s NOTICE %s :Total channels: %d.  Opless channels: %d. Hacked: %d.  Ignored: %d",
             me.name, sptr->name, total, total_opless, total_hackops,
	     total_ignoreops);

  return 1;
}

void domain_stats_count(char *host)
{
 int i = 0;
 char* c = host+strlen(host)-1;
 
 if(isdigit(*host)) /* dont count ips */
   c="IP";
 else      
   while(*c!='.' && c>host) /* look for domain */
     --c;

 /* firs look if its on some list */
 while(domainlist[i] && strcasecmp(c,domainlist[i]))
   ++i;   
 if(domainlist[i])
   domainhits[i]++; /* increase hit count */
 else
   {
     domainlist[i] = strdup(c);
     domainhits[i] = 1;
     domainlist[i+1] = NULL;
   }
}

int report_domain_stats(struct Client *sptr)
{
  struct Client *acptr;
  int i=0, i2=0;
  int max, maxi;
  int tmp;
  char *stmp;  
  int total = 0; 
  
  /* first reset stats */
  while(domainlist[i])
    free(domainlist[i++]);
  domainlist[0] = NULL;
  
  /* now count */
  for (acptr = GlobalClientList; acptr; acptr = acptr->next)
    {
      if(!IsPerson(acptr))
        continue;
      domain_stats_count(acptr->host);
      total++;
    }   

  i = 0;
  /* sort the list first - selection sort */
  while(domainlist[i])
    {
      i2=i;maxi=i;
      max=domainhits[i];
      while(domainlist[i2])
        {
          if(domainhits[i2]>max)
            {
              max = domainhits[i2];
              maxi = i2;
            }
	  ++i2;
        }
      tmp=domainhits[i];
      domainhits[i]=max;
      domainhits[maxi]=tmp;
      stmp=domainlist[i];
      domainlist[i]=domainlist[maxi];
      domainlist[maxi]=stmp;
      ++i;
    }
    
  /* show time */
  sendto_one(sptr, ":%s NOTICE %s: *** Users per domain ***",
  	me.name, sptr->name);
  i=0;  
  while(domainlist[i])
    {
      char tmps[512];
      sprintf(tmps, "%5s (%4i) [%2.1f%%]",
      	domainlist[i], domainhits[i], ((float)domainhits[i]/total)*100);
      sendto_one(sptr, ":%s NOTICE %s: %s",
        me.name, sptr->name, tmps);
      ++i;
    }

  sendto_one(sptr, ":%s NOTICE %s: ************************",
    me.name, sptr->name);
}
