/************************************************************************
 *   IRC - Internet Relay Chat, include/msg.h
 *   Copyright (C) 1990 Jarkko Oikarinen and
 *                      University of Oulu, Computing Center
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
 * $Id: msg.h,v 1.4 2005/08/27 16:23:49 jpinto Exp $
 */
#ifndef INCLUDED_msg_h
#define INCLUDED_msg_h
#ifndef INCLUDED_config_h
#include "config.h"
#endif

struct Client;

/* 
 * Message table structure 
 */
struct  Message
{
  char  *cmd;
  int   (* func)();
  unsigned int  count;                  /* number of times command used */
  int   parameters;
  char  flags;
  /* bit 0 set means that this command is allowed to be used
   * only on the average of once per 2 seconds -SRB */

  /* I could have defined other bit maps to above instead of the next two
     flags that I added. so sue me. -Dianora */

  char    allow_unregistered_use;       /* flag if this command can be
                                           used if unregistered */

  char    reset_idle;                   /* flag if this command causes
                                           idle time to be reset */
  unsigned long bytes;
};

struct MessageTree
{
  char*               final;
  struct Message*     msg;
  struct MessageTree* pointers[26];
}; 

typedef struct MessageTree MESSAGE_TREE;


#define MSG_PRIVATE  "PRIVMSG"  /* PRIV */
#define MSG_WHO      "WHO"      /* WHO  -> WHOC */
#define MSG_WHOIS    "WHOIS"    /* WHOI */
#define MSG_WHOWAS   "WHOWAS"   /* WHOW */
#define MSG_USER     "USER"     /* USER */
#define MSG_NICK     "NICK"     /* NICK */
#define MSG_SNICK    "SNICK"	/* SNICK */
#define MSG_SERVER   "SERVER"   /* SERV */
#define MSG_LIST     "LIST"     /* LIST */
#define MSG_TOPIC    "TOPIC"    /* TOPI */
#define MSG_INVITE   "INVITE"   /* INVI */
#define MSG_VERSION  "VERSION"  /* VERS */
#define MSG_QUIT     "QUIT"     /* QUIT */
#define MSG_SQUIT    "SQUIT"    /* SQUI */
#define MSG_KILL     "KILL"     /* KILL */
#define MSG_FLOODEX	"FLOODEX"	/* wtf */
#define MSG_INFO     "INFO"     /* INFO */
#define MSG_LINKS    "LINKS"    /* LINK */
#define MSG_STATS    "STATS"    /* STAT */
#define MSG_USERS    "USERS"    /* USER -> USRS */
#define MSG_HELP     "HELP"     /* HELP */
#define MSG_HELPSYS  "HELPSYS"  /* HELP */
#define MSG_ERROR    "ERROR"    /* ERRO */
#define MSG_AWAY     "AWAY"     /* AWAY */
#define MSG_CONNECT  "CONNECT"  /* CONN */
#define MSG_PING     "PING"     /* PING */
#define MSG_PONG     "PONG"     /* PONG */
#define MSG_OPER     "OPER"     /* OPER */
#define MSG_PASS     "PASS"     /* PASS */
#define MSG_WALLOPS  "WALLOPS"  /* WALL */
#define MSG_POST     "POST"     /* POST */
#define MSG_GLOBOPS  "GLOBOPS"	/* -> WALLOPS */
#define MSG_TIME     "TIME"     /* TIME */
#define MSG_NAMES    "NAMES"    /* NAME */
#define MSG_ADMIN    "ADMIN"    /* ADMI */
#define MSG_TRACE    "TRACE"    /* TRAC */
#define MSG_LTRACE   "LTRACE"   /* LTRA */
#define MSG_NOTICE   "NOTICE"   /* NOTI */
#define MSG_JOIN     "JOIN"     /* JOIN */
#define MSG_PART     "PART"     /* PART */
#define MSG_LUSERS   "LUSERS"   /* LUSE */
#define MSG_MOTD     "MOTD"     /* MOTD */
#define MSG_MODE     "MODE"     /* MODE */
#define MSG_SAMODE   "SAMODE"   /* MODE */
#define MSG_KICK     "KICK"     /* KICK */
#define MSG_USERHOST "USERHOST" /* USER -> USRH */
#define MSG_ISON     "ISON"     /* ISON */
#define MSG_REHASH   "REHASH"   /* REHA */
#define MSG_RESTART  "RESTART"  /* REST */
#define MSG_CLOSE    "CLOSE"    /* CLOS */
#define MSG_SVINFO   "SVINFO"   /* SVINFO */
#define MSG_SJOIN    "SJOIN"    /* SJOIN */
#define MSG_NJOIN    "NJOIN"	/* NJOIN */
#define MSG_NNICK    "NNICK"    /* NNICK */
#define MSG_CAPAB    "CAPAB"    /* CAPAB */
#define MSG_DIE      "DIE"      /* DIE */
#define MSG_DNS      "DNS"      /* DNS  -> DNSS */
#define MSG_KLINE    "KLINE"    /* KLINE */
#define MSG_UNDLINE  "UNDLINE"  /* UNDLINE */
#define MSG_UNKLINE  "UNKLINE"  /* UNKLINE */
#define MSG_DLINE    "DLINE"    /* DLINE */
#define MSG_HTM      "HTM"      /* HTM */
#define MSG_SET      "SET"      /* SET */
#define MSG_DCONF	 "DCONF"		/* DCONF */
#define MSG_SANOTICE	"SANOTICE"	/* SANOTICE */
#define MSG_NEWMASK		"NEWMASK"	/* NEWMASK */

