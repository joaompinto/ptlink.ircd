/************************************************************************
 *   IRC - Internet Relay Chat, src/s_user.c
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
 *  $Id: s_user.c,v 1.18 2005/10/16 15:01:33 jpinto Exp $
 */
#include "s_user.h"
#include "m_commands.h" 
#include "channel.h"
#include "class.h"
#include "client.h"
#include "common.h"
#include "fdlist.h"
#include "flud.h"
#include "hash.h"
#include "irc_string.h"
#include "ircd.h"
#include "list.h"
#include "listener.h"
#include "motd.h"
#include "msg.h"
#include "numeric.h"
#include "patchlevel.h"
#include "s_bsd.h"
#include "s_conf.h"
#include "s_log.h"
#include "s_serv.h"
#include "s_stats.h"
#include "scache.h"
#include "send.h"
#include "struct.h"
#include "supported.h"
#include "whowas.h"
#include "flud.h"
#include "spoof.h"
#include "throttle.h"
#include "s_conf.h"
#include "sqline.h"
#include "m_gline.h"
#include "vlinks.h"
#include "unicode.h"
#include "dconf_vars.h"

#ifdef ANTI_DRONE_FLOOD
#include "dbuf.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef PACE_WALLOPS
time_t LastUsedWallops = 0;
#endif

static int is_nnick = 0;

static int do_user (char *, aClient *, aClient*, char *, char *, char *,
                     char *, char*);

static int nickkilldone(aClient *, aClient *, int, char **, time_t, char *);
static void report_and_set_user_flags( aClient *, aConfItem * );
static int tell_user_off(aClient *,char **);

/* table of ascii char letters to corresponding bitmask */

FLAG_ITEM user_modes[] =
{ 
  {UMODE_SADMIN, 'a'},
  {UMODE_ADMIN, 'A'},
  {UMODE_BOT, 'B'},
  {UMODE_FLOODEX, 'f'},
  {UMODE_HELPER,  'h'},
  {UMODE_HIDEOPER, 'H'},
  {UMODE_INVISIBLE, 'i'},
  {UMODE_NETADMIN, 'N'},
  {UMODE_OPER, 'o'},
  {UMODE_LOCOP, 'O'},
  {UMODE_PRIVATE, 'p'},
  {UMODE_IDENTIFIED, 'r'},
  {UMODE_REGMSG, 'R'},
  {UMODE_SSL, 's'},         // UMODE_SSL patch by common
  {UMODE_STEALTH,'S'},
  {UMODE_TECHADMIN, 'T'},
  {UMODE_NODCC, 'v'},
  {UMODE_WALLOP, 'w'},
  {UMODE_SPY,'y'},
  {UMODE_ZOMBIE, 'z'},
  {0, 0}
};

/* memory is cheap. map 0-255 to equivalent mode */

int user_modes_from_c_to_bitmask[] =
{ 
  /* 0x00 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x0F */
  /* 0x10 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x1F */
  /* 0x20 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x2F */
  /* 0x30 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x3F */
  0,            /* @ */
  UMODE_ADMIN,  /* A */
  UMODE_BOT,    /* B */
  0,            /* C */
  0,            /* D */
  0,            /* E */
  0,            /* F */
  0,            /* G */
  UMODE_HIDEOPER,/* H */
  0,            /* I */
  0,            /* J */
  0,            /* K */
  0,            /* L */
  0,            /* M */
  UMODE_NETADMIN,/* N */
  UMODE_LOCOP,  /* O */
  0,            /* P */
  0,            /* Q */
  UMODE_REGMSG, /* R */
  UMODE_STEALTH,/* S */
  UMODE_TECHADMIN,/* T */
  0,            /* U */
  0,            /* V */
  0,            /* W */
  0,            /* X */
  0,            /* Y */
  0,            /* Z 0x5A */
  0, 0, 0, 0, 0, /* 0x5F */ 
  /* 0x60 */       0,
  UMODE_SADMIN, /* a */
  0,		/* b */
  0,  		/* c */
  0,  		/* d */
  0,		/* e */
  UMODE_FLOODEX, /* f */
  0,            /* g */
  UMODE_HELPER, /* h */
  UMODE_INVISIBLE, /* i */
  0,    	/* j */
  0,  		/* k */
  0,            /* l */
  0,            /* m */
  0, 		/* n */
  UMODE_OPER,   /* o */
  UMODE_PRIVATE,/* p */
  0,            /* q */
  UMODE_IDENTIFIED, /* r */
  UMODE_SSL, 		/* s */ // UMODE_SSL by common
  0,            /* t */
  0,            /* u */
  UMODE_NODCC,  /* v */
  UMODE_WALLOP, /* w */
  0, 		/* x */
  UMODE_SPY,    /* y */
  UMODE_ZOMBIE, /* z 0x7A */
  0,0,0,0,0,     /* 0x7B - 0x7F */

  /* 0x80 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x9F */
  /* 0x90 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x9F */
  /* 0xA0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xAF */
  /* 0xB0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xBF */
  /* 0xC0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xCF */
  /* 0xD0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xDF */
  /* 0xE0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xEF */
  /* 0xF0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  /* 0xFF */
};

/* internally defined functions */
unsigned long my_rand(void);    /* provided by orabidoo */

#ifdef FIZZER_CHECK
  char* fizzer_nick(void);
#endif

/*
 * show_opers - send the client a list of opers
 * inputs       - pointer to client to show opers to
 * output       - none
 * side effects - show who is opered on this server
 */
void show_opers(struct Client *cptr)
{
  register struct Client        *cptr2;
  register int j=0;

  for(cptr2 = oper_cptr_list; cptr2; cptr2 = cptr2->next_oper_client)
    {
      ++j;
      if (MyClient(cptr) && IsAnOper(cptr))
        {
          sendto_one(cptr, ":%s %d %s :[%c][%s] %s (%s@%s) Idle: %d",
                     me.name, RPL_STATSDEBUG, cptr->name,
                     IsOper(cptr2) ? 'O' : 'o',
                     oper_privs_as_string(cptr2,
                                          cptr2->confs->value.aconf->port),
                     cptr2->name,
                     cptr2->username, cptr2->host,
                     CurrentTime - cptr2->user->last);
        }
      else
        {
          sendto_one(cptr, ":%s %d %s :[%c] %s (%s@%s) Idle: %d",
                     me.name, RPL_STATSDEBUG, cptr->name,
                     IsOper(cptr2) ? 'O' : 'o',
                     cptr2->name,
                     cptr2->username, cptr2->host,
                     CurrentTime - cptr2->user->last);
        }
    }

  sendto_one(cptr, ":%s %d %s :%d OPER%s", me.name, RPL_STATSDEBUG,
             cptr->name, j, (j==1) ? "" : "s");
}

/*
 * show_lusers - total up counts and display to client
 */
int show_lusers(struct Client *cptr, struct Client *sptr, 
                int parc, char *parv[])
{
#define LUSERS_CACHE_TIME 60
  static long last_time=0;
  static int    s_count = 0, c_count = 0, u_count = 0, i_count = 0;
  static int    o_count = 0, m_client = 0, mserver = 0;
  int forced;
  struct Client *acptr;

/*  forced = (parc >= 2); */
  forced = (IsAnOper(sptr) && (parc > 3));

/* (void)collapse(parv[1]); */

  Count.unknown = 0;
  mserver = Count.myserver;
  m_client = Count.local;
  i_count  = Count.invisi;
  u_count  = Count.unknown;
  c_count  = Count.total-Count.invisi;
  s_count  = Count.server;
  o_count  = Count.oper;
  
  /* The only reason I see for some negative /lusers counts is when
	we have massive quits for +i users, in this case just show 0 */  
  if(c_count<0) 
	last_time = CurrentTime - LUSERS_CACHE_TIME - 1;
	
  if (forced || (CurrentTime > last_time+LUSERS_CACHE_TIME))
    {
      last_time = CurrentTime;
      /* only recount if more than a second has passed since last request */
      /* use LUSERS_CACHE_TIME instead... */
      s_count = 0; c_count = 0; u_count = 0; i_count = 0;
      o_count = 0; m_client = 0; mserver = 0;

      for (acptr = GlobalClientList; acptr; acptr = acptr->next)
        {
          switch (acptr->status)
            {
            case STAT_SERVER:
              if (MyConnect(acptr))
                mserver++;
            case STAT_ME:
              s_count++;
              break;
            case STAT_CLIENT:
              if (IsAnOper(acptr))
                o_count++;
#ifdef  SHOW_INVISIBLE_LUSERS
              if (MyConnect(acptr))
                m_client++;
              if (!IsInvisible(acptr))
                c_count++;
              else
                i_count++;
#else
              if (MyConnect(acptr))
                {
                  if (IsInvisible(acptr))
                    {
                      if (IsAnOper(sptr))
                        m_client++;
                    }
                  else
                    m_client++;
                }
              if (!IsInvisible(acptr))
                c_count++;
              else
                i_count++;
#endif
              break;
            default:
              u_count++;
              break;
            }
        }
      /*
       * We only want to reassign the global counts if the recount
       * time has expired, and NOT when it was forced, since someone
       * may supply a mask which will only count part of the userbase
       *        -Taner
       */
      if (!forced)
        {
          if (mserver != Count.myserver)
            {
              Count.myserver = mserver;
            }
          if (s_count != Count.server)
            {
              Count.server = s_count;
            }
          if (i_count != Count.invisi)
            {
              Count.invisi = i_count;
            }
          if ((c_count+i_count) != Count.total)
            {
              Count.total = c_count+i_count;
            }
          if (m_client != Count.local)
            {
              Count.local = m_client;
            }
          if (o_count != Count.oper)
            {
              Count.oper = o_count;
            }
          Count.unknown = u_count;
        } /* Complain & reset loop */
    } /* Recount loop */
  
#ifndef SHOW_INVISIBLE_LUSERS
  if (IsAnOper(sptr) && i_count)
#endif
    sendto_one(sptr, form_str(RPL_LUSERCLIENT), me.name, parv[0],
               c_count, i_count, s_count);
#ifndef SHOW_INVISIBLE_LUSERS
  else
    sendto_one(sptr,
               ":%s %d %s :There are %d users on %d servers", me.name,
               RPL_LUSERCLIENT, parv[0], c_count,
               s_count);
#endif
  if (o_count)
    sendto_one(sptr, form_str(RPL_LUSEROP),
               me.name, parv[0], o_count);
  if (u_count > 0)
    sendto_one(sptr, form_str(RPL_LUSERUNKNOWN),
               me.name, parv[0], u_count);
  /* This should be ok */
  if (Count.chan > 0)
    sendto_one(sptr, form_str(RPL_LUSERCHANNELS),
               me.name, parv[0], Count.chan);
  sendto_one(sptr, form_str(RPL_LUSERME),
             me.name, parv[0], m_client, mserver);
  sendto_one(sptr, form_str(RPL_LOCALUSERS), me.name, parv[0],
             Count.local, Count.max_loc);
  sendto_one(sptr, form_str(RPL_GLOBALUSERS), me.name, parv[0],
             Count.total, Count.max_tot);

  sendto_one(sptr, form_str(RPL_STATSCONN), me.name, parv[0],
             MaxConnectionCount, MaxClientCount,
             Count.totalrestartcount);
  if (m_client > MaxClientCount)
    MaxClientCount = m_client;
  if ((m_client + mserver) > MaxConnectionCount)
    {
      MaxConnectionCount = m_client + mserver;
    }

  return 0;
}


#ifdef UTF8
#define UTF8_LENGTH(Char)              \
  ((Char) < 0x80 ? 1 :                 \
   ((Char) < 0x800 ? 2 :               \
    ((Char) < 0x10000 ? 3 :            \
     ((Char) < 0x200000 ? 4 :          \
      ((Char) < 0x4000000 ? 5 : 6)))))
   
#define UNICODE_VALID(Char)                   \
    ((Char) < 0x110000 &&                     \
     (((Char) & 0xFFFFF800) != 0xD800) &&     \
     ((Char) < 0xFDD0 || (Char) > 0xFDEF) &&  \
     ((Char) & 0xFFFE) != 0xFFFE)

static char * g_utf8_find_next_char (const char *p, const char *end)
{
  if (*p)
    {
      if (end)
	for (++p; p < end && (*p & 0xc0) == 0x80; ++p)
	  ;
      else
	for (++p; (*p & 0xc0) == 0x80; ++p)
	  ;
    }
  return (p == end) ? NULL : (char *)p;
}

static inline int g_utf8_get_char_extended (const  char *p, int max_len)  
{
  unsigned int i, len;
  int wc = (unsigned char) *p;

  if (wc < 0x80)
    {
      return wc;
    }
  else if (wc < 0xc0)
    {
      return -1;
    }
  else if (wc < 0xe0)
    {
      len = 2;
      wc &= 0x1f;
    }
  else if (wc < 0xf0)
    {
      len = 3;
      wc &= 0x0f;
    }
  else if (wc < 0xf8)
    {
      len = 4;
      wc &= 0x07;
    }
  else if (wc < 0xfc)
    {
      len = 5;
      wc &= 0x03;
    }
  else if (wc < 0xfe)
    {
      len = 6;
      wc &= 0x01;
    }
  else
    {
      return -1;
    }
  
  if (max_len >= 0 && len > max_len)
    {
      for (i = 1; i < max_len; i++)
	{
	  if ((((unsigned char *)p)[i] & 0xc0) != 0x80)
	    return -1;
	}
      return -2;
    }

  for (i = 1; i < len; ++i)
    {
      int ch = ((unsigned char *)p)[i];
      
      if ((ch & 0xc0) != 0x80)
	{
	  if (ch)
	    return -1;
	  else
	    return -2;
	}

      wc <<= 6;
      wc |= (ch & 0x3f);
    }

  if (UTF8_LENGTH(wc) != len)
    return -1;
  
  return wc;
}

static int g_utf8_get_char_validated (const  char *p, int max_len)
{
  int result = g_utf8_get_char_extended (p, max_len);

  if (result & 0x80000000)
    return result;
  else if (!UNICODE_VALID (result))
    return -1;
  else
    return result;
}

