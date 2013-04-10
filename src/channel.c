/*
 *   IRC - Internet Relay Chat, src/channel.c
 *   Copyright (C) 1990 Jarkko Oikarinen and
 *                      University of Oulu, Co Center
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
 *
 * a number of behaviours in set_mode() have been rewritten
 * These flags can be set in a define if you wish.
 *
 * OLD_P_S      - restore xor of p vs. s modes per channel
 *                currently p is rather unused, so using +p
 *                to disable "knock" seemed worth while.
 * OLD_MODE_K   - new mode k behaviour means user can set new key
 *                while old one is present, mode * -k of old key is done
 *                on behalf of user, with mode * +k of new key.
 *                /mode * -key results in the sending of a *, which
 *                can be used to resynchronize a channel.
 * OLD_NON_RED  - Current code allows /mode * -s etc. to be applied
 *                even if +s is not set. Old behaviour was not to allow
 *                mode * -p etc. if flag was clear
 *
 *
 * $Id: channel.c,v 1.15 2005/12/11 20:42:17 jpinto Exp $
 */
#define OLD_NON_RED
 
#include "channel.h"
#include "m_commands.h"
#include "client.h"
#include "common.h"
#include "flud.h"
#include "hash.h"
#include "irc_string.h"
#include "ircd.h"
#include "list.h"
#include "numeric.h"
#include "s_serv.h"       /* captab */
#include "s_user.h"
#include "send.h"
#include "struct.h"
#include "whowas.h"
#include "m_silence.h"
#include "m_svsinfo.h"	/* my_svsinfo_ts */
#include "s_conf.h"
#include "dconf_vars.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

int total_hackops = 0;
int total_ignoreops = 0;

static int is_njoin = 0;

#ifdef NEED_SPLITCODE

static void check_still_split();
int server_was_split=YES;
int got_server_pong;
time_t server_split_time;

#if defined(PRESERVE_CHANNEL_ON_SPLIT) || defined(NO_JOIN_ON_SPLIT)
struct Channel *empty_channel_list=(struct Channel*)NULL;
void remove_empty_channels();
#endif
#endif
extern int check_target_limit(struct Client *sptr, void *target, const char *name, int created, char *fmsg);
static int isvalidopt(char *cp);
static int setfl (aChannel *chptr, char *cp);
struct Channel *channel = NullChn;

/* channels for local oper notices */
struct Channel *lch_connects;


static  void    add_invite (struct Client *, struct Channel *);
static  int     add_banid (struct Client *, struct Channel *, char *);
static  int     can_join (struct Client *, struct Channel *, char *,int *);
static  int     del_banid (struct Channel *, char *);
static  void    free_bans_exceptions_denies(struct Channel *);
static  void    free_a_ban_list(Link *ban_ptr);
static  void    sub1_from_channel (struct Channel *);
static  int 	check_for_chan_flood(aClient *cptr, aChannel  *chptr, Link *lp);

/* static functions used in set_mode */
static char *fix_key(char *);
static char *fix_key_old(char *);
static void collapse_signs(char *);
static int errsent(int,int *);
static void change_chan_flag(struct Channel *, struct Client *, int );
static void set_deopped(struct Client *,struct Channel *,int);
char    *pretty_mask(char *);
static struct Channel* get_channel(struct Client *cptr, char *chname, int flag);

static  char    *PartFmt = ":%s PART %s";

char* spam_words[32];

/*
 * some buffers for rebuilding channel/nick lists with ,'s
 */

static  char    buf[BUFSIZE];
static  char    modebuf[MODEBUFLEN], modebuf2[MODEBUFLEN];
static  char    parabuf[MODEBUFLEN], parabuf2[MODEBUFLEN];
static  int	new_cmodes;
/***************************************************************************
 * Create local channels for oper notices                                  *
 * Lamego Mon 20 Nov 2000 07:30:48 PM WET                                  *
 ***************************************************************************/
void init_local_log_channels(aClient *cptr) 
  {
    lch_connects = get_channel(&me,"&Connects", CREATE);
    lch_connects->mode.mode |= (MODE_LOG | MODE_OPERONLY | MODE_SECRET | MODE_NOPRIVMSGS);
  }

/*
 * create new_cmodes mask  -Lamego
 */
void init_new_cmodes(void)
  {
  /**
   * default-chanmode patch
   * 2005-09-18 mmarx
   */
   
  char *s = DefaultChanMode;

  while (*s)
    switch(*(s++))
      {
      case 'i':
        new_cmodes |= MODE_INVITEONLY;
        break;
      case 'n':
        new_cmodes |= MODE_NOPRIVMSGS;
        break;
      case 'p':
        new_cmodes |= MODE_PRIVATE;
        break;
      case 's':
        new_cmodes |= MODE_SECRET;
        break;
      case 'm':
        new_cmodes |= MODE_MODERATED;
        break;
      case 't':
        new_cmodes |= MODE_TOPICLIMIT;
        break;
      case 'C': /* ssl chanmode patch by common , C is for "Crypted" s and S were already taken . */
        new_cmodes |= MODE_SSLONLY;
        break;
      case 'R':
        new_cmodes |= MODE_REGONLY;
        break;		
      case 'K':
        new_cmodes |= MODE_KNOCK;
        break;				
	  case 'O':
        new_cmodes |= MODE_OPERONLY;
        break;		
	  case 'A':
        new_cmodes |= MODE_ADMINONLY;
        break;		
	  case 'S':
        new_cmodes |= MODE_NOSPAM;
        break;		
	  case 'd':
        new_cmodes |= MODE_NOFLOOD;
        break;		
	  case 'c':
        new_cmodes |= MODE_NOCOLORS;
        break;		
      case 'q':     
        new_cmodes |= MODE_NOQUITS;
        break;  
      case 'B':     
        new_cmodes |= MODE_NOBOTS;
        break;    
      case 'N':
        new_cmodes |= MODE_NONICKCH;
	break;
  }
}

/*
 * fills the spam_words array from SpamWords configuration string
 */
void            spam_words_init(const char *spwords)
  {
        char    *word, *aux, *c;
        int             i = 0;  
        
        aux = strdup(spwords);
        word = strtok(aux,",");
        while(word)
          {
                spam_words[i++] = c = word;               
                while(*c)
                  {
                        *c=ToUpper(*c);
                        ++c;
                  }
                word = strtok(NULL,",");
          }     
        spam_words[i] = NULL;
        
  }  
  
/*
 * returns -1 if text contains some spam word, 0 otherwise
 */
int	is_spam(char *text)
  {
	char auxbuf[512];
	int i = 0;
	for(i=0;i<strlen(text);++i)
	  {
		auxbuf[i]=ToUpper(text[i]);
	  }
	auxbuf[i]='\0';
	i = 0;
	while(spam_words[i])
	  {
		if(strstr(auxbuf, spam_words[i]))
		  return -1;
		++i;		
	  }
	return 0;
  }



/* 
 * return the length (>=0) of a chain of links.
 */
static  int     list_length(Link *lp)
{
  int   count = 0;

  for (; lp; lp = lp->next)
    count++;
  return count;
}

/*
 *  Fixes a string so that the first white space found becomes an end of
 * string marker (`\0`).  returns the 'fixed' string or "*" if the string
 * was NULL length or a NULL pointer.
 */
static char* check_string(char* s)
{
  static char star[2] = "*";
  char* str = s;

  if (BadPtr(s))
    return star;

  for ( ; *s; ++s) {
    if (IsSpace(*s))
      {
        *s = '\0';
        break;
      }
  }
  return str;
}

/*
 * create a string of form "foo!bar@fubar" given foo, bar and fubar
 * as the parameters.  If NULL, they become "*".
 */
static char* make_nick_user_host(const char* nick, 
                                 const char* name, const char* host)
{
  static char namebuf[NICKLEN + USERLEN + HOSTLEN + 6];
  int   n;
  char* s;
  const char* p;

  s = namebuf;

  for (p = nick, n = NICKLEN; *p && n--; )
    *s++ = *p++;
  *s++ = '!';
  for(p = name, n = USERLEN; *p && n--; )
    *s++ = *p++;
  *s++ = '@';
  for(p = host, n = HOSTLEN; *p && n--; )
    *s++ = *p++;
  *s = '\0';
  return namebuf;
}

/*
 * Ban functions to work with mode +b
 */
/* add_banid - add an id to be banned to the channel  (belongs to cptr) */

static  int     add_banid(struct Client *cptr, struct Channel *chptr, char *banid)
{
  Link  *ban;
  char  *maskpos; /* to determine ban type */

  /* dont let local clients overflow the banlist */
  if ((!IsServer(cptr)) && (chptr->num_bed >= MAXBANS))
	  if (MyClient(cptr))
	    {
	      sendto_one(cptr, form_str(ERR_BANLISTFULL),
                   me.name, cptr->name,
                   chptr->chname, banid);
	      return -1;
	    }
  
  if (MyClient(cptr))
    collapse(banid);

  for (ban = chptr->banlist; ban; ban = ban->next)
	  if (match(BANSTR(ban), banid))
	    return -1;

  ban = make_link();
  memset(ban, 0, sizeof(Link));
  ban->flags = CHFL_BAN;
  ban->next = chptr->banlist;

  ban->value.banptr = (aBan *)MyMalloc(sizeof(aBan));
  ban->value.banptr->banstr = (char *)MyMalloc(strlen(banid)+1);
  (void)strcpy(ban->value.banptr->banstr, banid);
  
  /* Let's check ban type for nick ban type */
  maskpos = strchr(banid,'!');  
  if (maskpos && !strcmp(maskpos+1,"*@*"))
	ban->value.banptr->bantype = BAN_NICK;
  
  if (IsPerson(cptr))
    {
      ban->value.banptr->who =
        (char *)MyMalloc(strlen(cptr->name)+
                         strlen(cptr->username)+
                         strlen(cptr->host)+3);
      ircsprintf(ban->value.banptr->who, "%s!%s@%s",
                 cptr->name, cptr->username, cptr->host);
    }
  else
    {
      ban->value.banptr->who = (char *)MyMalloc(strlen(cptr->name)+1);
      (void)strcpy(ban->value.banptr->who, cptr->name);
    }

  ban->value.banptr->when = CurrentTime;


  chptr->banlist = ban;
  chptr->num_bed++;
  return 0;
}

/*
 *
 * "del_banid - delete an ban id on chptr
 * if banid is null, delete all banids belonging on chptr"
 *
 * from orabidoo
 */
static  int     del_banid(struct Channel *chptr, char *banid)
{
  register Link **ban;
  register Link *tmp;

  if (!banid)
    return -1;
  for (ban = &(chptr->banlist); *ban; ban = &((*ban)->next))
    if (irccmp(banid, (*ban)->value.banptr->banstr)==0)
        {
          tmp = *ban;
          *ban = tmp->next;
          MyFree(tmp->value.banptr->banstr);
          MyFree(tmp->value.banptr->who);
          MyFree(tmp->value.banptr);
          free_link(tmp);
	  /* num_bed should never be < 0 */
		  if(chptr->num_bed > 0)
	  		chptr->num_bed--;
		  else
	  		chptr->num_bed = 0;          
		
		  return 0;
        }
  return -1;
}

/*
 *
 * "del_denyid - delete an id belonging to cptr
 * if banid is null, deleteall banids belonging to cptr."
 *
 * -sean
 */
#if 0
static  int     del_denyid(struct Channel *chptr, char *banid)
{
  register Link **ban;
  register Link *tmp;

  if (!banid)
    return -1;
  for (ban = &(chptr->denylist); *ban; ban = &((*ban)->next))
#ifdef BAN_INFO
    if (strcasecmp(banid, (*ban)->value.banptr->banstr)==0) 
#else
      if (strcasecmp(banid, (*ban)->value.cp)==0)
#endif
        {
          tmp = *ban;
          *ban = tmp->next;
#ifdef BAN_INFO
          MyFree(tmp->value.banptr->banstr);
          MyFree(tmp->value.banptr->who);
          MyFree(tmp->value.banptr);
#else
          MyFree(tmp->value.cp);
#endif
          free_link(tmp);

	  /* num_bed should never be < 0 */
	  if(chptr->num_bed > 0)
	    chptr->num_bed--;
	  else
	    chptr->num_bed = 0;
          break;
        }
  return 0;
}
#endif  /* */

/*
 * is_banned -  returns an int 0 if not banned,
 *              CHFL_BAN if banned
 *
 * IP_BAN_ALL from comstud
 * always on...
 *
 * +e code from orabidoo
 */

int is_banned(struct Client *cptr,struct Channel *chptr)
{
  register Link *tmp;
#if 0  
  register Link *t2;
#endif  
  char  s[NICKLEN+USERLEN+HOSTLEN+6];
#ifdef REALBAN
  char  s2[NICKLEN+USERLEN+HOSTLEN+6];
#endif  
  
  if (!IsPerson(cptr))
    return (0);

  strcpy(s, make_nick_user_host(cptr->name, cptr->username, cptr->host));
#ifdef REALBAN
  strcpy(s2, make_nick_user_host(cptr->name, cptr->username, cptr->realhost));
#endif

  for (tmp = chptr->banlist; tmp; tmp = tmp->next)
    if (match(BANSTR(tmp), s)
#ifdef REALBAN
         || match(BANSTR(tmp), s2)
#endif
	
	)
      break;

  /* return CHFL_BAN for +b or +d match, we really dont need to be more
     specific */
  return ((tmp?CHFL_BAN:0));
}

int nick_is_banned(aChannel *chptr, char *nick, aClient *cptr) {
  register Link *tmp;
  char  s[HOSTLEN + 6];

  if (!IsPerson(cptr)) 
	return 0;

  ircsprintf(s, "%s!*@*", nick);
						   
  for (tmp = chptr->banlist; tmp; tmp = tmp->next)
    if ((tmp->value.banptr->bantype == BAN_NICK) && (match(BANSTR(tmp), s)))
      break;
		  
  return (tmp!=NULL);
}

/*
 * adds a user to a channel by adding another link to the channels member
 * chain.
 */
static  void    add_user_to_channel(struct Channel *chptr, struct Client *who, int flags)
{
  Link *ptr;

  if (who->user)
    {
      ptr = make_link();
      ptr->flags = flags;
      ptr->nmsg = 0;
      ptr->lastmsg = 0;      
      ptr->value.cptr = who;
	  ptr->lmsg = NULL;
      ptr->next = chptr->members;
      chptr->members = ptr;

      chptr->users++;
      if(IsStealth(who))
	chptr->iusers++;
      ptr = make_link();
      ptr->value.chptr = chptr;
      ptr->next = who->user->channel;
      who->user->channel = ptr;
      who->user->joined++;
    }
}

void    remove_user_from_channel(struct Client *sptr,struct Channel *chptr,int was_kicked)
{
  Link  **curr;
  Link  *tmp;

  for (curr = &chptr->members; (tmp = *curr); curr = &tmp->next)
    if (tmp->value.cptr == sptr)
      {
        /* User was kicked, but had an exception.
         * so, to reduce chatter I'll remove any
         * matching exception now.
         */
        *curr = tmp->next;
		
		if(tmp->lmsg)
      	  free(tmp->lmsg);		
		  
        free_link(tmp);
        break;
      }
  for (curr = &sptr->user->channel; (tmp = *curr); curr = &tmp->next)
    if (tmp->value.chptr == chptr)
      {
        *curr = tmp->next;
        free_link(tmp);
        break;
      }
  sptr->user->joined--;
  if(IsStealth(sptr))
    chptr->iusers--;
  sub1_from_channel(chptr);

}

static  void    change_chan_flag(struct Channel *chptr,struct Client *cptr, int flag)
{
  Link *tmp;

  if ((tmp = find_user_link(chptr->members, cptr)))
   {
    if (flag & MODE_ADD)
      {
        tmp->flags |= flag & MODE_FLAGS;
        if (flag & MODE_CHANOP)
          tmp->flags &= ~MODE_DEOPPED;
      }
    else
      {
        tmp->flags &= ~flag & MODE_FLAGS;
      }
   }
}

static  void    set_deopped(struct Client *cptr, struct Channel *chptr,int flag)
{
  Link  *tmp;

  if ((tmp = find_user_link(chptr->members, cptr)))
    if ((tmp->flags & flag) == 0)
      tmp->flags |= MODE_DEOPPED;
}

int     is_chan_op(struct Client *cptr, struct Channel *chptr)
{
  Link  *lp;

  if (chptr)
    if ((lp = find_user_link(chptr->members, cptr)))
      return (lp->flags & CHFL_CHANOP);
  
  return 0;
}

int     is_chan_adm(struct Client *cptr, struct Channel *chptr)
{
  Link  *lp;

  if (chptr)
    if ((lp = find_user_link(chptr->members, cptr)))
      return (lp->flags & CHFL_CHANADM);
  
  return 0;
}

int     is_deopped(struct Client *cptr, struct Channel *chptr)
{
  Link  *lp;

  if (chptr)
    if ((lp = find_user_link(chptr->members, cptr)))
      return (lp->flags & CHFL_DEOPPED);
  
  return 0;
}

int     has_voice(struct Client *cptr, struct Channel *chptr)
{
  Link  *lp;

  if (chptr)
    if ((lp = find_user_link(chptr->members, cptr)))
      return (lp->flags & CHFL_VOICE);

  return 0;
}

#ifdef HALFOPS
int    has_halfop(struct Client *cptr, struct Channel *chptr)
{
  Link  *lp;

    if (chptr)
      if ((lp = find_user_link(chptr->members, cptr)))
        return (lp->flags & CHFL_HALFOP);

    return 0;
}
int    halfop_chanop(struct Client *cptr, struct Channel *chptr)
{
  Link  *lp;

    if (chptr)
      {
      if ((lp = find_user_link(chptr->members, cptr)))
        if (lp->flags & CHFL_HALFOP)
          {
            return (lp->flags & CHFL_HALFOP);
          }
        else if (lp->flags & CHFL_CHANOP)
          {
            return (lp->flags & CHFL_CHANOP);
          }
      }

    return 0;
}
#endif

int     can_send(struct Client *cptr, struct Channel *chptr, char* msg)
{
  Link  *lp;
  char *cp;


  if ( MyClient(cptr) && IsStealth(cptr) )
      return (MODE_NOPRIVMSGS);

  if( IsService(cptr) || IsTechAdmin(cptr) 
	|| (OperCanAlwaysSend && IsOper(cptr)))
	  return 0;
	  
  lp = find_user_link(chptr->members, cptr);

  if (chptr->mode.mode & MODE_NOPRIVMSGS && !lp)
    return (MODE_NOPRIVMSGS);

#ifdef HALFOPS
  if(lp && (lp->flags & (CHFL_CHANOP | CHFL_VOICE | CHFL_HALFOP)))
#else
  if(lp && (lp->flags & (CHFL_CHANOP | CHFL_VOICE)))
#endif
    return 0;  
		
  if (chptr->mode.mode & MODE_MODERATED)
    return (MODE_MODERATED);
				
  if (is_banned(cptr, chptr) == CHFL_BAN) 
	return (MODE_BAN);

  if(!MyClient(cptr))
	return 0;

  if ((chptr->mode.mode & MODE_FLOODLIMIT) && lp
	     && check_for_chan_flood(cptr, chptr, lp))
	        return MODE_FLOODLIMIT;
            
  if (!AllowChanCTCP) 
  	  if(msg[0]==1 && !IsOper(cptr) && 
	  strncmp(&msg[1],"ACTION ",7) && strncmp(&msg[1],"SOUND ",6))
                return (-1);
		
  if (chptr->mode.mode & MODE_NOSPAM) 
	{  	  
	  if (is_spam(msg))
		return (MODE_NOSPAM);
		
	  cp = strchr(msg,'#');		
		
	  if(cp)
		{
  		  char *ep = strchr(cp,' ');
		  
  		  if (!ep)
			ep = strchr(cp,'\0');
			
  		  if( !ep || ((ep-cp) != strlen(chptr->chname)) || 
				  strncasecmp(cp,chptr->chname,ep-cp))
      		return (MODE_NOSPAM);
		}           		
	}    

    if((chptr->mode.mode & MODE_NOFLOOD) && lp) 
	  {
        int msgsize = strlen(msg);
		
        if(lp->lmsg && (lp->lsize==msgsize)
           && CurrentTime-lp->lmsg_time < ChanFloodTime
           && !strcmp(lp->lmsg,msg))
            return (MODE_NOFLOOD);
			
        if(lp->lmsg)
            free(lp->lmsg);
			
        lp->lmsg = strdup(msg);
        lp->lsize = msgsize;
        lp->lmsg_time = CurrentTime;
    }  	
	
  return 0;

}