#define MSG_GLINE    	"GLINE"    	/* GLINE */
#define MSG_UNGLINE    "UNGLINE"    /* UNGLINE */

#define MSG_SXLINE    	"SXLINE"    /* SXLINE */
#define MSG_UNSXLINE    "UNSXLINE"	/* UNSXLINE */

#define MSG_SVLINE    	"SVLINE"    /* SVLINE */
#define MSG_UNSVLINE    "UNSVLINE"	/* UNSVLINE */

#define MSG_ZLINE    	"ZLINE"    	/* ZLINE */
#define MSG_UNZLINE    	"UNZLINE"  	/* UNZLINE */


#define MSG_SQLINE 	"SQLINE"    /* SQLINE */
#define MSG_UNSQLINE   	"UNSQLINE" 	/* UNSQLINE */

#define MSG_LOCOPS   	"LOCOPS"   	/* LOCOPS */
#ifdef LWALLOPS
#define MSG_LWALLOPS 	"LWALLOPS" 	/* Same as LOCOPS */
#endif /* LWALLOPS */
#ifdef USE_KNOCK
#define MSG_KNOCK       "KNOCK"     /* KNOCK */
#endif
#define MSG_MAP       	"MAP"       /* MAP */
#define MSG_NEWS       	"NEWS"      /* NEWS */
#define MSG_ZOMBIE	"ZOMBIE"    /* ZOMBIE */
#define MSG_UNZOMBIE    "UNZOMBIE"  /* UNZOMBIE */
#define MSG_DCCDENY	"DCCDENY"
#define MSG_DCCALLOW	"DCCALLOW"
#define MSG_SVSINFO	"SVSINFO"	/* SVSINFO */
#define MSG_CMSG	"CMSG"		/* CMSG */
#define MSG_CPRIVMSG	"CPRIVMSG"		/* CPRIVMSG */
#define MSG_CNOTICE	"CNOTICE"   /* CNOT */


/* Services only messages */
#define MSG_SVSJOIN	"SVSJOIN"
#define MSG_SVSPART	"SVSPART"
#define MSG_SVSMODE 	"SVSMODE"
#define MSG_SVSNICK	"SVSNICK"
#define MSG_SVSGUEST	"SVSGUEST"

/* Services aliases */
#define MSG_NICKSERV	"NICKSERV"
#define MSG_NS		"NS"
#define MSG_CHANSERV	"CHANSERV"
#define MSG_CS		"CS"
#define MSG_MEMOSERV	"MEMOSERV"
#define MSG_MS		"MS"
#define MSG_NEWSSERV	"NEWSSERV"
#define MSG_NWS		"NWS"
#define MSG_OPERSERV	"OPERSERV"
#define MSG_OS		"OS"
#define MSG_STATSERV	"STATSERV"
#define MSG_HELPSERV	"HELPSERV"
#define MSG_HS		"HS"
#define MSG_BOTSERV	"BOTSERV"
#define MSG_BS		"BS"
#define MSG_IDENTIFY	"IDENTIFY"

