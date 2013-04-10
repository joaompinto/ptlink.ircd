/* - Internet Relay Chat, include/client.h
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
 *
 * $Id: client.h,v 1.8 2005/10/05 19:00:09 jpinto Exp $
 */
#ifndef INCLUDED_client_h
#define INCLUDED_client_h
#ifndef INCLUDED_config_h
#include "config.h"
#endif
#if !defined(CONFIG_H_LEVEL_6_1)
#error Incorrect config.h for this revision of ircd.
#endif
#ifndef INCLUDED_sys_types_h
#include <sys/types.h>       /* time_t */
#define INCLUDED_sys_types_h
#endif
#ifndef INCLUDED_netinet_in_h
#include <netinet/in.h>      /* in_addr */
#define INCLUDED_netinet_in_h
#endif
#if defined(HAVE_STDDEF_H)
# ifndef INCLUDED_stddef_h
#  include <stddef.h>        /* offsetof */
#  define INCLUDED_stddef_h
# endif
#endif
#ifndef INCLUDED_ircd_defs_h
# include "ircd_defs.h"
#endif
#ifndef INCLUDED_dbuf_h
#include "dbuf.h"
#endif

#ifdef IPV6
#define HOSTIPLEN       53 /* sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255.ipv6") */
#else
#define HOSTIPLEN       16      /* Length of dotted quad form of IP        */
#endif
#define PASSWDLEN       20
#define IDLEN           12      /* this is the maximum length, not the actual
                                   generated length; DO NOT CHANGE! */
#define CLIENT_BUFSIZE 512      /* must be at least 512 bytes */

/*
 * pre declare structs
 */
struct SLink;
struct ConfItem;
struct Whowas;
struct fludbot;
struct Zdata;
struct Listener;
struct Client;

/*
 * Client structures
 */
struct User
{
  struct User*   next;          /* chain of anUser structures */
  struct SLink*  channel;       /* chain of channel pointer blocks */
  struct SLink*  invited;       /* chain of invite pointer blocks */
  struct SLink*  silence;       /* chain of silence pointer blocks  */
  char*          away;          /* pointer to away message */
  time_t         last;
  int            refcnt;        /* Number of times this block is referenced */
  int            joined;        /* number of channels joined */
  char           id[IDLEN + 1]; /* for future use *hint* */
  const char*    server;        /* pointer to scached server name */
  unsigned int	 news_mask;		/* user subscribed news bit mask */
  struct ConfItem* vlink;       /* virtual link */
  int		lang;		/* user language */
  /*
  ** In a perfect world the 'server' name
  ** should not be needed, a pointer to the
  ** client describing the server is enough.
  ** Unfortunately, in reality, server may
  ** not yet be in links while USER is
  ** introduced... --msa
  */
  /* with auto-removal of dependent links, this may no longer be the
  ** case, but it's already fixed by the scache anyway  -orabidoo
  */
};

struct Server
{
  struct User*     user;        /* who activated this connection */
  const char*      up;          /* Pointer to scache name */
  char             by[NICKLEN + 1];
  struct ConfItem* nline;       /* N-line pointer for this server */
  struct Client*   servers;     /* Servers on this server */
  struct Client*   users;       /* Users on this server */
  char             version[HOSTLEN + 1]; /* server version */
};

struct Client
{
  struct Client*    next;
  struct Client*    prev;
  struct Client*    hnext;
  struct Client*    idhnext;

/* QS */

  struct Client*    lnext;      /* Used for Server->servers/users */
  struct Client*    lprev;      /* Used for Server->servers/users */

/* LINKLIST */
  /* N.B. next_local_client, and previous_local_client
   * duplicate the link list referenced to by struct Server -> users
   * someday, we'll rationalize this... -Dianora
   */

  struct Client*    next_local_client;      /* keep track of these */
  struct Client*    previous_local_client;

  struct Client*    next_server_client;
  struct Client*    next_oper_client;

  struct User*      user;       /* ...defined, if this is a User */
  struct Server*    serv;       /* ...defined, if this is a server */
  struct Client*    servptr;    /* Points to server this Client is on */
  struct Client*    from;       /* == self, if Local Client, *NEVER* NULL! */