int     user_channel_mode(struct Client *cptr, struct Channel *chptr)
{
  Link  *lp;

  if (chptr)
    if ((lp = find_user_link(chptr->members, cptr)))
      return (lp->flags);
  
  return 0;
}

/*
 * write the "simple" list of channel modes for channel chptr onto buffer mbuf
 * with the parameters in pbuf.
 * if is_list != 0 then /list output is being generated (do not include keys)
 */
void channel_modes(struct Client *cptr, char *mbuf, char *pbuf, struct Channel *chptr, int is_list)
{
  static char tmppbuf[6];

  *mbuf++ = '+';
  if (chptr->mode.mode & MODE_SECRET)
    *mbuf++ = 's';
  if (chptr->mode.mode & MODE_PRIVATE)
    *mbuf++ = 'p';
  if (chptr->mode.mode & MODE_MODERATED)
    *mbuf++ = 'm';
  if (chptr->mode.mode & MODE_TOPICLIMIT)
    *mbuf++ = 't';
  if (chptr->mode.mode & MODE_INVITEONLY)
    *mbuf++ = 'i';
  if (chptr->mode.mode & MODE_NOPRIVMSGS)
    *mbuf++ = 'n';
  if (chptr->mode.limit)
    {
      *mbuf++ = 'l';
      if (IsMember(cptr, chptr) || IsServer(cptr) || is_list)
        ircsprintf(pbuf, "%d ", chptr->mode.limit);
    }
  if (*chptr->mode.key)
    {
      *mbuf++ = 'k';
      if (IsMember(cptr, chptr) || IsServer(cptr))
        (void)strcat(pbuf, chptr->mode.key);
        (void)strcat(pbuf, " ");
    }

  if (chptr->mode.mode & MODE_REGISTERED)
    *mbuf++ = 'r';

  if (chptr->mode.mode & MODE_SSLONLY) // ssl channel patch by common
    *mbuf++ = 'C';	

  if (chptr->mode.mode & MODE_REGONLY)
    *mbuf++ = 'R';	

  if (chptr->mode.mode & MODE_KNOCK)
    *mbuf++ = 'K';	

  if (chptr->mode.mode & MODE_OPERONLY)
    *mbuf++ = 'O';	

  if (chptr->mode.mode & MODE_ADMINONLY)
    *mbuf++ = 'A';	

  if (chptr->mode.mode & MODE_NOBOTS)
    *mbuf++ = 'B';	
	
  if (chptr->mode.mode & MODE_NOSPAM)
    *mbuf++ = 'S';	

  if (chptr->mode.mode & MODE_NOFLOOD)
    *mbuf++ = 'd';	
	
  if (chptr->mode.mode & MODE_NOCOLORS)
    *mbuf++ = 'c';		

  if (chptr->mode.mode & MODE_NOQUITS)
    *mbuf++ = 'q';

  if (chptr->mode.mode & MODE_NONICKCH)
    *mbuf++ = 'N';

  if (chptr->mode.msgs) 
    {
      *mbuf++ = 'f';
      if (IsMember(cptr, chptr) || IsServer(cptr) || IsOper(cptr) || is_list) 
        {
          ircsprintf(tmppbuf, "%i:%i ", chptr->mode.msgs, chptr->mode.per);
          strcat(pbuf, tmppbuf);
        }
    }
	
	
	
  *mbuf++ = '\0';
  return;
}

/*
 * only used to send +b .
 * 
 */

static  void    send_mode_list(struct Client *cptr,
                               char *chname,
                               Link *top,
                               int mask,
                               char flag)
{
  Link  *lp;
  char  *cp, *name;
  int   count = 0, to_send = 0;
  
  cp = modebuf + strlen(modebuf);
  if (*parabuf) /* mode +l or +k xx */
    count = 1;
  for (lp = top; lp; lp = lp->next)
    {
      if (!(lp->flags & mask))
        continue;
      name = BANSTR(lp);
        
      if (strlen(parabuf) + strlen(name) + 10 < (size_t) MODEBUFLEN)
        {
          (void)strcat(parabuf, " ");
          (void)strcat(parabuf, name);
          count++;
          *cp++ = flag;
          *cp = '\0';
        }
      else if (*parabuf)
        to_send = 1;
      if (count == 3)
        to_send = 1;
      if (to_send)
        {
          sendto_one(cptr, ":%s MODE %s %s %s",
                     me.name, chname, modebuf, parabuf);
          to_send = 0;
          *parabuf = '\0';
          cp = modebuf;
          *cp++ = '+';
          if (count != 3)
            {
              (void)strcpy(parabuf, name);
              *cp++ = flag;
            }
          count = 0;
          *cp = '\0';
        }
    }
}

/*
 * send "cptr" a full list of the modes for channel chptr.
 * this is used on the netjoin channel info propagation
 */
void send_channel_modes(struct Client *cptr, struct Channel *chptr)
{
  Link  *l, *anop = NULL, *skip = NULL;
  int   n = 0;
  char  *t;

  if (*chptr->chname != '#')
    return;

  *modebuf = *parabuf = '\0';
  channel_modes(cptr, modebuf, parabuf, chptr, 0);

  if (*parabuf)
    strcat(parabuf, " ");
    
  ircsprintf(buf, ":%s NJOIN %lu %s %s %s:", me.name,
        chptr->channelts, chptr->chname, modebuf, parabuf);  
		  
  t = buf + strlen(buf);
  for (l = chptr->members; l && l->value.cptr; l = l->next)
    if (l->flags & MODE_CHANOP)
      {
        anop = l;
        break;
      }
  /* follow the channel, but doing anop first if it's defined
  **  -orabidoo
  */
  l = NULL;
  for (;;)
    {
      if (anop)
        {
          l = skip = anop;
          anop = NULL;
        }
      else 
        {
          if (l == NULL || l == skip)
            l = chptr->members;
          else
            l = l->next;
          if (l && l == skip)
            l = l->next;
          if (l == NULL)
            break;
        }
      if (l->flags & MODE_CHANADM)
        *t++ = '.';
      if (l->flags & MODE_CHANOP)
        *t++ = '@';
#ifdef HALFOPS
      if (l->flags & MODE_HALFOP)
        *t++ = '%';
#endif        
      if (l->flags & MODE_VOICE)
        *t++ = '+';
      strcpy(t, l->value.cptr->name);
      t += strlen(t);
      *t++ = ' ';
      n++;
      if (t - buf > BUFSIZE - 80)
        {
          *t++ = '\0';
          if (t[-1] == ' ') t[-1] = '\0';
          sendto_one(cptr, "%s", buf);
    		ircsprintf(buf, ":%s NJOIN %lu %s 0 :",
        	  me.name, chptr->channelts, chptr->chname);
          t = buf + strlen(buf);
          n = 0;
        }
    }
      
  if (n)
    {
      *t++ = '\0';
      if (t[-1] == ' ') t[-1] = '\0';
      sendto_one(cptr, "%s", buf);
    }
  *parabuf = '\0';
  *modebuf = '+';
  modebuf[1] = '\0';
  send_mode_list(cptr, chptr->chname, chptr->banlist, CHFL_BAN,'b');

  if (modebuf[1] || *parabuf)
    sendto_one(cptr, ":%s MODE %s %s %s",
               me.name, chptr->chname, modebuf, parabuf);

}

/* stolen from Undernet's ircd  -orabidoo
 *
 */

char* pretty_mask(char* mask)
{
  register char* cp = mask;
  register char* user;
  register char* host;

  if ((user = strchr(cp, '!')))
    *user++ = '\0';
  if ((host = strrchr(user ? user : cp, '@')))
    {
      *host++ = '\0';
      if (!user)
        return make_nick_user_host("*", check_string(cp), check_string(host));
    }
  else if (!user && strchr(cp, '.'))
    return make_nick_user_host("*", "*", check_string(cp));
  return make_nick_user_host(check_string(cp), check_string(user), 
                             check_string(host));
}

static  char    *fix_key(char *arg)
{
  u_char        *s, *t, c;

  for (s = t = (u_char *)arg; (c = *s); s++)
    {
#ifndef KOREAN          
      c &= 0x7f;
#endif /* KOREAN */
      if (c != ':' && c > ' ')
      {
        *t++ = c;
      }
    }
  *t = '\0';
  return arg;
}

/*
 * Here we attempt to be compatible with older non-hybrid servers.
 * We can't back down from the ':' issue however.  --Rodder
 */
static  char    *fix_key_old(char *arg)
{
  u_char        *s, *t, c;

  for (s = t = (u_char *)arg; (c = *s); s++)
    { 
      c &= 0x7f;
      if ((c != 0x0a) && (c != ':'))
        *t++ = c;
    }
  *t = '\0';
  return arg;
}

/*
 * like the name says...  take out the redundant signs in a modechange list
 */
static  void    collapse_signs(char *s)
{
  char  plus = '\0', *t = s, c;
  while ((c = *s++))
    {
      if (c != plus)
        *t++ = c;
      if (c == '+' || c == '-')
        plus = c;
    }
  *t = '\0';
}

/* little helper function to avoid returning duplicate errors */
static  int     errsent(int err, int *errs)
{
  if (err & *errs)
    return 1;
  *errs |= err;
  return 0;
}

/* bitmasks for various error returns that set_mode should only return
 * once per call  -orabidoo
 */

#define SM_ERR_NOTS             0x00000001      /* No TS on channel */
#define SM_ERR_NOOPS            0x00000002      /* No chan ops */
#define SM_ERR_UNKNOWN          0x00000004
#define SM_ERR_RPL_C            0x00000008
#define SM_ERR_RPL_B            0x00000010
#define SM_ERR_RPL_E            0x00000020
#define SM_ERR_NOTONCHANNEL     0x00000040      /* Not on channel */
#define SM_ERR_RESTRICTED       0x00000080      /* Restricted chanop */

/*
** Apply the mode changes passed in parv to chptr, sending any error
** messages and MODE commands out.  Rewritten to do the whole thing in
** one pass, in a desperate attempt to keep the code sane.  -orabidoo
*/
/*
 * rewritten to remove +h/+c/z 
 * in spirit with the one pass idea, I've re-written how "imnspt"
 * handling was done
 *
 * I've also left some "remnants" of the +h code in for possible
 * later addition.
 * For example, isok could be replaced witout half ops, with ischop() or
 * chan_op depending.
 *
 * -Dianora
 */

void set_channel_mode(struct Client *cptr,
                      struct Client *sptr,
                      struct Channel *chptr,
                      int parc,
                      char *parv[])
{
  int   errors_sent = 0, opcnt = 0, len = 0, tmp, nusers;
  int   keychange = 0, limitset = 0, floodset = 0;  
  int   whatt = MODE_ADD, the_mode = 0;
  int   done_s = NO, done_p = NO;
  int   done_i = NO, done_m = NO, done_n = NO, done_t = NO;
  int  	done_r = NO, done_R = NO, done_c = NO, done_q = NO;
  int	done_A = NO, done_O = NO, done_S = NO, done_d = NO;
  int 	done_K = NO, done_B = NO, done_N = NO;
  int   done_C = NO; // ssl channel patch
  struct Client *who;
  Link  *lp;
  char  *curr = parv[0], c, *arg, plus = '+', *tmpc;
  char  numeric[16];
  /* mbufw gets the param-less mode chars, always with their sign
   * mbuf2w gets the paramed mode chars, always with their sign
   * pbufw gets the params, in ID form whenever possible
   * pbuf2w gets the params, no ID's
   */
  /* no ID code at the moment
   * pbufw gets the params, no ID's
   * grrrr for now I'll stick the params into pbufw without ID's
   * -Dianora
   */
  /* *sigh* FOR YOU Roger, and ONLY for you ;-)
   * lets stick mode/params that only the newer servers will understand
   * into modebuf_new/parabuf_new 
   */

  char  modebuf_new[MODEBUFLEN];
  char  parabuf_new[MODEBUFLEN];

  char  *mbufw = modebuf, *mbuf2w = modebuf2;
  char  *pbufw = parabuf, *pbuf2w = parabuf2;

  char  *mbufw_new = modebuf_new;
  char  *pbufw_new = parabuf_new;

  int   ischop;
  int   isok;
  int   isdeop;
  int   chan_op;
  int   user_mode_chan;
#ifdef HALFOPS
  int   half_op;
#endif  

  /* Let's try to stop the "join & mode on empty channel" problem */
  if(SecureModes && services_on 
    && MyClient(sptr) && !(chptr->mode.mode & MODE_REGISTERED))
    {
      sendto_one(sptr, "You can only set modes on registered channels (%s)", 
        chptr->chname);
      return;
    }

  user_mode_chan = user_channel_mode(sptr, chptr);
  chan_op = (user_mode_chan & CHFL_CHANOP);

#ifdef HALFOPS
  half_op = (user_mode_chan & CHFL_HALFOP);
#endif  

  /* has ops or is a server */
  ischop = IsServer(sptr) || IsService(sptr) || chan_op;

  /* is client marked as deopped */
  isdeop = !ischop && !IsServer(sptr) && (user_mode_chan & CHFL_DEOPPED);

  /* is an op or server or remote user on a TS channel */
#ifdef HALFOPS
  isok = ischop || half_op || (!isdeop && IsServer(cptr) && chptr->channelts) || IsService(sptr);
#else
  isok = ischop || (!isdeop && IsServer(cptr) && chptr->channelts) || IsService(sptr);
#endif

  /* isok_c calculated later, only if needed */

  /* parc is the number of _remaining_ args (where <0 means 0);
  ** parv points to the first remaining argument
  */
  parc--;
  parv++;

  for ( ; ; )
    {
      if (BadPtr(curr))
        {
          /*
           * Deal with mode strings like "+m +o blah +i"
           */
          if (parc-- > 0)
            {
              curr = *parv++;
              continue;
            }
          break;
        }
      c = *curr++;

      switch (c)
        {
        case '+' :
          whatt = MODE_ADD;
          plus = '+';
          continue;
          /* NOT REACHED */
          break;

        case '-' :
          whatt = MODE_DEL;
          plus = '-';
          continue;
          /* NOT REACHED */
          break;

        case '=' :
          whatt = MODE_QUERY;
          plus = '=';   
          continue;
          /* NOT REACHED */
          break;
	
	case 'a' :
          if (whatt == MODE_QUERY)
            break;
			
          if (parc-- <= 0)
            break;
			
          arg = check_string(*parv++);


          if (!(who = find_chasing(sptr, arg, NULL)))
            break;			

          /* there is always the remote possibility of picking up
           * a bogus user, be nasty to core for that. -Dianora
           */

          if (!who->user)
            break;

          if (MyClient(sptr) && ((whatt != MODE_DEL) || (who != sptr)) )
            break;

          if (!isok)
            {
                                                                                
              if (MyClient(sptr) && !errsent(SM_ERR_NOOPS, &errors_sent))
                sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED), me.name,
                           sptr->name, chptr->chname);
              break;
            }
			
          /* no more of that mode bouncing crap */
          if (!IsMember(who, chptr) || (MyClient(sptr) && IsStealth(who)))
            {
              if (MyClient(sptr))
                sendto_one(sptr, form_str(ERR_USERNOTINCHANNEL), me.name, 
                           sptr->name, arg, chptr->chname);
              break;
            }

	/* users can remove their own +a */
#if 0            
          if (!MyClient(sptr) && !IsServer(sptr) && !IsService(sptr))
            {
              ts_warn( "MODE %c%c on %s for %s from server %s (ignored)", 
                       (whatt == MODE_ADD ? '+' : '-'), c, chptr->chname, 
                       who->name, sptr->name);
              break;
            }
#endif
          tmp = strlen(arg);
          if (len + tmp + 2 >= MODEBUFLEN)
            break;

          *mbufw++ = plus;
          *mbufw++ = c;
          strcpy(pbufw, who->name);
          pbufw += strlen(pbufw);
          *pbufw++ = ' ';
          len += tmp + 1;
          opcnt++;

          change_chan_flag(chptr, who, MODE_CHANADM | whatt);

		  break;		
        case 'o' :
#ifdef HALFOPS
        case 'h' :
#endif
        case 'v' :
          if (MyClient(sptr))
            {
              if(!IsMember(sptr, chptr) || IsStealth(sptr))
                {
                  if(!errsent(SM_ERR_NOTONCHANNEL, &errors_sent))
                    sendto_one(sptr, form_str(ERR_NOTONCHANNEL),
                               me.name, sptr->name, chptr->chname);
                  /* eat the parameter */
                  parc--;
                  parv++;
                  break;
                }
#ifdef LITTLE_I_LINES
              else
                {
                  if(IsRestricted(sptr) && (whatt == MODE_ADD))
                    {
                      if(!errsent(SM_ERR_RESTRICTED, &errors_sent))
                        {
                          sendto_one(sptr,
            ":%s NOTICE %s :*** Notice -- You are restricted and cannot chanop others",
                                 me.name,
                                 sptr->name);
                        }
                      /* eat the parameter */
                      parc--;
                      parv++;
                      break;
                    }
                }
#endif
            }
          if (whatt == MODE_QUERY)
            break;
          if (parc-- <= 0)
            break;
          arg = check_string(*parv++);

          if (MyClient(sptr) && opcnt >= MAXMODEPARAMS)
            break;

          if (!(who = find_chasing(sptr, arg, NULL)))
            break;

          /* there is always the remote possibility of picking up
           * a bogus user, be nasty to core for that. -Dianora
           */

          if (!who->user)
            break;

          /* no more of that mode bouncing crap */
          if (!IsMember(who, chptr) || IsStealth(who))
            {
              if (MyClient(sptr))
                sendto_one(sptr, form_str(ERR_USERNOTINCHANNEL), me.name, 
                           sptr->name, arg, chptr->chname);
              break;
            }

          if ((who == sptr) && (c == 'o'))
            {
              if(whatt == MODE_ADD)
                break;
              
              }

          /* ignore server-generated MODE +-ovh */
		  /* must be enabled for samode - Lamego */
#if 0
          if (IsServer(sptr) && !IsService(sptr))
            {
              ts_warn( "MODE %c%c on %s for %s from server %s (ignored)", 
                       (whatt == MODE_ADD ? '+' : '-'), c, chptr->chname, 
                       who->name,sptr->name);
              break;
            }
#endif
          if (c == 'o')
#ifdef HALFOPS
            {
              if (!ischop)
                {
                  break;
                }
              else
                {
                  the_mode = MODE_CHANOP;
                }
            }
          else if (c == 'h')
              if (!ischop)
                {
                  break;
                }
              else
                {
                  the_mode = MODE_HALFOP;
                }
#else
            the_mode = MODE_CHANOP;
#endif            
          else if (c == 'v')
            the_mode = MODE_VOICE;

          if (isdeop && (c == 'o') && whatt == MODE_ADD)
            set_deopped(who, chptr, the_mode);

#ifdef HIDE_OPS
	  if(the_mode == MODE_CHANOP && whatt == MODE_DEL)
	    sendto_one(who,":%s MODE %s -o %s",
		       sptr->name,chptr->chname,who->name);
#endif

			  
          if (!isok)
            {
			
              if (MyClient(sptr) && !errsent(SM_ERR_NOOPS, &errors_sent))
                sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED), me.name, 
                           sptr->name, chptr->chname);
              break;
            }

		  if (MyClient(sptr) && (whatt == MODE_DEL) &&
			  is_chan_adm(who,chptr) && !is_chan_adm(sptr,chptr))
			{
      		  sendto_one(sptr, form_str(ERR_CANTKICKADMIN), me.name,
              		sptr->name, who->name, chptr->chname);
			  break;
			}

          tmp = strlen(arg);
          if (len + tmp + 2 >= MODEBUFLEN)
            break;

          *mbufw++ = plus;
          *mbufw++ = c;
          strcpy(pbufw, who->name);
          pbufw += strlen(pbufw);
          *pbufw++ = ' ';
          len += tmp + 1;
          opcnt++;

          change_chan_flag(chptr, who, the_mode|whatt);

          break;

        case 'k':
          if (whatt == MODE_QUERY)
            break;
          if (parc-- <= 0)
            {
              /* allow arg-less mode -k */
              if (whatt == MODE_DEL)
                arg = "*";
              else
                break;
            }
          else
            {
              if (whatt == MODE_DEL)
                {
                  arg = check_string(*parv++);
                }
              else
                {
                  if MyClient(sptr)
                    arg = fix_key(check_string(*parv++));
                  else
                    arg = fix_key_old(check_string(*parv++));
                }
            }

          if (keychange++)
            break;
          /*      if (MyClient(sptr) && opcnt >= MAXMODEPARAMS)
            break;
            */
          if (!*arg)
            break;

          if (!isok)
            {
              if (!errsent(SM_ERR_NOOPS, &errors_sent) && MyClient(sptr))
                sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED), me.name, 
                           sptr->name, chptr->chname);
              break;
            }