#endif
  

/*
** m_functions execute protocol messages on this server:
**
**      cptr    is always NON-NULL, pointing to a *LOCAL* client
**              structure (with an open socket connected!). This
**              identifies the physical socket where the message
**              originated (or which caused the m_function to be
**              executed--some m_functions may call others...).
**
**      sptr    is the source of the message, defined by the
**              prefix part of the message if present. If not
**              or prefix not found, then sptr==cptr.
**
**              (!IsServer(cptr)) => (cptr == sptr), because
**              prefixes are taken *only* from servers...
**
**              (IsServer(cptr))
**                      (sptr == cptr) => the message didn't
**                      have the prefix.
**
**                      (sptr != cptr && IsServer(sptr) means
**                      the prefix specified servername. (?)
**
**                      (sptr != cptr && !IsServer(sptr) means
**                      that message originated from a remote
**                      user (not local).
**
**              combining
**
**              (!IsServer(sptr)) means that, sptr can safely
**              taken as defining the target structure of the
**              message in this server.
**
**      *Always* true (if 'parse' and others are working correct):
**
**      1)      sptr->from == cptr  (note: cptr->from == cptr)
**
**      2)      MyConnect(sptr) <=> sptr == cptr (e.g. sptr
**              *cannot* be a local connection, unless it's
**              actually cptr!). [MyConnect(x) should probably
**              be defined as (x == x->from) --msa ]
**
**      parc    number of variable parameter strings (if zero,
**              parv is allowed to be NULL)
**
**      parv    a NULL terminated list of parameter pointers,
**
**                      parv[0], sender (prefix string), if not present
**                              this points to an empty string.
**                      parv[1]...parv[parc-1]
**                              pointers to additional parameters
**                      parv[parc] == NULL, *always*
**
**              note:   it is guaranteed that parv[0]..parv[parc-1] are all
**                      non-NULL pointers.
*/

/*
 * clean_nick_name - ensures that the given parameter (nick) is
 * really a proper string for a nickname (note, the 'nick'
 * may be modified in the process...)
 *
 *      RETURNS the length of the final NICKNAME (0, if
 *      nickname is illegal)
 *
 *  Nickname characters are in range
 *      'A'..'}', '_', '-', '0'..'9'
 *  anything outside the above set will terminate nickname.
 *  In addition, the first character cannot be '-'
 *  or a Digit.
 *
 *  Note:
 *      '~'-character should be allowed, but
 *      a change should be global, some confusion would
 *      result if only few servers allowed it...
 */
static int clean_nick_name(char* nick)
{
#ifdef KOREAN
  enum { ASCII, HANGUL_BYTE_1, HANGUL_BYTE_2 } status = ASCII;
#endif /* KOREAN */
#ifdef UTF8
  int unichar;
#endif
  char* ch   = nick;
  char* endp = ch + NICKLEN;
  assert(0 != nick);

  if (*nick == '-' || IsDigit(*nick)) /* first character in [0..9-] */
    return 0;

  for ( ; ch < endp && *ch; ++ch) {
    if (!IsNickChar(*ch))
      break;
  }
  *ch = '\0';

#ifdef KOREAN
  for (ch = nick; *ch; ch++) {
    if (!IsHangul(*ch))
	status = ASCII;
    else if (status == HANGUL_BYTE_1) {
	unsigned short hcode = ((((unsigned char)(*(ch-1))) << 8) | ((unsigned char)(*ch)));
	status = HANGUL_BYTE_2;

	/* if the hangul code is a special characters which supposed
	 not to be used in the nick, remove it. */
	if (hcode >= 0xa1a1 && hcode <= 0xacfe) {
	ch--;
	break;
      }
    }
    else
      status = HANGUL_BYTE_1;
  }

  /* if the last char is broken 1-byte hangul code, remove it */
  if (status == HANGUL_BYTE_1)
    ch--;

  *ch = '\0';
#endif /* KOREAN */
#ifdef UTF8
  ch = nick;

  while (ch < endp && *ch) {
    if ((unsigned char)*ch < 0x80) {
      ch ++;
      continue;
    }
    if (ch > nick && g_utf8_find_next_char(ch-1,NULL) == NULL)
      break;
    unichar = g_utf8_get_char_validated(ch,endp-ch);
    if (unichar == -1)
      break;
    ch +=  UTF8_LENGTH(unichar);
  }
  *ch = '\0';
#endif

  return (ch - nick);
}


/*
 * show_isupport
 *
 * inputs       - pointer to client
 * output       -
 * side effects - display to client what we support (for them)
 */
int show_isupport(struct Client *sptr)
{
  static char isupportbuffer[512];
  static char isupportbuffer2[512];
  char aux[128];
  static int is_defined = 0;
	
  if(!is_defined)
	{
		ircsprintf(isupportbuffer,FEATURES,FEATURESVALUES);
		ircsprintf(aux," NETWORK=%s MAXCHANNELS=%i ", NetworkName, MaxChansPerUser);
		strcat(isupportbuffer,aux);
          if(AdminWithDot)
#ifdef HALFOPS
            strcat(isupportbuffer,"PREFIX=(aohv).@%+");
#else
            strcat(isupportbuffer,"PREFIX=(aov).@+");
#endif                        
          else
#ifdef HALFOPS
            strcat(isupportbuffer,"PREFIX=(ohv)@%+");
#else            
            strcat(isupportbuffer,"PREFIX=(ov)@+");
#endif            
        ircsprintf(isupportbuffer2, "CODPAGE=%s", codepage_list());
		is_defined = -1;
	}

  sendto_one(sptr, form_str(RPL_ISUPPORT), me.name, sptr->name,
             isupportbuffer);
             
  if(CodePages)             
  sendto_one(sptr, form_str(RPL_ISUPPORT), me.name, sptr->name,
             isupportbuffer2);             
 
  return 0;
}   

/*
** register_user
**      This function is called when both NICK and USER messages
**      have been accepted for the client, in whatever order. Only
**      after this, is the USER message propagated.
**
**      NICK's must be propagated at once when received, although
**      it would be better to delay them too until full info is
**      available. Doing it is not so simple though, would have
**      to implement the following:
**
**      (actually it has been implemented already for a while) -orabidoo
**
**      1) user telnets in and gives only "NICK foobar" and waits
**      2) another user far away logs in normally with the nick
**         "foobar" (quite legal, as this server didn't propagate
**         it).
**      3) now this server gets nick "foobar" from outside, but
**         has already the same defined locally. Current server
**         would just issue "KILL foobar" to clean out dups. But,
**         this is not fair. It should actually request another
**         nick from local user or kill him/her...
*/

