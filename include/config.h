/************************************************************************
 *   IRC - Internet Relay Chat, include/config.h
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
 * $Id: config.h,v 1.11 2005/12/11 20:42:17 jpinto Exp $
 */
#ifndef INCLUDED_config_h
#define INCLUDED_config_h
#ifndef INCLUDED_setup_h
#include "setup.h"
#define INCLUDED_setup_h
#endif

/* PLEASE READ SECTION:
 *
 * I have commented out WHOIS_NOTICE and STATS_NOTICE
 * Personally, I feel opers do not need to know the information
 * returned by having those two defines defined, it is an invasion
 * of privacy. The original need/use of showing STATS_NOTICE
 * it seems to me, was to find stats flooders. They no longer
 * can do much damage with stats flooding, so why show it?
 * whois notice is just an invasion of privacy. Who cares?
 * I personally hope you agree with me, and leave these undef'ed
 * but at least you have been warned.
 *
 */



/***************** MAKE SURE THIS IS CORRECT!!!!!!!!! **************/
/* ONLY EDIT "HARD_FDLIMIT_" and "INIT_MAXCLIENTS" */

/* You may also need to hand edit the Makefile to increase
 * the value of FD_SETSIZE 
 */

/* These ultra low values pretty much guarantee the server will
 * come up initially, then you can increase these values to fit your
 * system limits. If you know what you are doing, increase them now
 */

#define HARD_FDLIMIT_   600
#define INIT_MAXCLIENTS 500

/*
 * This is how many 'buffer connections' we allow... 
 * Remember, MAX_BUFFER + MAX_CLIENTS can't exceed HARD_FDLIMIT :)
 */
#define MAX_BUFFER      60

/* NICKNAMEHISTORYLENGTH - size of WHOWAS array
 * this defines the length of the nickname history.  each time a user changes
 * nickname or signs off, their old nickname is added to the top of the list.
 * NOTE: this is directly related to the amount of memory ircd will use whilst
 *       resident and running - it hardly ever gets swapped to disk!  Memory
 *       will be preallocated for the entire whowas array when ircd is started.
 *       You will want to crank this down if you are on a small net.
 */
#define NICKNAMEHISTORYLENGTH 3000

/* Don't change this... */
#define HARD_FDLIMIT    (HARD_FDLIMIT_ - 10)
#define MASTER_MAX      (HARD_FDLIMIT - MAX_BUFFER)
/*******************************************************************/

/* MOTD Filenames */
#define IMOTDF   "ircd.motd"
#define OMOTDF   "opers.motd"
#define WMOTDF   "webchat.motd"

/*
 * CRYPT_AUTH_PASSWORD - I: line passwords need to be crypted with
 * mkpasswd to be valid
 * Credits:  Hwy/larne
 */
#undef CRYPT_AUTH_PASSWORD

/* POST_REGISTER - Reject a client that issues the POST command
 * during registration.  This is often done by HTTP proxies.
 * Credits:  Hwy, backported from -7
 */
#define POST_REGISTER

/* 
 * GLINE_ON_POST - Will gline host from wich the POST command
 * was received. 
 */
#undef GLINE_ON_POST

/*
 * FIZZER_CHECK - Will check clients on connection for the fizzer worm
 */ 
#define FIZZER_CHECK

/*
 * MIRC_DCC_BUG_CHECK - Will check DCCs for mIRC exploit 
 */
#define MIRC_DCC_BUG_CHECK

/* NEG_PORT - Allow a negative port number to be used in a C: line to
 * specify a port number, but disable autoconnect on that C: line
 * Idea from jv's +hemp patch for IRCnet
 *
 * Enabling this option will also hide the "autoconnect is disabled" warnings
 * that will normally be sent every connection frequency minutes
 */
#define NEG_PORT


/* REGLIST - Only +r channels will get listed */
#undef REGLIST

/* HIDE_OPS
 * Define this to prevent non chanops from seeing what ops a channel has
 * NOT ADEQUATELY TESTED YET, DON'T USE ON PRODUCTION NETWORK --Rodder
 */
/* #undef HIDE_OPS */

/* SERVERHIDE
 * Define this to prevent users from seeing what server a user is on.
 * It also hide IP address in STATS commands and various notices, disables
 * TRACE and LTRACE, and prevents hunting by nickname for nonopers.
 * NOT ADEQUATELY TESTED YET, DON'T USE ON PRODUCTION NETWORK --Rodder
 */