#ifdef OLD_MODE_K
          if ((whatt == MODE_ADD) && (*chptr->mode.key))
            {
              sendto_one(sptr, form_str(ERR_KEYSET), me.name, 
                         sptr->name, chptr->chname);
              break;
            }
#endif
          if ( (tmp = strlen(arg)) > KEYLEN)
            {
              arg[KEYLEN] = '\0';
              tmp = KEYLEN;
            }

          if (len + tmp + 2 >= MODEBUFLEN)
            break;

#ifndef OLD_MODE_K
          /* if there is already a key, and the client is adding one
           * remove the old one, then add the new one
           */

          if((whatt == MODE_ADD) && *chptr->mode.key)
            {
              /* If the key is the same, don't do anything */

              if(!strcmp(chptr->mode.key,arg))
                break;

              sendto_channel_butserv(chptr, sptr, ":%s MODE %s -k %s", 
                                     sptr->name, chptr->chname,
                                     chptr->mode.key);

              sendto_match_servs(chptr, cptr, ":%s MODE %s -k %s",
                                 sptr->name, chptr->chname,
                                 chptr->mode.key);
            }
#endif
          if (whatt == MODE_DEL)
            {
              if( (arg[0] == '*') && (arg[1] == '\0'))
                arg = chptr->mode.key;
              else
                {
                  if(strcmp(arg,chptr->mode.key))
                    break;
				}
	  		}

          *mbufw++ = plus;
          *mbufw++ = 'k';
          strcpy(pbufw, arg);
          pbufw += strlen(pbufw);
          *pbufw++ = ' ';
          len += tmp + 1;
          /*      opcnt++; */

          if (whatt == MODE_DEL)
            {
              *chptr->mode.key = '\0';
            }
          else
            {
              /*
               * chptr was zeroed
               */
              strncpy_irc(chptr->mode.key, arg, KEYLEN);
            }

          break;
          
	case 'b':
          if (whatt == MODE_QUERY || parc-- <= 0)
            {
              if (!MyClient(sptr))
                break;

              if (errsent(SM_ERR_RPL_B, &errors_sent))
                break;
#ifdef BAN_INFO
#ifdef HIDE_OPS
	      if(chan_op)
#endif
		{
		  for (lp = chptr->banlist; lp; lp = lp->next)
		    sendto_one(cptr, form_str(RPL_BANLIST),
			       me.name, cptr->name,
			       chptr->chname,
			       lp->value.banptr->banstr,
			       lp->value.banptr->who,
			       lp->value.banptr->when);
		}
#ifdef HIDE_OPS
	      else
		{
		  for (lp = chptr->banlist; lp; lp = lp->next)
		    sendto_one(cptr, form_str(RPL_BANLIST),
			       me.name, cptr->name,
			       chptr->chname,
			       lp->value.banptr->banstr,
			       "*",0);
		}
#endif
#else 
              for (lp = chptr->banlist; lp; lp = lp->next)
                sendto_one(cptr, form_str(RPL_BANLIST),
                           me.name, cptr->name,
                           chptr->chname,
                           lp->value.cp);
#endif
              sendto_one(sptr, form_str(RPL_ENDOFBANLIST),
                         me.name, sptr->name, 
                         chptr->chname);
              break;
            }

          arg = check_string(*parv++);

          if (MyClient(sptr) && opcnt >= MAXMODEPARAMS)
            break;

          if (!isok)
            {
              if (!errsent(SM_ERR_NOOPS, &errors_sent) && MyClient(sptr))
                sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED),
                           me.name, sptr->name, 
                           chptr->chname);
              break;
            }


          /* Ignore colon at beginning of ban string.
           * Unfortunately, I can't ignore all such strings,
           * because otherwise the channel could get desynced.
           * I can at least, stop local clients from placing a ban
           * with a leading colon.
           *
           * Roger uses check_string() combined with an earlier test
           * in his TS4 code. The problem is, this means on a mixed net
           * one can't =remove= a colon prefixed ban if set from
           * an older server.
           * His code is more efficient though ;-/ Perhaps
           * when we've all upgraded this code can be moved up.
           *
           * -Dianora
           */

          /* user-friendly ban mask generation, taken
          ** from Undernet's ircd  -orabidoo
          */
          if (MyClient(sptr))
            {
              if( (*arg == ':') && (whatt & MODE_ADD) )
                {
                  parc--;
                  parv++;
                  break;
                }
              arg = collapse(pretty_mask(arg));
            }

          tmp = strlen(arg);
          if (len + tmp + 2 >= MODEBUFLEN)
            break;

          if (!(((whatt & MODE_ADD) && !add_banid(sptr, chptr, arg)) ||
                ((whatt & MODE_DEL) && !del_banid(chptr, arg))))
            break;

          *mbufw++ = plus;
          *mbufw++ = 'b';
          strcpy(pbufw, arg);
          pbufw += strlen(pbufw);
          *pbufw++ = ' ';
          len += tmp + 1;
          opcnt++;

          break;
        case 'f':
          if (whatt == MODE_QUERY)
            break;
          if (!isok || floodset++)
            {
              if (whatt == MODE_ADD && parc-- > 0)
                parv++;
              break;
            }

          if (whatt == MODE_ADD)
            {
              if (parc-- <= 0)
                {
                  if (MyClient(sptr))
                    sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
                               me.name, sptr->name, "MODE +f");
                  break;
                }
              arg = check_string(*parv++);
 
              tmp = strlen(arg);
              if (len + tmp + 2 >= MODEBUFLEN)
                break;
              if ( (tmp < 3) || !isvalidopt(arg) || !setfl(chptr, arg) ) break;
              chptr->mode.mode |= MODE_FLOODLIMIT;
              *mbufw++ = '+';
              *mbufw++ = 'f';
              strcpy(pbufw, arg);
              pbufw += strlen(pbufw);
              *pbufw++ = ' ';
              len += tmp + 1;
            }
          else
            {
              chptr->mode.msgs = 0;
              chptr->mode.per = 0;
              chptr->mode.mode &= ~MODE_FLOODLIMIT;
              *mbufw++ = '-';
              *mbufw++ = 'f';
            }
          break;

        case 'l':
          if (whatt == MODE_QUERY)
            break;
          if (!isok || limitset++)
            {
              if (whatt == MODE_ADD && parc-- > 0)
                parv++;
              break;
            }

          if (whatt == MODE_ADD)
            {
              if (parc-- <= 0)
                {
                  if (MyClient(sptr))
                    sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
                               me.name, sptr->name, "MODE +l");
                  break;
                }
              
              arg = check_string(*parv++);
              /*              if (MyClient(sptr) && opcnt >= MAXMODEPARAMS)
                break; */
              if ((nusers = atoi(arg)) <= 0)
                break;
              ircsprintf(numeric, "%d", nusers);
              if ((tmpc = strchr(numeric, ' ')))
                *tmpc = '\0';
              arg = numeric;

              tmp = strlen(arg);
              if (len + tmp + 2 >= MODEBUFLEN)
                break;

              chptr->mode.limit = nusers;
              chptr->mode.mode |= MODE_LIMIT;

              *mbufw++ = '+';
              *mbufw++ = 'l';
              strcpy(pbufw, arg);
              pbufw += strlen(pbufw);
              *pbufw++ = ' ';
              len += tmp + 1;
              /*              opcnt++;*/
            }
          else
            {
              chptr->mode.limit = 0;
              chptr->mode.mode &= ~MODE_LIMIT;
              *mbufw++ = '-';
              *mbufw++ = 'l';
            }

          break;

          /* Traditionally, these are handled separately
           * but I decided to combine them all into this one case
           * statement keeping it all sane
           *
           * The disadvantage is a lot more code duplicated ;-/
           *
           * -Dianora
           */

        case 'i' :
          if (whatt == MODE_QUERY)      /* shouldn't happen. */
            break;
          if (!isok)
            {
              if (MyClient(sptr) && !errsent(SM_ERR_NOOPS, &errors_sent))
                sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED), me.name, 
                           sptr->name, chptr->chname);
              break;
            }

          if(MyClient(sptr))
            {
              if(done_i)
                break;
              else
                done_i = YES;

              /*              if ( opcnt >= MAXMODEPARAMS)
                break; */
            }

          if(whatt == MODE_ADD)
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(!(chptr->mode.mode & MODE_INVITEONLY))
#endif
                {
                  chptr->mode.mode |= MODE_INVITEONLY;
                  *mbufw++ = '+';
                  *mbufw++ = 'i';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          else
            {
              if (len + 2 >= MODEBUFLEN)
                break;
	  
              while ( (lp = chptr->invites) )
                del_invite(lp->value.cptr, chptr);

#ifdef OLD_NON_RED
              if(chptr->mode.mode & MODE_INVITEONLY)
#endif
                {
                  chptr->mode.mode &= ~MODE_INVITEONLY;
                  *mbufw++ = '-';
                  *mbufw++ = 'i';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          break;

        case 'm' :
          if (!isok)
            {
              if (MyClient(sptr) && !errsent(SM_ERR_NOOPS, &errors_sent))
                sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED), me.name, 
                           sptr->name, chptr->chname);
              break;
            }

          if(MyClient(sptr))
            {
              if(done_m)
                break;
              else
                done_m = YES;

              /*              if ( opcnt >= MAXMODEPARAMS)
                              break; */
            }

          if(whatt == MODE_ADD)
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(!(chptr->mode.mode & MODE_MODERATED))
#endif
                {
                  chptr->mode.mode |= MODE_MODERATED;
                  *mbufw++ = '+';
                  *mbufw++ = 'm';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          else
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(chptr->mode.mode & MODE_MODERATED)
#endif
                {
                  chptr->mode.mode &= ~MODE_MODERATED;
                  *mbufw++ = '-';
                  *mbufw++ = 'm';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          break;

        case 'n' :
          if (!isok)
            {
              if (MyClient(sptr) && !errsent(SM_ERR_NOOPS, &errors_sent))
                sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED), me.name, 
                           sptr->name, chptr->chname);
              break;
            }

          if(MyClient(sptr))
            {
              if(done_n)
                break;
              else
                done_n = YES;

              /*              if ( opcnt >= MAXMODEPARAMS)
                              break; */
            }

          if(whatt == MODE_ADD)
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(!(chptr->mode.mode & MODE_NOPRIVMSGS))
#endif
                {
                  chptr->mode.mode |= MODE_NOPRIVMSGS;
                  *mbufw++ = '+';
                  *mbufw++ = 'n';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          else
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(chptr->mode.mode & MODE_NOPRIVMSGS)
#endif
                {
                  chptr->mode.mode &= ~MODE_NOPRIVMSGS;
                  *mbufw++ = '-';
                  *mbufw++ = 'n';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          break;

        case 'p' :
          if (!isok)
            {
              if (MyClient(sptr) && !errsent(SM_ERR_NOOPS, &errors_sent))
                sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED), me.name, 
                           sptr->name, chptr->chname);
              break;
            }

          if(MyClient(sptr))
            {

              if(done_p)
                break;
              else
                done_p = YES;
              /*              if ( opcnt >= MAXMODEPARAMS)
                break; */
            }

          if(whatt == MODE_ADD)
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(!(chptr->mode.mode & MODE_PRIVATE))
#endif
                {
                  chptr->mode.mode |= MODE_PRIVATE;
                  *mbufw++ = '+';
                  *mbufw++ = 'p';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          else
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(chptr->mode.mode & MODE_PRIVATE)
#endif
                {
                  chptr->mode.mode &= ~MODE_PRIVATE;
                  *mbufw++ = '-';
                  *mbufw++ = 'p';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          break;

        case 's' :
          if (!isok)
            {
              if (MyClient(sptr) && !errsent(SM_ERR_NOOPS, &errors_sent))
                sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED), me.name, 
                           sptr->name, chptr->chname);
              break;
            }

          /* ickity poo, traditional +p-s nonsense */

          if(MyClient(sptr))
            {
              if(done_s)
                break;
              else
                done_s = YES;
              /*              if ( opcnt >= MAXMODEPARAMS)
                              break; */
            }

          if(whatt == MODE_ADD)
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_P_S
              if(chptr->mode.mode & MODE_PRIVATE)
                {
                  if (len + 2 >= MODEBUFLEN)
                    break;
                  *mbufw++ = '-';
                  *mbufw++ = 'p';
                  len += 2;
                  chptr->mode.mode &= ~MODE_PRIVATE;
                }
#endif
#ifdef OLD_NON_RED
              if(!(chptr->mode.mode & MODE_SECRET))
#endif
                {
                  chptr->mode.mode |= MODE_SECRET;
                  *mbufw++ = '+';
                  *mbufw++ = 's';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          else
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(chptr->mode.mode & MODE_SECRET)
#endif
                {
                  chptr->mode.mode &= ~MODE_SECRET;
                  *mbufw++ = '-';
                  *mbufw++ = 's';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          break;

        case 't' :
          if (!isok)
            {
              if (MyClient(sptr) && !errsent(SM_ERR_NOOPS, &errors_sent))
                sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED), me.name, 
                           sptr->name, chptr->chname);
              break;
            }

          if(MyClient(sptr))
            {
              if(done_t)
                break;
              else
                done_t = YES;

              /*              if ( opcnt >= MAXMODEPARAMS)
                              break; */
            }

          if(whatt == MODE_ADD)
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(!(chptr->mode.mode & MODE_TOPICLIMIT))
#endif
                {
                  chptr->mode.mode |= MODE_TOPICLIMIT;
                  *mbufw++ = '+';
                  *mbufw++ = 't';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          else
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(chptr->mode.mode & MODE_TOPICLIMIT)
#endif
                {
                  chptr->mode.mode &= ~MODE_TOPICLIMIT;
                  *mbufw++ = '-';
                  *mbufw++ = 't';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          break;

        case 'C' :    // ssl channel patch by common
            if (!isok)
              {
                if (MyClient(sptr) && !errsent(SM_ERR_NOOPS, &errors_sent))
                  sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED), me.name, 
                             sptr->name, chptr->chname);
                break;
              }

            if(MyClient(sptr))
              {
                if(done_C)
                  break;
                else
                  done_C = YES;

                /*              if ( opcnt >= MAXMODEPARAMS)
                                break; */
              }

            if(whatt == MODE_ADD)
              {
                if (len + 2 >= MODEBUFLEN)
                  break;
  #ifdef OLD_NON_RED
                if(!(chptr->mode.mode & MODE_SSLONLY))
  #endif
                  {
                    chptr->mode.mode |= MODE_SSLONLY;
                    *mbufw++ = '+';
                    *mbufw++ = 'C';
                    len += 2;
                    /*              opcnt++; */
                  }
              }
            else
              {
                if (len + 2 >= MODEBUFLEN)
                  break;
  #ifdef OLD_NON_RED
                if(chptr->mode.mode & MODE_SSLONLY)
  #endif
                  {
                    chptr->mode.mode &= ~MODE_SSLONLY;
                    *mbufw++ = '-';
                    *mbufw++ = 'C';
                    len += 2;
                    /*              opcnt++; */
                  }
              }
            break;


        case 'R' :
          if (!isok)
            {
              if (MyClient(sptr) && !errsent(SM_ERR_NOOPS, &errors_sent))
                sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED), me.name, 
                           sptr->name, chptr->chname);
              break;
            }

          if(MyClient(sptr))
            {
              if(done_R)
                break;
              else
                done_R = YES;

              /*              if ( opcnt >= MAXMODEPARAMS)
                              break; */
            }

          if(whatt == MODE_ADD)
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(!(chptr->mode.mode & MODE_REGONLY))
#endif
                {
                  chptr->mode.mode |= MODE_REGONLY;
                  *mbufw++ = '+';
                  *mbufw++ = 'R';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          else
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(chptr->mode.mode & MODE_REGONLY)
#endif
                {
                  chptr->mode.mode &= ~MODE_REGONLY;
                  *mbufw++ = '-';
                  *mbufw++ = 'R';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          break;

        case 'K' :
          if (!isok)
            {
              if (MyClient(sptr) && !errsent(SM_ERR_NOOPS, &errors_sent))
                sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED), me.name, 
                           sptr->name, chptr->chname);
              break;
            }

          if(MyClient(sptr))
            {
              if(done_K)
                break;
              else
                done_K = YES;

              /*              if ( opcnt >= MAXMODEPARAMS)
                              break; */
            }

          if(whatt == MODE_ADD)
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(!(chptr->mode.mode & MODE_KNOCK))
#endif
                {
                  chptr->mode.mode |= MODE_KNOCK;
                  *mbufw++ = '+';
                  *mbufw++ = 'K';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          else
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(chptr->mode.mode & MODE_KNOCK)
#endif
                {
                  chptr->mode.mode &= ~MODE_KNOCK;
                  *mbufw++ = '-';
                  *mbufw++ = 'K';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          break;

        case 'r' :
          if (!IsService(sptr))
            {
              if (MyClient(sptr) && !errsent(SM_ERR_NOOPS, &errors_sent))
                sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED), me.name, 
                           sptr->name, chptr->chname);
              break;
            }

          if(MyClient(sptr))
            {
              if(done_r)
                break;
              else
                done_r = YES;

              /*              if ( opcnt >= MAXMODEPARAMS)
                              break; */
            }

          if(whatt == MODE_ADD)
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(!(chptr->mode.mode & MODE_REGISTERED))
#endif
                {
                  chptr->mode.mode |= MODE_REGISTERED;
                  *mbufw++ = '+';
                  *mbufw++ = 'r';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          else
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(chptr->mode.mode & MODE_REGISTERED)
#endif
                {
                  chptr->mode.mode &= ~MODE_REGISTERED;
                  *mbufw++ = '-';
                  *mbufw++ = 'r';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          break;

        case 'O' :
		
          if (!IsOper(sptr) && !IsService(sptr) && !IsServer(sptr))
            {
              if (MyClient(sptr) && !errsent(SM_ERR_NOOPS, &errors_sent))
			      sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, sptr->name);				  
              break;
            }

          if(MyClient(sptr))
            {
              if(done_O)
                break;
              else
                done_O = YES;

              /*              if ( opcnt >= MAXMODEPARAMS)
                              break; */
            }

          if(whatt == MODE_ADD)
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(!(chptr->mode.mode & MODE_OPERONLY))
#endif
                {
                  chptr->mode.mode |= MODE_OPERONLY;
                  *mbufw++ = '+';
                  *mbufw++ = 'O';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          else
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(chptr->mode.mode & MODE_OPERONLY)
#endif
                {
                  chptr->mode.mode &= ~MODE_OPERONLY;
                  *mbufw++ = '-';
                  *mbufw++ = 'O';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          break;

        case 'A' :
		
          if (!(IsAdmin(sptr) || IsTechAdmin(sptr) || IsNetAdmin(sptr)) && !IsService(sptr) && !IsServer(sptr))
            {
              if (MyClient(sptr) && !errsent(SM_ERR_NOOPS, &errors_sent))
			    sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, sptr->name);
              break;
            }

          if(MyClient(sptr))
            {
              if(done_A)
                break;
              else
                done_A = YES;

              /*              if ( opcnt >= MAXMODEPARAMS)
                              break; */
            }

          if(whatt == MODE_ADD)
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(!(chptr->mode.mode & MODE_ADMINONLY))
#endif
                {
                  chptr->mode.mode |= MODE_ADMINONLY;
                  *mbufw++ = '+';
                  *mbufw++ = 'A';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          else
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(chptr->mode.mode & MODE_ADMINONLY)
#endif
                {
                  chptr->mode.mode &= ~MODE_ADMINONLY;
                  *mbufw++ = '-';
                  *mbufw++ = 'A';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          break;		  

        case 'S' :
				
          if (!isok)
            {
              if (MyClient(sptr) && !errsent(SM_ERR_NOOPS, &errors_sent))
                sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED), me.name, 
                           sptr->name, chptr->chname);
              break;
            }

          if(MyClient(sptr))
            {
              if(done_S)
                break;
              else
                done_S = YES;

              /*              if ( opcnt >= MAXMODEPARAMS)
                              break; */
            }

          if(whatt == MODE_ADD)
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(!(chptr->mode.mode & MODE_NOSPAM))
#endif
                {
                  chptr->mode.mode |= MODE_NOSPAM;
                  *mbufw++ = '+';
                  *mbufw++ = 'S';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          else
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(chptr->mode.mode & MODE_NOSPAM)
#endif
                {
                  chptr->mode.mode &= ~MODE_NOSPAM;
                  *mbufw++ = '-';
                  *mbufw++ = 'S';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          break;		  

        case 'd' :				
          if (!isok)
            {
              if (MyClient(sptr) && !errsent(SM_ERR_NOOPS, &errors_sent))
                sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED), me.name, 
                           sptr->name, chptr->chname);
              break;
            }

          if(MyClient(sptr))
            {
              if(done_d)
                break;
              else
                done_d = YES;

              /*              if ( opcnt >= MAXMODEPARAMS)
                              break; */
            }

          if(whatt == MODE_ADD)
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(!(chptr->mode.mode & MODE_NOFLOOD))
#endif
                {
                  chptr->mode.mode |= MODE_NOFLOOD;
                  *mbufw++ = '+';
                  *mbufw++ = 'd';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          else
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(chptr->mode.mode & MODE_NOFLOOD)
#endif
                {
                  chptr->mode.mode &= ~MODE_NOFLOOD;
                  *mbufw++ = '-';
                  *mbufw++ = 'd';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          break;		  

        case 'c' :				
          if (!isok)
            {
              if (MyClient(sptr) && !errsent(SM_ERR_NOOPS, &errors_sent))
                sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED), me.name, 
                           sptr->name, chptr->chname);
              break;
            }

          if(MyClient(sptr))
            {
              if(done_c)
                break;
              else
                done_c = YES;

              /*              if ( opcnt >= MAXMODEPARAMS)
                              break; */
            }

          if(whatt == MODE_ADD)
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(!(chptr->mode.mode & MODE_NOCOLORS))
#endif
                {
                  chptr->mode.mode |= MODE_NOCOLORS;
                  *mbufw++ = '+';
                  *mbufw++ = 'c';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          else
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(chptr->mode.mode & MODE_NOCOLORS)
#endif
                {
                  chptr->mode.mode &= ~MODE_NOCOLORS;
                  *mbufw++ = '-';
                  *mbufw++ = 'c';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          break;		  
		  		  

        case 'q' :
          if (!isok)
            {
              if (MyClient(sptr) && !errsent(SM_ERR_NOOPS, &errors_sent))
                sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED), me.name, 
                           sptr->name, chptr->chname);
              break;
            }

          if(MyClient(sptr))
            {
              if(done_q)
                break;
              else
                done_q = YES;

              /*              if ( opcnt >= MAXMODEPARAMS)
                              break; */
            }

          if(whatt == MODE_ADD)
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(!(chptr->mode.mode & MODE_NOQUITS))
#endif
                {
                  chptr->mode.mode |= MODE_NOQUITS;
                  *mbufw++ = '+';
                  *mbufw++ = 'q';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          else
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(chptr->mode.mode & MODE_NOQUITS)
#endif
                {
                  chptr->mode.mode &= ~MODE_NOQUITS;
                  *mbufw++ = '-';
                  *mbufw++ = 'q';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          break;		  
        case 'B' :
          if (!isok)
            {
              if (MyClient(sptr) && !errsent(SM_ERR_NOOPS, &errors_sent))
                sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED), me.name, 
                           sptr->name, chptr->chname);
              break;
            }

          if(MyClient(sptr))
            {
              if(done_B)
                break;
              else
                done_B = YES;

              /*              if ( opcnt >= MAXMODEPARAMS)
                              break; */
            }

          if(whatt == MODE_ADD)
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(!(chptr->mode.mode & MODE_NOBOTS))
#endif
                {
                  chptr->mode.mode |= MODE_NOBOTS;
                  *mbufw++ = '+';
                  *mbufw++ = 'B';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          else
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(chptr->mode.mode & MODE_NOBOTS)
#endif
                {
                  chptr->mode.mode &= ~MODE_NOBOTS;
                  *mbufw++ = '-';
                  *mbufw++ = 'B';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          break;		  

        case 'N' :				
          if (!isok)
            {
              if (MyClient(sptr) && !errsent(SM_ERR_NOOPS, &errors_sent))
                sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED), me.name, 
                           sptr->name, chptr->chname);
              break;
            }

          if(MyClient(sptr))
            {
              if(done_N)
                break;
              else
                done_N = YES;

              /*              if ( opcnt >= MAXMODEPARAMS)
                              break; */
            }

          if(whatt == MODE_ADD)
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(!(chptr->mode.mode & MODE_NONICKCH))
#endif
                {
                  chptr->mode.mode |= MODE_NONICKCH;
                  *mbufw++ = '+';
                  *mbufw++ = 'N';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          else
            {
              if (len + 2 >= MODEBUFLEN)
                break;
#ifdef OLD_NON_RED
              if(chptr->mode.mode & MODE_NONICKCH)
#endif
                {
                  chptr->mode.mode &= ~MODE_NONICKCH;
                  *mbufw++ = '-';
                  *mbufw++ = 'N';
                  len += 2;
                  /*              opcnt++; */
                }
            }
          break;		  		  		  
		  		  
        default:
          if (whatt == MODE_QUERY)
            break;

          /* only one "UNKNOWNMODE" per mode... we don't want
          ** to generate a storm, even if it's just to a 
          ** local client  -orabidoo
          */
          if (MyClient(sptr) && !errsent(SM_ERR_UNKNOWN, &errors_sent))
            sendto_one(sptr, form_str(ERR_UNKNOWNMODE), me.name, sptr->name, c);
          break;
        }
    }

  /*
  ** WHEW!!  now all that's left to do is put the various bufs
  ** together and send it along.
  */

  *mbufw = *mbuf2w = *pbufw = *pbuf2w = *mbufw_new = *pbufw_new = '\0';


  collapse_signs(modebuf);
/*  collapse_signs(modebuf2); */
  collapse_signs(modebuf_new);

  if(*modebuf)
    {
#ifdef HIDE_OPS
      sendto_channel_chanops_butserv(chptr, sptr, ":%s MODE %s %s %s", 
				     sptr->name, chptr->chname,
				     modebuf, parabuf);
#else
      sendto_channel_butserv(chptr, sptr, ":%s MODE %s %s %s", 
                           sptr->name, chptr->chname,
                           modebuf, parabuf);
#endif
      sendto_match_servs(chptr, cptr, ":%s MODE %s %s %s",
                         sptr->name, chptr->chname,
                         modebuf, parabuf);
    }

  if(*modebuf_new)
    {
      sendto_channel_butserv(chptr, sptr, ":%s MODE %s %s %s", 
                             sptr->name, chptr->chname,
                             modebuf_new, parabuf_new);

    }
                     
  return;
}