  struct Whowas*    whowas;     /* Pointers to whowas structs */
  time_t            lasttime;   /* ...should be only LOCAL clients? --msa */
  time_t            firsttime;  /* time client was created */
  time_t            since;      /* last time we parsed something */
  time_t            tsinfo;     /* TS on the nick, SVINFO on server */
  time_t			nexttarget; /* Time until a change in targets is allowed */
  unsigned char 	targets[MAXTARGETS]; /* Hash values of current targets */
  unsigned int      umodes;     /* opers, normal users subset */
  unsigned int	    imodes;	/* information modes */
  unsigned int      flags;      /* client flags */
  unsigned int      flags2;     /* ugh. overflow */
  int               fd;         /* >= 0, for local clients */
  int               hopcount;   /* number of servers to this 0 = local */
  int    status;     						/* Client type */
  char              nicksent;
  unsigned char     local_flag; /* if this is 1 this client is local */
  short    listprogress;        /* where were we when the /list blocked? */
  int      listprogress2;       /* where in the current bucket were we? */

  /*
   * client->name is the unique name for a client nick or host
   */
  char              name[HOSTLEN + 1]; 
  /* 
   * client->username is the username from ident or the USER message, 
   * If the client is idented the USER message is ignored, otherwise 
   * the username part of the USER message is put here prefixed with a 
   * tilde depending on the I:line, Once a client has registered, this
   * field should be considered read-only.
   */ 
  char              username[USERLEN + 1]; /* client's username */
  /*
   * client->realhost contains the resolved name or ip address
   * as a string for the user, it may be fiddled with for oper spoofing etc.
   * once it's changed the *real* address goes away. This should be
   * considered a read-only field after the client has registered.
   */
  char              realhost[HOSTLEN + 1];     /* client's hostname */
  /*
   * client->host contains a copy of client->realhost if client is coming
   * from a server not using hostname masking or the client hostname masked
   * at the client connection server.
   */
  char              host[HOSTLEN + 1];     /* client's hostname */  
  /*
   * client->info for unix clients will normally contain the info from the 
   * gcos field in /etc/passwd but anything can go here.
   */
  char              info[REALLEN + 1]; /* Free form additional client info */
#ifdef FLUD
  struct SLink*     fludees;
#endif
  /*
   * The following fields are allocated only for local clients
   * (directly connected to *this* server with a socket.
   * The first of them *MUST* be the "count"--it is the field
   * to which the allocation is tied to! *Never* refer to
   * these fields, if (from != self).
   */
  int               count;       /* Amount of data in buffer */
#ifdef FLUD
  time_t            fludblock;
  struct fludbot*   fluders;
#endif
#ifdef ANTI_SPAMBOT
  time_t            last_join_time;   /* when this client last 
                                         joined a channel */
  time_t            last_leave_time;  /* when this client last 
                                       * left a channel */
  int               join_leave_count; /* count of JOIN/LEAVE in less than 
                                         MIN_JOIN_LEAVE_TIME seconds */
  int               oper_warn_count_down; /* warn opers of this possible 
                                          spambot every time this gets to 0 */
#endif
#ifdef ANTI_DRONE_FLOOD
  time_t            first_received_message_time;
  int               received_number_of_privmsgs;
  int               drone_noticed;
#endif
  char  buffer[CLIENT_BUFSIZE]; /* Incoming message buffer */
#ifdef ZIP_LINKS
  struct Zdata*     zip;        /* zip data */
#endif
  short             lastsq;     /* # of 2k blocks when sendqueued called last*/
  struct DBuf       sendQ;      /* Outgoing message queue--if socket full */
  struct DBuf       recvQ;      /* Hold for data incoming yet to be parsed */
  /*
   * we want to use unsigned int here so the sizes have a better chance of
   * staying the same on 64 bit machines. The current trend is to use
   * I32LP64, (32 bit ints, 64 bit longs and pointers) and since ircd
   * will NEVER run on an operating system where ints are less than 32 bits, 
   * it's a relatively safe bet to use ints. Since right shift operations are
   * performed on these, it's not safe to allow them to become negative, 
   * which is possible for long running server connections. Unsigned values 
   * generally overflow gracefully. --Bleep
   */
  unsigned int      sendM;      /* Statistics: protocol messages send */
  unsigned int      sendK;      /* Statistics: total k-bytes send */
  unsigned int      receiveM;   /* Statistics: protocol messages received */
  unsigned int      receiveK;   /* Statistics: total k-bytes received */
  unsigned short    sendB;      /* counters to count upto 1-k lots of bytes */
  unsigned short    receiveB;   /* sent and received. */
  unsigned int      lastrecvM;  /* to check for activity --Mika */
  int               priority;
  struct Listener*  listener;   /* listener accepted from */
  struct SLink*     confs;      /* Configuration record associated */
  struct IN_ADDR    ip;         /* keep real ip# too */
  unsigned short    port;       /* and the remote port# too :-) */
#ifndef USE_ADNS
  struct DNSReply*  dns_reply;  /* result returned from resolver query */  
#else
  struct DNSQuery*  dns_query;  /* result returned from resolver query */  
#endif
#ifdef ANTI_NICK_FLOOD
  time_t            last_nick_change;
  int               number_of_nick_changes;
  int		    services_nick_change;
#endif
  time_t            last_services_use;
  int                number_of_services_use;
  time_t            last_knock; /* don't allow knock to flood */
  /*
   * client->sockhost contains the ip address gotten from the socket as a
   * string, this field should be considered read-only once the connection
   * has been made. (set in s_bsd.c only)
   */
  char              sockhost[HOSTIPLEN + 1]; /* This is the host name from the 
                                              socket ip address as string */
  /*
   * XXX - there is no reason to save this, it should be checked when it's
   * received and not stored, this is not used after registration
   */
  char              passwd[PASSWDLEN + 1];
  struct SLink*     watch; 		/* user's watch list */
  int 				watches; 	/* how many watches this user has set */     
  struct            ListOptions *lopt;          /* Saved /list options */
  int               caps;       /* capabilities bit-field */
  int		    codepage; /* codepage number (index within codepages[]) */
  int               hvc;
  /* SSL support */
#ifdef HAVE_SSL
  int    use_ssl;
  int	 ssl_hshake_done;
  struct SSL*	  ssl;   /* SSL instance */
  struct X509*    client_cert;  /* SSL client's certificate */
#endif  
};