/* #undef SERVERHIDE */

/* HIDE_SERVERS_IPS
 * Define this to prevent opers from seeing the IP of a server.
 * This will not show IPs of any server to anyone, to protect hidden
 * hubs from untrustable opers.
 */
/* #define HIDE_SERVERS_IPS */

/* SLAVE_SERVERS - Use this to send LOCOPS and KLINES to servers you define
 * uses U: lines in ircd.conf, each server defined in an U: line
 * is sent a copy of the locally placed K-line, and will also
 * accept K-lines from those servers.
 * This is useful for sites with more than one client server.
 */
#undef SLAVE_SERVERS

/* FNAME_USERLOG and FNAME_OPERLOG - logs of local USERS and OPERS
 * Define this filename to maintain a list of persons who log
 * into this server. Logging will stop when the file does not exist.
 * Logging will be disable also if you do not define this.
 * FNAME_USERLOG just logs user connections, FNAME_OPERLOG logs every
 * successful use of /oper.  These are either full paths or files within ETCPATH
 *
 * These need to be defined if you want to use SYSLOG logging, too.
 */
#define FNAME_USERLOG "users.log"
#define FNAME_OPERLOG "opers.log" 

/* RFC1035_ANAL
 * Defining this causes ircd to reject hostnames with non-compliant chars.
 * undef'ing it will allow hostnames with _ or / to connect
 */
#define RFC1035_ANAL

/* ALLOW_DOT_IN_IDENT
 * Defining this will allow periods in ident replies.  Use of this is
 * strongly discouraged on public networks
 */
#define ALLOW_DOT_IN_IDENT

/* MAX_MULTI_MESSAGES
 * Maximum number of recipients to a PRIVMSG.  Any more than MAX_MULTI_MESSAGES
 * will not be sent.  If MAX_MULTI_MESSAGES is 1, then any PRIVMSG with a ',' in
 * the target will be rejected
 */
#define MAX_MULTI_MESSAGES 1

/* NO_DUPE_MULTI_MESSAGES
 * Define this to check for duplicate recipients in PRIVMSG, at the expense
 * of noticeable CPU cycles.
 */
#define NO_DUPE_MULTI_MESSAGES

/* WARN_NO_NLINE
 * Define this if you want ops to get noticed about "things" trying to
 * connect as servers that don't have N: lines.  Twits with misconfigured
 * servers can get really annoying with this enabled.
 */
#define WARN_NO_NLINE

/* FAILED_OPER_NOTICE - send a notice to all opers when someone
 * tries to /oper and uses an incorrect password.
 */
#define FAILED_OPER_NOTICE

/* SHOW_FAILED_OPER_ID - if FAILED_OPER_NOTICE is defined, also notify when
 * a client fails to oper because of a identity mismatch (wrong host or nick)
 */
#define SHOW_FAILED_OPER_ID

/* SHOW_FAILED_OPER_PASSWD - if FAILED_OPER_NOTICE is defined, also show the
 * attempted passwd
 */
#undef SHOW_FAILED_OPER_PASSWD

/* CLIENT_SERVER - Don't be so fascist about idle clients ;)
 * changes behaviour of HTM code to make clients lag less.
 */
#define CLIENT_SERVER

/* REALBAN - if REALBAN is defined real hostname will be matched on bans
 */
#undef REALBAN

/* BAN_INFO - Shows you who and when someone did a ban
 */
#define BAN_INFO

/* TOPIC_INFO - Shows you who and when someone set the topic
 */
#define TOPIC_INFO

/* ANTI_NICK_FLOOD - prevents nick flooding
 * define if you want to block local clients from nickflooding
 */
#define ANTI_NICK_FLOOD

/* defaults allow 5 nick changes in 20 seconds */
#define MAX_NICK_TIME 20
#define MAX_NICK_CHANGES 5 

/*
 * Target flood time.
 * Minimum time between target changes.
 * (MAXTARGETS are allowed simultaneously however).
 * Its set to a power of 2 because we divide through it quite a lot.
 */
#define TARGET_DELAY 	128

#define MAXTARGETS      10