static int register_user(aClient *cptr, aClient *sptr, 
                         char *nick, char *username)
{
  aClient*    acptr;
  aConfItem*  aconf;
  char*       parv[3];
  static char ubuf[12];
  anUser*     user = sptr->user;
  char*       reason;
  char        tmpstr2[512];
  char 		  saveip[HOSTLEN];
  char*       mename = me.name;

  assert(0 != sptr);
  assert(sptr->username != username);
  
  if(user->vlink)
    mename = user->vlink->name;
    
  user->last = CurrentTime;
  parv[0] = sptr->name;
  parv[1] = parv[2] = NULL;

  /* pointed out by Mortiis, never be too careful */
  if(strlen(username) > USERLEN)
    username[USERLEN] = '\0';

  reason = NULL;

#define NOT_AUTHORIZED  	(-1)
#define SOCKET_ERROR    	(-2)
#define I_LINE_FULL     	(-3)
#define I_LINE_FULL2    	(-4) /* Too many connections from hostname */
#define BANNED_CLIENT   	(-5)
#define THROTTLED_CLIENT	(-6)



  if (MyConnect(sptr))
    {
      if (IsWebchat(sptr))
		{
		  strncpy_irc(saveip, sptr->host, HOSTLEN); /* save ip received from web */
		  
		  if(HostSpoofing && !(sptr->listener->options & LST_NOSPOOF))
  			strcpy(sptr->host,spoofed(sptr));
		
		  if (IPIdentifyMode)
			strncpy_irc(sptr->realhost, saveip, HOSTLEN); /* restore ip received from web */
		}	
  	  else
		{
		  strncpy_irc(sptr->realhost, sptr->host, HOSTLEN); /* save the real hostname */

		  if(HostSpoofing && !(sptr->listener->options & LST_NOSPOOF))
  			strcpy(sptr->host,spoofed(sptr));						
		  
		  if(IPIdentifyMode)
		    {
#ifdef IPV6
     			inetntop(AFINET, &sptr->ip, sptr->realhost, HOSTIPLEN);
#else
     			strncpy_irc(sptr->realhost, inetntoa((char*) &sptr->ip), HOSTLEN);
#endif		  
		    }
		}
	
      switch( check_client(sptr,username,&reason))
        {
        case SOCKET_ERROR:
          return exit_client(cptr, sptr, &me, "Socket Error");
          break;

        case I_LINE_FULL:
        case I_LINE_FULL2:
          sendto_ops_imodes(IMODE_FULL, "%s for %s (%s).",
                               "I-line is full",
                               get_client_host(sptr),
                               sptr->realhost);
          irclog(L_INFO,"Too many connections from %s.", get_client_host(sptr));
          ServerStats->is_ref++;
          return exit_client(cptr, sptr, &me, 
                 "No more connections allowed in your connection class" );
          break;

        case NOT_AUTHORIZED:

#ifdef REJECT_HOLD

          /* Slow down the reconnectors who are rejected */
          if( (reject_held_fds != REJECT_HELD_MAX ) )
            {
              SetRejectHold(cptr);
              reject_held_fds++;
              release_client_dns_reply(cptr);
              return 0;
            }
          else
#endif
            {
              ServerStats->is_ref++;
	/* jdc - lists server name & port connections are on */
	/*       a purely cosmetical change */
              sendto_ops_imodes(IMODE_CLIENTS,
				 "%s from %s [%s] on [%s/%u].",
                                 "Unauthorized client connection",
                                 get_client_host(sptr),
                                 sptr->realhost,
				 sptr->listener->name,
				 sptr->listener->port
				 );
              irclog(L_INFO,
		  "Unauthorized client connection from %s on [%s/%u].",
                  get_client_host(sptr),
		  sptr->listener->name,
		  sptr->listener->port
		  );

              ServerStats->is_ref++;
              ircsprintf(tmpstr2,"You are not authorized to use this server. Please try %s",
              			RandomHost);
						
  		if(RandomHost)
        		sendto_one(cptr, form_str(RPL_REDIR),
                         mename, cptr->name,
                         RandomHost, 6667);
	      //remove_clone_check(cptr);					
              return exit_client(cptr, sptr, &me, tmpstr2);
            }
          break;

        case BANNED_CLIENT:
          {
            if (!IsGotId(sptr))
              {
                if (IsNeedId(sptr))
                  {
                    *sptr->username = '~';
                    strncpy_irc(&sptr->username[1], username, USERLEN - 1);
                  }
                else
                  strncpy_irc(sptr->username, username, USERLEN);
                sptr->username[USERLEN] = '\0';
              }

            if ( tell_user_off( sptr, &reason ))
              {
                ServerStats->is_ref++;
                return exit_client(cptr, sptr, &me, "Banned" );
              }
            else
              return 0;

            break;
          }
		  
		case THROTTLED_CLIENT:
          {
            if (!IsGotId(sptr))
              {
                if (IsNeedId(sptr))
                  {
                    *sptr->username = '~';
                    strncpy_irc(&sptr->username[1], username, USERLEN - 1);
                  }
                else
                  strncpy_irc(sptr->username, username, USERLEN);
                sptr->username[USERLEN] = '\0';
              }
			  reason = "Too many fast connections from your host, please wait some minutes and try again";
            if ( tell_user_off( sptr, &reason ))
              {
                ServerStats->is_ref++;
                return exit_client(cptr, sptr, &me, "Banned" );
              }
            else
              return 0;

            break;
          }

		  
        default:
          release_client_dns_reply(cptr);
          break;
        }
      if(!valid_hostname(sptr->host))
        {
          sendto_one(sptr,":%s NOTICE %s :*** Notice -- You have an illegal character in your hostname", 
                     mename, sptr->name );

          strncpy_irc(sptr->host,sptr->sockhost,HOSTIPLEN+1);
        }
		
					  
      aconf = sptr->confs->value.aconf;
      if (!aconf)
        return exit_client(cptr, sptr, &me, "*** Not Authorized");
      if (!IsGotId(sptr))
        {
          if (IsNeedIdentd(aconf))
            {
              ServerStats->is_ref++;
              sendto_one(sptr,
 ":%s NOTICE %s :*** Notice -- You need to install identd to use this server",
                         mename, cptr->name);
               return exit_client(cptr, sptr, &me, "Install identd");
             }
           if (IsNoTilde(aconf) || !CheckIdentd)
             {
                strncpy_irc(sptr->username, username, USERLEN);
             }
           else
             {
                *sptr->username = '~';
                strncpy_irc(&sptr->username[1], username, USERLEN - 1);
             }
           sptr->username[USERLEN] = '\0';
        }

      /* password check */
#ifdef CRYPT_AUTH_PASSWORD
      if (!BadPtr(aconf->passwd) && 0 != strcmp(crypt(sptr->passwd,
                                   aconf->passwd), aconf->passwd))
#else      
      if (!BadPtr(aconf->passwd) && 0 != strcmp(sptr->passwd, aconf->passwd))
#endif      
        {
          ServerStats->is_ref++;
          sendto_one(sptr, form_str(ERR_PASSWDMISMATCH),
                     mename, parv[0]);
          return exit_client(cptr, sptr, &me, "Bad Password");
        }

/* this user uses ssl ? */
        if ( IsSecure(sptr) )
        {
            SetSsl(sptr);
        }else
        {
            ClearSsl(sptr);
        }



      /* report if user has &^>= etc. and set flags as needed in sptr */
      report_and_set_user_flags(sptr, aconf);

      /* Limit clients */
      /*
       * We want to be able to have servers and F-line clients
       * connect, so save room for "buffer" connections.
       * Smaller servers may want to decrease this, and it should
       * probably be just a percentage of the MAXCLIENTS...
       *   -Taner
       */
      /* Except "F:" clients */
      if ( (((Count.local + 1) >= (MAXCLIENTS+MAX_BUFFER))) ||
            (((Count.local +1) >= (MAXCLIENTS - 5)) && !(IsFlined(sptr))))
        {
          sendto_ops_imodes(IMODE_FULL,
                               "Too many clients, rejecting %s[%s].",
                               nick, sptr->host);
          ServerStats->is_ref++;
          ircsprintf(tmpstr2, "Sorry, server is full - try %s", RandomHost);
		  
  		  if(RandomHost)
        	sendto_one(cptr, form_str(RPL_REDIR),
                         mename, cptr->name,
                         RandomHost, 6667);

  	      remove_clone_check(cptr);                         
		  
          return exit_client(cptr, sptr, &me, tmpstr2);
        }

      /* valid user name check */

      if (!valid_username(sptr->username))
        {
          sendto_ops_imodes(IMODE_REJECTS,"Invalid username: %s (%s@%s)",
                             nick, sptr->username, sptr->host);
          strcpy(sptr->username, "invalid");
#if 0                           
          ServerStats->is_ref++;
          ircsprintf(tmpstr2, "Invalid username [%s]", sptr->username);
          return exit_client(cptr, sptr, &me, tmpstr2);
#endif          
        }
      /* end of valid user name check */

      if(!IsAnOper(sptr))
        {
          char *rreason;

          if ( (aconf = find_special_conf(sptr->info,CONF_XLINE)))
            {
              if(aconf->passwd)
                rreason = aconf->passwd;
              else
                rreason = "NONE";
              
              if(aconf->port)
                {
                  if (aconf->port == 1)
                    {
                      sendto_ops_imodes(IMODE_REJECTS,
                                           "X-line Rejecting [%s] [%s], user %s",
                                           sptr->info,
                                           rreason,
                                           get_client_name(cptr, FALSE));
                    }
                  ServerStats->is_ref++;      
                  return exit_client(cptr, sptr, &me, "Bad user info");
                }
              else
                sendto_ops_imodes(IMODE_REJECTS,
                                   "X-line Warning [%s] [%s], user %s",
                                   sptr->info,
                                   rreason,
                                   get_client_name(cptr, FALSE));
            }
         }
#ifndef NO_DEFAULT_INVISIBLE
      sptr->umodes |= UMODE_INVISIBLE;
#endif

      sendto_ops_imodes(IMODE_CLIENTS,
                         "Client connecting: %s (%s@%s) [%s] {%d}",
                         nick, sptr->username, sptr->host,
                         sptr->realhost,
                         get_client_class(sptr));

      if(IsInvisible(sptr))
        Count.invisi++;
					 
  	  sendto_channel_butserv(lch_connects, &me,
  		":%s PRIVMSG &Connects :Client >> %s (%s@%s) [%s] {%d}",
						 mename, 
						 nick, sptr->username, sptr->host,
                         sptr->realhost,
                         get_client_class(sptr));
                 
      if ((++Count.local) > Count.max_loc)
        {
          Count.max_loc = Count.local;
          if (!(Count.max_loc % 10))
            sendto_ops("New Max Local Clients: %d",
                       Count.max_loc);
        }
    }
  else
    strncpy_irc(sptr->username, username, USERLEN);

  SetClient(sptr);

  sptr->servptr = find_server(user->server);
  if (!sptr->servptr)
    {
      sendto_realops("Ghost killed: %s on invalid server %s",
                 sptr->name, sptr->user->server);
      sendto_one(cptr,":%s KILL %s :%s (Ghosted, %s doesn't exist)",
                 mename, sptr->name, mename, user->server);
      sptr->flags |= FLAGS_KILLED;
      return exit_client(NULL, sptr, &me, "Ghost");
    }
  add_client_to_llist(&(sptr->servptr->serv->users), sptr);

/* Increment our total user count here */
  if (++Count.total > Count.max_tot)
    Count.max_tot = Count.total;

  if (MyConnect(sptr))
    {
      int cpidx;
      
      /* set codepage according to the listener */
      cpidx = sptr->listener->codepage+1;
            
      /* codepage found - attach the user and re-translate the realname */
      if (cpidx) 
        {
          char *translated;
                       
          cps[cpidx - 1]->refcount++;                       
          if ((translated = codepage_to_unicode(sptr->info, cpidx - 1)) != sptr->info) 
            {
              strncpy(sptr->info, translated, sizeof(sptr->info));                  
              sptr->info[sizeof(sptr->info) - 1] = '\0';          
            }
          sptr->codepage = cpidx;
        } 
  
      if(hvc_enabled)
        cptr->hvc = (random()%998)+1;
      else
        sendto_one(sptr, form_str(RPL_WELCOME), mename, nick, NetworkName, nick);

      /* This is a duplicate of the NOTICE but see below...*/      
      if(user->vlink)      
        sendto_one(sptr, form_str(RPL_YOURHOST), mename, nick,
                 mename, SHORT_VERSION);
      else
        sendto_one(sptr, form_str(RPL_YOURHOST), mename, nick,
                 get_listener_name(sptr->listener), SHORT_VERSION);                 
      
      /*
      ** Don't mess with this one - IRCII needs it! -Avalon
      */
      if(user->vlink)
        sendto_one(sptr,
                 "NOTICE %s :*** Your host is %s, running version %s",
                 nick, mename, SHORT_VERSION);      
      else
        sendto_one(sptr,
                 "NOTICE %s :*** Your host is %s, running version %s",
                 nick, get_listener_name(sptr->listener), SHORT_VERSION);
      
      sendto_one(sptr, form_str(RPL_CREATED),mename,nick,creation);


      if (!(sptr->listener->options & LST_JAVACR))
        sendto_one(sptr, form_str(RPL_MYINFO), mename, parv[0],
                 mename, SHORT_VERSION);
                 
      /* Increment the total number of clients since (re)start */
      Count.totalrestartcount++;
	  show_isupport(sptr);
      show_lusers(sptr, sptr, 1, parv);
  if(!hvc_enabled)
  {
#ifdef SHORT_MOTD
      sendto_one(sptr,"NOTICE %s :*** Notice -- motd was last changed at %s",
                 sptr->name,
                 ConfigFileEntry.motd.lastChangedDate);

      sendto_one(sptr,
                 "NOTICE %s :*** Notice -- Please read the motd if you haven't read it",
                 sptr->name);
      
      sendto_one(sptr, form_str(RPL_MOTDSTART),
                 mename, sptr->name, mename);
      
      sendto_one(sptr,
                 form_str(RPL_MOTD),
                 mename, sptr->name,
                 "*** This is the short motd ***"
                 );

      sendto_one(sptr, form_str(RPL_ENDOFMOTD),
                 mename, sptr->name);
#else
  if (!IsWebchat(sptr))
       SendMessageFile(sptr, FindMotd(sptr->host));
  else
      SendMessageFile(sptr, &ConfigFileEntry.wmotd);	  
#endif
    if(NetworkAUP) 
    	sendto_one(sptr, ":%s NOTICE %s :%s", mename, sptr->name, NetworkAUP);
    if (IsSsl(sptr))    // ssl patch by common
        sendto_one(sptr,             ":%s NOTICE %s :*** You are using a SSL Connection. Nice Idea.",
                                  mename,sptr->name);      

  }

#ifdef LITTLE_I_LINES
      if(sptr->confs && sptr->confs->value.aconf &&
         (sptr->confs->value.aconf->flags
          & CONF_FLAGS_LITTLE_I_LINE))
        {
          SetRestricted(sptr);
          sendto_one(sptr,"NOTICE %s :*** Notice -- You are in a restricted access mode",nick);
          sendto_one(sptr,"NOTICE %s :*** Notice -- You can not chanop others",nick);
        }
#endif

#ifdef NEED_SPLITCODE
      if (server_was_split)
        {
          sendto_one(sptr,"NOTICE %s :*** Notice -- server is currently in split-mode",nick);
        }

      nextping = CurrentTime;
#endif


    }
  else if (IsServer(cptr))
    {
      aClient *acptr2;
      if ((acptr2 = find_server(user->server)) && acptr2->from != sptr->from)
        {
          sendto_ops_imodes(IMODE_DEBUG, 
                             "Bad User [%s] :%s USER %s@%s %s, != %s[%s]",
                             cptr->name, nick, sptr->username,
                             sptr->host, user->server,
                             acptr2->name, acptr2->from->name);
          sendto_one(cptr,
                     ":%s KILL %s :%s (%s != %s[%s] USER from wrong direction)",
                     me.name, sptr->name, me.name, user->server,
                     acptr2->from->name, acptr2->from->host);
          sptr->flags |= FLAGS_KILLED;
          return exit_client(sptr, sptr, &me,
                             "USER server wrong direction");
          
        }
      /*
       * Super GhostDetect:
       *        If we can't find the server the user is supposed to be on,
       * then simply blow the user away.        -Taner
       */
      if (!acptr2)
        {
          sendto_one(cptr,
                     ":%s KILL %s :%s GHOST (no server %s on the net)",
                     me.name,
                     sptr->name, me.name, user->server);
          sendto_realops("No server %s for user %s[%s@%s] from %s",
                          user->server, sptr->name, sptr->username,
                          sptr->host, sptr->from->name);
          sptr->flags |= FLAGS_KILLED;
          return exit_client(sptr, sptr, &me, "Ghosted Client");
        }
    }

  if(MyClient(sptr))
    send_umode(sptr, sptr, 0, USER_UMODES, ubuf);
  else
    send_umode(NULL, sptr, 0, USER_UMODES, ubuf);

  
  if (!*ubuf)
    {
      ubuf[0] = '+';
      ubuf[1] = '\0';
    }

  
  /* LINKLIST 
   * add to local client link list -Dianora
   * I really want to move this add to link list
   * inside the if (MyConnect(sptr)) up above
   * but I also want to make sure its really good and registered
   * local client
   *
   * double link list only for clients, traversing
   * a small link list for opers/servers isn't a big deal
   * but it is for clients -Dianora
   */

  if (MyConnect(sptr))
    {
      if(local_cptr_list)
        local_cptr_list->previous_local_client = sptr;
      sptr->previous_local_client = (aClient *)NULL;
      sptr->next_local_client = local_cptr_list;
      local_cptr_list = sptr;
    }
	
  if (IsService(sptr->servptr))
    SetService(sptr);
		
  hash_check_watch(sptr, RPL_LOGON); 		
  
  if(sptr->user->vlink)
    sendto_serv_butone(cptr, "%s %s %d %lu %s %s %s %s %s!%s :%s",
      is_nnick ? "NNICK" : "NICK",
      sptr->name, sptr->hopcount+1, sptr->tsinfo, ubuf,
      sptr->username, sptr->realhost, sptr->host,
      sptr->user->server, sptr->user->vlink->name, sptr->info);
  else
    sendto_serv_butone(cptr, "%s %s %d %lu %s %s %s %s %s :%s",
      is_nnick ? "NNICK" : "NICK",
      sptr->name, sptr->hopcount+1, sptr->tsinfo, ubuf,
      sptr->username, sptr->realhost, sptr->host,
      sptr->user->server, sptr->info);

  if (MyConnect(sptr)&& !BadPtr(sptr->passwd))
    {
      if((acptr = find_server(ServicesServer)))
        sendto_one(acptr, ":%s PRIVMSG NickServ@%s :IDENTIFY %s",
       	  parv[0], ServicesServer, sptr->passwd);
      if (MyConnect(sptr))
      memset(sptr->passwd,0, sizeof(sptr->passwd));       	  
    }
    
  if(MyConnect(sptr) && hvc_enabled)
      hvc_send_code(cptr);
  else
  if(MyConnect(sptr) && AutoJoinChan)
    {
      char* ajc = strdup(AutoJoinChan); /* we need duplicate because m_join will break it - Issue 164 */
      parv[1] = sptr->name;
      parv[2] = ajc;
      (void) m_join(cptr, sptr, 2, &parv[1]);
      free(ajc);
    }            	
  return 0;
}

/* 
 * valid_hostname - check hostname for validity
 *
 * Inputs       - pointer to user
 * Output       - YES if valid, NO if not
 * Side effects - NONE
 *
 * NOTE: this doesn't allow a hostname to begin with a dot and
 * will not allow more dots than chars.
 */
int valid_hostname(const char* hostname)
{
  int         dots  = 0;
  int         chars = 0;
  const char* p     = hostname;

  assert(0 != p);

  if ('.' == *p)
    return NO;

  while (*p) {
    if (!IsHostChar(*p))
      return NO;
    if ('.' == *p || ':' == *p)
    {
      ++p;
      ++dots;
    }
    else
    {
      ++p;
      ++chars;
    }
  }
  return ( 0 == dots || chars < dots) ? NO : YES;
}

/* 
 * valid_username - check username for validity
 *
 * Inputs       - pointer to user
 * Output       - YES if valid, NO if not
 * Side effects - NONE
 * 
 * Absolutely always reject any '*' '!' '?' '@' '.' in an user name
 * reject any odd control characters names.
 */
int valid_username(const char* username)
{
  const char *p = username;
  assert(0 != p);

  if (*p=='\0')
  	return NO;  

  if ('~' == *p)
    ++p;
        
  while (*p) 
	{
  	  if (!IsUserChar(*p))
    	return NO;
		
	  ++p;
  }
  return YES;
}

/* 
 * tell_user_off
 *
 * inputs       - client pointer of user to tell off
 *              - pointer to reason user is getting told off
 * output       - drop connection now YES or NO (for reject hold)
 * side effects -
 */

