/************************************************************************
 *
 *   IRC - Internet Relay Chat, include/channel.h
 *   Copyright (C) 1990 Jarkko Oikarinen
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
 * $Id: channel.h,v 1.7 2005/10/16 15:01:33 jpinto Exp $
 */

#ifndef INCLUDED_channel_h
#define INCLUDED_channel_h
#ifndef INCLUDED_config_h
#include "config.h"           /* config settings */
#endif
#ifndef INCLUDED_ircd_defs_h
#include "ircd_defs.h"        /* buffer sizes */
#endif
#ifndef INCLUDED_sys_types_h
#include <sys/types.h>        /* time_t */
#define INCLUDED_sys_types_h
#endif

struct SLink;
struct Client;

extern char* spam_words[32];

extern int total_hackops;
extern int total_ignoreops;

/* mode structure for channels */

struct Mode
{
  unsigned int  mode;
  int   limit;
  char  key[KEYLEN + 1];
  int   msgs; /* x */
  int   per;  /* y */  
};

/* channel structure */

struct Channel
{
  struct Channel* nextch;
  struct Channel* prevch;
  struct Channel* hnextch;
  struct Mode     mode;
  char            topic[TOPICLEN + 1];
#ifdef TOPIC_INFO
  char            topic_nick[NICKLEN + 1];
  time_t          topic_time;
#endif
  int             users;
  int		  iusers;   /* invisible users */
  struct SLink*   members;
  struct SLink*   invites;
  struct SLink*   banlist;
  int             num_bed;  /* number of bans+exceptions+denies */
  time_t          channelts;
#ifdef FLUD
  time_t          fludblock;
  struct fludbot* fluders;
#endif
  char            chname[1];
};

typedef struct  Channel aChannel;

extern  struct  Channel *channel;

#define CREATE 1        /* whether a channel should be
                           created or just tested for existance */

#define MODEBUFLEN      200

#define NullChn ((aChannel *)0)

#define ChannelExists(n)        (hash_find_channel(n, NullChn) != NullChn)

/* Maximum mode changes allowed per client, per server is different */
#define MAXMODEPARAMS   6

extern struct Channel* find_channel (char *, struct Channel *);
extern struct SLink*   find_channel_link(struct SLink *, struct Channel *);
extern void    remove_user_from_channel(struct Client *,struct Channel *,int);
extern void    del_invite (struct Client *, struct Channel *);
extern void    send_user_joins (struct Client *, struct Client *);
extern int     can_send (struct Client *, struct Channel *, char*);
extern int     is_chan_op (struct Client *, struct Channel *);
extern int     is_deopped (struct Client *, struct Channel *);
extern int     has_voice (struct Client *, struct Channel *);
#ifdef HALFOPS
extern int     has_halfop (struct Client *, struct Channel *);
extern int     halfop_chanop (struct Client *, struct Channel *);
#endif
extern int     user_channel_mode(struct Client *, struct Channel *);
extern int     count_channels (struct Client *);
extern int     m_names(struct Client *, struct Client *,int, char **);
extern void    send_channel_modes (struct Client *, struct Channel *);
extern void    del_invite (struct Client *, struct Channel *);
extern int     check_channel_name(const char* name);
extern void    channel_modes(struct Client *, char *, char *, struct Channel*, int);
extern void    set_channel_mode(struct Client *, struct Client *, 
                                struct Channel *, int, char **);
								
extern int 	    is_banned (struct Client *, struct Channel *);
extern int 		nick_is_banned(struct Channel *, char *, struct Client *cptr);
extern void		spam_words_init(const char *);
extern int		is_spam(char *text);
extern void	init_new_cmodes(void);

extern struct 	Channel *lch_connects;
extern void 	init_local_log_channels(struct Client *cptr);
int     is_chan_adm(struct Client *cptr, struct Channel *chptr);
int whisper(struct Client *cptr, struct Client *sptr, int parc, char *parv[], int notice);

/* this should eliminate a lot of ifdef's in the main code... -orabidoo */
#ifdef BAN_INFO
#  define BANSTR(l)  ((l)->value.banptr->banstr)
#else
#  define BANSTR(l)  ((l)->value.cp)
#endif

/*
** Channel Related macros follow
*/

/* Channel related flags */

#define CHFL_CHANOP     0x0001 /* Channel operator */
#define CHFL_VOICE      0x0002 /* the power to speak */
#define CHFL_DEOPPED    0x0004 /* deopped by us, modes need to be bounced */
#define CHFL_BAN        0x0008 /* ban channel flag */
/* #define CHFL_EXCEPTION  0x0010 reuse it *//* exception to ban channel flag */
#define CHFL_CHANADM    0x0020 /* channel administrator flag - from services */
#define CHFL_DENY   0x0040 /* regular expression deny flag */

#ifdef HALFOPS
#define CHFL_HALFOP     0x0080
#define MODE_HALFOP     CHFL_HALFOP
#endif