/* KLINE_WITH_REASON - show comment to client on exit
 * define this if you want users to exit with the kline/dline reason
 * (i.e. instead of "You have been K-lined" they will see the reason
 * and to see the kline/dline reason when they try to connect
 * It's a neat feature except for one thing... If you use a tcm
 * and it shows the nick of the oper doing the kline (as it does by default)
 * Your opers can be hit with retalitation... Or if your opers use
 * scripts that stick an ID into the comment field. etc. It's up to you
 * whether you want to use it or not.
 *
 * I have rewritten a portion of the k-line processing making it faster
 * unfortuantely, using KLINE_WITH_REASON would slow down this
 * processing slightly.. how much I can't say. For very few clients
 * being KLINED very little difference, but you have been forewarned
 *
 */
#define KLINE_WITH_REASON

/*
 * If KLINE_WITH_CONNECTION_CLOSED is defined and KLINE_WITH_REASON
 * above is undefined then the signoff reason will be "Connection
 * closed". This prevents other users seeing the client disconnect
 * from harassing the IRCops.
 * However, the client will still see the real reason upon connect attempts.
 */
#define KLINE_WITH_CONNECTION_CLOSED

/* STATS_NOTICE - See a notice when a user does a /stats
 *
 * This is left on by default.
 * Members of the development team were split on supporting the
 * default here.
 */
#define STATS_NOTICE

/* WHOIS_NOTICE - Shows a notice to an oper when a user does a
 * /whois on them
 * Why do opers need this at all? Its an invasion of privacy. bah.
 * you don't need this. -Dianora
 */
#define WHOIS_NOTICE

/* WHOIS_WAIT - minimum seconds between remote use of WHOIS before
 * max use count is reset
 */
#define WHOIS_WAIT 1

/* PACE_WAIT - minimum seconds between use of MOTD, INFO, HELP, LINKS, TRACE
 * -Dianora
 */
#define PACE_WAIT 0

/* KNOCK_DELAY 5 minutes per each KNOCK should be enough
 */
#define KNOCK_DELAY 300

/* If you are an admin that does not think operwall/wallops
 * should be used instead of a channel, define this.
 */
#define PACE_WALLOPS
#define WALLOPS_WAIT 10 

/* SHORT_MOTD
 * There are client ignoring the FORCE_MOTD MOTD numeric, there is
 * no point forcing MOTD on connecting clients IMO. Give them a short
 * NOTICE telling them they should read the motd, and leave it at that.
 */
#undef SHORT_MOTD

/* NO_OPER_FLOOD - disable flood control for opers
 * define this to remove flood control for opers
 */
#define NO_OPER_FLOOD

/* SHOW_INVISIBLE_LUSERS - show invisible clients in LUSERS
 * As defined this will show the correct invisible count for anyone who does
 * LUSERS on your server. On a large net this doesnt mean much, but on a
 * small net it might be an advantage to undefine it.
 */
#define SHOW_INVISIBLE_LUSERS

#if HAVE_LIBZ
/* ZIP_LINKS - Compress server-to-server links
 * Use c: lines in the conf to disable zipped connection.
 *
 * Note that you may have to increase your sendQ size between server
 * if you have problems during particularly heavy bursts
 */
#define ZIP_LINKS
#endif

/* NO_DEFAULT_INVISIBLE - clients not +i by default
 * When defined, your users will not automatically be attributed with user
 * mode "i" (i == invisible). Invisibility means people dont showup in
 * WHO or NAMES unless they are on the same channel as you.
 */
#undef  NO_DEFAULT_INVISIBLE

/*
 * The compression level used for zipped links. (Suggested values: 1 to 5)
 * Above 4 will only give a rather marginal increase in compression for a
 * large increase in CPU usage.
 */
#define ZIP_LEVEL       2

/*
 * OPER_UMODES LOCOP_UMODES - set these to be the initial umodes when OPER'ing
 * These can be over-ridden in ircd.conf file, with flags in the umodes field
 */
#define OPER_UMODES   (UMODE_OPER|UMODE_WALLOP)
#define LOCOP_UMODES   (UMODE_LOCOP|UMODE_WALLOP)

/*
 * OPER_DEFAULT_IMODES -  set these to be the initial imodes when OPER'ing
 * These can be over-ridden in ircd.conf file, with flags in last O field
 */