static int
tell_user_off(aClient *cptr, char **preason )
{
#ifdef KLINE_WITH_REASON
  char* p = 0;
#endif /* KLINE_WITH_REASON */
  char *mename = me.name;

  if(cptr->user && cptr->user->vlink)
    mename = cptr->user->vlink->name;
  /* Ok... if using REJECT_HOLD, I'm not going to dump
   * the client immediately, but just mark the client for exit
   * at some future time, .. this marking also disables reads/
   * writes from the client. i.e. the client is "hanging" onto
   * an fd without actually being able to do anything with it
   * I still send the usual messages about the k line, but its
   * not exited immediately.
   * - Dianora
   */
            
#ifdef REJECT_HOLD
  if( (reject_held_fds != REJECT_HELD_MAX ) )
    {
      SetRejectHold(cptr);
      reject_held_fds++;
#endif

#ifdef KLINE_WITH_REASON
      if(*preason)
        {
          if(( p = strchr(*preason, '|')) )
            *p = '\0';

		  sendto_one(cptr, form_str(ERR_YOUREBANNEDCREEP),
                  		  mename, cptr->name, *preason );
/*
          sendto_one(cptr, ":%s NOTICE %s :*** Banned: %s",
                     mename,cptr->name,*preason);
*/            
          if(p)
            *p = '|';
        }
      else
#endif
	    sendto_one(cptr, form_str(ERR_YOUREBANNEDCREEP),
                  		  mename, cptr->name,"No reason");
/*
        sendto_one(cptr, ":%s NOTICE %s :*** Banned: No Reason",
                   mename,cptr->name);
*/
#ifdef REJECT_HOLD
      return NO;
    }
#endif

  return YES;
}

/* report_and_set_user_flags
 *
 * Inputs       - pointer to sptr
 *              - pointer to aconf for this user
 * Output       - NONE
 * Side effects -
 * Report to user any special flags they are getting, and set them.
 */

static void 
report_and_set_user_flags(aClient *sptr,aConfItem *aconf)
{
  char *mename = me.name;

  if(sptr->user && sptr->user->vlink)
    mename = sptr->user->vlink->name;


  /* If this user is in the exception class, Set it "E lined" */
  if(IsConfElined(aconf))
    {
      SetElined(sptr);
      sendto_one(sptr,
         ":%s NOTICE %s :*** You are exempt from K/D/G lines. congrats.",
                 mename,sptr->name);
    }

  else if(IsConfExemptGline(aconf))
    {
      SetExemptGline(sptr);
      sendto_one(sptr,
         ":%s NOTICE %s :*** You are exempt from G lines. congrats.",
                 mename,sptr->name);
    }

  /* If this user is exempt from user limits set it F lined" */
  if(IsConfFlined(aconf))
    {
      SetFlined(sptr);
      sendto_one(sptr,
                 ":%s NOTICE %s :*** You are exempt from user limits. congrats.",
                 mename,sptr->name);
    }
#ifdef IDLE_CHECK
  /* If this user is exempt from idle time outs */
  if(IsConfIdlelined(aconf))
    {
      SetIdlelined(sptr);
      sendto_one(sptr,
         ":%s NOTICE %s :*** You are exempt from idle limits. congrats.",
                 mename,sptr->name);
    }
#endif
}

/*
 * nickkilldone
 *
 * input        - pointer to physical aClient
 *              - pointer to source aClient
 *              - argument count
 *              - arguments
 *              - newts time
 *              - nick
 * output       -
 * side effects -
 */

static int nickkilldone(aClient *cptr, aClient *sptr, int parc,
                 char *parv[], time_t newts,char *nick)
{

  Link *lp;
  int real_nick_change=irccmp(parv[0], nick); /* is this a real or just case nick change ? */
	
  if (IsServer(sptr))
    {
      /* A server introducing a new client, change source */
      
      sptr = make_client(cptr);
      add_client_to_list(sptr);         /* double linked list */
      if (parc > 2)
        sptr->hopcount = atoi(parv[2]);
      if (newts)
        sptr->tsinfo = newts;
      else
        {
          newts = sptr->tsinfo = CurrentTime;
          ts_warn("Remote nick %s (%s) introduced without a TS", nick, parv[0]);
        }
      (void)strcpy(sptr->name, nick);
      (void)strncpy_irc(sptr->name, nick, NICKLEN);	  
      (void)add_to_client_hash_table(nick, sptr);
      if (parc > 8)
        {
          int   flag;
          char* m;
          
          /*
          ** parse the usermodes -orabidoo
          */
          m = &parv[4][1];
          while (*m)
            {
              flag = user_modes_from_c_to_bitmask[(unsigned char)*m];
              if( flag & UMODE_INVISIBLE )
                {
                  Count.invisi++;
                }
              if( flag & UMODE_OPER )
                {
                  Count.oper++;
                }
              sptr->umodes |= flag & ALL_UMODES;
              m++;
            }
          return do_user(nick, cptr, sptr, parv[5], 
						 parv[6], parv[7],
                         parv[8], parv[9]);
        }
    }
  else if (sptr->name[0])
    {
      /*
      ** Client just changing his/her nick. If he/she is
      ** on a channel, send note of change to all clients
      ** on that channel. Propagate notice to other servers.
      */
      if (real_nick_change) 
      	  sptr->tsinfo = newts ? newts : CurrentTime;

      if(MyConnect(sptr) && IsRegisteredUser(sptr))
        {     

          if(LockNickChange)
            {
		sendto_one(sptr,
                  ":%s NOTICE %s :*** Nick changes are not allowed on this server.",
                         me.name,
                         sptr->name);
                return 0;
            }
               /* before we change their nick, make sure they're not banned
                * on any channels, and!! make sure they're not changing to
                * a banned nick -sed */
               /* a little cleaner - lucas */
 
                for (lp = sptr->user->channel; lp; lp = lp->next)
                {
                  if ((is_banned(sptr, lp->value.chptr) == CHFL_BAN))
                  {
                    sendto_one(sptr, form_str(ERR_BANONCHAN), me.name,
                               sptr->name, nick, lp->value.chptr->chname);
                    return 0;
                  }
                  if ((nick_is_banned(lp->value.chptr, nick, sptr))
#ifdef HALFOPS
                        && !(lp->flags & (CHFL_CHANOP | CHFL_VOICE | CHFL_HALFOP)))
#else
                  	&& !(lp->flags & (CHFL_CHANOP | CHFL_VOICE)))                  
#endif                  	
                  {
                    sendto_one(sptr, form_str(ERR_BANNICKCHANGE), me.name,
                               sptr->name, lp->value.chptr->chname);
                    return 0;
                  }
                  if((lp->value.chptr->mode.mode & MODE_NONICKCH) && 
#ifdef HALFOPS
                    !is_chan_op(sptr, lp->value.chptr) && !has_voice(sptr, lp->value.chptr) && !has_halfop(sptr, lp->value.chptr) && !IsOper(sptr))
#else
                    !is_chan_op(sptr, lp->value.chptr) && !has_voice(sptr, lp->value.chptr) && !IsOper(sptr)) 
#endif                    
		  {
		    sendto_one(sptr, ":%s NOTICE %s :*** Cannot change nickname. "
		     "%s is in No Nickname Change Mode [+N].", me.name, parv[0],
		      lp->value.chptr->chname);
		    return 0;
		  }
		}		
#ifdef ANTI_NICK_FLOOD

          if( (sptr->last_nick_change + MAX_NICK_TIME) < CurrentTime)
            sptr->number_of_nick_changes = 0;
            
          sptr->last_nick_change = CurrentTime;
          sptr->number_of_nick_changes++;

          if(sptr->number_of_nick_changes <= MAX_NICK_CHANGES)
            {
#endif
			  if( IsIdentified(sptr) && real_nick_change )
				{
				  if(IsRegMsg(sptr))
					sendto_one(sptr,":%s MODE %s -rR",sptr->name,sptr->name);
				  else
					sendto_one(sptr,":%s MODE %s -r",sptr->name,sptr->name);
					
				  ClearIdentified(sptr);
				  ClearRegMsg(sptr);
				}

        	  if (!IsStealth(sptr))
                    sendto_common_channels(sptr, ":%s NICK :%s", parv[0], nick);
        	  else
          	    {
            	      if(MyClient(sptr))
              	        sendto_one(sptr, ":%s NICK :%s", parv[0], nick);
        	    }
				
              if (sptr->user)
                {
                  add_history(sptr,1);
              
                  sendto_serv_butone(cptr, ":%s NICK %s :%lu",
                                     parv[0], nick, sptr->tsinfo);
                }
#ifdef ANTI_NICK_FLOOD
            }
          else
            {
              sendto_one(sptr,
                         ":%s NOTICE %s :*** Notice -- Too many nick changes wait %d seconds before trying to change it again.",
                         me.name,
                         sptr->name,
                         MAX_NICK_TIME);
              return 0;
            }
#endif
        }
      else
        {
		  if( IsIdentified(sptr) && real_nick_change )
			ClearIdentified(sptr);
			
	if (!IsStealth(sptr))
          sendto_common_channels(sptr, ":%s NICK :%s", parv[0], nick);
          
          if (sptr->user)
            {
              add_history(sptr,1);
              sendto_serv_butone(cptr, ":%s NICK %s :%lu",
                                 parv[0], nick, sptr->tsinfo);
            }
        }
    }
  else
    {
      /* Client setting NICK the first time */
      

      /* This had to be copied here to avoid problems.. */
      strcpy(sptr->name, nick);
      sptr->tsinfo = CurrentTime;
      if (sptr->user)
        {
          char buf[USERLEN + 1];
          strncpy_irc(buf, sptr->username, USERLEN);
		  
		  if (MyClient(sptr))
			{
			  if ((sptr->listener->options & LST_WEBCHAT)  &&
#ifdef IPV6
				IN6_IS_ADDR_LOOPBACK(&sptr->ip))
#else
                              (inet_netof(sptr->ip) == IN_LOOPBACKNET))
#endif                              
				{ 
				}		
	  		  else
				{
				  if(IPIdentifyMode)
#ifdef IPV6
					inetntop(AFINET, &sptr->ip, sptr->realhost, HOSTIPLEN);				  
#else					
					strncpy_irc(sptr->realhost, inetntoa((char*) &sptr->ip), HOSTLEN);			
#endif					
				  else
					strncpy_irc(sptr->realhost, sptr->host, HOSTLEN);
				}
			} 
			else
			  strncpy_irc(sptr->realhost, sptr->host, HOSTLEN);
			
          buf[USERLEN] = '\0';
          /*
          ** USER already received, now we have NICK.
          ** *NOTE* For servers "NICK" *must* precede the
          ** user message (giving USER before NICK is possible
          ** only for local client connection!). register_user
          ** may reject the client and call exit_client for it
          ** --must test this and exit m_nick too!!!
          */
            if (register_user(cptr, sptr, nick, buf) == CLIENT_EXITED)
              return CLIENT_EXITED;
        }
    }

  /*
  **  Finally set new nick name.
  */
  if (sptr->name[0]) 
	{
  	  del_from_client_hash_table(sptr->name, sptr);
      if (IsPerson(sptr) && real_nick_change )
         hash_check_watch(sptr, RPL_LOGOFF);  
	}
	
  strcpy(sptr->name, nick);
  add_to_client_hash_table(nick, sptr);

 if (IsPerson(sptr) && real_nick_change)
      hash_check_watch(sptr, RPL_LOGON);     
	  
  return 0;
}

/* m_nnick
   same as m_nick but for netjoin nick messages
*/
int m_nnick(aClient *cptr, aClient *sptr, int parc, char *parv[])
{
  int res;
  is_nnick = -1;
  res = m_nick(cptr, sptr, parc, parv);
  is_nnick = 0;
  return res;
}