/*
 * status macros.
 */
#define STAT_CONNECTING 0x01 
#define STAT_HANDSHAKE  0x02 
#define STAT_ME         0x04
#define STAT_UNKNOWN    0x08
#define STAT_SERVER     0x10 
#define STAT_CLIENT     0x20 


#define IsRegisteredUser(x)     ((x)->status == STAT_CLIENT)
#define IsRegistered(x)         ((x)->status  > STAT_UNKNOWN)
#define IsConnecting(x)         ((x)->status == STAT_CONNECTING)
#define IsHandshake(x)          ((x)->status == STAT_HANDSHAKE)
#define IsMe(x)                 ((x)->status == STAT_ME)
#define IsUnknown(x)            ((x)->status == STAT_UNKNOWN)
#define IsServer(x)             ((x)->status == STAT_SERVER)
#define IsClient(x)             ((x)->status == STAT_CLIENT)

#define SetConnecting(x)        ((x)->status = STAT_CONNECTING)
#define SetHandshake(x)         ((x)->status = STAT_HANDSHAKE)
#define SetMe(x)                ((x)->status = STAT_ME)
#define SetUnknown(x)           ((x)->status = STAT_UNKNOWN)
#define SetServer(x)            ((x)->status = STAT_SERVER)
#define SetClient(x)            ((x)->status = STAT_CLIENT)

#define STAT_CLIENT_PARSE (STAT_UNKNOWN | STAT_CLIENT)
#define STAT_SERVER_PARSE (STAT_CONNECTING | STAT_HANDSHAKE | STAT_SERVER)

#define PARSE_AS_CLIENT(x)      ((x)->status & STAT_CLIENT_PARSE)
#define PARSE_AS_SERVER(x)      ((x)->status & STAT_SERVER_PARSE)

/*
 * ts stuff
 */
#define TS_CURRENT      10      /* current TS protocol version */
#define TS_MIN          9       /* minimum supported TS protocol version */
#define TS_DOESTS       0x20000000
#define DoesTS(x)       ((x)->tsinfo == TS_DOESTS)