#define MSG_SILENCE	"SILENCE"
#define	MSG_WATCH	"WATCH"
#define MSG_IRCOPS	"IRCOPS"
#define MSG_SVSADMIN	"SVSADMIN"
#define MSG_REDIRECT    "REDIRECT"
#define MSG_LNOTICE	"LNOTICE"
#define MSG_RNOTICE	"RNOTICE"
#define MSG_CODEPAGE	"CODEPAGE"
#define MSG_VLINK	"VLINK"
#define MSG_LOST	"LOST"
#define MSG_IMODE	"IMODE"
#define MSG_LOGIN	"LOGIN"
#define MSG_HVC		"HVC"

/* special lists */
#define MSG_BOTS	"BOTS"
#define MSG_HELPERS	"HELPERS"
#define MSG_DCCDENYS	"DCCDENYS"
#define MSG_ZOMBIES	"ZOMBIES"

/* setname */
#define MSG_SETNAME	"SETNAME"
#define MSG_CHGHOST	"CHGHOST"

#define MAXPARA    15 


#ifdef MSGTAB
#ifndef INCLUDED_m_commands_h
#include "m_commands.h"       /* m_xxx */
#endif
struct Message msgtab[] = {
#ifdef IDLE_FROM_MSG   /* reset idle time only if privmsg used */
#ifdef IDLE_CHECK       /* reset idle time only if valid target for privmsg
                           and target is not source */

  /*                                        |-- allow use even when unreg.
                                            v   yes/no                  */
  { MSG_PRIVATE, m_private,  0, MAXPARA, 1, 0, 0, 0L },
#else
  { MSG_PRIVATE, m_private,  0, MAXPARA, 1, 0, 1, 0L },
#endif

  /*                                           ^
                                               |__ reset idle time when 1 */
#else   /* IDLE_FROM_MSG */
#ifdef  IDLE_CHECK      /* reset idle time on anything but privmsg */
  { MSG_PRIVATE, m_private,  0, MAXPARA, 1, 0, 1, 0L },
#else
  { MSG_PRIVATE, m_private,  0, MAXPARA, 1, 0, 0, 0L },
  /*                                           ^
                                               |__ reset idle time when 0 */
#endif  /* IDLE_CHECK */
#endif  /* IDLE_FROM_MSG */

  { MSG_NICK,    m_nick,     0, MAXPARA, 1, 1, 0, 0L },
  { MSG_SNICK,   m_nick,     0, MAXPARA, 1, 1, 0, 0L },  
  { MSG_NNICK,   m_nnick,    0, MAXPARA, 1, 1, 0, 0L },    
  { MSG_NOTICE,  m_notice,   0, MAXPARA, 1, 0, 0, 0L },
  { MSG_JOIN,    m_join,     0, MAXPARA, 1, 0, 0, 0L },
  { MSG_MODE,    m_mode,     0, MAXPARA, 1, 0, 0, 0L },
  { MSG_SAMODE,  m_samode,   0, MAXPARA, 1, 0, 0, 0L },
  { MSG_QUIT,    m_quit,     0, MAXPARA, 1, 1, 0, 0L },
  { MSG_PART,    m_part,     0, MAXPARA, 1, 0, 0, 0L },
#ifdef USE_KNOCK
  { MSG_KNOCK,   m_knock,    0, MAXPARA, 1, 0, 0, 0L },
#endif
  { MSG_TOPIC,   m_topic,    0, MAXPARA, 1, 0, 0, 0L },
  { MSG_INVITE,  m_invite,   0, MAXPARA, 1, 0, 0, 0L },
  { MSG_KICK,    m_kick,     0, MAXPARA, 1, 0, 0, 0L },
  { MSG_WALLOPS, m_wallops,  0, MAXPARA, 1, 0, 0, 0L },
#ifdef POST_REGISTER
  { MSG_POST,   m_post,      0, MAXPARA, 1, 1, 0, 0L },
#endif /* POST_REGISTER */  
  { MSG_GLOBOPS, m_globops,  0,       1, 1, 0, 0, 0L },
  { MSG_LOCOPS,  m_locops,   0,       1, 1, 0, 0, 0L },
#ifdef LWALLOPS
  { MSG_LWALLOPS,m_locops,   0,       1, 1, 0, 0, 0L },
#endif /* LWALLOPS */

#ifdef IDLE_FROM_MSG