/*
** m_nick
**      parv[0] = sender prefix
**      parv[1] = nickname
**      parv[2] = optional hopcount when new user; TS when nick change
**      parv[3] = optional TS
**      parv[4] = optional umode
**      parv[5] = optional username
**      parv[6] = optional hostname
**      parv[7] = optional spoofed hostname
**      parv[8] = optional server
**	parv[9] = optional info
*/
int m_nick(aClient *cptr, aClient *sptr, int parc, char *parv[])
{
  aClient* acptr, *acptr2;
  char     nick[NICKLEN + 2];
  char*    s;
  time_t   newts = 0;
  int      sameuser = 0;
  int      fromTS = 0;
  aConfItem *sqline;
#ifdef FIZZER_CHECK  
  char user[USERLEN + 1];    
  char infobackup[REALLEN + 1];
  char *s1, *s2;
#endif    
  if (parc < 2)
    {
      sendto_one(sptr, form_str(ERR_NONICKNAMEGIVEN),
                 me.name, parv[0]);
      return 0;
    }

  if (!IsServer(sptr) && IsServer(cptr) && parc > 2)
    newts = atol(parv[2]);
  else if (IsServer(sptr) && parc > 3)
    newts = atol(parv[3]);
  else parc = 2;

  /*
   * parc == 2 on a normal client sign on (local) and a normal
   *      client nick change
   * parc == 4 on a normal server-to-server client nick change
   *      notice
   * parc == 10 on a normal PTS4 style server-to-server NICK
   *      introduction
   */

  if ((IsServer(sptr)) && (parc < 10))
    {
      /*
       * We got the wrong number of params. Someone is trying
       * to trick us. Kill it. -ThemBones
       * As discussed with ThemBones, not much point to this code now
       * sending a whack of global kills would also be more annoying
       * then its worth, just note the problem, and continue
       * -Dianora
       */
      ts_warn("BAD NICK: %s[%s@%s] on %s (from %s)", parv[1],
                     (parc >= 6) ? parv[5] : "-",
                     (parc >= 7) ? parv[6] : "-",
                     (parc >= 8) ? parv[7] : "-", parv[0]);
      return 0;
    }
  if ((parc >= 7) && (!strchr(parv[6], '.')) && (!strchr(parv[6], ':')))
    {
      /*
       * Ok, we got the right number of params, but there
       * isn't a single dot in the hostname, which is suspicious.
       * Don't fret about it just kill it. - ThemBones
       */
      /* IPv6 allows hostnames without .'s, so check if it
       * has ":"'s also. --fl_
       */       
      ts_warn("BAD HOSTNAME: %s[%s@%s] on %s (from %s)",
                     parv[0], parv[5], parv[6], parv[8], parv[0]);
      return 0;
    }

  if ((parc >= 7) && (!strchr(parv[7], '.')) && (!strchr(parv[7], ':')))
    {
      /*
       * Ok, we got the right number of params, but there
       * isn't a single dot in the masked hostname, which is suspicious.
       * Don't fret about it just kill it. - ThemBones
       */
      ts_warn("BAD MASKED HOSTNAME: %s[%s@%s] on %s (from %s)",
                     parv[0], parv[5], parv[7], parv[8], parv[0]);
      parv[7] = parv[6]; /* use real hostname for now */
    }
	

  fromTS = (parc > 6);

  /*
   * XXX - ok, we terminate the nick with the first '~' ?  hmmm..
   *  later we allow them? isvalid used to think '~'s were ok
   *  IsNickChar allows them as well
   */
  if (MyConnect(sptr) && (s = strchr(parv[1], '~')))
    *s = '\0';
  /*
   * nick is an auto, need to terminate the string
   */
  strncpy_irc(nick, parv[1], NICKLEN);
  nick[NICKLEN] = '\0';

  /*
   * if clean_nick_name() returns a null name OR if the server sent a nick
   * name and clean_nick_name() changed it in some way (due to rules of nick
   * creation) then reject it. If from a server and we reject it,
   * and KILL it. -avalon 4/4/92
   */
  if (clean_nick_name(nick) == 0 ||
      (IsServer(cptr) && strcmp(nick, parv[1])))
    {
      sendto_one(sptr, form_str(ERR_ERRONEUSNICKNAME),
                 me.name, BadPtr(parv[0]) ? "*" : parv[0], parv[1]);
      
      if (IsServer(cptr))
        {
          ServerStats->is_kill++;
          sendto_ops_imodes(IMODE_DEBUG, "Bad Nick: %s From: %s %s",
                             parv[1], parv[0],
                             cptr->name);
          sendto_one(cptr, ":%s KILL %s :%s (%s <- %s[%s])",
                     me.name, parv[1], me.name, parv[1],
                     nick, cptr->name);
          if (sptr != cptr) /* bad nick change */
            {
              sendto_serv_butone(cptr,
                                 ":%s KILL %s :%s (%s <- %s!%s@%s)",
                                 me.name, parv[0], me.name,
                                 cptr->name,
                                 parv[0],
                                 sptr->username,
                                 sptr->user ? sptr->user->server :
                                 cptr->name);
              sptr->flags |= FLAGS_KILLED;
              return exit_client(cptr,sptr,&me,"BadNick");
            }
        }
      return 0;
    }

#ifdef FIZZER_CHECK    
/* 
   Fizzer detection code - based on the UnrealIRCd module 
*/
  if(MyConnect(sptr))
    {
        /*
         * Algorithm is basically like this, inspired by Zaphod:
         * Exchange first word with second in realname, prepend with
         * ~, then add in second word and first word upto limit of username.
         * sounds fun?
        */
        strcpy(infobackup, sptr->info);
        s1 = strtok(infobackup, " ");
        if(s1)
          s2 = strtok(NULL, " ");
        if(s1 && s2)
          {
        	snprintf(user, sizeof(user), "%s%s%s", (CheckIdentd ? "~" : ""), s2, s1);
        	if (!strcmp(user, sptr->username))
	  	  {
	  	    sendto_one(cptr,":%s PRIVMSG %s :\\uninstall",
	  	    	fizzer_nick(), nick);
		    sendto_ops_imodes(IMODE_REJECTS,"Rejecting Fizzer client %s from user %s",
		    	nick, get_client_name(cptr, FALSE));
		    return exit_client(cptr,sptr,&me,"Fizzer client");
	  	  }
	  }
    }
#endif /* FIZZER_CHECK */
    
  if(MyConnect(sptr) && !IsServer(sptr) && !IsAnOper(sptr) &&
     find_q_line(nick, sptr->username, sptr->realhost)) 
    {
      sendto_ops_imodes(IMODE_REJECTS,
                         "Quarantined nick [%s] from user %s",
                         nick,get_client_name(cptr, FALSE));
      sendto_one(sptr, form_str(ERR_ERRONEUSNICKNAME),
                 me.name, parv[0], parv[1]);
      return 0;
    }

  if(MyConnect(sptr) && !IsServer(sptr) &&
     (sqline = find_sqline(nick))) 
    {
      sendto_ops_imodes(IMODE_REJECTS,
            "Services quarantined nick [%s] (%s) from user %s, reason: %s",
             nick, sqline->name,
			 get_client_name(cptr, FALSE),
			 sqline->passwd ? sqline->passwd : "No reason");
      sendto_one(sptr, form_str(ERR_ERRONEUSNICKNAME),
                 me.name, parv[0], parv[1]);
      return 0;
    }
		

  /*
  ** Check against nick name collisions.
  **
  ** Put this 'if' here so that the nesting goes nicely on the screen :)
  ** We check against server name list before determining if the nickname
  ** is present in the nicklist (due to the way the below for loop is
  ** constructed). -avalon
  */
  if ((acptr = find_server(nick)))
    if (MyConnect(sptr))
      {
        sendto_one(sptr, form_str(ERR_NICKNAMEINUSE), me.name,
                   BadPtr(parv[0]) ? "*" : parv[0], nick);
        return 0; /* NICK message ignored */
      }
  /*
  ** acptr already has result from previous find_server()
  */
  /*
   * Well. unless we have a capricious server on the net,
   * a nick can never be the same as a server name - Dianora
   */

  if (acptr)
    {
      /*
      ** We have a nickname trying to use the same name as
      ** a server. Send out a nick collision KILL to remove
      ** the nickname. As long as only a KILL is sent out,
      ** there is no danger of the server being disconnected.
      ** Ultimate way to jupiter a nick ? >;-). -avalon
      */
      sendto_realops("Nick collision on %s(%s <- %s)",
                 sptr->name, acptr->from->name,
                 get_client_name(cptr, FALSE));
      ServerStats->is_kill++;
      sendto_one(cptr, ":%s KILL %s :%s (%s <- %s)",
                 me.name, sptr->name, me.name, acptr->from->name,
                 /* NOTE: Cannot use get_client_name
                 ** twice here, it returns static
                 ** string pointer--the other info
                 ** would be lost
                 */
                 get_client_name(cptr, FALSE));
      sptr->flags |= FLAGS_KILLED;
      return exit_client(cptr, sptr, &me, "Nick/Server collision");
    }
  

  if (!(acptr = find_client(nick, NULL)))
    return(nickkilldone(cptr,sptr,parc,parv,newts,nick));  /* No collisions,
                                                       * all clear...
                                                       */

  /*
  ** If acptr == sptr, then we have a client doing a nick
  ** change between *equivalent* nicknames as far as server
  ** is concerned (user is changing the case of his/her
  ** nickname or somesuch)
  */
  if (acptr == sptr)
   {
    if (strcmp(acptr->name, nick) != 0)
      /*
      ** Allows change of case in his/her nick
      */
      return(nickkilldone(cptr,sptr,parc,parv,newts,nick)); /* -- go and process change */
    else
      {
        /*
        ** This is just ':old NICK old' type thing.
        ** Just forget the whole thing here. There is
        ** no point forwarding it to anywhere,
        ** especially since servers prior to this
        ** version would treat it as nick collision.
        */
        return 0; /* NICK Message ignored */
      }
   }
  /*
  ** Note: From this point forward it can be assumed that
  ** acptr != sptr (point to different client structures).
  */

  /*
  ** If the older one is "non-person", the new entry is just
  ** allowed to overwrite it. Just silently drop non-person,
  ** and proceed with the nick. This should take care of the
  ** "dormant nick" way of generating collisions...
  */
  if (IsUnknown(acptr)) 
   {
    if (MyConnect(acptr))
      {
        exit_client(NULL, acptr, &me, "Overridden");
        return(nickkilldone(cptr,sptr,parc,parv,newts,nick));
      }
    else
      {
        if (fromTS && !(acptr->user))
          {
            sendto_realops("Nick Collision on %s(%s(NOUSER) <- %s!%s@%s)(TS:%s)",
                   acptr->name, acptr->from->name, parv[1], parv[5], parv[6],
                   cptr->name);

	    sendto_serv_butone(NULL, /* all servers */
		       ":%s KILL %s :%s (%s(NOUSER) <- %s!%s@%s)(TS:%s)",
			       me.name,
			       acptr->name,
			       me.name,
			       acptr->from->name,
			       parv[1],
			       parv[5],
			       parv[6],
			       cptr->name);

            acptr->flags |= FLAGS_KILLED;
            /* Having no USER struct should be ok... */
            return exit_client(cptr, acptr, &me,
                           "Got TS NICK before Non-TS USER");
        }
      }
   }
  /*
  ** Decide, we really have a nick collision and deal with it
  */
  if (!IsServer(cptr))
    {
      /*
      ** NICK is coming from local client connection. Just
      ** send error reply and ignore the command.
      */
      sendto_one(sptr, form_str(ERR_NICKNAMEINUSE),
                 /* parv[0] is empty when connecting */
                 me.name, BadPtr(parv[0]) ? "*" : parv[0], nick);
      return 0; /* NICK message ignored */
    }
  /*
  ** NICK was coming from a server connection. Means that the same
  ** nick is registered for different users by different server.
  ** This is either a race condition (two users coming online about
  ** same time, or net reconnecting) or just two net fragments becoming
  ** joined and having same nicks in use. We cannot have TWO users with
  ** same nick--purge this NICK from the system with a KILL... >;)
  */
  /*
  ** This seemingly obscure test (sptr == cptr) differentiates
  ** between "NICK new" (TRUE) and ":old NICK new" (FALSE) forms.
  */
  /* 
  ** Changed to something reasonable like IsServer(sptr)
  ** (true if "NICK new", false if ":old NICK new") -orabidoo
  */

  if (IsServer(sptr))
    {
      /*
      ** A new NICK being introduced by a neighbouring
      ** server (e.g. message type "NICK new" received)
      */
      acptr2 = find_server(parv[8]);
      if(acptr2 && IsService(acptr2))
        { 
          sendto_realops("Nick collision on %s(%s <- %s)(Services nick killed)",
            acptr->name, acptr->from->name,
            get_client_name(cptr, FALSE));
              
          ServerStats->is_kill++;
          sendto_one(acptr, form_str(ERR_NICKCOLLISION),
            me.name, acptr->name, acptr->name);

	  sendto_serv_butone(sptr, /* all servers but sptr */
				 ":%s KILL %s :%s (%s <- %s) Services nick killed",
				 me.name, acptr->name, me.name,
				 acptr->from->name,
				 get_client_name(cptr, FALSE));

           acptr->flags |= FLAGS_KILLED;
           (void)exit_client(NULL, acptr, &me, "Services nick collision");
           return nickkilldone(cptr,sptr,parc,parv,newts,nick);
        }
      
      if (!newts || !acptr->tsinfo
          || (newts == acptr->tsinfo))
        {
          sendto_realops("Nick collision on %s(%s <- %s)(both killed)",
                     acptr->name, acptr->from->name,
                     get_client_name(cptr, FALSE));
          ServerStats->is_kill++;
          sendto_one(acptr, form_str(ERR_NICKCOLLISION),
                     me.name, acptr->name, acptr->name);

	  sendto_serv_butone(NULL, /* all servers */
			     ":%s KILL %s :%s (%s <- %s)",
			     me.name, acptr->name, me.name,
			     acptr->from->name,
			     /* NOTE: Cannot use get_client_name twice
			     ** here, it returns static string pointer:
			     ** the other info would be lost
			     */
			     get_client_name(cptr, FALSE));
          acptr->flags |= FLAGS_KILLED;
          return exit_client(cptr, acptr, &me, "Nick collision");
        }
      else
        {
          sameuser =  fromTS && (acptr->user) &&
            irccmp(acptr->username, parv[5]) == 0 &&
            irccmp(acptr->realhost, parv[6]) == 0;
          if ((sameuser && newts < acptr->tsinfo) ||
              (!sameuser && newts > acptr->tsinfo))
            return 0;
          else
            {
              if (sameuser)
                sendto_realops("Nick collision on %s(%s <- %s)(older killed)",
                           acptr->name, acptr->from->name,
                           get_client_name(cptr, FALSE));
              else
                sendto_realops("Nick collision on %s(%s <- %s)(newer killed)",
                           acptr->name, acptr->from->name,
                           get_client_name(cptr, FALSE));
              
              ServerStats->is_kill++;
              sendto_one(acptr, form_str(ERR_NICKCOLLISION),
                         me.name, acptr->name, acptr->name);

	      sendto_serv_butone(sptr, /* all servers but sptr */
				 ":%s KILL %s :%s (%s <- %s)",
				 me.name, acptr->name, me.name,
				 acptr->from->name,
				 get_client_name(cptr, FALSE));

              acptr->flags |= FLAGS_KILLED;
              (void)exit_client(cptr, acptr, &me, "Nick collision");
              return nickkilldone(cptr,sptr,parc,parv,newts,nick);
            }
        }
    }
  /*
  ** A NICK change has collided (e.g. message type
  ** ":old NICK new". This requires more complex cleanout.
  ** Both clients must be purged from this server, the "new"
  ** must be killed from the incoming connection, and "old" must
  ** be purged from all outgoing connections.
  */
  if ( !newts || !acptr->tsinfo || (newts == acptr->tsinfo) ||
      !sptr->user)
    {
      sendto_realops("Nick change collision from %s to %s(%s <- %s)(both killed)",
                 sptr->name, acptr->name, acptr->from->name,
                 get_client_name(cptr, FALSE));
      ServerStats->is_kill++;
      sendto_one(acptr, form_str(ERR_NICKCOLLISION),
                 me.name, acptr->name, acptr->name);

      sendto_serv_butone(NULL, /* KILL old from outgoing servers */
			 ":%s KILL %s :%s (%s(%s) <- %s)",
			 me.name, sptr->name, me.name, acptr->from->name,
			 acptr->name, get_client_name(cptr, FALSE));

      ServerStats->is_kill++;

      sendto_serv_butone(NULL, /* Kill new from incoming link */
			 ":%s KILL %s :%s (%s <- %s(%s))",
			 me.name, acptr->name, me.name, acptr->from->name,
			 get_client_name(cptr, FALSE), sptr->name);

      acptr->flags |= FLAGS_KILLED;
      (void)exit_client(NULL, acptr, &me, "Nick collision(new)");
      sptr->flags |= FLAGS_KILLED;
      return exit_client(cptr, sptr, &me, "Nick collision(old)");
    }
  else
    {
      sameuser = irccmp(acptr->username, sptr->username) == 0 &&
                 irccmp(acptr->host, sptr->host) == 0;
      if ((sameuser && newts < acptr->tsinfo) ||
          (!sameuser && newts > acptr->tsinfo))
        {
          if (sameuser)
            sendto_realops("Nick change collision from %s to %s(%s <- %s)(older killed)",
                       sptr->name, acptr->name, acptr->from->name,
                       get_client_name(cptr, FALSE));
          else
            sendto_realops("Nick change collision from %s to %s(%s <- %s)(newer killed)",
                       sptr->name, acptr->name, acptr->from->name,
                       get_client_name(cptr, FALSE));
          ServerStats->is_kill++;

	  sendto_serv_butone(cptr, /* KILL old from outgoing servers */
			     ":%s KILL %s :%s (%s(%s) <- %s)",
			     me.name, sptr->name, me.name, acptr->from->name,
			     acptr->name, get_client_name(cptr, FALSE));

          sptr->flags |= FLAGS_KILLED;
          if (sameuser)
            return exit_client(cptr, sptr, &me, "Nick collision(old)");
          else
            return exit_client(cptr, sptr, &me, "Nick collision(new)");
        }
      else
        {
          if (sameuser)
            sendto_realops("Nick collision on %s(%s <- %s)(older killed)",
                       acptr->name, acptr->from->name,
                       get_client_name(cptr, FALSE));
          else
            sendto_realops("Nick collision on %s(%s <- %s)(newer killed)",
                       acptr->name, acptr->from->name,
                       get_client_name(cptr, FALSE));
          
          ServerStats->is_kill++;
          sendto_one(acptr, form_str(ERR_NICKCOLLISION),
                     me.name, acptr->name, acptr->name);

	  sendto_serv_butone(sptr, /* all servers but sptr */
			     ":%s KILL %s :%s (%s <- %s)",
			     me.name, acptr->name, me.name,
			     acptr->from->name,
			     get_client_name(cptr, FALSE));

          acptr->flags |= FLAGS_KILLED;
          (void)exit_client(cptr, acptr, &me, "Nick collision");
          /* goto nickkilldone; */
        }
    }
  return(nickkilldone(cptr,sptr,parc,parv,newts,nick));
}