#define OPER_DEFAULT_IMODES	(IMODE_KILLS | IMODE_REJECTS | IMODE_GENERIC |\
	IMODE_EXTERNAL | IMODE_FULL)

/* MAXIMUM LINKS - max links for class 0 if no Y: line configured
 *
 * This define is useful for leaf nodes and gateways. It keeps you from
 * connecting to too many places. It works by keeping you from
 * connecting to more than "n" nodes which you have C:blah::blah:6667
 * lines for.
 *
 * Note that any number of nodes can still connect to you. This only
 * limits the number that you actively reach out to connect to.
 *
 * Leaf nodes are nodes which are on the edge of the tree. If you want
 * to have a backup link, then sometimes you end up connected to both
 * your primary and backup, routing traffic between them. To prevent
 * this, #define MAXIMUM_LINKS 1 and set up both primary and
 * secondary with C:blah::blah:6667 lines. THEY SHOULD NOT TRY TO
 * CONNECT TO YOU, YOU SHOULD CONNECT TO THEM.
 *
 * Gateways such as the server which connects Australia to the US can
 * do a similar thing. Put the American nodes you want to connect to
 * in with C:blah::blah:6667 lines, and the Australian nodes with
 * C:blah::blah lines. Have the Americans put you in with C:blah::blah
 * lines. Then you will only connect to one of the Americans.
 *
 * This value is only used if you don't have server classes defined, and
 * a server is in class 0 (the default class if none is set).
 *
 */
#define MAXIMUM_LINKS 1

/* INIT_LOG_LEVEL - what level of information is logged to ircd.log
 * options are:
 *   L_CRIT, L_ERROR, L_WARN, L_NOTICE, L_TRACE, L_INFO, L_DEBUG
 */
#define INIT_LOG_LEVEL L_TRACE

/* USE_LOGFILE - log errors and such to VARPATH
 * If you wish to have the server send 'vital' messages about server
 * to a logfile, define USE_LOGFILE.
 */
#define USE_LOGFILE

/* USE_SYSLOG - log errors and such to syslog()
 * If you wish to have the server send 'vital' messages about server
 * through syslog, define USE_SYSLOG. Only system errors and events critical
 * to the server are logged although if this is defined with FNAME_USERLOG,
 * syslog() is used instead of the above file. It is not recommended that
 * this option is used unless you tell the system administrator beforehand
 * and obtain their permission to send messages to the system log files.
 */
#undef USE_SYSLOG

#ifdef  USE_SYSLOG
/* SYSLOG_KILL SYSLOG_SQUIT SYSLOG_CONNECT SYSLOG_USERS SYSLOG_OPER
 * If you use syslog above, you may want to turn some (none) of the
 * spurious log messages for KILL,SQUIT,etc off.
 */
#undef  SYSLOG_KILL     /* log all operator kills to syslog */
#undef  SYSLOG_SQUIT    /* log all remote squits for all servers to syslog */
#undef  SYSLOG_CONNECT  /* log remote connect messages for other all servs */
#undef  SYSLOG_USERS    /* send userlog stuff to syslog */
#undef  SYSLOG_OPER     /* log all users who successfully become an Op */

/* LOG_FACILITY - facility to use for syslog()
 * Define the facility you want to use for syslog().  Ask your
 * sysadmin which one you should use.
 */
#define LOG_FACILITY LOG_LOCAL4

#endif /* USE_SYSLOG */

/* CRYPT_OPER_PASSWORD - use crypted oper passwords in the ircd.conf
 * define this if you want to use crypted passwords for operators in your
 * ircd.conf file.
 */
#define CRYPT_OPER_PASSWORD

/* CRYPT_LINK_PASSWORD - use crypted N-line passwords in the ircd.conf
 * If you want to store encrypted passwords in N-lines for server links,
 * define this.  For a C/N pair in your ircd.conf file, the password
 * need not be the same for both, as long as the opposite end has the
 * right password in the opposite line.
 */
#undef  CRYPT_LINK_PASSWORD

/* IDLE_FROM_MSG - Idle-time reset only from privmsg
 * Idle-time reset only from privmsg, if undefined idle-time
 * is reset from everything except ping/pong.
 * 
 */
#define IDLE_FROM_MSG

/* MAXSENDQLENGTH - Max amount of internal send buffering
 * Max amount of internal send buffering when socket is stuck (bytes)
 */