/* Channel Visibility macros */

#define MODE_CHANOP     CHFL_CHANOP
#define MODE_VOICE      CHFL_VOICE
#define MODE_DEOPPED    CHFL_DEOPPED
#define MODE_CHANADM	CHFL_CHANADM
#define MODE_PRIVATE    0x00000008
#define MODE_SECRET     0x00000010
#define MODE_MODERATED  0x00000020
#define MODE_TOPICLIMIT 0x00000040
#define MODE_INVITEONLY 0x00000080
#define MODE_NOPRIVMSGS 0x00000100
#define MODE_KEY        0x00000200
#define MODE_BAN        0x00000400
/* #define MODE_EXCEPTION  0x00000800 reuse it later */
#define MODE_DENY       0x00001000
#define MODE_LIMIT      0x00002000
#define MODE_REGISTERED 0x00004000  /* channel is registered */
#define	MODE_REGONLY	0x00008000  /* registered nicks only */

#define	MODE_OPERONLY	0x00010000	/* Opers only channel */
#define	MODE_ADMINONLY	0x00020000	/* Server Admins only channel */
#define MODE_NOSPAM		0x00040000  /* no cpam messages channel */
#define MODE_NOFLOOD	0x00080000	/* no repetead messages channel */
#define MODE_NOCOLORS	0x00100000	/* no colors channel */
#define MODE_NOQUITS    0x00200000  /* no quit messages channel */
#define MODE_KNOCK		0x00400000	/* knock notice on failed join */
#define MODE_FLOODLIMIT 0x00800000  /* limit messages per time */
#define MODE_NOBOTS     0x01000000  /* no bots allowed */
#define MODE_NONICKCH	0x02000000  /* No nickname changes */
#define	MODE_SSLONLY	0x04000000  /* ssl users only patch by common*/
#define MODE_FLAGS      0x01ffffff

#define MODE_LOG		0x10000000	/* log channel, can never be cleared */

/* some ban types */
#define BAN_NICK		1		/* ban nick */

#ifdef NEED_SPLITCODE

extern int server_was_split;
extern time_t server_split_time;

#ifdef SPLIT_PONG
extern int got_server_pong;
#endif /* SPLIT_PONG */

#endif /* NEED_SPLITCODE */


/*
 * mode flags which take another parameter (With PARAmeterS)
 */
#ifdef HALFOPS
#define MODE_WPARAS (MODE_CHANOP|MODE_VOICE|MODE_BAN|\
                     MODE_CHANADM|MODE_KEY|MODE_LIMIT|MODE_HALFOP)
#else
#define MODE_WPARAS (MODE_CHANOP|MODE_VOICE|MODE_BAN|\
                     MODE_CHANADM|MODE_KEY|MODE_LIMIT)
#endif

/*
 * Undefined here, these are used in conjunction with the above modes in
 * the source.
#define MODE_QUERY      0x10000000
#define MODE_DEL       0x40000000
#define MODE_ADD       0x80000000
 */

/* used in SetMode() in channel.c and m_umode() in s_msg.c */

#define MODE_NULL      0
#define MODE_QUERY     0x10000000
#define MODE_ADD       0x40000000
#define MODE_DEL       0x20000000

#define HoldChannel(x)          (!(x))
/* name invisible */
#define SecretChannel(x)        ((x) && ((x)->mode.mode & MODE_SECRET))
/* channel not shown but names are */
#define HiddenChannel(x)        ((x) && ((x)->mode.mode & MODE_PRIVATE))
/* channel visible */
#define ShowChannel(v,c)        (PubChannel(c) || IsMember((v),(c)) || IsAnOper(v))
#define PubChannel(x)           ((!x) || ((x)->mode.mode &\
                                 (MODE_PRIVATE | MODE_SECRET)) == 0)

#define IsMember(blah,chan) ((blah && blah->user && \
                find_channel_link((blah->user)->channel, chan)) ? 1 : 0)

#define IsChannelName(name) ((name) && (*(name) == '#' || *(name) == '&'))


/*
  Move BAN_INFO information out of the SLink struct
  its _only_ used for bans, no use wasting the memory for it
  in any other type of link. Keep in mind, doing this that
  it makes it slower as more Malloc's/Free's have to be done, 
  on the plus side bans are a smaller percentage of SLink usage.
  Over all, the th+hybrid coding team came to the conclusion
  it was worth the effort.

  - Dianora
*/
typedef struct Ban      /* also used for exceptions -orabidoo */
{
  char *banstr;
  int bantype;
  char *who;
  time_t when;
} aBan;


extern 	char    *pretty_mask(char *);	/* user on m_silence.c */

#ifdef NEED_SPLITCODE

extern int server_was_split;
#if defined(SPLIT_PONG)
extern int got_server_pong;
#endif

#endif  /* NEED_SPLITCODE */

#endif  /* INCLUDED_channel_h */