/* housekeeping flags */
#define FLAGS_PINGSENT     0x0001 /* Unreplied ping sent */
#define FLAGS_DEADSOCKET   0x0002 /* Local socket is dead--Exiting soon */
#define FLAGS_KILLED       0x0004 /* Prevents "QUIT" from being sent for this*/
#define FLAGS_BLOCKED      0x0008 /* socket is in a blocked condition */
#define FLAGS_REJECT_HOLD  0x0010 /* client has been klined */
#define FLAGS_CLOSING      0x0020 /* set when closing to suppress errors */
#define FLAGS_CHKACCESS    0x0040 /* ok to check clients access if set */
#define FLAGS_GOTID        0x0080 /* successful ident lookup achieved */
#define FLAGS_NEEDID       0x0100 /* I-lines say must use ident return */
#define FLAGS_NONL         0x0200 /* No \n in buffer */
#define FLAGS_NORMALEX     0x0400 /* Client exited normally */
#define FLAGS_SENDQEX      0x0800 /* Sendq exceeded */
#define FLAGS_IPHASH       0x1000 /* iphashed this client */
#define FLAGS_SERVICE	     0x2000 /* Is Service server/client ? */
#define FLAGS_SSL	   			 0x4000 /* SSL handshake done ? */
#define FLAGS_SCS          0x8000 /* SCS cache synchronized ? */
#define FLAGS_SPATH	   		 0x10000 /* On services path */
#define FLAGS_AUTOAWAY     0x20000 /* user is away from autoaway */
#define FLAGS_WEBHOST			 0x40000 /* user comming from a webchat service? */

/* umodes, settable flags */
#define UMODE_WALLOP       	0x0001 /* send wallops to them */
#define UMODE_INVISIBLE    	0x0002 /* makes user invisible */
#define UMODE_ZOMBIE       	0x0004 /* zombie, settable by command */

/* settable by services */
#define UMODE_IDENTIFIED   	0x0008 /* registered and identified nick */
#define UMODE_SADMIN		0x0010 /* Services admin */

/* user information flags, only settable by remote mode or local oper */
#define UMODE_SSL           	0x00200 /* ssl connection */
#define UMODE_SPY		0x2000 /* for /whois notices */
#define UMODE_OPER         	0x4000 /* Operator */
#define UMODE_LOCOP        	0x8000 /* Local operator -- SRB */


#define UMODE_ADMIN		0x00020000 /* Server Admin */
#define UMODE_TECHADMIN		0x00040000 /* Technical Admin */
#define UMODE_NETADMIN		0x00080000 /* Network Admin */
#define UMODE_HELPER		0x00100000 /* Network Admin */
#define UMODE_BOT		0x00200000 /* bot client */
#define UMODE_NODCC		0x00400000 /* dcc blocked */
#define UMODE_PRIVATE		0x00800000 /* hide channels from whois */
#define UMODE_REGMSG		0x01000000 /* receive privmsg only from +r nicks */
#define UMODE_STEALTH		0x02000000 /* stealth mode */
#define UMODE_HIDEOPER		0x04000000 /* hide oper status */
#define UMODE_FLOODEX		0x08000000 /* flood exempt */

/* end of umodes */


/* information modes (imodes) */
#define IMODE_BOTS		0x00000001 /* bots detection */
#define IMODE_CLIENTS		0x00000002 /* clients connects/quits */
#define IMODE_KILLS		0x00000004 /* kills */
#define IMODE_REJECTS		0x00000008 /* client rejection */
#define IMODE_GENERIC		0x00000010 /* generic */
#define IMODE_VLINES		0x00000020 /* vlines */
#define IMODE_SPY		0x00000040 /* whois/info/motd notices */
#define IMODE_TARGET		0x00000080 /* target limit */
#define IMODE_DEBUG		0x00000100 /* debug */
#define IMODE_EXTERNAL		0x00000200 /* remote connects/squits */
#define IMODE_FULL		0x00000400 /* full server */

/* *sigh* overflow flags */
#define FLAGS2_RESTRICTED   	0x0001      /* restricted client */
#define FLAGS2_PING_TIMEOUT 	0x0002
#define FLAGS2_E_LINED      	0x0004      /* client is graced with E line */
#define FLAGS2_F_LINED      	0x0008      /* client is graced with F line */
#define FLAGS2_EXEMPTGLINE  	0x0010      /* client can't be G-lined */