#define MAXSENDQLENGTH 9000000    /* Recommended value: 7050000 for efnet */

/*  BUFFERPOOL - the maximum size of the total of all sendq's.
 *  Recommended value is 4 times MAXSENDQLENGTH.
 */
#define BUFFERPOOL (MAXSENDQLENGTH * 4)

/* CLIENT_FLOOD - client excess flood threshold
 * this controls the number of bytes the server will allow a client to
 * send to the server without processing before disconnecting the client for
 * flooding it.  Values greater than 8000 make no difference to the server.
 */
#define CLIENT_FLOOD    2560

/* NOISY_HTM - should HTM be noisy by default
 * should be YES or NO
 */
#define NOISY_HTM YES

/*
 * LITTLE_I_LINE support
 * clients with a little i instead of an I in their I line
 * can be chanopped, but cannot chanop anyone else.
 */
#define LITTLE_I_LINES

/*
 * define either NO_CHANOPS_ON_SPLIT or NO_JOIN_ON_SPLIT 
 *
 * choose =one= only
 */


/* 
 * NO_CHANOPS_ON_SPLIT
 * 
 * When this is defined, users will not be allowed to join channels
 * while the server is split.
 */
#undef NO_CHANOPS_ON_SPLIT

/*
 * NO_JOIN_ON_SPLIT
 *
 * When this is defined, users will not be allowed to join channels
 * while the server is split.
 */
#undef PRESERVE_CHANNEL_ON_SPLIT
#undef  NO_JOIN_ON_SPLIT

/*
 * SPLIT_SMALLNET_SIZE defines what constitutes a split from 
 * the net. for a leaf, 2 is fine. If the number of servers seen
 * on the net gets less than 2, a split is deemed to have happened.
 */
#define SPLIT_SMALLNET_SIZE 2

/*
 * SPLIT_SMALLNET_USER_SIZE defines how many global users on the
 * net constitute a "normal" net size. It's used in conjunction
 * with SPLIT_SMALLNET_SIZE to help determine the end of a split.
 * if number of server seen on net > SPLIT_SMALLNET_SIZE &&
 * number of users seen on net > SPLIT_SMALLNET_USER_SIZE start
 * counting down the SERVER_SPLIT_RECOVERY_TIME
 */
#define SPLIT_SMALLNET_USER_SIZE 10000

/*
 * SPLIT_PONG will send a PING to a server after the connect burst.
 * It will stay in "split" mode until it receives a PONG in addition
 * to meeting the other conditions.  This is very useful for true
 * leafs, less useful for "clustered" servers.  If this is enabled,
 * you should be able to crank DEFAULT_SERVER_SPLIT_RECOVERY_TIME
 * down to 1.
 */
#define SPLIT_PONG

/*
 * DEFAULT_SERVER_SPLIT_RECOVERY_TIME - determines how long to delay split
 * status after resyncing
 */
#define DEFAULT_SERVER_SPLIT_RECOVERY_TIME 1

/* LIMIT_UH
 * If this is defined, Y line limit is made against the actual
 * username not the ip. i.e. if you limit the connect frequency line
 * to 1, that allows only 1 username to connect instead of 1 client per ip
 * i.e. you can have 10 clients all with different usernames, but each user
 * can only connect once. Each non-idented client counts as the same user
 * i.e. ~a and ~b result in a count of two.
 *
 */
#undef LIMIT_UH

/* IDLE_CHECK
 * If this is defined, each client is checked for excessive idleness
 * This adds some CPU... you might not want to use this on a large server.
 * However, if defined, and a client is discovered idling more than
 * IDLE_TIME minutes, it is t-klined for 1 minute to discourage
 * reconnects. The idle time is settable via /quote set
 * /quote set idletime
 *
 * -Dianora
 *
 * IDLE_IGNORE will prevent the server from idle'ing clients connected from
 * the listed IP#s.  This should probably be moved into a .conf file entry
 * at some point in the future.
 * It has been. add '<' to I line prefix
 *
 * OPER_IDLE allows operators to remain idle when they idle
 * beyond the idle limit
 */
#undef  IDLE_CHECK
#define IDLE_TIME 60
#define OPER_IDLE