static  int     can_join(struct Client *sptr, struct Channel *chptr, char *key, int *flags)
{
  Link  *lp;
  int ban_or_exception;
  
  
  if( IsService(sptr) )
	return 0;
	
  
  if (OperCanAlwaysJoin && IsOper(sptr) && !(chptr->mode.mode & MODE_ADMINONLY))
	return 0;
  

  if(IsOper(sptr) && key && OperByPass && !irccmp(OperByPass, key)) 
	{
	  sendto_ops("Oper %s (%s@%s) joining %s with Oper bypass",
                 sptr->name,
                 sptr->username, sptr->realhost,chptr->chname);
				 
	  sendto_serv_butone(NULL,":%s GLOBOPS :Oper %s (%s@%s) joining %s with Oper bypass",
		  me.name,
          sptr->name,
          sptr->username, sptr->realhost,chptr->chname);
		  
	  return 0;
	}
	
  /* Invited users can always join (except for +k) */
  if(!(*chptr->mode.key))
	{
	  for (lp = sptr->user->invited; lp; lp = lp->next)
  		if (lp->value.chptr == chptr)
  		  return 0;

	  /* we only check for +i if no key is set */
	  if ((chptr->mode.mode & MODE_INVITEONLY) && !(*chptr->mode.key))
		{
		  if(chptr->mode.mode & MODE_KNOCK)
			sendto_channelops_butone(NULL, &me, chptr,
                ":%s NOTICE @%s :%s failed to join (+i) channel %s",
   	            me.name, chptr->chname, sptr->name,chptr->chname);
				
		  return (ERR_INVITEONLYCHAN);		  
		}
	}

  if ((chptr->mode.mode & MODE_ADMINONLY) && !(IsAdmin(sptr) || IsTechAdmin(sptr) 
	    || IsNetAdmin(sptr)))
	{
	  if(chptr->mode.mode & MODE_KNOCK)	
		sendto_channelops_butone(NULL, &me, chptr,
              ":%s NOTICE @%s :%s failed to join (+A) channel %s",
   	          me.name, chptr->chname, sptr->name,chptr->chname);
  	  return (ERR_INVITEONLYCHAN);
	}	  

  if ((chptr->mode.mode & MODE_OPERONLY) && !IsOper(sptr))
	{
	  if(chptr->mode.mode & MODE_KNOCK)
		sendto_channelops_butone(NULL, &me, chptr,
              ":%s NOTICE @%s :%s failed to join (+O) channel %s",
   	          me.name, chptr->chname, sptr->name,chptr->chname);	
  	  return (ERR_INVITEONLYCHAN);  
	}
  
  if ( !IsSsl(sptr) && (chptr->mode.mode & MODE_SSLONLY) )
    {
	  if(chptr->mode.mode & MODE_KNOCK)
	    sendto_channelops_butone(NULL, &me, chptr,
            ":%s NOTICE @%s :%s failed to join (+C) channel %s",
   	        me.name, chptr->chname, sptr->name,chptr->chname);

	  return (ERR_SSLREQUIRED);
    }    


  if ( !IsIdentified(sptr) && (chptr->mode.mode & MODE_REGONLY) )
    {
	  if(chptr->mode.mode & MODE_KNOCK)
	    sendto_channelops_butone(NULL, &me, chptr,
            ":%s NOTICE @%s :%s failed to join (+R) channel %s",
   	        me.name, chptr->chname, sptr->name,chptr->chname);  
	  return (ERR_NEEDREGGEDNICK);
    }    

  if ((chptr->mode.mode & MODE_NOBOTS) && IsBot(sptr))
    {
	  if(chptr->mode.mode & MODE_KNOCK)
	    sendto_channelops_butone(NULL, &me, chptr,
            ":%s NOTICE @%s :%s failed to join (+B) channel %s",
   	        me.name, chptr->chname, sptr->name,chptr->chname);  
	  return (ERR_INVITEONLYCHAN);
    }

  if (*chptr->mode.key && (BadPtr(key) || irccmp(chptr->mode.key, key)))
	{
	  if(chptr->mode.mode & MODE_KNOCK)
		sendto_channelops_butone(NULL, &me, chptr,
              ":%s NOTICE @%s :%s failed to join (+k) channel %s",
   	          me.name, chptr->chname, sptr->name,chptr->chname);		
  	  return (ERR_BADCHANNELKEY);
	}

  if (chptr->mode.limit && chptr->users >= chptr->mode.limit)
    {
  	  if(chptr->mode.mode & MODE_KNOCK)
        sendto_channelops_butone(NULL, &me, chptr,
          ":%s NOTICE @%s :%s failed to join (+l) channel %s",
          me.name, chptr->chname, sptr->name,chptr->chname);
  
      return (ERR_CHANNELISFULL);
    }

  if ( (ban_or_exception = is_banned(sptr, chptr)) == CHFL_BAN)
    {
	  if(chptr->mode.mode & MODE_KNOCK)
	    sendto_channelops_butone(NULL, &me, chptr,
          ":%s NOTICE @%s :%s failed to join (+b) channel %s",
   	      me.name, chptr->chname, sptr->name,chptr->chname);    
      return (ERR_BANNEDFROMCHAN);
    }  
  else
    *flags |= ban_or_exception; /* Mark this client as "charmed" */

  return 0;
}

/*
 * check_channel_name - check channel name for invalid characters
 * return true (1) if name ok, false (0) otherwise
 */
int check_channel_name(const char* name)
{
  assert(0 != name);
  
  for ( ; *name; ++name) {
    if (!IsChanChar(*name))
      return 0;
  }
  return 1;
}

/*
**  Get Channel block for chname (and allocate a new channel
**  block, if it didn't exist before).
*/
static struct Channel* get_channel(struct Client *cptr, char *chname, int flag)
{
  struct Channel *chptr;
  int   len;

  if (BadPtr(chname))
    return NULL;

  len = strlen(chname);
  if (MyClient(cptr) && len > CHANNELLEN)
    {
      len = CHANNELLEN;
      *(chname + CHANNELLEN) = '\0';
    }
  if ((chptr = hash_find_channel(chname, NULL)))
    return (chptr);

  /*
   * If a channel is created during a split make sure its marked
   * as created locally 
   * Also make sure a created channel has =some= timestamp
   * even if it get over-ruled later on. Lets quash the possibility
   * an ircd coder accidentally blasting TS on channels. (grrrrr -db)
   *
   * Actually, it might be fun to make the TS some impossibly huge value (-db)
   */

  if (flag == CREATE)
    {
      chptr = (struct Channel*) MyMalloc(sizeof(struct Channel) + len + 1);
      memset(chptr, 0, sizeof(struct Channel));
      /*
       * NOTE: strcpy ok here, we have allocated strlen + 1
       */
      strcpy(chptr->chname, chname);
      if (channel)
        channel->prevch = chptr;
      chptr->prevch = NULL;
      chptr->nextch = channel;
      channel = chptr;
      chptr->channelts = CurrentTime;     /* doesn't hurt to set it here */
      add_to_channel_hash_table(chname, chptr);
      Count.chan++;
    }
  return chptr;
}

static  void    add_invite(struct Client *cptr,struct Channel *chptr)
{
  Link  *inv, **tmp;

  del_invite(cptr, chptr);
  /*
   * delete last link in chain if the list is max length
   */
  if (list_length(cptr->user->invited) >= MaxChansPerUser)
    {
      /*                This forgets the channel side of invitation     -Vesa
                        inv = cptr->user->invited;
                        cptr->user->invited = inv->next;
                        free_link(inv);
*/
      del_invite(cptr, cptr->user->invited->value.chptr);
 
    }
  /*
   * add client to channel invite list
   */
  inv = make_link();
  inv->value.cptr = cptr;
  inv->next = chptr->invites;
  chptr->invites = inv;
  /*
   * add channel to the end of the client invite list
   */
  for (tmp = &(cptr->user->invited); *tmp; tmp = &((*tmp)->next))
    ;
  inv = make_link();
  inv->value.chptr = chptr;
  inv->next = NULL;
  (*tmp) = inv;
}

/*
 * Delete Invite block from channel invite list and client invite list
 */
void    del_invite(struct Client *cptr,struct Channel *chptr)
{
  Link  **inv, *tmp;

  for (inv = &(chptr->invites); (tmp = *inv); inv = &tmp->next)
    if (tmp->value.cptr == cptr)
      {
        *inv = tmp->next;
        free_link(tmp);
        break;
      }

  for (inv = &(cptr->user->invited); (tmp = *inv); inv = &tmp->next)
    if (tmp->value.chptr == chptr)
      {
        *inv = tmp->next;
        free_link(tmp);
        break;
      }
}

/*
**  Subtract one user from channel (and free channel
**  block, if channel became empty).
*/
static  void    sub1_from_channel(struct Channel *chptr)
{
  Link *tmp;

  if (--chptr->users <= 0)
    {
      chptr->users = 0; /* if chptr->users < 0, make sure it sticks at 0
                         * It should never happen but...
                         */

	if((RestrictedChans && (chptr->mode.mode & MODE_REGISTERED))
	  || (chptr->mode.mode & (MODE_ADMINONLY|MODE_OPERONLY|MODE_LOG)))
		return;
		
       {
          /*
           * Now, find all invite links from channel structure
           */
          while ((tmp = chptr->invites))
            del_invite(tmp->value.cptr, chptr);

	  /* free all bans/exceptions/denies */
	  free_bans_exceptions_denies( chptr );

          if (chptr->prevch)
            chptr->prevch->nextch = chptr->nextch;
          else
            channel = chptr->nextch;
          if (chptr->nextch)
            chptr->nextch->prevch = chptr->prevch;

#ifdef FLUD
          free_fluders(NULL, chptr);
#endif
          del_from_channel_hash_table(chptr->chname, chptr);
          MyFree((char*) chptr);
          Count.chan--;
        }
    }
}


/*
 * free_bans_exceptions_denies
 *
 * inputs	- pointer to channel structure
 * output	- none
 * side effects	- all bans/exceptions denies are freed for channel
 */

static void free_bans_exceptions_denies(struct Channel *chptr)
{
  free_a_ban_list(chptr->banlist);

  chptr->banlist = NULL;
  chptr->num_bed = 0;
}

static void
free_a_ban_list(Link *ban_ptr)
{
  Link *ban;
  Link *next_ban;

  for(ban = ban_ptr; ban; ban = next_ban)
    {
      next_ban = ban->next;
      MyFree(ban->value.banptr->banstr);
      MyFree(ban->value.banptr->who);   
      MyFree(ban->value.banptr);
      free_link(ban);
    }
}

#ifdef NEED_SPLITCODE

/*
 * check_still_split()
 *
 * inputs       -NONE
 * output       -NONE
 * side effects -
 * Check to see if the server split timer has expired, if so
 * check to see if there are now a decent number of servers connected
 * and users present, so I can consider this split over.
 *
 * -Dianora
 */