/* oper priv flags */
#define FLAGS2_OPER_GLOBAL_KILL 0x0020  /* oper can global kill */
#define FLAGS2_OPER_REMOTE      0x0040  /* oper can do squits/connects */
#define FLAGS2_OPER_UNKLINE     0x0080  /* oper can use unkline */
#define FLAGS2_OPER_GLINE       0x0100  /* oper can use gline */
/* #define FLAGS2_OPER_N           0x0200 */ /* oper can umode n */
#define FLAGS2_OPER_K           0x0400  /* oper can kill/kline */
#define FLAGS2_OPER_DIE         0x0800  /* oper can die */
#define FLAGS2_OPER_REHASH      0x1000  /* oper can rehash */
#define FLAGS2_OPER_FLAGS       (FLAGS2_OPER_GLOBAL_KILL | \
                                 FLAGS2_OPER_REMOTE | \
                                 FLAGS2_OPER_UNKLINE | \
                                 FLAGS2_OPER_GLINE | \
                                 FLAGS2_OPER_K | \
                                 FLAGS2_OPER_DIE | \
                                 FLAGS2_OPER_REHASH)
/* ZIP_LINKS */

#define FLAGS2_ZIP           	0x00004000  /* (server) link is zipped */
#define FLAGS2_ZIPFIRST      	0x00008000  /* start of zip (ignore any CR/LF) */
#define FLAGS2_CBURST       	0x000010000  /* connection burst being sent */

#define FLAGS2_DOINGLIST    	0x20000  /* client is doing a list */
#ifdef IDLE_CHECK
#define FLAGS2_IDLE_LINED   	0x40000
#endif
#define FLAGS2_ALREADY_EXITED   0x80000   	/* kludge grrrr */
#define FLAGS2_SENDQ_POP  	0x100000  	/* sendq exceeded (during list) */
#define FLAGS2_WEBCHAT          0x200000  	/* webchat user */

#define USER_UMODES	 (UMODE_INVISIBLE | UMODE_WALLOP | UMODE_HELPER | \
					  UMODE_BOT | UMODE_IDENTIFIED | \
					  UMODE_PRIVATE | UMODE_REGMSG | \
					  UMODE_STEALTH | UMODE_SSL | UMODE_FLOODEX )
#define INTERNAL_UMODES (UMODE_BOT | UMODE_IDENTIFIED | UMODE_STEALTH | \
			UMODE_SADMIN | UMODE_SSL | UMODE_FLOODEX )
#define ALL_UMODES   (USER_UMODES | UMODE_LOCOP | UMODE_SPY | \
			UMODE_OPER | UMODE_LOCOP | UMODE_SADMIN | \
			 UMODE_ZOMBIE | UMODE_HELPER | UMODE_HIDEOPER | \
			 UMODE_ADMIN | UMODE_TECHADMIN | UMODE_NETADMIN )

#ifndef OPER_UMODES
#define OPER_UMODES  (UMODE_OPER | UMODE_WALLOP | UMODE_SPY)
#endif /* OPER_UMODES */

#ifndef LOCOP_UMODES
#define LOCOP_UMODES (UMODE_LOCOP | UMODE_WALLOP | UMODE_SPY)
#endif /* LOCOP_UMODES */

#define FLAGS_ID     (FLAGS_NEEDID | FLAGS_GOTID)

/*
 * flags macros.
 */
#define IsPerson(x)             (IsClient(x) && (x)->user)
#define DoAccess(x)             ((x)->flags & FLAGS_CHKACCESS)
#define IsLocal(x)              ((x)->flags & FLAGS_LOCAL)
#define IsDead(x)               ((x)->flags & FLAGS_DEADSOCKET)
#define SetAccess(x)            ((x)->flags |= FLAGS_CHKACCESS)
#define NoNewLine(x)            ((x)->flags & FLAGS_NONL)
#define ClearAccess(x)          ((x)->flags &= ~FLAGS_CHKACCESS)
#define MyConnect(x)            ((x)->local_flag != 0)
#define MyClient(x)             (MyConnect(x) && IsClient(x))