/* SEND_FAKE_KILL_TO_CLIENT - make the client think it's being /KILL'ed
 * 
 * This was originally intended to prevent clients from reconnecting to the
 * server after being dropped for idleness.  It can probably be used for
 * other events too.
 *
 * This really only works if the
 * client was compiled with QUIT_ON_OPERATOR_KILL which was mandatory policy
 * on UMich.Edu hosts.
 */
#define SEND_FAKE_KILL_TO_CLIENT
 
/*
 * Limited Trace - Reports only link and oper traces even when O:line is
 * active.
 *
 * Displays only Oper, Serv, Link, and Class reports even if the O-line is
 * active.  Useful for just showing pertinent info of a specific server. 
 * Note however that if the target server is not running this option then
 * you will still receive a normal trace output. 
 */
#undef LTRACE

/*
 * LWALLOPS - Like wallops but only local.
 *
 * This is actually a compatibility command which really calls m_locops().
 */
#define LWALLOPS

/*
 * Define this to reduce dns reverse lookup timeout,
 * less chances the host gets resolved, but connections for unknown hosts will
 * be much faster - Lamego
 */
#define FASTDNS

/*
 * Define this to support webtv users by providing an irc pseudo-client
 * which will parse received commans via msg.
 * example: /MSG irc KILL nick :reason
 */
#undef WEBTV_SUPPORT

/*
 * comstud and I have both noted that solaris 2.5 at least, takes a hissy
 * fit if you don't read a fd that becomes ready right away. Unfortunately
 * the dog3 priority code relies upon not having to read a ready fd right away.
 * If you have HTM mode set low as it is normally, the server will
 * eventually grind to a halt.
 * Don't complain if Solaris lags if you don't define this. I warned you.
 *
 * -Dianora
 */

#undef NO_PRIORITY

/*   STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP  */

/* You shouldn't change anything below this line, unless absolutely needed. */

/* INITIAL_DBUFS - how many dbufs to preallocate
 */
#define INITIAL_DBUFS 1000 /* preallocate 4 megs of dbufs */ 

/* MAXBUFFERS - increase socket buffers
 *
 * Increase send & receive socket buffer up to 64k,
 * keeps clients at 8K and only raises servers to 64K
 */
#define MAXBUFFERS

 /* PORTNUM - default port that ircd uses to connect to remote servers, if
  * a port is not specified in the M: line.
  */
#define PORTNUM 6667


/* HLC - Hub Lag Check --fabulous (needs to be fixed --Lamego)
 * when defined will disable client connections by setting MAXCLIENTS to 1
 * when the lat to the hub becomes greater or equal to LAGLIMIT
 */
#undef HLC

/* HLC settings */
#define HCF 60          /* hub check frequency in seconds */
#define LAGLIMIT HCF*3  /* (3m) lag limit (if more or equal, lagmode=on) */
#define LAGMIN 15       /* if server is in lagmode but actual lag becomes
                           less than LAGMIN, return to normal mode */


/* define this tu enable String Cache System */
//#define SCS

/* MAXCONNECTIONS - don't touch - change the HARD_FDLIMIT_ instead
 * Maximum number of network connections your server will allow.  This should
 * never exceed max. number of open file descriptors and wont increase this.
 */
/* change the HARD_FDLIMIT_ instead */
#define MAXCONNECTIONS  HARD_FDLIMIT

/* TIMESEC - Time interval to wait and if no messages have been received,
 * then check for PINGFREQUENCY and CONNECTFREQUENCY 
 */
#define TIMESEC  5              /* Recommended value: 5 */

/* PINGFREQUENCY - ping frequency for idle connections
 * If daemon doesn't receive anything from any of its links within
 * PINGFREQUENCY seconds, then the server will attempt to check for
 * an active link with a PING message. If no reply is received within
 * (PINGFREQUENCY * 2) seconds, then the connection will be closed.
 *
 * This only specifies the default for class 0, only used for lines
 * not specifying a class, or specifying an invalid class 
 */
#define PINGFREQUENCY    120    /* Recommended value: 120 */

/* CONNECTFREQUENCY - time to wait before auto-reconencting
 * If the connection to to uphost is down, then attempt to reconnect every 
 * CONNECTFREQUENCY  seconds.
 *
 * This only specifies the default for class 0 
 */
#define CONNECTFREQUENCY 600    /* Recommended value: 600 */