static void check_still_split()
{
  if((server_split_time + SPLITDELAY) < CurrentTime)
    {
      if((Count.server >= SPLITNUM) &&
#ifdef SPLIT_PONG
         (got_server_pong == YES) &&
#endif
         (Count.total >= SPLITUSERS))
        {
          /* server hasn't been split for a while.
           * -Dianora
           */
          server_was_split = NO;
          sendto_ops("Net Rejoined, split-mode deactivated");
          cold_start = NO;
        }
      else
        {
          server_split_time = CurrentTime; /* still split */
          server_was_split = YES;
        }
    }
}
#endif


/*
** m_join
**      parv[0] = sender prefix
**      parv[1] = channel
**      parv[2] = channel password (key)
*/
int     m_join(struct Client *cptr,
               struct Client *sptr,
               int parc,
               char *parv[])
{
  static char   jbuf[BUFSIZE];
  Link  *lp;
  struct Channel *chptr = NULL;
  char  *name, *key = NULL;
  int   i, tmplen, flags = 0;
#ifdef NO_CHANOPS_ON_SPLIT  
  int   allow_op=YES;
#endif
  char  *p = NULL, *p2 = NULL;
#ifdef ANTI_SPAMBOT
  int   successful_join_count = 0; /* Number of channels successfully joined */
#endif
  
  if (!(sptr->user))
    {
      /* something is *fucked* - bail */
      return 0;
    }

  if (parc < 2 || *parv[1] == '\0')
    {
      sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
                 me.name, parv[0], "JOIN");
      return 0;
    }
    
  if(secure_mode && MyClient(sptr) && !IsIdentified(sptr))
    {
      sendto_one(sptr,
        form_str(ERR_NEEDREGGEDNICK), me.name, parv[0], "");
        return 0;
    }

#ifdef NEED_SPLITCODE

  /* Check to see if the timer has timed out, and if so, see if
   * there are a decent number of servers now connected 
   * to consider any possible split over.
   * -Dianora
   */

  if (server_was_split)
    check_still_split();

#endif

  *jbuf = '\0';
  /*
  ** Rebuild list of channels joined to be the actual result of the
  ** JOIN.  Note that "JOIN 0" is the destructive problem.
  */
  for (i = 0, name = strtoken(&p, parv[1], ","); name;
       name = strtoken(&p, (char *)NULL, ","))
    {
      if (!check_channel_name(name))
        {
          sendto_one(sptr, form_str(ERR_BADCHANNAME),
                       me.name, parv[0], (unsigned char*) name);
          continue;
        }
      if (*name == '&' && !MyConnect(sptr))
        continue;
      if (*name == '0' && !atoi(name) && !MyConnect(sptr))
        *jbuf = '\0';
      else if (!IsChannelName(name))
        {
          if (MyClient(sptr))
            sendto_one(sptr, form_str(ERR_NOSUCHCHANNEL),
                       me.name, parv[0], name);
          continue;
        }

#ifdef NO_JOIN_ON_SPLIT
      if (server_was_split && MyClient(sptr) && (*name != '&') &&
          !IsAnOper(sptr))
        {
              sendto_one(sptr, ":%s NOTICE %s :You cannot join %s due to network split",
                         me.name, parv[0], name);
              continue;
        }	
#endif /* NO_JOIN_ON_SPLIT */
      tmplen = strlen(name);
      if (i + tmplen + 2 /* comma and \0 */
          >= sizeof(jbuf) )
        {
          break;
        }

      if (*jbuf)
        {
          jbuf[i++] = ',';
        }
      (void)strcpy(jbuf + i, name);
      i += tmplen;

    }
  /*    (void)strcpy(parv[1], jbuf);*/

  p = NULL;
  if (parv[2])
    key = strtoken(&p2, parv[2], ",");
  parv[2] = NULL;       /* for m_names call later, parv[parc] must == NULL */
  for (name = strtoken(&p, jbuf, ","); name;
       key = (key) ? strtoken(&p2, NULL, ",") : NULL,
         name = strtoken(&p, NULL, ","))
    {
	  chptr = NULL;
      /*
      ** JOIN 0 sends out a part for all channels a user
      ** has joined.
      */
      if (*name == '0' && !atoi(name))
        {
          if (sptr->user->channel == NULL)
            continue;
          while ((lp = sptr->user->channel))
            {
              chptr = lp->value.chptr;
              if(!IsStealth(sptr))
              	sendto_channel_butserv(chptr, sptr, PartFmt,
                                     parv[0], chptr->chname);
              else if(MyClient(sptr))
              	sendto_one(sptr, PartFmt, parv[0], chptr->chname);
              remove_user_from_channel(sptr, chptr, 0);
            }

#ifdef ANTI_SPAMBOT       /* Dianora */

          if( MyConnect(sptr) && !IsAnOper(sptr) )
            {
              if(SPAMNUM && (sptr->join_leave_count >= SPAMNUM))
                {
                  sendto_ops_imodes(IMODE_BOTS,
                                     "User %s (%s@%s) is a possible spambot",
                                     sptr->name,
                                     sptr->username, sptr->host);
                  sptr->oper_warn_count_down = OPER_SPAM_COUNTDOWN;
                }
              else
                {
                  int t_delta;

                  if( (t_delta = (CurrentTime - sptr->last_leave_time)) >
                      JOIN_LEAVE_COUNT_EXPIRE_TIME)
                    {
                      int decrement_count;
                      decrement_count = (t_delta/JOIN_LEAVE_COUNT_EXPIRE_TIME);

                      if(decrement_count > sptr->join_leave_count)
                        sptr->join_leave_count = 0;
                      else
                        sptr->join_leave_count -= decrement_count;
                    }
                  else
                    {
                      if((CurrentTime - (sptr->last_join_time)) < SPAMTIME)
                        {
                          /* oh, its a possible spambot */
                          sptr->join_leave_count++;
                        }
                    }
                  sptr->last_leave_time = CurrentTime;
                }
            }
#endif
          sendto_match_servs(NULL, cptr, ":%s JOIN 0", parv[0]);
          continue;
        }
      
      if (MyConnect(sptr))
        {
          /*
          ** local client is first to enter previously nonexistent
          ** channel so make them (rightfully) the Channel
          ** Operator.
          */
           /*     flags = (ChannelExists(name)) ? 0 : CHFL_CHANOP; */

          /* To save a redundant hash table lookup later on */
           
          if((chptr = get_channel(sptr, name, 0)))
          	  flags = 0;			  
      	  else
			{
  			  if (RestrictedChans && !IsOper(sptr))
				{
				  sendto_one(sptr,":%s NOTICE %s :%s", &me.name, parv[0],
					RestrictedChansMsg);
				  return 0;
				}
			  else
            	flags = CHFL_CHANOP;
			}

           /* if its not a local channel, or isn't an oper
            * and server has been split
            */

#ifdef NO_CHANOPS_ON_SPLIT
          if((*name != '&') && server_was_split)
            {
              allow_op = NO;

              if(!IsRestricted(sptr) && (flags & CHFL_CHANOP))
                sendto_one(sptr,":%s NOTICE %s :*** Notice -- Due to a network split, you can not obtain channel operator status in a new channel at this time.",
                       me.name,
                       sptr->name);
            }
#endif
		  
          if (((sptr->user->joined >= MaxChansPerUser && (!MaxChansPerRegUser || !IsIdentified(sptr))
            || (MaxChansPerRegUser && IsIdentified(sptr) && sptr->user->joined >= MaxChansPerRegUser))
          ) && !IsService(sptr) &&
             (!IsAnOper(sptr) || (sptr->user->joined >= MaxChansPerUser*10)))
            {
              sendto_one(sptr, form_str(ERR_TOOMANYCHANNELS),
                         me.name, parv[0], name);
#ifdef ANTI_SPAMBOT
              if(successful_join_count)
                sptr->last_join_time = CurrentTime;
#endif
              return 0;
            }


#ifdef ANTI_SPAMBOT       /* Dianora */
          if(flags == 0)        /* if channel doesn't exist, don't penalize */
            successful_join_count++;
          if( SPAMNUM && (sptr->join_leave_count >= SPAMNUM))
            { 
              /* Its already known as a possible spambot */
 
              if(sptr->oper_warn_count_down > 0)  /* my general paranoia */
                sptr->oper_warn_count_down--;
              else
                sptr->oper_warn_count_down = 0;
 
              if(sptr->oper_warn_count_down == 0)
                {
                  sendto_ops_imodes(IMODE_BOTS,
                    "User %s (%s@%s) trying to join %s is a possible spambot",
                             sptr->name,
                             sptr->username,
                             sptr->host,
                             name);     
                  sptr->oper_warn_count_down = OPER_SPAM_COUNTDOWN;
                }
#ifndef ANTI_SPAMBOT_WARN_ONLY
              return 0; /* Don't actually JOIN anything, but don't let
                           spambot know that */
#endif
            }
#endif
        }
      else
        {
          /*
          ** complain for remote JOINs to existing channels
          ** (they should be SJOINs) -orabidoo
          */

          if (!ChannelExists(name))
            ts_warn("User on %s remotely JOINing new channel", 
                    sptr->user->server);
        }

      if(!chptr)        /* If I already have a chptr, no point doing this */
        chptr = get_channel(sptr, name, CREATE);

      if(chptr)
        {
          if (IsMember(sptr, chptr))    /* already a member, ignore this */
            continue;
        }
      else
        {
      	  sendto_one(sptr, ":%s NOTICE %s :You cannot join %s due to network split",
             me.name, parv[0], name);
			
#ifdef ANTI_SPAMBOT
          if(successful_join_count > 0)
            successful_join_count--;
#endif
          continue;
        }

      /*
       * can_join checks for +i key, bans.
       * If a ban is found but an exception to the ban was found
       * flags will have CHFL_EXCEPTION set
       */

      if (MyConnect(sptr) && (i = can_join(sptr, chptr, key, &flags)))
        {
          sendto_one(sptr,
                    form_str(i), me.name, parv[0], name);
#ifdef ANTI_SPAMBOT
          if(successful_join_count > 0)
            successful_join_count--;
#endif
          continue;
        }

      /*
      **  Complete user entry to the new channel (if any)
      */

#ifdef NO_CHANOPS_ON_SPLIT
      if(allow_op)
        {
          add_user_to_channel(chptr, sptr, flags);
        }
      else
        {
          add_user_to_channel(chptr, sptr, 0);
        }
#else
      add_user_to_channel(chptr, sptr, flags);
#endif

     /*
      **  Set timestamp if appropriate, and propagate
      */
      if (MyClient(sptr) && (flags & CHFL_CHANOP) )
        {      	
          chptr->channelts = CurrentTime;
#ifdef NO_CHANOPS_ON_SPLIT
          if(allow_op)
            {
              sendto_match_servs(chptr, cptr,
                ":%s %cJOIN %lu %s %s :@%s", me.name, 
                is_njoin ? 'N' : 'S',
                chptr->channelts, name, DefaultChanMode, parv[0]);
            }                                
          else
            sendto_match_servs(chptr, cptr,
              ":%s %cJOIN %lu %s %s :%s", me.name,
              is_njoin ? 'N' : 'S',
              chptr->channelts, name, DefaultChanMode, parv[0]);
#else
          sendto_match_servs(chptr, cptr,
            ":%s %cJOIN %lu %s %s :@%s", me.name,
            is_njoin ? 'N' : 'S',
            chptr->channelts, name, DefaultChanMode, parv[0]);          
#endif
        }
      else if (MyClient(sptr))
        {
          sendto_match_servs(chptr, cptr,
            ":%s %cJOIN %lu %s + :%s", me.name,
            is_njoin ? 'N' : 'S',
            chptr->channelts, name, parv[0]);
        }
      else
        sendto_match_servs(chptr, cptr, ":%s %cJOIN :%s", parv[0],
          is_njoin ? 'N' : 'S',
          name);

      /*
      ** notify all other users on the channel
      */
      if(!IsStealth(sptr))
      	sendto_channel_butserv(chptr, sptr, ":%s JOIN :%s",
                             parv[0], name);
      else if(MyClient(sptr))
      	sendto_one(sptr,":%s JOIN :%s", parv[0], name);
      	
      if (MyClient(sptr))
        {
          if( flags & CHFL_CHANOP && !IsStealth(sptr))
            {
              chptr->mode.mode |= new_cmodes;

              sendto_channel_butserv(chptr, sptr,
                ":%s MODE %s %s",
              me.name, chptr->chname, DefaultChanMode);
            }
          del_invite(sptr, chptr);
          if (chptr->topic[0] != '\0')
            {
              sendto_one(sptr, form_str(RPL_TOPIC), me.name,
                         parv[0], name, chptr->topic);

              sendto_one(sptr, form_str(RPL_TOPICWHOTIME),
                         me.name, parv[0], name,
                         chptr->topic_nick,
                         chptr->topic_time);

            }
          parv[1] = name;
          (void)m_names(cptr, sptr, 2, parv);
        }
    }

#ifdef ANTI_SPAMBOT
  if(MyConnect(sptr) && successful_join_count)
    sptr->last_join_time = CurrentTime;
#endif
  return 0;
}

/*
** m_part
**      parv[0] = sender prefix
**      parv[1] = channel
*/
int     m_part(struct Client *cptr,
               struct Client *sptr,
               int parc,
               char *parv[])
{
  struct Channel *chptr;
  char  *p, *name;

  if (parc < 2 || parv[1][0] == '\0')
    {
      sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
                 me.name, parv[0], "PART");
      return 0;
    }

  name = strtoken( &p, parv[1], ",");

#ifdef ANTI_SPAMBOT     /* Dianora */
      /* if its my client, and isn't an oper */

      if (name && MyConnect(sptr) && !IsAnOper(sptr))
        {
          if(SPAMNUM && (sptr->join_leave_count >= SPAMNUM))
            {
              sendto_ops_imodes(IMODE_BOTS,
                               "User %s (%s@%s) is a possible spambot",
                               sptr->name,
                               sptr->username, sptr->host);
              sptr->oper_warn_count_down = OPER_SPAM_COUNTDOWN;
            }
          else
            {
              int t_delta;

              if( (t_delta = (CurrentTime - sptr->last_leave_time)) >
                  JOIN_LEAVE_COUNT_EXPIRE_TIME)
                {
                  int decrement_count;
                  decrement_count = (t_delta/JOIN_LEAVE_COUNT_EXPIRE_TIME);

                  if(decrement_count > sptr->join_leave_count)
                    sptr->join_leave_count = 0;
                  else
                    sptr->join_leave_count -= decrement_count;
                }
              else
                {
                  if( (CurrentTime - (sptr->last_join_time)) < SPAMTIME)
                    {
                      /* oh, its a possible spambot */
                      sptr->join_leave_count++;
                    }
                }
              sptr->last_leave_time = CurrentTime;
            }
        }
#endif

  while ( name )
    {
      chptr = get_channel(sptr, name, 0);
      if (!chptr)
        {
          sendto_one(sptr, form_str(ERR_NOSUCHCHANNEL),
                     me.name, parv[0], name);
          name = strtoken(&p, (char *)NULL, ",");
          continue;
        }

      if (!IsMember(sptr, chptr))
        {
          sendto_one(sptr, form_str(ERR_NOTONCHANNEL),
                     me.name, parv[0], name);
          name = strtoken(&p, (char *)NULL, ",");
          continue;
        }
      /*
      **  Remove user from the old channel (if any)
      */
            
      sendto_match_servs(chptr, cptr, PartFmt, parv[0], name);
      if(!IsStealth(sptr))	            
         sendto_channel_butserv(chptr, sptr, PartFmt, parv[0], name);
      else if(MyClient(sptr))
         sendto_one(sptr, PartFmt, parv[0], name);
      remove_user_from_channel(sptr, chptr, 0);
      name = strtoken(&p, (char *)NULL, ",");
    }
  return 0;
}

/*
** m_kick
**      parv[0] = sender prefix
**      parv[1] = channel
**      parv[2] = client to kick
**      parv[3] = kick comment
*/
/*
 * I've removed the multi channel kick, and the multi user kick
 * though, there are still remnants left ie..
 * "name = strtoken(&p, parv[1], ",");" in a normal kick
 * it will just be "KICK #channel nick"
 * A strchr() is going to be faster than a strtoken(), so rewritten
 * to use a strchr()
 *
 * It appears the original code was supposed to support 
 * "kick #channel1,#channel2 nick1,nick2,nick3." For example, look at
 * the original code for m_topic(), where 
 * "topic #channel1,#channel2,#channel3... topic" was supported.
 *
 * -Dianora
 */
int     m_kick(struct Client *cptr,
               struct Client *sptr,
               int parc,
               char *parv[])
{
  struct Client *who;
  struct Channel *chptr;
  int   chasing = 0;
  char  *comment;
  char  *name;
  char  *p = (char *)NULL;
  char  *user;

  if (parc < 3 || *parv[1] == '\0')
    {
      sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
                 me.name, parv[0], "KICK");
      return 0;
    }
	
  if (IsServer(sptr))
    sendto_ops("KICK from %s for %s %s",
               parv[0], parv[1], parv[2]);
			   
  comment = (BadPtr(parv[3])) ? parv[0] : parv[3];
  if (strlen(comment) > (size_t) TOPICLEN)
    comment[TOPICLEN] = '\0';

  *buf = '\0';
  p = strchr(parv[1],',');
  if(p)
    *p = '\0';
  name = parv[1]; /* strtoken(&p, parv[1], ","); */

  chptr = get_channel(sptr, name, !CREATE);
  if (!chptr)
    {
      sendto_one(sptr, form_str(ERR_NOSUCHCHANNEL),
                 me.name, parv[0], name);
      return(0);
    }

  /* You either have chan op privs, or you don't -Dianora */
  /* orabidoo and I discussed this one for a while...
   * I hope he approves of this code, (he did) users can get quite confused...
   *    -Dianora
   */

#ifdef HALFOPS
  if (!IsServer(sptr) && !IsService(sptr) && !halfop_chanop(sptr, chptr))
#else
  if (!IsServer(sptr) && !IsService(sptr) && !is_chan_op(sptr, chptr)) 