  /* Only m_private has reset idle flag set */
  { MSG_PONG,    m_pong,     0, MAXPARA, 1, 0, 0, 0L },
  { MSG_PING,    m_ping,     0, MAXPARA, 1, 0, 0, 0L },

#else

  /* else for IDLE_FROM_MSG */
  /* reset idle flag sense is reversed, only reset idle time
   * when its 0, for IDLE_FROM_MSG ping/pong do not reset idle time
   */

  { MSG_PONG,    m_pong,     0, MAXPARA, 1, 0, 1, 0L },
  { MSG_PING,    m_ping,     0, MAXPARA, 1, 0, 1, 0L },


#endif  /* IDLE_FROM_MSG */
  { MSG_UNDLINE, m_undline,  0, MAXPARA, 1, 0, 0, 0L },
  { MSG_ERROR,   m_error,    0, MAXPARA, 1, 1, 0, 0L },
  { MSG_KILL,    m_kill,     0, MAXPARA, 1, 0, 0, 0L },
  { MSG_FLOODEX, m_floodex,  0, MAXPARA, 1, 0, 0, 0L },
  { MSG_USER,    m_user,     0, MAXPARA, 1, 1, 0, 0L },
  { MSG_AWAY,    m_away,     0, MAXPARA, 1, 0, 0, 0L },
#ifdef IDLE_FROM_MSG
  { MSG_ISON,    m_ison,     0, 1,       1, 0, 0, 0L },
#else
  /* ISON should not reset idle time ever
   * remember idle flag sense is reversed when IDLE_FROM_MSG is undefined
   */
  { MSG_ISON,    m_ison,     0, 1,       1, 0, 1, 0L },
#endif /* !IDLE_FROM_MSG */
  { MSG_SERVER,  m_server,   0, MAXPARA, 1, 1, 0, 0L },
  { MSG_SQUIT,   m_squit,    0, MAXPARA, 1, 0, 0, 0L },
  { MSG_WHOIS,   m_whois,    0, MAXPARA, 1, 0, 0, 0L },
  { MSG_WHO,     m_who,      0, MAXPARA, 1, 0, 0, 0L },
  { MSG_WHOWAS,  m_whowas,   0, MAXPARA, 1, 0, 0, 0L },
  { MSG_LIST,    m_list,     0, MAXPARA, 1, 0, 0, 0L },
  { MSG_NAMES,   m_names,    0, MAXPARA, 1, 0, 0, 0L },
  { MSG_USERHOST,m_userhost, 0, 1,       1, 0, 0, 0L },
  { MSG_TRACE,   m_trace,    0, MAXPARA, 1, 0, 0, 0L },
#ifdef LTRACE
  { MSG_LTRACE,  m_ltrace,   0, MAXPARA, 1, 0, 0, 0L },
#endif /* LTRACE */
  { MSG_PASS,    m_pass,     0, MAXPARA, 1, 1, 0, 0L },
  { MSG_LUSERS,  m_lusers,   0, MAXPARA, 1, 0, 0, 0L },
  { MSG_TIME,    m_time,     0, MAXPARA, 1, 0, 0, 0L },
  { MSG_OPER,    m_oper,     0, MAXPARA, 1, 0, 0, 0L },
  { MSG_CONNECT, m_connect,  0, MAXPARA, 1, 0, 0, 0L },
  { MSG_VERSION, m_version,  0, MAXPARA, 1, 0, 0, 0L },
  { MSG_STATS,   m_stats,    0, MAXPARA, 1, 0, 0, 0L },
  { MSG_LINKS,   m_links,    0, MAXPARA, 1, 0, 0, 0L },
  { MSG_ADMIN,   m_admin,    0, MAXPARA, 1, 1, 0, 0L },
  { MSG_USERS,   m_users,    0, MAXPARA, 1, 0, 0, 0L },
  { MSG_HELP,    m_helpsys,  0, MAXPARA, 1, 0, 0, 0L },
  { MSG_HELPSYS, m_helpsys,  0, MAXPARA, 1, 0, 0, 0L },
  { MSG_INFO,    m_info,     0, MAXPARA, 1, 0, 0, 0L },
  { MSG_MOTD,    m_motd,     0, MAXPARA, 1, 0, 0, 0L },
  { MSG_SVINFO,  m_svinfo,   0, MAXPARA, 1, 1, 0, 0L },
  { MSG_SJOIN,   m_sjoin,    0, MAXPARA, 1, 0, 0, 0L },
  { MSG_NJOIN,   m_njoin,    0, MAXPARA, 1, 0, 0, 0L },  
  { MSG_CAPAB,   m_capab,    0, MAXPARA, 1, 1, 0, 0L },
  { MSG_CLOSE,   m_close,    0, MAXPARA, 1, 0, 0, 0L },
  { MSG_KLINE,   m_kline,    0, MAXPARA, 1, 0, 0, 0L },
  { MSG_UNKLINE, m_unkline,  0, MAXPARA, 1, 0, 0, 0L },
  { MSG_DLINE,   m_dline,    0, MAXPARA, 1, 0, 0, 0L },
  { MSG_GLINE,   m_gline,    0, MAXPARA, 1, 0, 0, 0L },
  { MSG_UNGLINE, m_ungline,  0, MAXPARA, 1, 0, 0, 0L }, 
  { MSG_SXLINE,  m_sxline,   0, MAXPARA, 1, 0, 0, 0L },
  { MSG_UNSXLINE,m_unsxline, 0, MAXPARA, 1, 0, 0, 0L },   
  { MSG_SVLINE,  m_svline,   0, MAXPARA, 1, 0, 0, 0L },
  { MSG_UNSVLINE,m_unsvline, 0, MAXPARA, 1, 0, 0, 0L },     
  { MSG_ZLINE,   m_zline,    0, MAXPARA, 1, 0, 0, 0L },
  { MSG_UNZLINE, m_unzline,  0, MAXPARA, 1, 0, 0, 0L },    
  { MSG_SQLINE,  m_sqline,   0, MAXPARA, 1, 0, 0, 0L },
  { MSG_UNSQLINE,m_unsqline, 0, MAXPARA, 1, 0, 0, 0L },    
  { MSG_SILENCE, m_silence,	 0, MAXPARA, 1, 0, 0, 0L },
  { MSG_WATCH,	 m_watch,  	 0, 	  1, 1, 0, 0, 0L },
  { MSG_REHASH,  m_rehash,   0, MAXPARA, 1, 0, 0, 0L },
  { MSG_RESTART, m_restart,  0, MAXPARA, 1, 0, 0, 0L },
  { MSG_DIE,	 m_die,	     0, MAXPARA, 1, 0, 0, 0L },
  { MSG_HTM,     m_htm,      0, MAXPARA, 1, 0, 0, 0L },
  { MSG_SET,     m_set,      0, MAXPARA, 1, 0, 0, 0L },
  { MSG_SVSINFO, m_svsinfo,  0, MAXPARA, 1, 0, 0, 0L },
  { MSG_SVSJOIN, m_svsjoin,  0, MAXPARA, 1, 0, 0, 0L },
  { MSG_SVSPART, m_svspart,  0, MAXPARA, 1, 0, 0, 0L },
  { MSG_SVSMODE, m_svsmode,  0, MAXPARA, 1, 0, 0, 0L },
  { MSG_SVSNICK, m_svsnick,  0, MAXPARA, 1, 0, 0, 0L },
  { MSG_SVSGUEST,m_svsguest, 0, MAXPARA, 1, 0, 0, 0L },    
  { MSG_NICKSERV,m_nickserv, 0,       1, 1, 0, 0, 0L },
  { MSG_NS	,m_nickserv, 0,	      1, 1, 0, 0, 0L },
  { MSG_CHANSERV,m_chanserv, 0,       1, 1, 0, 0, 0L },
  { MSG_CS	,m_chanserv, 0,       1, 1, 0, 0, 0L },
  { MSG_MEMOSERV,m_memoserv, 0,       1, 1, 0, 0, 0L },
  { MSG_MS	,m_memoserv, 0,       1, 1, 0, 0, 0L },
  { MSG_NEWSSERV,m_newsserv, 0,       1, 1, 0, 0, 0L },
  { MSG_NWS	,m_newsserv, 0,       1, 1, 0, 0, 0L },
  { MSG_OPERSERV,m_operserv, 0,       1, 1, 0, 0, 0L },
  { MSG_OS	,m_operserv, 0,       1, 1, 0, 0, 0L },
  { MSG_HELPSERV,m_helpserv, 0,	      1, 1, 0, 0, 0L },
  { MSG_HS	,m_helpserv, 0,       1, 1, 0, 0, 0L },
  { MSG_STATSERV,m_statserv, 0,       1, 1, 0, 0, 0L },
  { MSG_BOTSERV ,m_botserv, 0,       1, 1, 0, 0, 0L },
  { MSG_BS	,m_botserv, 0,	     1, 1, 0, 0, 0L}, 
  { MSG_IDENTIFY,m_identify, 0,       1, 1, 0, 0, 0L },
  { MSG_DCONF,   m_dconf,    0,       4, 1, 0, 0, 0L },
  { MSG_SANOTICE,m_sanotice, 0, MAXPARA, 1, 0, 0, 0L },
  { MSG_NEWMASK, m_newmask,  0, MAXPARA, 1, 0, 0, 0L },
  { MSG_MAP,     m_map, 	 0, MAXPARA, 1, 0, 0, 0L },
  { MSG_IRCOPS,  m_ircops, 	 0, MAXPARA, 1, 0, 0, 0L },
  { MSG_NEWS,  	 m_news, 	 0, MAXPARA, 1, 0, 0, 0L },
  { MSG_ZOMBIE,	 m_zombie,	 0, MAXPARA, 1, 0, 0, 0L },
  { MSG_UNZOMBIE,m_unzombie, 0, MAXPARA, 1, 0, 0, 0L },
  { MSG_DCCDENY, m_dccdeny,	 0, MAXPARA, 1, 0, 0, 0L },
  { MSG_DCCALLOW,m_dccallow, 0, MAXPARA, 1, 0, 0, 0L },
  { MSG_SVSADMIN,m_svsadmin, 0, MAXPARA, 1, 0, 0, 0L },  
  { MSG_CMSG,    m_cmsg,     0, MAXPARA, 1, 0, 0, 0L },
  { MSG_CPRIVMSG,m_cmsg,     0, MAXPARA, 1, 0, 0, 0L },  
  { MSG_CNOTICE, m_cnotice,  0, MAXPARA, 1, 0, 0, 0L },  
  { MSG_REDIRECT,m_redirect, 0, MAXPARA, 1, 0, 0, 0L },
  { MSG_LNOTICE, m_lnotice,  0, 1, 1, 0, 0, 0L },
  { MSG_RNOTICE, m_rnotice,  0, 3, 1, 0, 0, 0L },  
  { MSG_CODEPAGE,m_codepage, 0, MAXPARA, 1, 0, 0, 0L },  
  { MSG_VLINK,   m_vlink, 0, MAXPARA, 1, 0, 0, 0L },    
  { MSG_LOST,    m_lost, 0, MAXPARA, 1, 0, 0, 0L },      
  { MSG_IMODE,   m_imode, 0, MAXPARA, 1, 0, 0, 0L },
  { MSG_BOTS,  m_bots,       0, MAXPARA, 1, 0, 0, 0L },
  { MSG_ZOMBIES,  m_zombies,       0, MAXPARA, 1, 0, 0, 0L },
  { MSG_DCCDENYS,  m_dccdenys,       0, MAXPARA, 1, 0, 0, 0L },
  { MSG_HELPERS,  m_helpers,       0, MAXPARA, 1, 0, 0, 0L },
  { MSG_SETNAME,  m_setname,	   0, MAXPARA, 1, 0, 0, 0L },
  { MSG_CHGHOST,  m_chghost,	   0, MAXPARA, 1, 0, 0, 0L },
  { MSG_LOGIN,    m_login,	   0, MAXPARA, 1, 1, 0, 0L },
  { MSG_HVC,      m_hvc,	   0, MAXPARA, 1, 0, 0, 0L },  
  { (char *) 0, (int (*)()) 0 , 0, 0,    0, 0, 0, 0L }
};

struct MessageTree* msg_tree_root;

#else
extern struct Message       msgtab[];
extern struct MessageTree*  msg_tree_root;
#endif

#endif /* INCLUDED_msg_h */