/* oper flags */
#define MyOper(x)               (MyConnect(x) && IsOper(x))
#define IsOper(x)               ((x)->umodes & UMODE_OPER)
#define IsLocOp(x)              ((x)->umodes & UMODE_LOCOP)
#define IsAnOper(x)             ((x)->umodes & (UMODE_OPER|UMODE_LOCOP))
#define IsHelper(x)		((x)->umodes & UMODE_HELPER)
#define IsSAdmin(x)		((x)->umodes & UMODE_SADMIN)
#define IsAdmin(x)		((x)->umodes & UMODE_ADMIN)
#define IsNetAdmin(x)		((x)->umodes & UMODE_NETADMIN)
#define IsTechAdmin(x)		((x)->umodes & UMODE_TECHADMIN)
#define SetOper(x)              ((x)->umodes |= UMODE_OPER)
#define SetLocOp(x)             ((x)->umodes |= UMODE_LOCOP)
#define SetHelper(x)		((x)->umodes |= UMODE_HELPER)
#define SetSAdmin(x)		((x)->umodes |= UMODE_SADMIN)
#define SetAdmin(x)		((x)->umodes |= UMODE_ADMIN)
#define SetNetAdmin(x)		((x)->umodes |= UMODE_NETADMIN)
#define SetTechAdmin(x)		((x)->umodes |= UMODE_TECHADMIN)
#define ClearOper(x)            ((x)->umodes &= ~UMODE_OPER)
#define ClearLocOp(x)           ((x)->umodes &= ~UMODE_LOCOP)
#define ClearHelper(x)		((x)->umodes &= ~UMODE_HELPER)
#define ClearSAdmin(x)		((x)->umodes &= ~UMODE_SADMIN)
#define ClearAdmin(x)		((x)->umodes &= ~UMODE_ADMIN)
#define ClearNetAdmin(x)	((x)->umodes &= ~UMODE_NETADMIN)
#define ClearTechAdmin(x)	((x)->umodes &= ~UMODE_TECHADMIN)

#define IsPrivileged(x)         (IsAnOper(x) || IsServer(x))

/* umode flags */
#define IsPrivate(x)          	((x)->umodes & UMODE_PRIVATE)

#define IsSsl(x)		((x)->umodes & UMODE_SSL)
#define SetSsl(x)		((x)->umodes |= UMODE_SSL)
#define ClearSsl(x)		((x)->umodes &= ~UMODE_SSL)

#define IsFloodEx(x)		((x)->umodes & UMODE_FLOODEX)
#define SetFloodEx(x)		((x)->umodes |= UMODE_FLOODEX)
#define ClearFloodEx(x)		((x)->umodes &= ~UMODE_FLOODEX)



#define IsStealth(x)		((x)->umodes & UMODE_STEALTH)
#define SetStealth(x)		((x)->umodes |= UMODE_STEALTH)
#define ClearStealth(x)		((x)->umodes &= ~UMODE_STEALTH)

#define IsHideOper(x)		((x)->umodes & UMODE_HIDEOPER)
#define SetHideOper(x)		((x)->umodes |= UMODE_HIDEOPER)
#define ClearHideOper(x)	((x)->umodes &= ~UMODE_HIDEOPER)

#define IsBot(x)          	((x)->umodes & UMODE_BOT)
#define SetBot(x)		((x)->umodes |= UMODE_BOT)
#define ClearBot(x)		((x)->umodes &= ~UMODE_BOT)

#define IsInvisible(x)          ((x)->umodes & UMODE_INVISIBLE)
#define SetInvisible(x)         ((x)->umodes |= UMODE_INVISIBLE)
#define ClearInvisible(x)       ((x)->umodes &= ~UMODE_INVISIBLE)

#define IsIdentified(x)		((x)->umodes & UMODE_IDENTIFIED)
#define SetIdentified(x)        ((x)->umodes |= UMODE_IDENTIFIED)
#define ClearIdentified(x)      ((x)->umodes &= ~UMODE_IDENTIFIED)

#define IsRegMsg(x)		((x)->umodes & UMODE_REGMSG)
#define SetRegMsg(x)        	((x)->umodes |= UMODE_REGMSG)
#define ClearRegMsg(x)      	((x)->umodes &= ~UMODE_REGMSG)

#define IsZombie(x)		((x)->umodes & UMODE_ZOMBIE)
#define SetZombie(x)        	((x)->umodes |= UMODE_ZOMBIE)
#define ClearZombie(x)      	((x)->umodes &= ~UMODE_ZOMBIE)