#endif  
    { 
      /* was a user, not a server, and user isn't seen as a chanop here */
      
      if(MyConnect(sptr))
        {
          /* user on _my_ server, with no chanops.. so go away */
          
          sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED),
                     me.name, parv[0], chptr->chname);
          return(0);
        }

      if(chptr->channelts == 0)
        {
          /* If its a TS 0 channel, do it the old way */
          
          sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED),
                     me.name, parv[0], chptr->chname);
          return(0);
        }

      /* Its a user doing a kick, but is not showing as chanop locally
       * its also not a user ON -my- server, and the channel has a TS.
       * There are two cases we can get to this point then...
       *
       *     1) connect burst is happening, and for some reason a legit
       *        op has sent a KICK, but the SJOIN hasn't happened yet or 
       *        been seen. (who knows.. due to lag...)
       *
       *     2) The channel is desynced. That can STILL happen with TS
       *        
       *     Now, the old code roger wrote, would allow the KICK to 
       *     go through. Thats quite legit, but lets weird things like
       *     KICKS by users who appear not to be chanopped happen,
       *     or even neater, they appear not to be on the channel.
       *     This fits every definition of a desync, doesn't it? ;-)
       *     So I will allow the KICK, otherwise, things are MUCH worse.
       *     But I will warn it as a possible desync.
       *
       *     -Dianora
       */

      /*          sendto_one(sptr, form_str(ERR_DESYNC),
       *           me.name, parv[0], chptr->chname);
       */

      /*
       * After more discussion with orabidoo...
       *
       * The code was sound, however, what happens if we have +h (TS4)
       * and some servers don't understand it yet? 
       * we will be seeing servers with users who appear to have
       * no chanops at all, merrily kicking users....
       * -Dianora
       */
    }

  p = strchr(parv[2],',');
  if(p)
    *p = '\0';
  user = parv[2]; /* strtoken(&p2, parv[2], ","); */

  if (!(who = find_chasing(sptr, user, &chasing)))
    {
      return(0);
    }

  if (MyClient(sptr) && IsStealth(who)) {
    sendto_one(sptr, form_str(ERR_USERNOTINCHANNEL),
               me.name, parv[0], user, name);
    return 0;
  }
  


  if (IsMember(who, chptr))  
    {        
      if (MyClient(sptr))
        {
          if(OperKickProtection) 
          {
    	    /* patch from mihaim@tfm.ro */
    	    /* Just moved it to the dynamic conf - ^Stinger^ */
    	    /* <cop> */
	    if (MyClient(sptr) && !IsOper(sptr) && IsOper(who)) 
	      {
  	        sendto_one(who, ":%s NOTICE %s :%s is trying to kick you from %s",
		  me.name, who->name, parv[0], chptr->chname);
	        sendto_one(sptr, form_str(ERR_CANTKICKADMIN), me.name,
      	         parv[0], parv[2], chptr->chname);
                return(0);
	      }  	
	    /* </cop> */
          }
                            
	  /* only services can kick services */                            
          if(IsService(who)&&!IsService(sptr)&&IsSAdmin(sptr)) 
 	    sendto_one(sptr, form_str(ERR_CANTKICKADMIN), me.name,
    		parv[0], parv[2], chptr->chname);
    		
    	  /* only opers/services can kick on +O chans */ 
    	  if((chptr->mode.mode & MODE_OPERONLY) && !IsOper(sptr) && !IsService(sptr))
 	    sendto_one(sptr, form_str(ERR_CANTKICKADMIN), me.name,
    		parv[0], parv[2], chptr->chname);
    		
    	  /* only admins/services can kick on +A chans */ 
    	  if((chptr->mode.mode & MODE_ADMINONLY) && !IsAdmin(sptr) && !IsService(sptr))
 	    sendto_one(sptr, form_str(ERR_CANTKICKADMIN), me.name,
    		parv[0], parv[2], chptr->chname);    		
    	  	
          if ((!IsServer(cptr) && !IsService(sptr) && is_chan_adm(who, chptr)
                    && !is_chan_adm(sptr, chptr)) || IsService(who)) 
		    {
		  
        		sendto_one(sptr, form_str(ERR_CANTKICKADMIN), me.name,
              		parv[0], parv[2], chptr->chname);
					
                return(0);
      	    }
#ifdef HALFOPS
         if (has_halfop(cptr, chptr))
           {
             if (is_chan_op(who, chptr) || has_halfop(who, chptr))
               {
                  sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED),
                             me.name, parv[0], chptr->chname);
                  return(0);
                }
           }
#endif      	      	
        }
#ifdef HIDE_OPS
      sendto_channel_chanops_butserv(chptr, sptr,
                             ":%s KICK %s %s :%s", parv[0],
                             name, who->name, comment);
      sendto_channel_non_chanops_butserv(chptr, sptr,
                             ":%s KICK %s %s :%s", NetworkName,
                             name, who->name, comment);

#else
      sendto_channel_butserv(chptr, sptr,
                             ":%s KICK %s %s :%s", parv[0],
                             name, who->name, comment);
#endif
      sendto_match_servs(chptr, cptr,
                         ":%s KICK %s %s :%s",
                         parv[0], name,
                         who->name, comment);

    /*
     * if he was kicked he wasn't a good guy, lets
     * penalize him with 10 secs (it also prevents kick/join flood)
     * --fabulous
     */

      if (MyClient(who)) 
		who->since = CurrentTime + 10;

						 
      remove_user_from_channel(who, chptr, 1);
    }
  else
    sendto_one(sptr, form_str(ERR_USERNOTINCHANNEL),
               me.name, parv[0], user, name);

  return (0);
}

int     count_channels(struct Client *sptr)
{
  struct Channel      *chptr;
  int   count = 0;

  for (chptr = channel; chptr; chptr = chptr->nextch)
    count++;
  return (count);
}

/* m_knock
**    parv[0] = sender prefix
**    parv[1] = channel
**  The KNOCK command has the following syntax:
**   :<sender> KNOCK <channel>
**  If a user is not banned from the channel they can use the KNOCK
**  command to have the server NOTICE the channel operators notifying
**  they would like to join.  Helpful if the channel is invite-only, the
**  key is forgotten, or the channel is full (INVITE can bypass each one
**  of these conditions.  Concept by Dianora <db@db.net> and written by
**  <anonymous>
**
** Just some flood control added here, five minute delay between each
** KNOCK -Dianora
**/
int     m_knock(struct Client *cptr,
               struct Client *sptr,
               int parc,
               char *parv[])
{
  struct Channel      *chptr;
  char  *p, *name;

  /* anti flooding code,
   * I did have this in parse.c with a table lookup
   * but I think this will be less inefficient doing it in each
   * function that absolutely needs it
   *
   * -Dianora
   * 
   */
  static time_t last_used=0L;

  if (parc < 2)
    {
      sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS), me.name, parv[0],
                 "KNOCK");
      return 0;
    }


  /* We will cut at the first comma reached, however we will not *
   * process anything afterwards.                                */

  p = strchr(parv[1],',');
  if(p)
    *p = '\0';
  name = parv[1]; /* strtoken(&p, parv[1], ","); */

  if (!IsChannelName(name) || !(chptr = hash_find_channel(name, NullChn)))
    {
      sendto_one(sptr, form_str(ERR_NOSUCHCHANNEL), me.name, parv[0],
                 name);
      return 0;
    }

  if(!((chptr->mode.mode & MODE_INVITEONLY) ||
       (*chptr->mode.key) ||
       (chptr->mode.limit && chptr->users >= chptr->mode.limit )
       ))
    {
      sendto_one(sptr,":%s NOTICE %s :*** Notice -- Channel is open!",
                 me.name,
                 sptr->name);
      return 0;
    }

  /* don't allow a knock if the user is banned, or the channel is secret */
  if ((chptr->mode.mode & MODE_SECRET) || 
      (is_banned(sptr, chptr) == CHFL_BAN) )
    {
      sendto_one(sptr, form_str(ERR_CANNOTSENDTOCHAN), me.name, parv[0],
                 name);
      return 0;
    }

  /* if the user is already on channel, then a knock is pointless! */
  if (IsMember(sptr, chptr))
    {
      sendto_one(sptr,":%s NOTICE %s :*** Notice -- You are on channel already!",
                 me.name,
                 sptr->name);
      return 0;
    }

  /* flood control server wide, clients on KNOCK
   * opers are not flood controlled.
   */

  if(!IsAnOper(sptr))
    {
      if((last_used + PACE_WAIT) > CurrentTime)
        return 0;
      else
        last_used = CurrentTime;
    }

  /* flood control individual clients on KNOCK
   * the ugly possibility still exists, 400 clones could all KNOCK
   * on a channel at once, flooding all the ops. *ugh*
   * Remember when life was simpler?
   * -Dianora
   */

  /* opers are not flow controlled here */
  if( !IsAnOper(sptr) && (sptr->last_knock + KNOCK_DELAY) > CurrentTime)
    {
      sendto_one(sptr,":%s NOTICE %s :*** Notice -- Wait %d seconds before another knock",
                 me.name,
                 sptr->name,
                 KNOCK_DELAY - (CurrentTime - sptr->last_knock));
      return 0;
    }

  sptr->last_knock = CurrentTime;

  sendto_one(sptr,":%s NOTICE %s :*** Notice -- Your KNOCK has been delivered",
                 me.name,
                 sptr->name);

  /* using &me and me.name won't deliver to clients not on this server
   * so, the notice will have to appear from the "knocker" ick.
   *
   * Ideally, KNOCK would be routable. Also it would be nice to add
   * a new channel mode. Perhaps some day.
   * For now, clients that don't want to see KNOCK requests will have
   * to use client side filtering. 
   *
   * -Dianora
   */

  {
    char message[NICKLEN*2+CHANNELLEN+USERLEN+HOSTLEN+30];

    /* bit of paranoid, be a shame if it cored for this -Dianora */
    if(sptr->user)
      {
        ircsprintf(message,"KNOCK: %s (%s [%s@%s] has asked for an invite)",
                   chptr->chname,
                   sptr->name,
                   sptr->username,
                   sptr->host);
        send_knock(sptr, cptr, chptr, MODE_CHANOP, message,
                 (parc > 3) ? parv[3] : "");
        
      }
  }
  return 0;
}

/*
** m_topic
**      parv[0] = sender prefix
**      parv[1] = channel
** 		parv[2] = new topic
** from TS server
**      parv[0] = sender prefix
**      parv[1] = channel
**      parv[2] = topic nick
**      parv[3] = topic time
**      parv[4] = topic text
*/
int     m_topic(struct Client *cptr,
                struct Client *sptr,
                int parc,
                char *parv[])
{
  struct Channel *chptr = NullChn;
  char *topic = NULL, *name, *p = NULL, *tnick = NULL; 
  time_t ttime = 0;
  
  if(IsZombie(sptr))
    return 0;
  
  if (parc < 2)
    {
      sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
                 me.name, parv[0], "TOPIC");
      return 0;
    }

  p = strchr(parv[1],',');
  if(p)
    *p = '\0';
  name = parv[1]; /* strtoken(&p, parv[1], ","); */

  /* multi channel topic's are now known to be used by cloners
   * trying to flood off servers.. so disable it *sigh* - Dianora
   */

  if (name && IsChannelName(name))
    {
      chptr = hash_find_channel(name, NullChn);
      if (!chptr)
        {
          sendto_one(sptr, form_str(ERR_NOSUCHCHANNEL), me.name, parv[0],
              name);
          return 0;
        }

      if (!IsService(sptr) && !IsServer(sptr) && !IsMember(sptr, chptr))
        {
          sendto_one(sptr, form_str(ERR_NOTONCHANNEL), me.name, parv[0],
              name);
          return 0;
        }

      if (parc > 2) /* setting topic */
        topic = parv[2];

      if (IsServer(cptr) && (parc > 4)) 
		{
      	  tnick = parv[2];
      	  ttime = atol(parv[3]);
      	  topic = parv[4];
    	}     
	  
      if(topic) /* a little extra paranoia never hurt */
        {
          if ((chptr->mode.mode & MODE_TOPICLIMIT) == 0 
			|| is_chan_op(sptr, chptr) || IsService(sptr) || IsServer(sptr))
            {
			
	  	/*
		    We are going to keep out topic if older, 
		    unless we get a different topic from services  - Lamego 
		*/
	  	if(IsServer(sptr) && (
		   (ttime == chptr->topic_time) ||
		   (!services_on && chptr->topic_time && chptr->topic_time<=ttime)||
		   (services_on && !IsServicesPath(cptr))))
			return 0;
			
		if(strcmp(chptr->topic, topic) == 0) /* same topic */
		  {		    
		    chptr->topic_time = ttime;
		    return 0;
		  }
				
              /* setting a topic */
              /*
               * chptr zeroed
               */
              strncpy_irc(chptr->topic, topic, TOPICLEN);
              /*
               * XXX - this truncates the topic_nick if
               * strlen(sptr->name) > NICKLEN
               */
              strncpy_irc(chptr->topic_nick, (tnick) ? tnick : sptr->name, NICKLEN);
              chptr->topic_time = (ttime) ? ttime : CurrentTime;

    		  sendto_match_servs(chptr, cptr, ":%s TOPIC %s %s %lu :%s",
                                   parv[0], chptr->chname,
                                   chptr->topic_nick, 
								   chptr->topic_time,
                                   chptr->topic);
		  								   
              sendto_channel_butserv(chptr, sptr, ":%s TOPIC %s :%s",
                                     parv[0],
                                     chptr->chname, chptr->topic);
            }
          else 
            sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED),
                       me.name, parv[0], chptr->chname);
        }
      else  /* only asking  for topic  */
        {
          if (chptr->topic[0] == '\0')
            sendto_one(sptr, form_str(RPL_NOTOPIC),
                       me.name, parv[0], chptr->chname);
          else
            {
              sendto_one(sptr, form_str(RPL_TOPIC),
                         me.name, parv[0],
                         chptr->chname, chptr->topic);

              sendto_one(sptr, form_str(RPL_TOPICWHOTIME),
                         me.name, parv[0], chptr->chname,
                         chptr->topic_nick,
                         chptr->topic_time);

            }
        }
    }
  else
    {
      sendto_one(sptr, form_str(ERR_NOSUCHCHANNEL),
                 me.name, parv[0], name);
    }

  return 0;
}

/*
** m_invite
**      parv[0] - sender prefix
**      parv[1] - user to invite
**      parv[2] - channel number
*/
int     m_invite(struct Client *cptr,
                 struct Client *sptr,
                 int parc,
                 char *parv[])
{
  struct Client *acptr;
  struct Channel *chptr;
  int need_invite=NO;

  if (IsZombie(sptr))
       return 0;

  if (parc < 3 || *parv[1] == '\0')
    {
      sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
                 me.name, parv[0], "INVITE");
      return -1;
    }

#ifdef SERVERHIDE
  if (!IsAnOper(sptr) && parv[2][0] == '&') {
    sendto_one(sptr, ":%s NOTICE %s :INVITE to local channels is disabled in this server",
               me.name, parv[0]);
    return 0;
  }
#endif

  /* A little sanity test here */
  if(!sptr->user)
    return 0;

  if (!(acptr = find_person(parv[1], (struct Client *)NULL)))
    {
      sendto_one(sptr, form_str(ERR_NOSUCHNICK),
                 me.name, parv[0], parv[1]);
      return 0;
    }

  if(!IsService(sptr) && !IsIdentified(sptr) && (IsRegMsg(acptr)||secure_mode))
     return -1;
                          
  if( is_silenced(sptr, acptr) )/* source is silenced by target */
  	  return 0;

  if (!check_channel_name(parv[2]))
    { 
      sendto_one(sptr, form_str(ERR_BADCHANNAME),
                 me.name, parv[0], (unsigned char *)parv[2]);
      return 0;
    }

  if (!IsChannelName(parv[2]))
    {
      if (MyClient(sptr))
        sendto_one(sptr, form_str(ERR_NOSUCHCHANNEL),
                   me.name, parv[0], parv[2]);
      return 0;
    }

  /* Do not send local channel invites to users if they are not on the
   * same server as the person sending the INVITE message. 
   */
  /* Possibly should be an error sent to sptr */
  if (!MyConnect(acptr) && (parv[2][0] == '&'))
    return 0;

  if (!(chptr = hash_find_channel(parv[2], NullChn)))
    {
      if (MyClient(sptr))
        sendto_one(sptr, form_str(ERR_NOSUCHCHANNEL),
                   me.name, parv[0], parv[2]);
      return 0;
    }

  /* By this point, chptr is non NULL */  

  if (!IsService(sptr) && !IsMember(sptr, chptr))
    {
      if (MyClient(sptr))
        sendto_one(sptr, form_str(ERR_NOTONCHANNEL),
                   me.name, parv[0], parv[2]);
      return 0;
    }

  if ((IsMember(acptr, chptr)) && (!IsStealth(acptr)))  
    {
      if (MyClient(sptr))
        sendto_one(sptr, form_str(ERR_USERONCHANNEL),
                   me.name, parv[0], parv[1], parv[2]);
      return 0;
    }

  if (chptr && (chptr->mode.mode & (MODE_OPERONLY | MODE_ADMINONLY)))
	{
      need_invite = YES;

      if (!IsOper(sptr) && !IsService(sptr))
        {
          if (MyClient(sptr))
            sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED),
                       me.name, parv[0], parv[2]);
          return -1;
        }
    }
		
  else if (chptr && (chptr->mode.mode & (MODE_INVITEONLY | MODE_NOBOTS)))
    {
      need_invite = YES;

      if (!is_chan_op(sptr, chptr) && !IsService(sptr))
        {
          if (MyClient(sptr))
            sendto_one(sptr, form_str(ERR_CHANOPRIVSNEEDED),
                       me.name, parv[0], parv[2]);
          return -1;
        }
    }

  /*
   * due to some whining I've taken out the need for the channel
   * being +i before sending an INVITE. It was intentionally done this
   * way, it makes no sense (to me at least) letting the server send
   * an unnecessary invite when a channel isn't +i !
   * bah. I can't be bothered arguing it
   * -Dianora
   */
  if (MyConnect(sptr) /* && need_invite*/ )
    {
      sendto_one(sptr, form_str(RPL_INVITING), me.name, parv[0],
                 acptr->name, ((chptr) ? (chptr->chname) : parv[2]));
				 
      if (acptr->user->away)
        sendto_one(sptr, form_str(RPL_AWAY), me.name, parv[0],
                   acptr->name, acptr->user->away);
      

      if (check_target_limit(sptr, acptr, acptr->name, 0, ((chptr) ? (chptr->chname) : parv[2])))      
		return 0;
		
      if( need_invite )
        {
          /* Send a NOTICE to all channel operators concerning chanops who  *
           * INVITE other users to the channel when it is invite-only (+i). *
           * The NOTICE is sent from the local server.                      */

          /* Only allow this invite notice if the channel is +p
           * i.e. "paranoid"
           * -Dianora
           */

          if (chptr && (chptr->mode.mode & MODE_PRIVATE))
            { 
              char message[NICKLEN*2+CHANNELLEN+USERLEN+HOSTLEN+30];

              /* bit of paranoia, be a shame if it cored for this -Dianora */
              if(acptr->user)
                {
                  ircsprintf(message,
                             "INVITE: %s (%s invited %s [%s@%s])",
                             chptr->chname,
                             sptr->name,
                             acptr->name,
                             acptr->username,
                             acptr->host);

                  sendto_channel_type(cptr, sptr, chptr,
                                      MODE_CHANOP,
                                      chptr->chname,
                                      "PRIVMSG",
                                      message);
                }
            }
        }
    }

  if(MyConnect(acptr) && need_invite)
    add_invite(acptr, chptr);

  sendto_prefix_one(acptr, sptr, ":%s INVITE %s :%s",
                    parv[0], acptr->name, parv[2]);
  return 0;
}


/************************************************************************
 * m_names() - Added by Jto 27 Apr 1989
 ************************************************************************/

/*
** m_names
**      parv[0] = sender prefix
**      parv[1] = channel
*/
/*
 * Modified to report possible names abuse
 * drastically modified to not show all names, just names
 * on given channel names.
 *
 * -Dianora
 */
/* maximum names para to show to opers when abuse occurs */
#define TRUNCATED_NAMES 20