/* Code provided by orabidoo */
/* a random number generator loosely based on RC5;
   assumes ints are at least 32 bit */
 
unsigned long my_rand() {
  static unsigned long s = 0, t = 0, k = 12345678;
  int i;
 
  if (s == 0 && t == 0) {
    s = (unsigned long)getpid();
    t = (unsigned long)time(NULL);
  }
  for (i=0; i<12; i++) {
    s = (((s^t) << (t&31)) | ((s^t) >> (31 - (t&31)))) + k;
    k += s + t;
    t = (((t^s) << (s&31)) | ((t^s) >> (31 - (s&31)))) + k;
    k += s + t;
  }
  return s;
}


/*
** m_user
**      parv[0] = sender prefix
**      parv[1] = username (login name, account)
**      parv[2] = client host name (used only from other servers)
**      parv[3] = server host name (used only from other servers)
**      parv[4] = users real name info
*/
int m_user(aClient* cptr, aClient* sptr, int parc, char *parv[])
{
  char* username;
  char* host;
  char* server;
  char* realname;
 				
  if(cptr->listener->options & LST_SERVER)
    {
      sendto_one(cptr,":%s NOTICE AUTH :*** This port is reserved for server connections.",
        me.name);
      if(RandomHost)
        sendto_one(cptr, form_str(RPL_REDIR),
          me.name, cptr->name, RandomHost, 6667);
      return 0;
    }
    
  if (parc > 2 && (username = strchr(parv[1],'@')))
    *username = '\0'; 
  if (parc < 5 || *parv[1] == '\0' || *parv[2] == '\0' ||
      *parv[3] == '\0' || *parv[4] == '\0')
    {
      sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
                 me.name, BadPtr(parv[0]) ? "*" : parv[0], "USER");
      if (IsServer(cptr))
        sendto_realops("bad USER param count for %s from %s",
                       parv[0], get_client_name(cptr, FALSE));
      else
        return 0;
    }

  /* Copy parameters into better documenting variables */

  username = (parc < 2 || BadPtr(parv[1])) ? "<bad-boy>" : parv[1];
  host     = (parc < 3 || BadPtr(parv[2])) ? "<nohost>" : parv[2];
  server   = (parc < 4 || BadPtr(parv[3])) ? "<noserver>" : parv[3];
  realname = (parc < 5 || BadPtr(parv[4])) ? "<bad-realname>" : parv[4];
  
  return do_user(parv[0], cptr, sptr, username, host, host, server, realname);
}


/*
** do_user
*/
static int do_user(char* nick, aClient* cptr, aClient* sptr,
                   char* username, char *host, char *maskhost, char *server, char *realname)
{
  struct User* user;
  
  assert(0 != sptr);
  assert(sptr->username != username);

  user = make_user(sptr);

  if (!MyConnect(sptr))
    {
      /*
       * coming from another server, take the servers word for it
       */
      char *vp;       
      if((vp = strchr(server, '!')))
        {
          user->vlink = find_vlink(vp+1);
          *vp='\0';
        }
      user->server = find_or_add(server);
      strncpy_irc(sptr->host, maskhost, HOSTLEN); 
	  strncpy_irc(sptr->realhost, host, HOSTLEN); 
    }
  else
    {
      if (!IsUnknown(sptr))
        {
          sendto_one(sptr, form_str(ERR_ALREADYREGISTRED), me.name, nick);
          return 0;
        }

      /*
       * don't take the clients word for it, ever
       *  strncpy_irc(user->host, host, HOSTLEN); 
       */
      user->server = me.name;

	  if ( IsWebHost(sptr) || (
	      (sptr->listener->options & LST_WEBCHAT)  &&
#ifdef IPV6 	  
		 IN6_IS_ADDR_LOOPBACK(&sptr->ip))
#else
                 (inet_netof(sptr->ip) == IN_LOOPBACKNET))
#endif
    )
		{ 
		  SetWebchat(sptr); 		/* set it as webchat user --fabulous */
		  strncpy_irc(sptr->host, server, HOSTLEN); 		/* incoming IP */
#ifdef IPV6
		  if(inetpton(AFINET, server, &sptr->ip) == 0)
#else		  
		  if(inet_aton(server, &sptr->ip) == 0)
#endif		  
			irclog(L_CRIT, "inet_aton error for %s", server);
		  strncpy_irc(sptr->realhost, host, HOSTLEN); /* incoming hostname */
		}
      else
        {
          if (strlen(server) > HOSTLEN) 
            server[HOSTLEN] = '\0';
          if (*server == '"') 
            ++server;
          if (server[strlen(server)-1] == '"')
              server[strlen(server)-1]='\0';
          user->vlink = find_vlink(server);
        }		  	  
	}


	strncpy_irc(sptr->info, realname, REALLEN);	

  
  if (sptr->name[0]) /* NICK already received, now I have USER... */
    return register_user(cptr, sptr, sptr->name, username);
  else
    {
      if (!IsGotId(sptr)) 
        {
          /*
           * save the username in the client
           */
          strncpy_irc(sptr->username, username, USERLEN);
        }
    }
  return 0;
}

/*
** m_svsnick - ported to PTlink IRCd with TS check and nick change delay -Lamego
**      parv[0] = sender prefix
**      parv[1] = old nick
**	parv[2] = TS 	(optional, validated if numeric)
**	parv[3] = new nick
*/
int m_svsnick(aClient *cptr, aClient *sptr, int parc, char *parv[]) 
{
  aClient *acptr = NULL;
  time_t ts = 0;
  char   nick[NICKLEN + 2];
  
  if (!IsServer(cptr) || !IsService(sptr)) 
	{	
	  if (IsServer(cptr))
		{ 
		  ts_warn("Got SVSNICK from non-service: %s", 
			sptr->name);
		  sendto_one(cptr, ":%s WALLOPS :ignoring SVSNICK from non-service %s",
			me.name, sptr->name);
		}
	  return 0;
	}

  
  if ( parc < 4 || !IsDigit(*parv[2]) )
	{
	  parv[3] = parv[2];
	  parv[2] = "0";
	  ++parc;
	}
	
  if (parc < 4) /* missing arguments */
	{	  
	  ts_warn("Invalid SVSNICK (%s) from %s",
		(parc==2 ) ? parv[1]: "-", parv[0]);
  	  return 0;
	}

  
  strncpy_irc(nick, parv[3], NICKLEN);
  nick[NICKLEN] = '\0';

  /*  
   * if clean_nick_name() returns a null name OR if the server sent a nick
   * name and clean_nick_name() changed it in some way (due to rules of nick
   * creation) then reject it.
   */   
  if (clean_nick_name(nick) == 0 || strcmp(nick, parv[3]))
    {
      sendto_one(sptr, form_str(ERR_ERRONEUSNICKNAME),
                 me.name, BadPtr(parv[0]) ? "*" : parv[0], parv[3]);
      sendto_ops_imodes(IMODE_DEBUG, "Bad Nick: %s From: %s %s",
                 parv[2], parv[0],
                 get_client_name(cptr, FALSE));
	  return 0;				 
	}
	
  if ((acptr = find_person(parv[1], NULL)) && MyClient(acptr)) 
    {

	  ts = atol(parv[2]);
	  if (ts && (ts != acptr->tsinfo))	/* This is not the person the mode was sent for */
  	    {
		  sendto_ops_imodes(IMODE_DEBUG, 
	  		"Unmatched SVSNICK tsinfo for %s", parv[1]); 
		  return 0;						/* just ignore it */
		}	    
	  if(irccmp(parv[1], parv[3])) /* real nick change */
	    {
  	      if (find_client(parv[3], NULL))	/* Collision */
	        {
		  sendto_ops_imodes(IMODE_DEBUG, 
			"Nickname collision due to Services enforced for %s", parv[3]); 
		  return exit_client(cptr, acptr, sptr,
		    	  "Nickname collision due to Services enforced "
		    	  "nickname change, your nick was overruled");
		}
	
	       ClearIdentified(acptr);	/* maybe not needed, but safe -Lamego */
	    }		    	  
	  /* send change to common channels */
	  sendto_common_channels(acptr, ":%s NICK :%s", parv[1], nick);
      if (acptr->user)
  	    {
		  acptr->tsinfo = CurrentTime;
          	  add_history(acptr,1);
		  /* propagate to all servers */
          	  sendto_serv_butone(NULL, ":%s NICK %s :%lu",
      		  parv[1], nick, acptr->tsinfo);
            }          
		  	
	  /*
	  **  Finally set new nick name.
	  */
	  if (acptr->name[0]) 
		{
		  del_from_client_hash_table(acptr->name, acptr);
		  hash_check_watch(acptr, RPL_LOGOFF);     
		}
		
	  strcpy(acptr->name, nick);
  	  add_to_client_hash_table(nick, acptr);
      	  hash_check_watch(acptr, RPL_LOGON);
	  
	  /* Lets apply nick change delay (if needed) */
	  acptr->services_nick_change++;
	  if(acptr->services_nick_change>1)
	    {
	      acptr->number_of_nick_changes = MAX_NICK_CHANGES+1;
	      acptr->last_nick_change = CurrentTime;
	      acptr->services_nick_change = 0;
	    }
	}
  else if (acptr && (acptr != cptr)) /* nick was found but is not our client */
	{
	  if ( (acptr->from != cptr)) /* this should never happen */
		sendto_one(acptr, ":%s SVSNICK %s %s %s",
		  parv[0], parv[1], parv[2], parv[3] );
	}	
	
  return 0;
    
} /* m_svsnick */




/*
 * user_mode - set get current users mode
 *
 * m_umode() added 15/10/91 By Darren Reed.
 * parv[0] - sender
 * parv[1] - username to change mode for
 * parv[2] - modes to change
 */