#define IsNoDCC(x)		((x)->umodes & UMODE_NODCC)
#define SetNoDCC(x)        	((x)->umodes |= UMODE_NODCC)
#define ClearNoDCC(x)      	((x)->umodes &= ~UMODE_NODCC)

#define SendWallops(x)          ((x)->umodes & UMODE_WALLOP)
#define ClearWallops(x)         ((x)->umodes &= ~UMODE_WALLOP)

#define SetWallops(x)           ((x)->umodes |= UMODE_WALLOP)

#define SendSpyNotice(x)        ((x)->umodes & UMODE_SPY)

/* service flags */
#define SetService(x)           ((x)->flags |= FLAGS_SERVICE)
#define IsService(x)        	((x)->flags & FLAGS_SERVICE)
#define ClearService(x)        	((x)->flags &= ~FLAGS_SERVICE)

#define SetServicesPath(x)	((x)->flags |= FLAGS_SPATH)
#define IsServicesPath(x)	((x)->flags & FLAGS_SPATH)
#define ClearServicesPath(x)	 ((x)->flags &= ~FLAGS_SPATH)

/* SCS */
#define SetSCS(x)           ((x)->flags |= FLAGS_SCS)
#define IsSCS(x)        	((x)->flags & FLAGS_SCS)
#define ClearSCS(x)        	((x)->flags &= ~FLAGS_SCS)

/* SSL */
#define SetSecure(x)            ((x)->flags |= FLAGS_SSL)
#define IsSecure(x)        	    ((x)->flags & FLAGS_SSL)


#ifdef REJECT_HOLD
#define IsRejectHeld(x)         ((x)->flags & FLAGS_REJECT_HOLD)
#define SetRejectHold(x)        ((x)->flags |= FLAGS_REJECT_HOLD)
#endif

#define SetIpHash               ((x)->flags |= FLAGS_IPHASH)
#define ClearIpHash             ((x)->flags &= ~FLAGS_IPHASH)
#define IsIpHash                ((x)->flags & FLAGS_IPHASH)

#define SetNeedId(x)            ((x)->flags |= FLAGS_NEEDID)
#define IsNeedId(x)             (((x)->flags & FLAGS_NEEDID) != 0)

#define SetGotId(x)             ((x)->flags |= FLAGS_GOTID)
#define IsGotId(x)              (((x)->flags & FLAGS_GOTID) != 0)

#define SetAutoAway(x)		((x)->flags |= FLAGS_AUTOAWAY)
#define ClearAutoAway(x)	((x)->flags &= ~FLAGS_AUTOAWAY)
#define IsAutoAway(x)		((x)->flags & FLAGS_AUTOAWAY)

#define SetWebHost(x)		((x)->flags |= FLAGS_WEBHOST)
#define ClearWebHost(x)	((x)->flags &= ~FLAGS_WEBHOST)
#define IsWebHost(x)		((x)->flags & FLAGS_WEBHOST)

/*
 * flags2 macros.
 */
#define IsRestricted(x)         ((x)->flags2 & FLAGS2_RESTRICTED)
#define SetRestricted(x)        ((x)->flags2 |= FLAGS2_RESTRICTED)
#define ClearDoingList(x)       ((x)->flags2 &= ~FLAGS2_DOINGLIST)
#define SetDoingList(x)         ((x)->flags2 |= FLAGS2_DOINGLIST)
#define IsDoingList(x)          ((x)->flags2 & FLAGS2_DOINGLIST)
#define ClearSendqPop(x)        ((x)->flags2 &= ~FLAGS2_SENDQ_POP)
#define SetSendqPop(x)          ((x)->flags2 |= FLAGS2_SENDQ_POP)
#define IsSendqPopped(x)        ((x)->flags2 & FLAGS2_SENDQ_POP)
#define SetWebchat(x)           ((x)->flags2 |= FLAGS2_WEBCHAT)
#define IsWebchat(x)            ((x)->flags2 & FLAGS2_WEBCHAT)
#define IsElined(x)             ((x)->flags2 & FLAGS2_E_LINED)
#define SetElined(x)            ((x)->flags2 |= FLAGS2_E_LINED)
#define IsFlined(x)             ((x)->flags2 & FLAGS2_F_LINED)
#define SetFlined(x)            ((x)->flags2 |= FLAGS2_F_LINED)
#define IsExemptGline(x)	  ((x)->flags2 & FLAGS2_EXEMPTGLINE)
#define SetExemptGline(x)	  ((x)->flags2 |= FLAGS2_EXEMPTGLINE)