/* HANGONGOODLINK and HANGONGOODLINK
 * Often the net breaks for a short time and it's useful to try to
 * establishing the same connection again faster than CONNECTFREQUENCY
 * would allow. But, to keep trying on bad connection, we require
 * that connection has been open for certain minimum time
 * (HANGONGOODLINK) and we give the net few seconds to steady
 * (HANGONRETRYDELAY). This latter has to be long enough that the
 * other end of the connection has time to notice it broke too.
 * 1997/09/18 recommended values by ThemBones for modern Efnet
 */

#define HANGONGOODLINK 3600     /* Recommended value: 30-60 minutes */
#define HANGONRETRYDELAY 60     /* Recommended value: 30-60 seconds */

/* WRITEWAITDELAY - Number of seconds to wait for write to
 * complete if stuck.
 */
#define WRITEWAITDELAY     15   /* Recommended value: 15 */

/* CONNECTTIMEOUT -
 * Number of seconds to wait for a connect(2) call to complete.
 * NOTE: this must be at *LEAST* 10.  When a client connects, it has
 * CONNECTTIMEOUT - 10 seconds for its host to respond to an ident lookup
 * query and for a DNS answer to be retrieved.
 */
#define CONNECTTIMEOUT  30      /* Recommended value: 30 */

/* KILLCHASETIMELIMIT -
 * Max time from the nickname change that still causes KILL
 * automatically to switch for the current nick of that user. (seconds)
 */
#define KILLCHASETIMELIMIT 90   /* Recommended value: 90 */

/* SENDQ_ALWAYS - should always be defined.
 * SendQ-Always causes the server to put all outbound data into the sendq and
 * flushing the sendq at the end of input processing. This should cause more
 * efficient write's to be made to the network.
 * There *shouldn't* be any problems with this method.
 * -avalon
 */
#define SENDQ_ALWAYS

/* FLUD - CTCP Flood Detection and Protection
 * 
 * This enables server CTCP flood detection and protection for local clients.
 * It works well against fludnets and flood clones.  The effect of this code
 * on server CPU and memory usage is minimal, however you may not wish to
 * take the risk, or be fundamentally opposed to checking the contents of
 * PRIVMSG's (though no privacy is breached).  This code is not useful for
 * routing only servers (ie, HUB's with little or no local client base), and
 * the hybrid team strongly recommends that you do not use FLUD with HUB.
 * The following default thresholds may be tweaked, but these seem to work
 * well.
 */
#define FLUD

/* ANTI_DRONE_FLOOD - anti flooding code for drones
 * This code adds server side ignore for a client who gets
 * messaged more than drone_count times within drone_time seconds
 * unfortunately, its a great DOS, but at least the client won't flood off.
 * I have no idea what to use for values here, trying 8 privmsgs
 * within 1 seconds. (I'm told it is usually that fast)
 * I'll do better next time, this is a Q&D -Dianora
 */
#define ANTI_DRONE_FLOOD
#define DEFAULT_DRONE_TIME 1
#define DEFAULT_DRONE_COUNT 8

/* 
 * ANTI_SPAMBOT
 * if ANTI_SPAMBOT is defined try to discourage spambots
 * The defaults =should= be fine for the timers/counters etc.
 * but you can play with them. -Dianora
 *
 * Defining this also does a quick check whether the client sends
 * us a "user foo x x :foo" where x is just a single char.  More
 * often than not, it's a bot if it did. -ThemBones
 */
#define ANTI_SPAMBOT

/* ANTI_SPAMBOT parameters, don't touch these if you don't
 * understand what is going on.
 *
 * if a client joins MAX_JOIN_LEAVE_COUNT channels in a row,
 * but spends less than MIN_JOIN_LEAVE_TIME seconds
 * on each one, flag it as a possible spambot.
 * disable JOIN for it and PRIVMSG but give no indication to the client
 * that this is happening.
 * every time it tries to JOIN OPER_SPAM_COUNTDOWN times, flag
 * all opers on local server.
 * If a client doesn't LEAVE a channel for at least 2 minutes
 * the join/leave counter is decremented each time a LEAVE is done
 *
 */
#define MIN_JOIN_LEAVE_TIME  60
#define MAX_JOIN_LEAVE_COUNT  5
#define OPER_SPAM_COUNTDOWN   5 
#define JOIN_LEAVE_COUNT_EXPIRE_TIME 120