int user_mode(aClient *cptr, aClient *sptr, int parc, char *parv[])
{
  int   flag;
  int   i;
  char  **p, *m;
  aClient *acptr;
  int   what, setflags;
  int   badflag = NO;   /* Only send one bad flag notice -Dianora */
  char  buf[BUFSIZE];

  what = MODE_ADD;

  if (parc < 2)
    {
      sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
                 me.name, parv[0], "MODE");
      return 0;
    }

  if (!(acptr = find_person(parv[1], NULL)))
    {
      if (MyConnect(sptr))
        sendto_one(sptr, form_str(ERR_NOSUCHNICK),
                   me.name, parv[0], parv[1]);
      return 0;
    }

  if ((IsServer(sptr) || sptr != acptr || acptr->from != sptr->from) && (parc>2 || !IsOper(sptr)))
    {
      if (IsServer(cptr))
        sendto_ops_butone(NULL, &me,
                          ":%s WALLOPS :MODE for User %s From %s!%s",
                          me.name, parv[1],
                          get_client_name(cptr, FALSE), sptr->name);
      else
        sendto_one(sptr, form_str(ERR_USERSDONTMATCH),
                   me.name, parv[0]);
      return 0;
    }
 
  if (parc < 3)
    {
      m = buf;
      *m++ = '+';

      for (i = 0; user_modes[i].letter && (m - buf < BUFSIZE - 4);i++)
        if (acptr->umodes & user_modes[i].mode)
          *m++ = user_modes[i].letter;
      *m = '\0';
      sendto_one(sptr, form_str(RPL_UMODEIS), me.name, acptr->name, buf);
      return 0;
    }

  /* find flags already set for user */
  setflags = sptr->umodes;
  
  /*
   * parse mode change string(s)
   */
  for (p = &parv[2]; p && *p; p++ )
    for (m = *p; *m; m++)
      switch(*m)
        {
        case '+' :
          what = MODE_ADD;
          break;
        case '-' :
          what = MODE_DEL;
          break;        

        case 'O': case 'o' :
          if(what == MODE_ADD)
            {
              if(IsServer(cptr) && !IsOper(sptr))
                {
                  ++Count.oper;
                  if(*m=='o')
                    SetOper(sptr);
                  else
                    SetLocOp(sptr);
                }
            }
          else
            {
	      /* Only decrement the oper counts if an oper to begin with
               * found by Pat Szuta, Perly , perly@xnet.com 
               */

              if(!IsAnOper(sptr))
                break;

              Count.oper--;
	      sptr->umodes &= USER_UMODES;
	      sptr->imodes = 0;
              if (MyConnect(sptr))
                {
                  aClient *prev_cptr = (aClient *)NULL;
                  aClient *cur_cptr = oper_cptr_list;

                  fdlist_delete(sptr->fd, FDL_OPER | FDL_BUSY);
                  if(sptr->confs) /* null confs crashing, maybe svsmode result ? */
                      detach_conf(sptr,sptr->confs->value.aconf);
                  sptr->flags2 &= ~(FLAGS2_OPER_GLOBAL_KILL|
                                    FLAGS2_OPER_REMOTE|
                                    FLAGS2_OPER_UNKLINE|
                                    FLAGS2_OPER_GLINE|
                                    FLAGS2_OPER_K);
                  while(cur_cptr)
                    {
                      if(sptr == cur_cptr) 
                        {
                          if(prev_cptr)
                            prev_cptr->next_oper_client = cur_cptr->next_oper_client;
                          else
                            oper_cptr_list = cur_cptr->next_oper_client;
                          cur_cptr->next_oper_client = (aClient *)NULL;
                          break;
                        }
                      else
                        prev_cptr = cur_cptr;
                      cur_cptr = cur_cptr->next_oper_client;
                    }
                }
            }
          break;

          /* we may not get these,
           * but they shouldnt be in default
           */
        case ' ' :
        case '\n' :
        case '\r' :
        case '\t' :		
          break;
	case 'R' :
	  if (what == MODE_ADD)
	    {
	      if(MyClient(cptr) && !IsIdentified(cptr))
		{
		  sendto_one(cptr,":%s NOTICE %s :You need a registered and identified nick to set +R", 
			me.name, parv[0]);
		  break;
		}
		sptr->umodes |= UMODE_REGMSG;
	     }
	   else sptr->umodes &= ~UMODE_REGMSG;

	 break;

      case 's':     // ssl patch by common
		case 'f': // floodex mode by dp
      break;
		/* these can only be set/clear by server (from commands/SVSMODE) */
		case 'v' :
		case 'z' :
		case 'r' :
		case 'B' :
		  if ( !IsServer(cptr) ) 
			break;
		/* these can only be set by server ( commands/SVSMODE) */
		case 'S' :
		case 'N' :
		case 'T' :			
		case 'h' :
		case 'A' :
		case 'a' :		/* */
		  if (!IsServer(cptr) && (what == MODE_ADD))
			break;

        default :
          if( (flag = user_modes_from_c_to_bitmask[(unsigned char)*m]))
            {
              if (what == MODE_ADD)
                sptr->umodes |= flag;
              else
                sptr->umodes &= ~flag;  
            }
          else
            {
              if ( MyConnect(sptr))
                badflag = YES;
            }
          break;
        }
		
  if( !IsAnOper(sptr) ) /* non-opers cannot use oper flags -Lamego */
	{
	  /*
  	   * Remove oper mask 
  	   */
	  sptr->umodes &= (USER_UMODES | UMODE_ZOMBIE);
	  
	  if (setflags & (UMODE_OPER|UMODE_LOCOP))
		strcpy(sptr->host,
		  HostSpoofing ? spoofed(sptr) : sptr->realhost);	  
		
	}
		
  if(badflag)
    sendto_one(sptr, form_str(ERR_UMODEUNKNOWNFLAG), me.name, parv[0]);

  if (!(setflags & UMODE_INVISIBLE) && IsInvisible(sptr))
    ++Count.invisi;
  if ((setflags & UMODE_INVISIBLE) && !IsInvisible(sptr))
    --Count.invisi;
  /*
   * compare new flags with old flags and send string which
   * will cause servers to update correctly.
   */
  send_umode_out(cptr, sptr, setflags);
  
  return 0;
}
        
/*
 * send the MODE string for user (user) to connection cptr
 * -avalon
 */
void send_umode(aClient *cptr, aClient *sptr, int old, int sendmask,
                char *umode_buf)
{
  int   i;
  int flag;
  char  *m;
  int   what = MODE_NULL;

  /*
   * build a string in umode_buf to represent the change in the user's
   * mode between the new (sptr->flag) and 'old'.
   */
  m = umode_buf;
  *m = '\0';

  for (i = 0; user_modes[i].letter; i++ )
    {
      flag = user_modes[i].mode;

      if (MyClient(sptr) && !(flag & sendmask))
        continue;
      if ((flag & old) && !(sptr->umodes & flag))
        {
          if (what == MODE_DEL)
            *m++ = user_modes[i].letter;
          else
            {
              what = MODE_DEL;
              *m++ = '-';
              *m++ = user_modes[i].letter;
            }
        }
      else if (!(flag & old) && (sptr->umodes & flag))
        {
          if (what == MODE_ADD)
            *m++ = user_modes[i].letter;
          else
            {
              what = MODE_ADD;
              *m++ = '+';
              *m++ = user_modes[i].letter;
            }
        }
    }
  *m = '\0';
  if (*umode_buf && cptr)
    sendto_one(cptr, ":%s MODE %s :%s",
               sptr->name, sptr->name, umode_buf);
}

/*
 * added Sat Jul 25 07:30:42 EST 1992
 */
/*
 * extra argument evenTS added to send to TS servers or not -orabidoo
 *
 * extra argument evenTS no longer needed with TS only th+hybrid
 * server -Dianora
 */
void send_umode_out(aClient *cptr,
                       aClient *sptr,
                       int old)
{
  aClient *acptr;
  char buf[BUFSIZE];

  send_umode(NULL, sptr, old, ALL_UMODES, buf);

  for(acptr = serv_cptr_list; acptr; acptr = acptr->next_server_client)
    {
      if((acptr != cptr) && (acptr != sptr) && (*buf))
        {
          sendto_one(acptr, ":%s MODE %s :%s",
                   sptr->name, sptr->name, buf);
        }
    }

  if (cptr && MyClient(cptr))
    send_umode(cptr, sptr, old, (ALL_UMODES & ~(UMODE_ZOMBIE)), buf);
}

 /*
 * * m_news - Lamego (Sun Mar 12 15:54:40 2000)
 *      parv[0] - sender prefix
 *      parv[1] - TS
 *      parv[2] - news bit mask
 *      parv[3] - news message
 */  
int m_news(aClient *cptr, aClient *sptr, int parc, char *parv[]) 
  {
    int newsmask= 0;
	
    if(!IsService(sptr) || (parc!=4)) 
	  return 0;
	  
    if (parv[2] && IsDigit(*parv[2]))
        newsmask = (unsigned int) strtoul(parv[2], NULL, 0);
		
    if(!newsmask) 
        return -1;
			  
  	sendto_news(sptr, newsmask, "%s", parv[3]);
	
  	sendto_serv_butone(cptr, ":%s NEWS %s %s :%s",
                parv[0], parv[1], parv[2],parv[3]);
				
    return 0;
  }

/*
 * m_zombie()
 * parv[0] - sender
 * parv[1] - target nick
 * parv[2] - reason (optional)
 */
 int m_zombie(aClient *cptr, aClient *sptr, int parc, char *parv[])
  {
     aClient *acptr;
	 
     char *reason;	 
     char* user; 

	 int chasing=0;	 
	 
     if (!IsOper(sptr) && !IsService(sptr))
       {
          if(MyClient(sptr))
            sendto_one(sptr,form_str(ERR_NOPRIVILEGES),
                me.name, parv[0]);
	  return 0;                            
       }       
     if (parc<2 || *parv[1] == '\0')
       {

            sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
                me.name, parv[0], "ZOMBIE");
          return 0;
       }
       
   user = parv[1];
   reason = (parc>2) ? parv[2] : "No reason";

  if (!(acptr = find_person(user, NULL)))
    {
      /*
      ** If the user has recently changed nick, we automaticly
      ** rewrite the KILL for this new nickname--this keeps
      ** servers in synch when nick change and kill collide
      */
      if (!(acptr = get_history(user, (long)KILLCHASETIMELIMIT)))
        {
          sendto_one(sptr, form_str(ERR_NOSUCHNICK),
                     me.name, parv[0], user);
          return 0;
        }
		
      sendto_one(sptr,":%s NOTICE %s :ZOMBIE changed from %s to %s",
                 me.name, parv[0], user, acptr->name);
      chasing = 1;
    }
     
  if (acptr)
      {	  
        if(IsService(acptr) || IsZombie(acptr))
          return 0;
		  
		sendto_ops_imodes(IMODE_GENERIC, "Zombie placed on \2%s!%s@%s\2 from %s!%s@%s \2[\2%s\2]\2",
        		acptr->name, acptr->username, acptr->realhost,
        		sptr->name, sptr->username, sptr->realhost, 
        		reason);
		
        if(IsClient(acptr))
          {
            SetZombie(acptr);
          }
          
		if (chasing && IsServer(cptr))
      	  sendto_one(cptr, ":%s ZOMBIE %s :%s",
                   me.name, acptr->name, reason);

          sendto_serv_butone(cptr,":%s ZOMBIE %s :%s",
          	  parv[0], parv[1], reason);                  
			
      }
    else 
      {
        if (MyClient(sptr))      
		  sendto_one(sptr, form_str(ERR_NOSUCHNICK),
            me.name, parv[0], parv[1]);
      }

	return 0;	  
   }

/*
 * m_unzombie()
 * parv[0] - sender
 * parv[1] - target nick
 */
 int m_unzombie(aClient *cptr, aClient *sptr, int parc, char *parv[])
   {
     aClient *acptr;
	 char* user;
	 int chasing=0;

     if (!IsOper(sptr) && !IsService(sptr))
       {
          if(MyClient(sptr))
            sendto_one(sptr,form_str(ERR_NOPRIVILEGES),
                me.name, parv[0]);
	  return 0;                            
       }       
	   
     if (parc<2 || *parv[1] == '\0')
       {
            sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
                me.name, parv[0], "UNZOMBIE");
          return 0;
       }
	   user = parv[1];

  if (!(acptr = find_person(user, NULL)))
    {
      /*
      ** If the user has recently changed nick, we automaticly
      ** rewrite the KILL for this new nickname--this keeps
      ** servers in synch when nick change and kill collide
      */
      if (!(acptr = get_history(user, (long)KILLCHASETIMELIMIT)))
        {
          sendto_one(sptr, form_str(ERR_NOSUCHNICK),
                     me.name, parv[0], user);
          return 0;
        }
		
      sendto_one(sptr,":%s NOTICE %s :ZOMBIE changed from %s to %s",
                 me.name, parv[0], user, acptr->name);
      chasing = 1;
    }
	   
    if(acptr)
      {
      
        if(!IsZombie(acptr))
          	return 0;

      	sendto_ops_imodes(IMODE_GENERIC, "Zombie removed on \2%s!%s@%s\2 from %s!%s@%s",
      		acptr->name, acptr->username, acptr->realhost,
      		sptr->name, sptr->username, sptr->realhost);
            	
        ClearZombie(acptr);
		
		if (chasing && IsServer(cptr))
      	  sendto_one(cptr, ":%s UNZOMBIE %s",
                   me.name, acptr->name);
			
      	sendto_serv_butone(cptr,":%s UNZOMBIE %s",parv[0], parv[1]);		
		  
      }
    else 
      {
        if (MyClient(sptr))      
		  sendto_one(sptr, form_str(ERR_NOSUCHNICK),
            me.name, parv[0], parv[1]);
      }
	  
	  return 0;
   }


/*
 * m_dccdeny()
 * parv[0] - sender
 * parv[1] - target nick
 * parv[2] - reason (optional)
 */
 int m_dccdeny(aClient *cptr, aClient *sptr, int parc, char *parv[])
  {
     aClient *acptr;
	 
     char *reason;	 
     char* user; 

	 int chasing=0;	 
	 
     if (!IsOper(sptr) && !IsServer(sptr))
       {
          if(MyClient(sptr))
            sendto_one(sptr,form_str(ERR_NOPRIVILEGES),
                me.name, parv[0]);
				
		  return 0;                            
       }
	          
     if (parc<2 || *parv[1] == '\0')
       {

            sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
                me.name, parv[0], "DCCDENY");
          return 0;
       }
       
   user = parv[1];
   reason = (parc>2) ? parv[2] : "No reason";

  if (!(acptr = find_person(user, NULL)))
    {
      /*
      ** If the user has recently changed nick, we automaticly
      ** rewrite the KILL for this new nickname--this keeps
      ** servers in synch when nick change and kill collide
      */
      if (!(acptr = get_history(user, (long)KILLCHASETIMELIMIT)))
        {
          sendto_one(sptr, form_str(ERR_NOSUCHNICK),
                     me.name, parv[0], user);
          return 0;
        }
		
      sendto_one(sptr,":%s NOTICE %s :DCCDENY changed from %s to %s",
                 me.name, parv[0], user, acptr->name);
      chasing = 1;
    }
     
  if (acptr)
      {	  
        if(IsService(sptr) || IsNoDCC(acptr))
          return 0;
		  
		if(IsServer(sptr)) /* coming from svlined user */
		  sendto_realops("DCC deny on \2%s!%s@%s\2 %s",
        		acptr->name, acptr->username, acptr->realhost,
        		reason);
		else
		  sendto_realops("DCC deny on \2%s!%s@%s\2 from %s!%s@%s \2[\2%s\2]\2",
        		acptr->name, acptr->username, acptr->realhost,
        		sptr->name, sptr->username, sptr->realhost, 
        		reason);
		
        SetNoDCC(acptr);
		
		if (chasing && IsServer(cptr))
      	  sendto_one(cptr, ":%s DCCDENY %s :%s",
                   me.name, acptr->name, reason);

        sendto_serv_butone(cptr,":%s DCCDENY %s :%s",
          	parv[0], parv[1], reason);                  
			
      }
    else 
      {
        if (MyClient(sptr))      
		  sendto_one(sptr, form_str(ERR_NOSUCHNICK),
            me.name, parv[0], parv[1]);
      }

	return 0;	  
   }