int     m_names( struct Client *cptr,
                 struct Client *sptr,
                 int parc,
                 char *parv[])
{ 
  struct Channel *chptr;
  struct Client *c2ptr;
  Link  *lp;
  struct Channel *ch2ptr = NULL;
  int   idx, flag = 0, len, mlen;
  char  *s, *para = parc > 1 ? parv[1] : NULL;
  int comma_count=0;
  int char_count=0;
  char *mename = me.name;
  /* And throw away non local names requests that do get here -Dianora */
  if(!MyConnect(sptr))
    return 0;

  if(sptr->user && sptr->user->vlink)
    mename = sptr->user->vlink->name;
  /*
   * names is called by m_join() when client joins a channel,
   * hence I cannot easily rate limit it.. perhaps that won't
   * be necessary now that remote names is prohibited.
   *
   * -Dianora
   */

  mlen = strlen(mename) + NICKLEN + 7;

  if (!BadPtr(para))
    {
      /* Here is the lamer detection code
       * P.S. meta, GROW UP
       * -Dianora 
       */
      for(s = para; *s; s++)
        {
          char_count++;
          if(*s == ',')
            comma_count++;
          if(comma_count > 1)
            {
              if(char_count > TRUNCATED_NAMES)
                para[TRUNCATED_NAMES] = '\0';
              else
                {
                  s++;
                  *s = '\0';
                }
              sendto_realops("POSSIBLE /names abuser %s [%s]",
                             para,
                             get_client_name(sptr,FALSE));
              sendto_one(sptr, form_str(ERR_TOOMANYTARGETS),
                         mename, sptr->name, "NAMES",1);
              return 0;
            }
        }

      s = strchr(para, ',');
      if (s)
        *s = '\0';
      if (!check_channel_name(para))
        { 
          sendto_one(sptr, form_str(ERR_BADCHANNAME),
                     mename, parv[0], (unsigned char *)para);
          return 0;
        }

      ch2ptr = hash_find_channel(para, NULL);
    }

  *buf = '\0';
  
  /* 
   *
   * First, do all visible channels (public and the one user self is)
   */

  for (chptr = channel; chptr; chptr = chptr->nextch)
    {
      if ((chptr != ch2ptr) && !BadPtr(para))
        continue; /* -- wanted a specific channel */
      if (!MyConnect(sptr) && BadPtr(para))
        continue;
      if (!ShowChannel(sptr, chptr))
        continue; /* -- users on this are not listed */
      
      /* Find users on same channel (defined by chptr) */

      (void)strcpy(buf, "* ");
      len = strlen(chptr->chname);
      (void)strcpy(buf + 2, chptr->chname);
      (void)strcpy(buf + 2 + len, " :");

      if (PubChannel(chptr))
        *buf = '=';
      else if (SecretChannel(chptr))
        *buf = '@';
      idx = len + 4;
      flag = 1;
      for (lp = chptr->members; lp; lp = lp->next)
        {
          c2ptr = lp->value.cptr;
          if (IsInvisible(c2ptr) && !IsMember(sptr,chptr) && !IsAnOper(sptr))
            continue;
          if (IsStealth(c2ptr) && (c2ptr!=sptr))
            continue;
#ifdef HIDE_OPS
	  if(is_chan_op(sptr,chptr))
#endif
	    {
	      if (AdminWithDot && lp->flags & CHFL_CHANADM)
		{
		  strcat(buf, ".");
		  idx++;
		}
        
	      if (lp->flags & CHFL_CHANOP)
		{
		  strcat(buf, "@");
		  idx++;
		}
#ifdef HALFOPS
             else if (lp->flags & CHFL_HALFOP)
               {
                 strcat(buf, "%");
                 idx++;
               }
#endif
	      else if (lp->flags & CHFL_VOICE)
		{
		  strcat(buf, "+");
		  idx++;
		}
	    }
          strncat(buf, c2ptr->name, NICKLEN);
          idx += strlen(c2ptr->name) + 1;
          flag = 1;
          strcat(buf," ");
          if (mlen + idx + NICKLEN > BUFSIZE - 3)
            {
              sendto_one(sptr, form_str(RPL_NAMREPLY),
                         mename, parv[0], buf);
              strncpy_irc(buf, "* ", 3);
              strncpy_irc(buf + 2, chptr->chname, len + 1);
              strcat(buf, " :");
              if (PubChannel(chptr))
                *buf = '=';
              else if (SecretChannel(chptr))
                *buf = '@';
              idx = len + 4;
              flag = 0;
            }
        }
      if (flag)
        sendto_one(sptr, form_str(RPL_NAMREPLY),
                   mename, parv[0], buf);
    }
  if (!BadPtr(para))
    {
      sendto_one(sptr, form_str(RPL_ENDOFNAMES), mename, parv[0],
                 para);
      return(1);
    }

  /* Second, do all non-public, non-secret channels in one big sweep */

  strncpy_irc(buf, "* * :", 6);
  idx = 5;
  flag = 0;
  for (c2ptr = GlobalClientList; c2ptr; c2ptr = c2ptr->next)
    {
      struct Channel *ch3ptr;
      int       showflag = 0, secret = 0;

      if (!IsPerson(c2ptr) || IsInvisible(c2ptr) || IsStealth(c2ptr))
        continue;
      lp = c2ptr->user->channel;
      /*
       * dont show a client if they are on a secret channel or
       * they are on a channel sptr is on since they have already
       * been show earlier. -avalon
       */
      while (lp)
        {
          ch3ptr = lp->value.chptr;
          if (PubChannel(ch3ptr) || IsMember(sptr, ch3ptr))
            showflag = 1;
          if (SecretChannel(ch3ptr))
            secret = 1;
          lp = lp->next;
        }
      if (showflag) /* have we already shown them ? */
        continue;
      if (secret) /* on any secret channels ? */
        continue;
      (void)strncat(buf, c2ptr->name, NICKLEN);
      idx += strlen(c2ptr->name) + 1;
      (void)strcat(buf," ");
      flag = 1;
      if (mlen + idx + NICKLEN > BUFSIZE - 3)
        {
          sendto_one(sptr, form_str(RPL_NAMREPLY),
                     mename, parv[0], buf);
          strncpy_irc(buf, "* * :", 6);
          idx = 5;
          flag = 0;
        }
    }

  if (flag)
    sendto_one(sptr, form_str(RPL_NAMREPLY), mename, parv[0], buf);

  sendto_one(sptr, form_str(RPL_ENDOFNAMES), mename, parv[0], "*");
  return(1);
}


void    send_user_joins(struct Client *cptr, struct Client *user)
{
  Link  *lp;
  struct Channel *chptr;
  int   cnt = 0, len = 0, clen;
  char   *mask;

  *buf = ':';
  (void)strcpy(buf+1, user->name);
  (void)strcat(buf, " JOIN ");
  len = strlen(user->name) + 7;

  for (lp = user->user->channel; lp; lp = lp->next)
    {
      chptr = lp->value.chptr;
      if (*chptr->chname == '&')
        continue;
      if ((mask = strchr(chptr->chname, ':')))
        if (!match(++mask, cptr->name))
          continue;
      clen = strlen(chptr->chname);
      if (clen > (size_t) BUFSIZE - 7 - len)
        {
          if (cnt)
            sendto_one(cptr, "%s", buf);
          *buf = ':';
          (void)strcpy(buf+1, user->name);
          (void)strcat(buf, " JOIN ");
          len = strlen(user->name) + 7;
          cnt = 0;
        }
      (void)strcpy(buf + len, chptr->chname);
      cnt++;
      len += clen;
      if (lp->next)
        {
          len++;
          (void)strcat(buf, ",");
        }
    }
  if (*buf && cnt)
    sendto_one(cptr, "%s", buf);

  return;
}

static  void sjoin_sendit(struct Client *cptr,
                          struct Client *sptr,
                          struct Channel *chptr,
                          char *from)
{
  sendto_channel_butserv(chptr, sptr, ":%s MODE %s %s %s",
		   from, chptr->chname, modebuf, parabuf);
}
/*
 * m_njoin (same as sjoin but for netjoin messages)
 */ 
int m_njoin(struct Client *cptr, struct Client *sptr, int parc, char *parv[]) 
{
  int res;
  is_njoin = -1;
  res  = m_sjoin(cptr, sptr, parc, parv);
  is_njoin = 0;
  return res;
}
/*
 * m_sjoin
 * parv[0] - sender
 * parv[1] - TS
 * parv[2] - channel
 * parv[3] - modes + n arguments (key and/or limit)
 * parv[4+n] - flags+nick list (all in one parameter)

 * 
 * process a SJOIN, taking the TS's into account to either ignore the
 * incoming modes or undo the existing ones or merge them, and JOIN
 * all the specified users while sending JOIN/MODEs to non-TS servers
 * and to clients
 */


int     m_sjoin(struct Client *cptr,
                struct Client *sptr,
                int parc,
                char *parv[])
{
  struct Channel *chptr;
  struct Client       *acptr;
  time_t        newts;
  time_t        oldts;
  time_t        tstosend;
  static        Mode mode, *oldmode;
  Link  *l;
  int   args = 0, haveops = 0, keep_our_modes = 1, keep_new_modes = 1;
  int   doesop = 0, what = 0, pargs = 0, fl, people = 0, isnew;
  /* loop unrolled this is now redundant */
  /*  int ip; */
  register      char *s, *s0;
  static        char numeric[16], sjbuf[BUFSIZE];
  char  *mbuf = modebuf, *t = sjbuf, *p;
  
  if (IsClient(sptr) || parc < 5)
    return 0;
  if (!IsChannelName(parv[2]))
    return 0;

  if (!check_channel_name(parv[2]))
     { 
       return 0;
     }

  /* comstud server did this, SJOIN's for
   * local channels can't happen.
   */

  if(*parv[2] == '&')
    return 0;

  newts = atol(parv[1]);
  memset(&mode, 0, sizeof(mode));

  s = parv[3];
  while (*s)
    switch(*(s++))
      {
      case 'i':
        mode.mode |= MODE_INVITEONLY;
        break;
      case 'n':
        mode.mode |= MODE_NOPRIVMSGS;
        break;
      case 'p':
        mode.mode |= MODE_PRIVATE;
        break;
      case 's':
        mode.mode |= MODE_SECRET;
        break;
      case 'm':
        mode.mode |= MODE_MODERATED;
        break;
      case 't':
        mode.mode |= MODE_TOPICLIMIT;
        break;
      case 'k':
        strncpy_irc(mode.key, parv[4 + args], KEYLEN);
        args++;
        if (parc < 5+args) return 0;
        break;
      case 'l':
        mode.limit = atoi(parv[4+args]);
        args++;
        if (parc < 5+args) return 0;
        break;
      case 'r':
        mode.mode |= MODE_REGISTERED;
        break;
      case 'R':
        mode.mode |= MODE_REGONLY;
        break;		
      case 'C':                     // ssl patch by common
        mode.mode |= MODE_SSLONLY;
        break;		
      case 'K':
        mode.mode |= MODE_KNOCK;
        break;				
	  case 'O':
        mode.mode |= MODE_OPERONLY;
        break;		
	  case 'A':
        mode.mode |= MODE_ADMINONLY;
        break;		
	  case 'S':
        mode.mode |= MODE_NOSPAM;
        break;		
	  case 'd':
        mode.mode |= MODE_NOFLOOD;
        break;		
	  case 'c':
        mode.mode |= MODE_NOCOLORS;
        break;		
      case 'q':     
        mode.mode |= MODE_NOQUITS;
        break;  
      case 'B':     
        mode.mode |= MODE_NOBOTS;
        break;  
      case 'N':
        mode.mode |= MODE_NONICKCH;
	break;

      case 'f':
        {
          int xxi, xyi;
          char *xp;
          char *cp = parv[4+args];
          xp = index(cp, ':');
          if (!xp) 
            break;          
          args++;
          *xp = '\0';
          xxi = atoi(cp);
          xp++;
          xyi = atoi(xp);
          xp--;
          *xp = ':';
          mode.msgs = xxi;
          mode.per = xyi;
          if (parc < 5+args) return 0;
        }
        break;
        			  
      }

  *parabuf = '\0';

  isnew = ChannelExists(parv[2]) ? 0 : 1;
  chptr = get_channel(sptr, parv[2], CREATE);


  oldts = chptr->channelts;

  /*
   * If an SJOIN ever happens on a channel, assume the split is over
   * for this channel. best I think we can do for now -Dianora
   */

  /* If the TS goes to 0 for whatever reason, flag it
   * ya, I know its an invasion of privacy for those channels that
   * want to keep TS 0 *shrug* sorry
   * -Dianora
   */

  if(!isnew && !newts && oldts)
    {
      sendto_channel_butserv(chptr, &me,
             ":%s NOTICE %s :*** Notice -- TS for %s changed from %lu to 0",
              me.name, chptr->chname, chptr->chname, oldts);
      sendto_realops("Server %s changing TS on %s from %lu to 0",
                     sptr->name,parv[2],oldts);
    }

  doesop = (parv[4+args][0] == '@' || parv[4+args][1] == '@');

  for (l = chptr->members; l && l->value.cptr; l = l->next)
    if (l->flags & MODE_CHANOP)
      {
        haveops++;
        break;
      }

  oldmode = &chptr->mode;

  if (isnew)
    chptr->channelts = tstosend = newts;
  else if (newts == 0 || oldts == 0)
    chptr->channelts = tstosend = 0;
  else if (newts == oldts)
    tstosend = oldts;
  else if (newts < oldts)
  {
  /* new behaviour, never keep our modes */
#ifdef TS5
    keep_our_modes = NO;
    chptr->channelts = tstosend = newts;
    
/* old behaviour, keep our modes if their sjoin is opless */
#else
    if(doesop)
      keep_our_modes = NO;
    if(haveops && !doesop)
	  {
    		tstosend = oldts;
		total_ignoreops++;
	  }
    else
      chptr->channelts = tstosend = newts;
#endif      
    }
  else
    {
#ifdef TS5
      keep_new_modes = NO;
      tstosend = oldts;
#else
      if(haveops)

        keep_new_modes = NO;

      if (doesop && !haveops)
        {
		  total_hackops++;
          chptr->channelts = tstosend = newts;
        }
      else
        tstosend = oldts;
#endif 	/* TS5 */
     }
  if (!keep_new_modes)
    mode = *oldmode;
  else if (keep_our_modes)
    {
      mode.mode |= oldmode->mode;
      if (oldmode->limit > mode.limit)
        mode.limit = oldmode->limit;
      if (strcmp(mode.key, oldmode->key) < 0)
        strcpy(mode.key, oldmode->key);
      if (oldmode->msgs) 
        mode.msgs = oldmode->msgs;
      if (oldmode->per) 
        mode.per = oldmode->per;        
        
    }

  /* This loop unrolled below for speed
   */
  /*
  for (ip = 0; flags[ip].mode; ip++)
    if ((flags[ip].mode & mode.mode) && !(flags[ip].mode & oldmode->mode))
      {
        if (what != 1)
          {
            *mbuf++ = '+';
            what = 1;
          }
        *mbuf++ = flags[ip].letter;
      }
      */

  if((MODE_PRIVATE    & mode.mode) && !(MODE_PRIVATE    & oldmode->mode))
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;
        }
      *mbuf++ = 'p';
    }
  if((MODE_SECRET     & mode.mode) && !(MODE_SECRET     & oldmode->mode))
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;
        }
      *mbuf++ = 's';
    }
  if((MODE_MODERATED  & mode.mode) && !(MODE_MODERATED  & oldmode->mode))
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;
        }
      *mbuf++ = 'm';
    }
  if((MODE_NOPRIVMSGS & mode.mode) && !(MODE_NOPRIVMSGS & oldmode->mode))
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;
        }
      *mbuf++ = 'n';
    }
  if((MODE_TOPICLIMIT & mode.mode) && !(MODE_TOPICLIMIT & oldmode->mode))
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;
        }
      *mbuf++ = 't';
    }
  if((MODE_INVITEONLY & mode.mode) && !(MODE_INVITEONLY & oldmode->mode))
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;             /* This one is actually redundant now */
        }
      *mbuf++ = 'i';
    }

  if((MODE_REGISTERED & mode.mode) && !(MODE_REGISTERED & oldmode->mode))
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;             
        }
      *mbuf++ = 'r';
    }

  if((MODE_REGONLY & mode.mode) && !(MODE_REGONLY & oldmode->mode))
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;             
        }
      *mbuf++ = 'R';
    }
  // ssl patch by common
  if((MODE_SSLONLY & mode.mode) && !(MODE_SSLONLY & oldmode->mode))
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;             
        }
      *mbuf++ = 'C';
    }


  if((MODE_KNOCK & mode.mode) && !(MODE_KNOCK & oldmode->mode))
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;             
        }
      *mbuf++ = 'K';
    }

  if((MODE_OPERONLY & mode.mode) && !(MODE_OPERONLY & oldmode->mode))
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;             
        }
      *mbuf++ = 'O';
    }

  if((MODE_ADMINONLY & mode.mode) && !(MODE_ADMINONLY & oldmode->mode))
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;             
        }
      *mbuf++ = 'A';
    }

  if((MODE_NOBOTS & mode.mode) && !(MODE_NOBOTS & oldmode->mode))
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;     
        }
      *mbuf++ = 'B';
    }

  if((MODE_NOSPAM & mode.mode) && !(MODE_NOSPAM & oldmode->mode))
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;             
        }
      *mbuf++ = 'S';
    }

  if((MODE_NOFLOOD & mode.mode) && !(MODE_NOFLOOD & oldmode->mode))
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;             
        }
      *mbuf++ = 'd';
    }

  if((MODE_NOCOLORS & mode.mode) && !(MODE_NOCOLORS & oldmode->mode))
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;             
        }
      *mbuf++ = 'c';
    }
	
  if((MODE_NOQUITS & mode.mode) && !(MODE_NOQUITS & oldmode->mode))
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;     
        }
      *mbuf++ = 'q';
    }

   if((MODE_NONICKCH & mode.mode) && !(MODE_NONICKCH & oldmode->mode))
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;     
        }
      *mbuf++ = 'N';
    }		

  /* This loop unrolled below for speed
   */
  /*
  for (ip = 0; flags[ip].mode; ip++)
    if ((flags[ip].mode & oldmode->mode) && !(flags[ip].mode & mode.mode))
      {
        if (what != -1)
          {
            *mbuf++ = '-';
            what = -1;
          }
        *mbuf++ = flags[ip].letter;
      }
      */
  if((MODE_PRIVATE    & oldmode->mode) && !(MODE_PRIVATE    & mode.mode))
    {
      if (what != -1)
        {
          *mbuf++ = '-';
          what = -1;
        }
      *mbuf++ = 'p';
    }
  if((MODE_SECRET     & oldmode->mode) && !(MODE_SECRET     & mode.mode))
    {
      if (what != -1)
        {
          *mbuf++ = '-';
          what = -1;
        }
      *mbuf++ = 's';
    }
  if((MODE_MODERATED  & oldmode->mode) && !(MODE_MODERATED  & mode.mode))
    {
      if (what != -1)
        {
          *mbuf++ = '-';
          what = -1;
        }
      *mbuf++ = 'm';
    }
  if((MODE_NOPRIVMSGS & oldmode->mode) && !(MODE_NOPRIVMSGS & mode.mode))
    {
      if (what != -1)
        {
          *mbuf++ = '-';
          what = -1;
        }
      *mbuf++ = 'n';
    }
  if((MODE_TOPICLIMIT & oldmode->mode) && !(MODE_TOPICLIMIT & mode.mode))
    {
      if (what != -1)
        {
          *mbuf++ = '-';
          what = -1;
        }
      *mbuf++ = 't';
    }
  if((MODE_INVITEONLY & oldmode->mode) && !(MODE_INVITEONLY & mode.mode))
    {
      if (what != -1)
        {
          *mbuf++ = '-';
          what = -1;
        }
      *mbuf++ = 'i';
    }

  if((MODE_REGISTERED & oldmode->mode) && !(MODE_REGISTERED & mode.mode))
    {
      if (what != -1)
        {
          *mbuf++ = '-';
          what = -1;
        }
      *mbuf++ = 'r';
    }

  if((MODE_REGONLY & oldmode->mode) && !(MODE_REGONLY & mode.mode))
    {
      if (what != -1)
        {
          *mbuf++ = '-';
          what = -1;
        }
      *mbuf++ = 'R';
    }
  // ssl patch by common
  if((MODE_SSLONLY & oldmode->mode) && !(MODE_SSLONLY & mode.mode))
    {
      if (what != -1)
        {
          *mbuf++ = '-';
          what = -1;
        }
      *mbuf++ = 'C';
    }


  if((MODE_OPERONLY & oldmode->mode) && !(MODE_OPERONLY & mode.mode))
    {
      if (what != -1)
        {
          *mbuf++ = '-';
          what = -1;
        }
      *mbuf++ = 'O';
    }

  if((MODE_ADMINONLY & oldmode->mode) && !(MODE_ADMINONLY & mode.mode))
    {
      if (what != -1)
        {
          *mbuf++ = '-';
          what = -1;
        }
      *mbuf++ = 'A';
    }

  if((MODE_NOBOTS & oldmode->mode) && !(MODE_NOBOTS & mode.mode))
    {
      if (what != -1)
        {
          *mbuf++ = '-';
          what = -1;
        }
      *mbuf++ = 'B';
    }

  if((MODE_NOSPAM & oldmode->mode) && !(MODE_NOSPAM & mode.mode))
    {
      if (what != -1)
        {
          *mbuf++ = '-';
          what = -1;
        }
      *mbuf++ = 'S';
    }

  if((MODE_NOFLOOD & oldmode->mode) && !(MODE_NOFLOOD & mode.mode))
    {
      if (what != -1)
        {
          *mbuf++ = '-';
          what = -1;
        }
      *mbuf++ = 'd';
    }

  if((MODE_NOCOLORS & oldmode->mode) && !(MODE_NOCOLORS & mode.mode))
    {
      if (what != -1)
        {
          *mbuf++ = '-';
          what = -1;
        }
      *mbuf++ = 'c';
    }

  if((MODE_NOQUITS & oldmode->mode) && !(MODE_NOQUITS & mode.mode))
    {
      if (what != -1)
        {
          *mbuf++ = '-';
          what = -1;
        }
      *mbuf++ = 'q';
    }

  if((MODE_NONICKCH & oldmode->mode) && !(MODE_NONICKCH & mode.mode))
    {
      if (what != -1)
        {
          *mbuf++ = '-';
          what = -1;
        }
      *mbuf++ = 'N';
    }

  if (oldmode->msgs && !mode.msgs)
    {
      if (what != -1)
        {
          *mbuf++ = '-';
          what = -1;
        }
      *mbuf++ = 'f';
    }
				
		
  if (oldmode->limit && !mode.limit)
    {
      if (what != -1)
        {
          *mbuf++ = '-';
          what = -1;
        }
      *mbuf++ = 'l';
    }
    
  if (oldmode->key[0] && !mode.key[0])
    {
      if (what != -1)
        {
          *mbuf++ = '-';
          what = -1;
        }
      *mbuf++ = 'k';
      strcat(parabuf, oldmode->key);
      strcat(parabuf, " ");
      pargs++;
    }

  if ((mode.msgs) && ((mode.msgs != oldmode->msgs) ||
     (mode.per != oldmode->per)))
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;
        }
      *mbuf++ = 'f';
      (void) ircsprintf(numeric, "%i:%i ", mode.msgs, mode.per);      
      (void) strcat(parabuf, numeric);
      pargs++;
    }
		
  if (mode.limit && oldmode->limit != mode.limit)
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;
        }
      *mbuf++ = 'l';
      ircsprintf(numeric, "%d", mode.limit);
      if ((s = strchr(numeric, ' ')))
        *s = '\0';
      strcat(parabuf, numeric);
      strcat(parabuf, " ");
      pargs++;
    }
  if (mode.key[0] && strcmp(oldmode->key, mode.key))
    {
      if (what != 1)
        {
          *mbuf++ = '+';
          what = 1;
        }
      *mbuf++ = 'k';
      strcat(parabuf, mode.key);
      strcat(parabuf, " ");
      pargs++;
    }
  
  chptr->mode = mode;

  if (!keep_our_modes)
    {
      what = 0;
      for (l = chptr->members; l && l->value.cptr; l = l->next)
        {
          if (l->flags & MODE_CHANADM)
            {
              if (what != -1)
                {
                  *mbuf++ = '-';
                  what = -1;
                }
              *mbuf++ = 'a';
              strcat(parabuf, l->value.cptr->name);
              strcat(parabuf, " ");
              pargs++;
              if (pargs >= MAXMODEPARAMS)
                {
                  *mbuf = '\0';
#ifdef SERVERHIDE
                  sjoin_sendit(&me,  sptr, chptr, parv[0]);
#else
                  sjoin_sendit(cptr, sptr, chptr,
                               parv[0]);
#endif
                  mbuf = modebuf;
                  *mbuf = parabuf[0] = '\0';
                  pargs = what = 0;
                }
              l->flags &= ~MODE_CHANADM;
            }        
          if (l->flags & MODE_CHANOP)
            {
              if (what != -1)
                {
                  *mbuf++ = '-';
                  what = -1;
                }
              *mbuf++ = 'o';
              strcat(parabuf, l->value.cptr->name);
              strcat(parabuf, " ");
              pargs++;
              if (pargs >= MAXMODEPARAMS)
                {
                  *mbuf = '\0';
#ifdef SERVERHIDE
                  sjoin_sendit(&me,  sptr, chptr, parv[0]);
#else
                  sjoin_sendit(cptr, sptr, chptr,
                               parv[0]);
#endif
                  mbuf = modebuf;
                  *mbuf = parabuf[0] = '\0';
                  pargs = what = 0;
                }
              l->flags &= ~MODE_CHANOP;
            }
#ifdef HALFOPS
          if (l->flags & MODE_HALFOP)
           {
             if (what != -1)
               {
                 *mbuf++ = '-';
                 what = -1;
               }
              *mbuf++ = 'h';
              strcat(parabuf, l->value.cptr->name);
              strcat(parabuf, " ");
              pargs++;
              if (pargs >= MAXMODEPARAMS)
                {
                  *mbuf = '\0';
#ifdef SERVERHIDE
                 sjoin_sendit(&me,  sptr, chptr, parv[0]);
#else
                 sjoin_sendit(cptr, sptr, chptr, parv[0]);
#endif
                 mbuf = modebuf;
                 *mbuf = parabuf[0] = '\0';
                 pargs = what = 0;
               }
             l->flags &= ~MODE_HALFOP;
           }
#endif            
          if (l->flags & MODE_VOICE)
            {
              if (what != -1)
                {
                  *mbuf++ = '-';
                  what = -1;
                }
              *mbuf++ = 'v';
              strcat(parabuf, l->value.cptr->name);
              strcat(parabuf, " ");
              pargs++;
              if (pargs >= MAXMODEPARAMS)
                {
                  *mbuf = '\0';
#ifdef SERVERHIDE
                  sjoin_sendit(&me,  sptr, chptr, parv[0]);
#else
                  sjoin_sendit(cptr, sptr, chptr,
                               parv[0]);
#endif
                  mbuf = modebuf;
                  *mbuf = parabuf[0] = '\0';
                  pargs = what = 0;
                }
              l->flags &= ~MODE_VOICE;
            }
        }
        sendto_channel_butserv(chptr, &me,
            ":%s NOTICE %s :*** Notice -- TS for %s changed from %lu to %lu",
            me.name, chptr->chname, chptr->chname, oldts, newts);
    }
  if (mbuf != modebuf)
    {
      *mbuf = '\0';
#ifdef SERVERHIDE
      sjoin_sendit(&me,  sptr, chptr, parv[0]);
#else
      sjoin_sendit(cptr, sptr, chptr, parv[0]);
#endif
    }

  *modebuf = *parabuf = '\0';
  if (parv[3][0] != '0' && keep_new_modes)
    channel_modes(sptr, modebuf, parabuf, chptr, 0);
  else
    {
      modebuf[0] = '0';
      modebuf[1] = '\0';
    }

  ircsprintf(t, ":%s SJOIN %lu %s %s %s :", parv[0], tstosend, parv[2],
          modebuf, parabuf);
  t += strlen(t);

  mbuf = modebuf;
  parabuf[0] = '\0';
  pargs = 0;
  *mbuf++ = '+';

  for (s = s0 = strtoken(&p, parv[args+4], " "); s;
       s = s0 = strtoken(&p, (char *)NULL, " "))
    {
	   
	  fl = 0;   	  
#ifdef HALFOPS
      while (*s == '.' || *s == '@' || *s == '%' || *s == '+')
#else
      while (*s == '.' || *s == '@' || *s == '+') 
#endif      
		{
    	  if (*s == '.')
      		fl |= MODE_CHANADM;
    	  else if (*s == '@')
      		fl |= MODE_CHANOP;
#ifdef HALFOPS
          else if (*s == '%')
               fl |= MODE_HALFOP;
#endif
    	  else if (*s == '+')
      		fl |= MODE_VOICE;
      	  s++;
		}

      if (!keep_new_modes)
       {
        if ((fl & MODE_CHANOP) && !(fl & MODE_CHANADM))
          {
            fl = MODE_DEOPPED;
          }
        else
          {
            fl = 0;
          }
       }
		
      if (!(acptr = find_chasing(sptr, s, NULL)))
        continue;
      if (acptr->from != cptr)
        continue;
      people++;
      if (!IsMember(acptr, chptr))
        {
          add_user_to_channel(chptr, acptr, fl);
          if (!IsStealth(acptr))
            sendto_channel_butserv(chptr, acptr, ":%s JOIN :%s",
                                 s, parv[2]);
        }
      if (keep_new_modes)
        strcpy(t, s0);
      else
        strcpy(t, s);
      t += strlen(t);
      *t++ = ' ';
      if (fl & MODE_CHANADM)
        {
          *mbuf++ = 'a';
          strcat(parabuf, s);
          strcat(parabuf, " ");
          pargs++;
          if (pargs >= MAXMODEPARAMS)
            {
              *mbuf = '\0';
#ifdef SERVERHIDE
              sjoin_sendit(&me,  sptr, chptr, parv[0]);
#else
              sjoin_sendit(cptr, sptr, chptr, parv[0]);
#endif
              mbuf = modebuf;
              *mbuf++ = '+';
              parabuf[0] = '\0';
              pargs = 0;
            }
        }      
      if (fl & MODE_CHANOP)
        {
          *mbuf++ = 'o';
          strcat(parabuf, s);
          strcat(parabuf, " ");
          pargs++;
          if (pargs >= MAXMODEPARAMS)
            {
              *mbuf = '\0';
#ifdef SERVERHIDE
              sjoin_sendit(&me,  sptr, chptr, parv[0]);
#else
              sjoin_sendit(cptr, sptr, chptr, parv[0]);
#endif
              mbuf = modebuf;
              *mbuf++ = '+';
              parabuf[0] = '\0';
              pargs = 0;
            }
        }
#ifdef HALFOPS
      if (fl & MODE_HALFOP)
        {
          *mbuf++ = 'h';
          strcat(parabuf, s);
          strcat(parabuf, " ");
          pargs++;
          if (pargs >= MAXMODEPARAMS)
            {
              *mbuf = '\0';
#ifdef SERVERHIDE
              sjoin_sendit(&me,  sptr, chptr, parv[0]);
#else
              sjoin_sendit(cptr, sptr, chptr, parv[0]);
#endif
              mbuf = modebuf;
              *mbuf++ = '+';
              parabuf[0] = '\0';
              pargs = 0;
            }
        }
#endif
      if (fl & MODE_VOICE)
        {
          *mbuf++ = 'v';
          strcat(parabuf, s);
          strcat(parabuf, " ");
          pargs++;
          if (pargs >= MAXMODEPARAMS)
            {
              *mbuf = '\0';
#ifdef SERVERHIDE
              sjoin_sendit(&me,  sptr, chptr, parv[0]);
#else
              sjoin_sendit(cptr, sptr, chptr, parv[0]);
#endif
              mbuf = modebuf;
              *mbuf++ = '+';
              parabuf[0] = '\0';
              pargs = 0;
            }
        }
    }
  
  *mbuf = '\0';
  if (pargs)