#ifdef IDLE_CHECK
#define SetIdlelined(x)         ((x)->flags2 |= FLAGS2_IDLE_LINED)
#define IsIdlelined(x)          ((x)->flags2 & FLAGS2_IDLE_LINED)
#endif

#define SetOperGlobalKill(x)    ((x)->flags2 |= FLAGS2_OPER_GLOBAL_KILL)
#define IsOperGlobalKill(x)     ((x)->flags2 & FLAGS2_OPER_GLOBAL_KILL)
#define SetOperRemote(x)        ((x)->flags2 |= FLAGS2_OPER_REMOTE)
#define IsOperRemote(x)         ((x)->flags2 & FLAGS2_OPER_REMOTE)
#define SetOperUnkline(x)       ((x)->flags2 |= FLAGS2_OPER_UNKLINE)
#define IsSetOperUnkline(x)     ((x)->flags2 & FLAGS2_OPER_UNKLINE)
#define SetOperGline(x)         ((x)->flags2 |= FLAGS2_OPER_GLINE)
#define IsSetOperGline(x)       ((x)->flags2 & FLAGS2_OPER_GLINE)
#define SetOperK(x)             ((x)->flags2 |= FLAGS2_OPER_K)
#define IsSetOperK(x)           ((x)->flags2 & FLAGS2_OPER_K)
#define SetOperDie(x)           ((x)->flags2 |= FLAGS2_OPER_DIE)
#define IsOperDie(x)            ((x)->flags2 & FLAGS2_OPER_DIE)
#define SetOperRehash(x)        ((x)->flags2 |= FLAGS2_OPER_REHASH)
#define IsOperRehash(x)         ((x)->flags2 & FLAGS2_OPER_REHASH)
#define CBurst(x)               ((x)->flags2 & FLAGS2_CBURST)

/*
 * 'offsetof' is defined in ANSI-C. The following definition
 * is not absolutely portable (I have been told), but so far
 * it has worked on all machines I have needed it. The type
 * should be size_t but...  --msa
 */
#ifndef offsetof
#define offsetof(t, m) (size_t)((&((t *)0L)->m))
#endif

#define CLIENT_LOCAL_SIZE sizeof(struct Client)
#define CLIENT_REMOTE_SIZE offsetof(struct Client, count)

/*
 * definitions for get_client_name
 */
#define HIDE_IP 0
#define SHOW_IP 1
#define MASK_IP 2
#define REAL_IP 3

extern time_t         check_pings(time_t current);
extern const char*    get_client_name(struct Client* client, int show_ip);
extern const char*    get_client_host(struct Client* client);
extern void           release_client_dns_reply(struct Client* client);
extern void           init_client_heap(void);
extern void           clean_client_heap(void);
extern struct Client* make_client(struct Client* from);
extern void 	      free_client(struct Client* cptr);
extern void           add_client_to_list(struct Client* client);
extern void           remove_client_from_list(struct Client *);
extern void           add_client_to_llist(struct Client** list, 
                                          struct Client* client);
extern void           del_client_from_llist(struct Client** list, 
                                            struct Client* client);
extern int            exit_client(struct Client*, struct Client*, 
                                  struct Client*, const char* comment);

extern void     count_local_client_memory(int *, int *);
extern void     count_remote_client_memory(int *, int *);
extern  int     check_registered (struct Client *);
extern  int     check_registered_user (struct Client *);

extern struct Client* find_chasing (struct Client *, char *, int *);
extern struct Client* find_client(const char* name, struct Client* client);
extern struct Client* find_server_by_name(const char* name);
extern struct Client* find_person (char *, struct Client *);
extern struct Client* find_server(const char* name);
extern struct Client* find_userhost (char *, char *, struct Client *, int *);
extern struct Client* next_client(struct Client* next, const char* name);
extern struct Client* next_client_double(struct Client* next, 
                                         const char* name);

/* 
 * Time we allow clients to spend in unknown state, before tossing.
 */
#define UNKNOWN_TIME 20
#endif /* INCLUDED_client_h */