/*
 * If ANTI_SPAMBOT_WARN_ONLY is #define'd 
 * Warn opers about possible spambots only, do not disable
 * JOIN and PRIVMSG if possible spambot is noticed
 * Depends on your policies.
 */
#undef ANTI_SPAMBOT_WARN_ONLY

#ifdef FLUD
#define FLUD_NUM        10      /* Number of flud messages to trip alarm */
#define FLUD_TIME       10      /* Seconds in which FLUD_NUM msgs must occur */
#define FLUD_BLOCK      20      /* Seconds to block fluds */
#endif

/* REJECT_HOLD 
 * clients that reconnect but are k-lined will have their connections
 * "held" for REJECT_HOLD_TIME seconds, they cannot PRIVMSG. The idea
 * is to keep a reconnecting client from forcing the ircd to re-scan
 * mtrie_conf.
 *
 */
  
#undef REJECT_HOLD
#define REJECT_HOLD_TIME 30 

/* maximum number of fd's that will be used for reject holding */
#define REJECT_HELD_MAX 25



/*
 * OLD_Y_LIMIT
 *
 * #define this if you prefer the old behaviour of I lines
 * the default behaviour is to limit the total number of clients
 * using the max client limit in the corresponding Y line (class)
 * The old behaviour was to limit the client count per I line
 * without regard to the total class limit. Each have advantages
 * and disadvantages. In an open I line server, the default behaviour
 * i.e. #undef OLD_Y_LIMIT makes more sense, because you can limit
 * the total number of clients in a class. In a closed I line server
 * The old behaviour can make more sense.
 *
 * -Dianora
*/
#undef OLD_Y_LIMIT

/*
 * If the OS has SOMAXCONN use that value, otherwise
 * Use the value in HYBRID_SOMAXCONN for the listen(); backlog
 * try 5 or 25. 5 for AIX and SUNOS, 25 should work better for other OS's
*/
#define HYBRID_SOMAXCONN 25


/* DEBUGMODE is used mostly for internal development, it is likely
 * to make your client server very sluggish.
 * You usually shouldn't need this. -Dianora
*/
#undef DEBUGMODE               /* define DEBUGMODE to enable debugging mode.*/


/* ----------------- archaic and/or broken section -------------------- */
#undef DNS_DEBUG

/* SETUID_ROOT - plock - keep the ircd from being swapped out.
 * BSD swapping criteria do not match the requirements of ircd.
 * Note that the server needs to be setuid root for this to work.
 * The result of this is that the text segment of the ircd will be
 * locked in core; thus swapper cannot touch it and the behavior
 * noted above will not occur.  This probably doesn't work right
 * anymore.  IRCD_UID MUST be defined correctly if SETUID_ROOT.
 */
#undef SETUID_ROOT
 
/* SUN_GSO_BUG support removed
 *
 * if you still have a machine with this bug, it doesn't belong on EFnet
 */

/* ------------------------- END CONFIGURATION SECTION -------------------- */
#define MAX_CLIENTS INIT_MAXCLIENTS

#if defined(CLIENT_FLOOD) && ((CLIENT_FLOOD > 8000) || (CLIENT_FLOOD < 512))
#error CLIENT_FLOOD needs redefining.
#endif

#if !defined(CLIENT_FLOOD)
#error CLIENT_FLOOD undefined.
#endif

#if defined(DEBUGMODE)
#  define Debug(x) debug x
#  define LOGFILE "ircd.log"
#else
#  define Debug(x) ;
#  define LOGFILE "/dev/null"
#endif

#define REPORT_DLINE_TO_USER

#ifdef NO_JOIN_ON_SPLIT
#define NO_CHANOPS_ON_SPLIT
#endif

#if defined(NO_CHANOPS_ON_SPLIT) || defined(NO_JOIN_ON_SPLIT)
#define NEED_SPLITCODE
#endif

#ifdef ANTI_SPAMBOT
#define MIN_SPAM_NUM 5
#define MIN_SPAM_TIME 60
#endif

#ifdef IDLE_CHECK
#define MIN_IDLETIME 1800
#endif

#define CONFIG_H_LEVEL_6_1

#endif /* INCLUDED_config_h */