#ifdef SERVERHIDE
    sjoin_sendit(&me,  sptr, chptr, parv[0]);
#else
    sjoin_sendit(cptr, sptr, chptr, parv[0]);
#endif
  if (people)
    {
      if (t[-1] == ' ')
        t[-1] = '\0';
      else
        *t = '\0';
      sendto_match_servs(chptr, cptr, "%s", sjbuf);
    }
  return 0;
}


/* m_samode - Just bout the same as df
 *  - Raistlin 
 * parv[0] = sender
 * parv[1] = channel
 * parv[2] = modes
 */
int m_samode(aClient *cptr, aClient *sptr, int parc, char *parv[]) 
{
	aChannel *chptr;
	
	if (!IsPrivileged(cptr)) 
	{
		sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, parv[0]);
		return 0;
	}
	
	if (parc<3) 
	  return 0;
	  
	chptr = hash_find_channel(parv[1], NULL);
	
	if (chptr==NULL) return 0;
	
	if(!check_channel_name(parv[1]))
	  return 0;
	  
	    set_channel_mode(&me, &me, chptr, parc - 2, &parv[2]);
	
    	sendto_match_servs(chptr, cptr, ":%s MODE %s ",
			me.name, chptr->chname, parv[2]);
			
		  if(MyClient(sptr)) {
			  sendto_serv_butone(NULL, ":%s GLOBOPS :%s used SAMODE (%s %s%s%s)",
				  me.name, sptr->name, chptr->chname, modebuf,					
				  (*parabuf!=0 ? " " : ""), parabuf);
			  sendto_ops("from %s: %s used SAMODE (%s %s%s%s)",
				me.name, sptr->name, chptr->chname, modebuf, 
				  (*parabuf!=0 ? " " : ""), parabuf);
		  }

	return 0;
}

static int isvalidopt(char *cp)
{
    char *xp;
   for (xp = cp; *xp; xp++)
   
   if ((!IsDigit(*xp)) && (*xp != ':'))
      return 0;
      
   if (!strchr(cp, ':')) return 0;
   if (strchr(cp, ':') != strrchr(cp, ':')) return 0;
   return 1;
}

static int setfl (aChannel *chptr, char *cp)
{
    int xxi,xyi;
    char *xp;

    xp = index(cp, ':');
    *xp = '\0';
    xxi = atoi(cp);
    xp++;
    xyi = atoi(xp);
    xp--;
    *xp = ':';
    if ( (xxi > 99) || (xyi > 99) || (xxi <= 1) || (xyi == 0)) return 0;
    chptr->mode.msgs = xxi;
    chptr->mode.per = xyi;
    return 1;
}

static int check_for_chan_flood(aClient *cptr, aChannel  *chptr, Link *lp)
  {

    if ((chptr->mode.msgs <= 1) || (chptr->mode.per < 1))
      return 0;
	
	/* Theory here is 
	   If current - lastmsgtime <= mode.per
	    and nummsg is higher than mode.msgs
	    then block the message
	*/
	
    if ( (CurrentTime - (lp->lastmsg))  >= /* current - lastmsgtime */
	     chptr->mode.per)            /* mode.per */
	  {
		/* reset the message counter */
                lp->lastmsg      = CurrentTime;
                lp->nmsg         = 1;
		return 0; /* forget about it.. */
	  }
	
	/* increase msgs */
    lp->nmsg++;
    lp->lastmsg = CurrentTime; 
	
    if ( (lp->nmsg) >= chptr->mode.msgs )
	  {
        static char comment[1024];            

        ircsprintf(comment, FloodLimit_Msg,
          chptr->mode.msgs, chptr->mode.per);
                    
  	    sendto_one(cptr, form_str(ERR_CANNOTSENDTOCHAN),
          me.name, cptr->name, chptr->chname, comment);
          
        cptr->since = CurrentTime + 10;
      
		return 1;
	}	
    
  return 0;
}

/* 
 * whisper - called from m_cnotice and m_cprivmsg.
 *
 * parv[0] = sender prefix
 * parv[1] = nick
 * parv[2] = #channel
 * parv[3] = Private message text
 *
 * Added 971023 by Run.
 * Reason: Allows channel operators to sent an arbitrary number of private
 *   messages to users on their channel, avoiding the max.targets limit.
 *   Building this into m_private would use too much cpu because we'd have
 *   to a cross channel lookup for every private message!
 * Note that we can't allow non-chan ops to use this command, it would be
 *   abused by mass advertisers.
 */
int whisper(struct Client *cptr, struct Client *sptr, int parc, char *parv[], int notice)
{
  int s_is_member = 0, s_is_chanop = 0, t_is_member = 0, s_is_voice = 0;
#ifdef HALFOPS
  int s_is_halfop = 0;
#endif
  struct Client *tcptr;
  struct Channel *chptr;
  register Link *lp;

  if (!MyClient(sptr))
    return 0;
	
  if (parc < 4 || BadPtr(parv[3]))
  {
    sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
        me.name, parv[0], notice ? "CNOTICE" : "CMSG");
    return 0;
  }
  
  if (!(chptr = hash_find_channel(parv[2], NULL)))
  {
    sendto_one(sptr, form_str(ERR_NOSUCHCHANNEL), me.name, parv[0], parv[2]);
    return 0;
  }
  
  if (!(tcptr = find_person(parv[1], NULL)))
  {
    sendto_one(sptr, form_str(ERR_NOSUCHNICK), me.name, parv[0], parv[1]);
    return 0;
  }
  
  for (lp = chptr->members; lp; lp = lp->next)
  {
    register aClient *mcptr = lp->value.cptr;
    if (mcptr == sptr)
    {
      s_is_member = 1;
#ifdef HALFOPS
      if ((lp->flags & CHFL_VOICE) || (lp->flags & CHFL_CHANOP) || (lp->flags & CHFL_HALFOP))
#else
      if ((lp->flags & CHFL_VOICE) || (lp->flags & CHFL_CHANOP))
#endif      
      {
        s_is_voice = 1;
        s_is_chanop = 1;
#ifdef HALFOPS
        s_is_halfop = 1;
#endif
      }
      else
        break;
      if (t_is_member)
        break;
    }
    if (mcptr == tcptr)
    {
      t_is_member = 1;
#ifdef HALFOPS
      if (s_is_chanop || s_is_voice || s_is_halfop)
#else      
      if (s_is_chanop || s_is_voice)
#endif      
        break;
    }
  }
  
#ifdef HALFOPS
  if (!s_is_chanop || !s_is_voice || !s_is_halfop)
#else
  if (!s_is_chanop || !s_is_voice)
#endif  
  {
    sendto_one(sptr, form_str(s_is_member ?
        ERR_CHANOPRIVSNEEDED : ERR_NOTONCHANNEL),
        me.name, parv[0], chptr->chname);
    return 0;
  }
  if (!t_is_member)
  {
    sendto_one(sptr, form_str(ERR_USERNOTINCHANNEL),
        me.name, parv[0], tcptr->name, chptr->chname);
    return 0;
  }
  if (is_silenced(sptr, tcptr))
    return 0;

  if (tcptr->user && tcptr->user->away)
    sendto_one(sptr, form_str(RPL_AWAY),
        me.name, parv[0], tcptr->name, tcptr->user->away);
  if (IsClient(tcptr))
    sendto_prefix_one(tcptr, sptr, ":%s %s %s :%s",
        parv[0], notice ? "NOTICE" : "PRIVMSG", tcptr->name, parv[3]);
		
  return 0;
}

int m_cnotice(aClient *cptr, aClient *sptr, int parc, char *parv[])
{
  return whisper(cptr, sptr, parc, parv, 1);
}

int m_cmsg(aClient *cptr, aClient *sptr, int parc, char *parv[])
{
  int i,size=0;
  char *newparv,*parvtmp;
  if(parc>4) {
    for(i=3;i<parc;i++) size+=strlen(parv[i])+1;
    newparv=(char *)malloc(size+1);
    *newparv='\0';
    for(i=3;i<parc;i++) {
      strcat(newparv,parv[i]);
      strcat(newparv," ");
    }
    parvtmp=parv[3];
    parv[3]=newparv;
    i=whisper(cptr, sptr, parc, parv, 0);
    parv[3]=parvtmp;
    free(newparv);
    return i;
  }
    return whisper(cptr, sptr, parc, parv, 0);
}