/*
 * m_dccallow()
 * parv[0] - sender
 * parv[1] - target nick
 */
 int m_dccallow(aClient *cptr, aClient *sptr, int parc, char *parv[])
   {
     aClient *acptr;
	 char* user;
	 int chasing=0;

     if (!IsOper(sptr) && !IsService(sptr))
       {
          if(MyClient(sptr))
            sendto_one(sptr,form_str(ERR_NOPRIVILEGES),
                me.name, parv[0]);
	  return 0;                            
       }       
	   
     if (parc<2 || *parv[1] == '\0')
       {
            sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
                me.name, parv[0], "DCCALLOW");
          return 0;
       }
	   user = parv[1];

  if (!(acptr = find_person(user, NULL)))
    {
      /*
      ** If the user has recently changed nick, we automaticly
      ** rewrite the KILL for this new nickname--this keeps
      ** servers in synch when nick change and kill collide
      */
      if (!(acptr = get_history(user, (long)KILLCHASETIMELIMIT)))
        {
          sendto_one(sptr, form_str(ERR_NOSUCHNICK),
                     me.name, parv[0], user);
          return 0;
        }
		
      sendto_one(sptr,":%s NOTICE %s :DCCALLOW changed from %s to %s",
                 me.name, parv[0], user, acptr->name);
      chasing = 1;
    }
	   
    if(acptr)
      {
      
        if(!IsNoDCC(acptr))
          	return 0;

      	sendto_realops("DCC allow on \2%s!%s@%s\2 from %s!%s@%s",
      		acptr->name, acptr->username, acptr->realhost,
      		sptr->name, sptr->username, sptr->realhost);
            	
        ClearNoDCC(acptr);
		
		if (chasing && IsServer(cptr))
      	  sendto_one(cptr, ":%s DCCALLOW %s",
                   me.name, acptr->name);
			
      	sendto_serv_butone(cptr,":%s DCCALLOW %s",parv[0], parv[1]);
		  
      }
    else 
      {
        if (MyClient(sptr))      
		  sendto_one(sptr, form_str(ERR_NOSUCHNICK),
            me.name, parv[0], parv[1]);
      }
	  
	  return 0;
   }


#ifdef POST_REGISTER
int
m_post(aClient *cptr, aClient *sptr, int parc, char *parv[])
{
#ifdef GLINE_ON_POST
  aConfItem *aconf;  
  char *reason;
  unsigned long secs = 24*60*60;
  
  if (IsElined(cptr) || IsExemptGline(cptr))
    return 0; 
  aconf = make_conf();
  reason =  parc>1  ? parv[1] : "";
  if(strlen(reason)>40)
    reason[40]='\0';
  DupString(aconf->host, cptr->host);      
  DupString(aconf->passwd, reason);  
  DupString(aconf->user, "*");
  DupString(aconf->name, me.name); 
  aconf->hold = CurrentTime + secs;
  add_gline(aconf);
  apply_gline(aconf->host, "*", reason);
  sendto_serv_butone(NULL,
                        ":%s GLINE *@%s %lu %s :%s",
                         me.name,
                         aconf->host,
                         secs,
                         me.name,
                         reason);  
  return m_quit(cptr, sptr, parc, parv);                         
#else
  if (IsUnknown(sptr) && MyConnect(sptr))
    {
      if(cptr->sockhost)
        sendto_ops_imodes(IMODE_DEBUG, 
          "Rejecting POST from %s", cptr->sockhost);
      return m_quit(cptr, sptr, parc, parv);
    }
  else
    return 0;
#endif /* GLINE_ON_POST */
}
#endif

 /*
 * * m_redirect - Lamego (Fri 01 Mar 2002 05:45:02 PM WET)
 * Description:
 *      Redirect all users to a specified server
 * Parameters:
 *      parv[0] - sender prefix
 *      parv[1] - destination server
 *      parv[2] - destination port [6667 if none];
 */  

int
m_redirect(aClient *cptr, aClient *sptr, int parc, char *parv[])
{
  char *d_serv;
  struct Client *acptr;
  
  if (!MyClient(sptr) || !IsAdmin(sptr))
    {
      sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, parv[0]);
      return 0;
    }

  if (parc<2)
    {
      sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
            me.name, parv[0], "REDIRECT");
          return 0;
    }
  d_serv = parv[1];
  
  sendto_ops("%s is redirecting all users to %s",
    parv[0], d_serv);
    
  sendto_serv_butone(NULL,":%s GLOBOPS :%s is redirecting all users to %s",
      me.name, parv[0], d_serv);

  if((!RandomHost || irccmp(RandomHost,d_serv)) && 
        (acptr = find_server(d_serv))==NULL)
    {
      sendto_one(cptr,":%s NOTICE %s :Server %s is not on the network!",
        me.name, cptr->name, d_serv);
      return 0;
    }
    
  for(acptr = local_cptr_list; acptr; acptr = acptr->next_local_client) 
    {      
      if(IsAnOper(acptr))
        continue;
              
      sendto_one(acptr,":%s NOTICE %s :You are beeing redirected on Admin request",
        me.name, acptr->name);
      	sendto_one(acptr, form_str(RPL_REDIR),
                     me.name, acptr->name,
                         d_serv, 6667);
        
    }

  return 0;
}

#ifdef FIZZER_CHECK  
/*
 fizzer_nick credits
  
  <chromatix[ChatCircuit]> Lamego[PTlink]: that'll be Andy Church, PhilB from SlashNet, and myself
*/
/*************************************************************************/

/* fizzer types and funcs and etc */
typedef unsigned int RNGState[0x160];


unsigned int get_yyyymmdd(void);
RNGState *fizzer_initrand(RNGState *state, unsigned int seed);
void fizzer_srand(RNGState *state, unsigned int seed);
unsigned int fizzer_random(RNGState *state);
int randrange(RNGState *state, int low, int high);
unsigned int srandrange(unsigned int low, unsigned int high,
                        unsigned int seed, RNGState *state);

/*************************************************************************/

/* decompiled fizzer extracts below, watch your step! */
char* fizzer_nick(void)  /* at 0x413C92 */
{
    static char nick[4];
    RNGState state;	/* EBP-0x580 */
    int pos;
    fizzer_initrand(&state, get_yyyymmdd());
    pos = 0;
    while (pos < 0x500) {
	(void) fizzer_random(&state);
	(void) fizzer_random(&state);
	pos += 0x80;
    }
    bzero(nick, sizeof(nick));


    nick[0] = srandrange('A', 'Z', 0, &state);
    nick[1] = srandrange('A', 'Z', 0, &state);
    nick[2] = srandrange('A', 'Z', 0, &state);
    return nick;
}
unsigned int get_yyyymmdd(void)
{

    time_t t = time(NULL);
    struct tm *tm = gmtime(&t);
    unsigned int x = 0;

    x += (tm->tm_year+1900) * 10000;
    x += (tm->tm_mon+1) * 100;
    x += tm->tm_mday;

    return x;
}

RNGState *fizzer_initrand(RNGState *state, unsigned int seed)
{
    fizzer_srand(state, seed);
    return state;
}
void fizzer_srand(RNGState *state, unsigned int seed)
{
    int i;
    (*state)[0x15F] = 0;
    while ((*state)[0x15F] < 0x15F) {
	seed *= 0x1C8E815;
	seed--;
	(*state)[(*state)[0x15F]++] = seed;
    }
    for (i = 1000; i != 0; i--)
	(void) fizzer_random(state);
}

unsigned int fizzer_random(RNGState *state)
{
    if ((*state)[0x15F] >= 0x15F) {
	int j = 0xAF;
	unsigned int *ptr = (unsigned int *)state;
	int i = 0x15E;
	while (i != 0) {
	    unsigned int b = ptr[0];  /* b because stored in ebx */
	    unsigned int a = ptr[1];  /* likewise */
	    a ^= b;
	    a &= 0x7FFFF;
	    a ^= b;
	    b ^= a;
	    b &= 1;
	    b = -((signed) b);
	    b &= 0xE4BD75F5;  /* 0xE4BD75F5 or zero */
	    b ^= (*state)[j];
	    a >>= 1;
	    b ^= a;
	    *ptr = b;
	    j++;
	    if (j >= 0x15F)
		j = 0;
	    i--;
	    ptr++;
	}  /* while (i != 0) */
	{
	    unsigned int d = (*state)[0x15E];
	    unsigned int a = d;
	    a ^= (*state)[0];
	    a &= 0x7FFFF;
	    a ^= d;
	    d = -(a & 1) & 0xE4BD75F5;
	    d ^= (*state)[0xAE];
	    a >>= 1;
	    d ^= a;
	    (*state)[0x15F] = 0;
	    (*state)[0x15E] = d;
	}
    }  /* if ((*state)[0x15F] >= 0x15F) */
    return (*state)[(*state)[0x15F]++];
}

int randrange(RNGState *state, int low, int high)
{
    double tmp1, tmp2, tmp3;
    int val;
    tmp1 = fizzer_random(state);
  
    tmp1 /= (double)0xFFFFFFFF;
    tmp2 = high;
    tmp3 = low;
    if (tmp3 < 0)
	tmp3 = 0;
    if (tmp2 > 0xFFFFFFFF)
	tmp2 = 0xFFFFFFFF;
    tmp2 -= tmp3;
    tmp2 += 1;
    tmp2 *= tmp1;
    val = (int)(tmp2+0.5) + low;
    if (val > high)
	val = high;
    if (val < low)
	val = low;
    return val;
}

unsigned int srandrange(unsigned int low, unsigned int high,
                        unsigned int seed, RNGState *state)
{
    return randrange(state, low, high);
}
#endif /* FIZZER_CHECK */

/* get_mode_string 
 * Requesting a user his usermodes for umodes on the whois 
 */

char *get_mode_string(struct Client *acptr)
{
    int i;
    char *m;
    static char buf[32];
     
     m = buf;
     *m++ = '+';

      for (i = 0; user_modes[i].letter && (m - buf < BUFSIZE - 4);i++)
        if (acptr->umodes & user_modes[i].mode)
          *m++ = user_modes[i].letter;
      *m = '\0'; 
      return buf;
}

/* check idle actions for client */
void check_idle_actions(aClient* cptr)
{
  char *parv[4];

  if(!IsPerson(cptr))
    return;
  
  /* check idle oper */
  if(MaxOperIdleTime && (cptr->user) && IsAnOper(cptr) &&
    CurrentTime - cptr->user->last > MaxOperIdleTime)
    {
      parv[0] = cptr->name;
      parv[1] = cptr->name;
      parv[2] = "-o";
      parv[3] = NULL;
      user_mode(cptr, cptr, 3, parv);
      sendto_one(cptr, ":%s NOTICE %s :Excessive idle time, oper removed",
        me.name, cptr->name);
    }
    /* check awat for idle user */
    if(AutoAwayIdleTime && (cptr->user->away == NULL) &&
        CurrentTime - cptr->user->last > AutoAwayIdleTime)
      {
        parv[0] = cptr->name;
      	parv[1] = "Auto Away on Idle";
      	parv[2] = NULL;
      	m_away(cptr, cptr, 2, parv);
      	sendto_one(cptr, ":%s NOTICE %s :Auto-Away from server, idle!",
      	  me.name, cptr->name);
        SetAutoAway(cptr);
      }
}

/* remove autoway if set  */
void remove_autoaway(aClient *cptr)
{
  char *parv[2];
  if(IsAutoAway(cptr) && cptr->user && cptr->user->away)
    {
      parv[0] = cptr->name;  
      parv[1] = NULL;
      m_away(cptr, cptr, 1, parv);
      sendto_one(cptr, ":%s NOTICE %s :Auto-Away was removed",
        me.name, cptr->name);
      ClearAutoAway(cptr);
    }
}

/* m_login
   login command for human verification code
   */
int m_login(aClient *cptr, aClient *sptr, int parc, char *parv[])
{
  int inum;
  char* mename = me.name;
  
  if(!hvc_enabled || (cptr->hvc == 0) || (parc < 2))
    return;
    
  inum = atoi(parv[1]);
  if(cptr->hvc == inum)
  {
    SetClient(sptr);
    cptr->hvc == 0;
    sendto_one(sptr, form_str(RPL_WELCOME), 
      mename, cptr->name, NetworkName, cptr->name);
#ifdef SHORT_MOTD
      sendto_one(sptr,"NOTICE %s :*** Notice -- motd was last changed at %s",
                 sptr->name,
                 ConfigFileEntry.motd.lastChangedDate);

      sendto_one(sptr,
                 "NOTICE %s :*** Notice -- Please read the motd if you haven't read it",
                 sptr->name);
      
      sendto_one(sptr, form_str(RPL_MOTDSTART),
                 mename, sptr->name, mename);
      
      sendto_one(sptr,
                 form_str(RPL_MOTD),
                 mename, sptr->name,
                 "*** This is the short motd ***"
                 );

      sendto_one(sptr, form_str(RPL_ENDOFMOTD),
                 mename, sptr->name);
#else
  if (!IsWebchat(sptr))
       SendMessageFile(sptr, FindMotd(sptr->host));
  else
      SendMessageFile(sptr, &ConfigFileEntry.wmotd);	  
#endif
    if(NetworkAUP) 
    	sendto_one(sptr, ":%s NOTICE %s :%s", mename, sptr->name, NetworkAUP);
    if (IsSsl(sptr))    // ssl patch by common
        sendto_one(sptr,             ":%s NOTICE %s :*** You are using a SSL Connection. Nice Idea.",
                                  mename,sptr->name);        
  }
  cptr->hvc = 0;
}
