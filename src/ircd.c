/************************************************************************
 *   IRC - Internet Relay Chat, src/ircd.c
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
 * $Id: ircd.c,v 1.9 2005/12/11 20:42:17 jpinto Exp $
 */
#include "ircd.h"
#include "channel.h"
#include "class.h"
#include "client.h"
#include "common.h"
#ifdef HAVE_SSL
#include "ssl.h"
#endif
#include "dline_conf.h"
#include "fdlist.h"
#include "hash.h"
#include "help.h"
#include "irc_string.h"
#include "ircd_signal.h"
#include "list.h"
#include "m_gline.h"
#include "motd.h"
#include "patchlevel.h"     /* PATCHLEVEL */
#include "msg.h"         /* msgtab */
#include "mtrie_conf.h"
#include "numeric.h"
#include "parse.h"
#include "res.h"
#include "restart.h"
#include "s_auth.h"
#include "s_bsd.h"
#include "s_conf.h"
#include "s_debug.h"
#include "s_log.h"
#include "s_misc.h"
#include "s_serv.h"      /* try_connections */
#include "s_stats.h"
#include "s_zip.h"
#include "scache.h"
#include "send.h"
#include "struct.h"
#include "whowas.h"
#include "path.h"
#include "help.h"
#include "dconf_vars.h"
#include "dconf.h"
#include "unicode.h"
#include "hvc.h"
#include "setup.h" /* We need this for PIDFILE */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/param.h>
#include <signal.h>


#if defined(HAVE_GETOPT_H)
#include <getopt.h>
#endif /* HAVE_GETOPT_H */

#ifdef SETUID_ROOT
#include <sys/lock.h>
#include <unistd.h>
#endif /* SETUID_ROOT */

/*
 * for getopt
 * ZZZ this is going to need confirmation on other OS's
 *
 * #include <getopt.h>
 * Solaris has getopt.h, you should too... hopefully
 * BSD declares them in stdlib.h
 * extern char *optarg;
 * 
 * for FreeBSD the following are defined:
 *
 * extern char *optarg;
 * extern int optind;
 * extern in optopt;
 * extern int opterr;
 * extern in optreset;
 */

/*
 * Try and find the correct name to use with getrlimit() for setting the max.
 * number of files allowed to be open by this process.
 */
#ifdef RLIMIT_FDMAX
# define RLIMIT_FD_MAX   RLIMIT_FDMAX
#else
# ifdef RLIMIT_NOFILE
#  define RLIMIT_FD_MAX RLIMIT_NOFILE
# else
#  ifdef RLIMIT_OPEN_MAX
#   define RLIMIT_FD_MAX RLIMIT_OPEN_MAX
#  else
#   undef RLIMIT_FD_MAX
#  endif
# endif
#endif


#ifdef  REJECT_HOLD
int reject_held_fds = 0;
#endif

#ifdef NEED_SPLITCODE
extern time_t server_split_time;
extern int    server_was_split;
#endif

int cold_start = YES;   /* set if the server has just fired up */

/* /quote set variables */
struct SetOptions GlobalSetOptions;

/* config.h config file paths etc */
ConfigFileEntryType ConfigFileEntry; 

struct  Counter Count;

time_t  CurrentTime;            /* GLOBAL - current system timestamp */
int     ircntp_offset;          /* GLOBAL - irc ntp offset */
int     ServerRunning;          /* GLOBAL - server execution state */
struct Client me;                     /* That's me */
int         no_ssl = 0;
int 	secure_mode = 0;
int	services_on = 0;

struct Client* GlobalClientList = 0; /* Pointer to beginning of Client list */
/* client pointer lists -Dianora */ 
struct Client *local_cptr_list = NULL;
struct Client *oper_cptr_list  = NULL;
struct Client *serv_cptr_list  = NULL;

static size_t      initialVMTop = 0;   /* top of virtual memory at init */
static int         bootDaemon  = 1;

char**  myargv;
int     dorehash    = 0;
int		silent_start= 0;
int     dconf_check_opt = 0;
int	debug_opt = 0;
int     debuglevel	= -1;        /* Server debug level */
char*   debugmode	= "";        /*  -"-    -"-   -"-  */

char	ircdpath[255];		/* Absolute ircd path */

int     rehashed = YES;
int     dline_in_progress = NO; /* killing off matching D lines ? */
time_t  nextconnect = 1;        /* time for next try_connections call */
time_t  nextping = 1;           /* same as above for check_pings() */

char*   ajoin_chans[5];		/* list of channels to join on connect */
int 	hvc_enabled = 0;

#ifdef HLC   
/* those vars below are for HLC (hub-lag-checking) --fabulous */
time_t  nexthubcheck = 1;       /* same as above and above for check_hubping() */
int     sentping = 0;           /* have I pinged my uplink? */
time_t  receivedat = 0;         /* last pong received from hub */
int     hlcison = 0;
#endif

/* code added by mika nystrom (mnystrom@mit.edu) */
/* this flag is used to signal globally that the server is heavily loaded,
   something which can be taken into account when processing e.g. user commands
   and scheduling ping checks */
/* Changed by Taner Halicioglu (taner@CERF.NET) */

#define LOADCFREQ 5     /* every 5s */
#define LOADRECV 40     /* 40k/s */

int    LRV = LOADRECV;
time_t LCF = LOADCFREQ;
float currlife = 0.0;
float cursend = 0.0;

/*
 * get_vm_top - get the operating systems notion of the resident set size
 */
static size_t get_vm_top(void)
{
  /*
   * NOTE: sbrk is not part of the ANSI C library or the POSIX.1 standard
   * however it seems that everyone defines it. Calling sbrk with a 0
   * argument will return a pointer to the top of the process virtual
   * memory without changing the process size, so this call should be
   * reasonably safe (sbrk returns the new value for the top of memory).
   * This code relies on the notion that the address returned will be an 
   * offset from 0 (NULL), so the result of sbrk is cast to a size_t and 
   * returned. We really shouldn't be using it here but...
   */
  void* vptr = sbrk(0);
  return (size_t) vptr;
}

/*
 * get_maxrss - get the operating systems notion of the resident set size
 */
size_t get_maxrss(void)
{
  return get_vm_top() - initialVMTop;
}

/*
 * init_sys
 */
static void init_sys(int boot_daemon)
{
#ifdef RLIMIT_FD_MAX
  struct rlimit limit;

  if (!getrlimit(RLIMIT_FD_MAX, &limit))
    {

      if (limit.rlim_max < MAXCONNECTIONS)
        {
          fprintf(stderr,"ircd fd table too big\n");
          fprintf(stderr,"Hard Limit: %ld IRC max: %d\n",
                        (long) limit.rlim_max, MAXCONNECTIONS);
          fprintf(stderr,"Fix MAXCONNECTIONS\n");
          exit(-1);
        }

      limit.rlim_cur = limit.rlim_max; /* make soft limit the max */
      if (setrlimit(RLIMIT_FD_MAX, &limit) == -1)
        {
          fprintf(stderr,"error setting max fd's to %ld\n",
                        (long) limit.rlim_cur);
          exit(-1);
        }

#ifndef USE_POLL
      if( MAXCONNECTIONS > FD_SETSIZE )
        {
          fprintf(stderr, "FD_SETSIZE = %d MAXCONNECTIONS = %d\n",
                  FD_SETSIZE, MAXCONNECTIONS);
          fprintf(stderr,
            "Make sure your kernel supports a larger FD_SETSIZE then " \
            "recompile with -DFD_SETSIZE=%d\n", MAXCONNECTIONS);
          exit(-1);
        }
#endif /* USE_POLL */
    }	
#endif        /* RLIMIT_FD_MAX */

  /* This is needed to not fork if -s is on */
  if (boot_daemon)
    {
      int pid;
      if((pid = fork()) < 0)
        {
          fprintf(stderr, "Couldn't fork: %s\n", strerror(errno));
          exit(0);
        }
      else if (pid > 0)
        exit(0);
#ifdef TIOCNOTTY
      { /* scope */
        int fd;
        if ((fd = open("/dev/tty", O_RDWR)) >= 0)
          {
            ioctl(fd, TIOCNOTTY, NULL);
            close(fd);
          }
      }
#endif 
     setsid();
    }
  close_all_connections();
}

/*
 * bad_command
 *      This is called when the commandline is not acceptable.
 *      Give error message and exit without starting anything.
 */
static void bad_command()
{
  fprintf(stderr, 
          "Usage: ircd [-c] [-d] [-e etcpath] [-h servername] "
          "[-l varpath] [-n] [-s] [-v] [-x debuglevel] | [--help]\n");
  fprintf(stderr, "Server not started\n");
  exit(-2);
}


char* cmd_line_options[]={
 PATCHLEVEL,
 " All command line parameters have the syntax \"-f string\" or \"-fstring\"",
 " OPTIONS:",
 " -c          - dconf check",
 " -d          - debug (logs all in/out data)",
 " -h hostname - specify server name",
 " -l path     - specify var parth",
 " -n          - do not fork, run in foreground",
 " -s          - silent start - no start messages",
 " -v          - print version and exit",
#ifdef  DEBUGMODE
 " -x          - set debug level",
#endif
  NULL
};
  
static void parse_command_line(int argc, char* argv[])
{
  const char* options = "e:h:l:nvx:csd"; 
  int         opt;
  char**      line = cmd_line_options;

  if((argc>1) && !irccmp(argv[1],"--help"))
    {
      while (*line)
        printf("%s\n", *line++);
      exit(0);
    }    
    
  while ((opt = getopt(argc, argv, options)) != EOF) {
    switch (opt) {
    case 'd':
      debug_opt = -1;
      break;
    case 'c':
      dconf_check_opt = -1;
      break;
    case 'e': 
      if (optarg)
        ConfigFileEntry.etcpath = optarg;
      break;
    case 'h':
      if (optarg)
        strncpy_irc(me.name, optarg, HOSTLEN);
      break;
    case 'l':
      if (optarg)
        ConfigFileEntry.varpath = optarg;
      break;
    case 'n':
      bootDaemon = 0; 
      break;
    case 'v':
      printf("ircd %s\n\tzlib %s\n\tircd_dir: %s\n", ircdversion,
#ifndef ZIP_LINKS
             "not used",
#else
              zlib_version,
#endif
              ConfigFileEntry.etcpath);
      exit(0);
      break;   /* NOT REACHED */
    case 'x':
#ifdef  DEBUGMODE
      if (optarg) {
        debuglevel = atoi(optarg);
        debugmode = optarg;
      }
#endif
      break;
	case 's':
	  silent_start = -1;
	  break;
    default:
      bad_command();
      break;
    }
  }
}

static time_t io_loop(time_t delay)
{
  static char   to_send[200];
  static time_t lasttime  = 0;
  static long   lastrecvK = 0, lastsendK = 0;  
  static int    lrv       = 0;
  time_t        lasttimeofday;

  lasttimeofday = CurrentTime;
  if ((CurrentTime = time(NULL)) == -1)
    {
      irclog(L_ERROR, "Clock Failure (%d)", errno);
      sendto_ops("Clock Failure (%d), TS can be corrupted", errno);
      restart("Clock Failure");
    }

  if (CurrentTime < lasttimeofday)
    {
      ircsprintf(to_send, "System clock is running backwards - (%d < %d)",
                 CurrentTime, lasttimeofday);
      report_error(to_send, me.name, 0);
    }

  /*
   * This chunk of code determines whether or not
   * "life sucks", that is to say if the traffic
   * level is so high that standard server
   * commands should be restricted
   *
   * Changed by Taner so that it tells you what's going on
   * as well as allows forced on (long LCF), etc...
   */
  
  if ((CurrentTime - lasttime) >= LCF)
    {
      lrv = LRV * LCF;
      lasttime = CurrentTime;
	  cursend = (float)((long)me.sendK - lastsendK)/(float)LCF;
      currlife = (float)((long)me.receiveK - lastrecvK)/(float)LCF;
      if (((long)me.receiveK - lrv) > lastrecvK )
        {
          if (!LIFESUX)
            {
              LIFESUX = 1;

              if (NOISYHTM)
                {
                  sprintf(to_send, 
                        "Entering high-traffic mode - (%.1fk/s > %dk/s)",
                                (float)currlife, LRV);
                  sendto_ops("%s", to_send);
                }
            }
          else
            {
              LIFESUX++;                /* Ok, life really sucks! */
              LCF += 2;                 /* Wait even longer */
              if (NOISYHTM) 
                {
                  sprintf(to_send,
                        "Still high-traffic mode %d%s (%d delay): %.1fk/s",
                                LIFESUX,
                                (LIFESUX & 0x04) ?  " (TURBO)" : "",
                                (int)LCF, (float)currlife);
                  sendto_ops("%s", to_send);
                }
            }
        }
      else
        {
          LCF = LOADCFREQ;
          if (LIFESUX)
            {
              LIFESUX = 0;
              if (NOISYHTM)
                sendto_ops("Resuming standard operation . . . .");
            }
        }
      lastrecvK = (long)me.receiveK;
	  lastsendK = (long)me.sendK;
    }

  /*
  ** We only want to connect if a connection is due,
  ** not every time through.  Note, if there are no
  ** active C lines, this call to Tryconnections is
  ** made once only; it will return 0. - avalon
  */
  if (nextconnect && CurrentTime >= nextconnect)
    nextconnect = try_connections(CurrentTime);

#ifndef USE_ADNS
  /*
   * DNS checks, use smaller of resolver delay or next ping
   */
#ifndef NORES
  delay = IRCD_MIN(timeout_resolver(CurrentTime), nextping);
#endif
#endif  
  /*
  ** take the smaller of the two 'timed' event times as
  ** the time of next event (stops us being late :) - avalon
  ** WARNING - nextconnect can return 0!
  */
  if (nextconnect)
    delay = IRCD_MIN(nextping, nextconnect);
  delay -= CurrentTime;
  /*
  ** Adjust delay to something reasonable [ad hoc values]
  ** (one might think something more clever here... --msa)
  ** We don't really need to check that often and as long
  ** as we don't delay too long, everything should be ok.
  ** waiting too long can cause things to timeout...
  ** i.e. PINGS -> a disconnection :(
  ** - avalon
  */
  if (delay < 1)
    delay = 1;
  else
    delay = IRCD_MIN(delay, TIMESEC);
  /*
   * We want to read servers on every io_loop, as well
   * as "busy" clients (which again, includes servers.
   * If "lifesux", then we read servers AGAIN, and then
   * flush any data to servers.
   *    -Taner
   */

#ifndef NO_PRIORITY
  read_message(0, FDL_SERVER);
  read_message(0, FDL_BUSY);
  if (LIFESUX)
    {
      read_message(0, FDL_SERVER);
      if (LIFESUX & 0x4)
        {       /* life really sucks */
          read_message(0, FDL_BUSY);
          read_message(0, FDL_SERVER);
        }
      flush_server_connections();
    }

  /*
   * CLIENT_SERVER = TRUE:
   *    If we're in normal mode, or if "lifesux" and a few
   *    seconds have passed, then read everything.
   * CLIENT_SERVER = FALSE:
   *    If it's been more than lifesux*2 seconds (that is, 
   *    at most 1 second, or at least 2s when lifesux is
   *    != 0) check everything.
   *    -Taner
   */
  {
    static time_t l_lasttime=0;
#ifdef CLIENT_SERVER
    if (!LIFESUX || (l_lasttime + LIFESUX) < CurrentTime)
      {
#else
    if ((l_lasttime + (LIFESUX + 1)) < CurrentTime)
      {
#endif
        read_message(0, FDL_ALL); /*  check everything! */
        l_lasttime = CurrentTime;
      }
   }
#else
  read_message(0, FDL_ALL); /*  check everything! */
  flush_server_connections();
#endif

  /*
  ** ...perhaps should not do these loops every time,
  ** but only if there is some chance of something
  ** happening (but, note that conf->hold times may
  ** be changed elsewhere--so precomputed next event
  ** time might be too far away... (similarly with
  ** ping times) --msa
  */

  if (CurrentTime >= nextping) {
    nextping = check_pings(CurrentTime);
    timeout_auth_queries(CurrentTime);
  }

#ifdef HLC
    /* code for HLC (hub-lag-checking) --fabulous */

  if ((CurrentTime >= nexthubcheck) && serv_cptr_list) 
    {
      /* if we are a stand-alone server, don't count with services */
      if ((serv_cptr_list->next_server_client==NULL) &&
              !IsService(serv_cptr_list))
        {
          time_t lag = (CurrentTime - receivedat);
          if (lag >= HCF)
             lag -= HCF;
          if (!sentping) 
            {
              sendto_one(serv_cptr_list, "PING :%lu", CurrentTime);
              sentping = 1;
            }
          sendto_realops_flags(UMODE_DEBUG, "Current: %lu, Last received at: %lu, Lag: %lu",
                      CurrentTime, receivedat, lag);
          if (lag >= LAGLIMIT) 
            {
              if (MAXCLIENTS != 1) 
                {
                  /* aConfItem *acptr; */
                  sendto_realops("Lag: %d seconds - disabling client connections", lag);
                  sendto_serv_butone(&me, ":%s GLOBOPS :Lag: %d seconds - refusing connections",
                      me.name, lag);
                        LIFESUX = 1;
                        LCF = 1200;
                        MAXCLIENTS = 1;
                        hlcison = 1;
                }
            } 
          else if ((lag <= LAGMIN) && (MAXCLIENTS == 1)) 
            {
                resetHLC();
                sendto_realops("lag: %d seconds - enabling client connections", lag);
                sendto_serv_butone(&me, ":%s GLOBOPS :lag: %d seconds - enabling client connections",
                  me.name, lag);
            }
        } 
      else if (MAXCLIENTS == 1) { resetHLC(); }
      nexthubcheck = CurrentTime + HCF;
    }
    
  /* we should drop HLC when we get stand-alone */
  if(hlcison && (serv_cptr_list==NULL))
    resetHLC();
    
#endif /* HLC */  
  if (dorehash && !LIFESUX)
    {
      rehash(&me, &me, 1);
      dorehash = 0;
    }
  /*
  ** Flush output buffers on all connections now if they
  ** have data in them (or at least try to flush)
  ** -avalon
  */
  flush_connections(0);

#ifndef NO_PRIORITY
  fdlist_check(CurrentTime);
#endif

  return delay;

}
#ifdef HLC
void resetHLC(void) 
{
  MAXCLIENTS = MAX_CLIENTS;
  sentping = 0;
  hlcison = 0;
  if ((LIFESUX == 1) && (LCF == 1200)) 
    {
      LIFESUX = 0;
      LCF = LOADCFREQ;
    }
}
#endif
/*
 * initalialize_global_set_options
 *
 * inputs       - none
 * output       - none
 * side effects - This sets all global set options needed 
 */

static void initialize_global_set_options(void)
{
  memset( &GlobalSetOptions, 0, sizeof(GlobalSetOptions));

  MAXCLIENTS = MAX_CLIENTS;
  NOISYHTM = NOISY_HTM;
  GlobalSetOptions.autoconn = 1;

#ifdef FLUD
  FLUDNUM = FLUD_NUM;
  FLUDTIME = FLUD_TIME;
  FLUDBLOCK = FLUD_BLOCK;
#endif

#ifdef IDLE_CHECK
  IDLETIME = MIN_IDLETIME;
#endif

#ifdef ANTI_SPAMBOT
  SPAMTIME = MIN_JOIN_LEAVE_TIME;
  SPAMNUM = MAX_JOIN_LEAVE_COUNT;
#endif

#ifdef ANTI_DRONE_FLOOD
  DRONETIME = DEFAULT_DRONE_TIME;
  DRONECOUNT = DEFAULT_DRONE_COUNT;
#endif

#ifdef NEED_SPLITCODE
 SPLITDELAY = (DEFAULT_SERVER_SPLIT_RECOVERY_TIME * 60);
 SPLITNUM = SPLIT_SMALLNET_SIZE;
 SPLITUSERS = SPLIT_SMALLNET_USER_SIZE;
 server_split_time = CurrentTime;
#endif

 /* End of global set options */

}

/*
 * initialize_message_files
 *
 * inputs       - none
 * output       - none
 * side effects - Set up all message files needed, motd etc.
 */

static void initialize_message_files(void)
  {
    InitMessageFile( USER_MOTD, IMOTDF, &ConfigFileEntry.motd );
    InitMessageFile( OPER_MOTD, OMOTDF, &ConfigFileEntry.opermotd );
    InitMessageFile( WEB_MOTD,  WMOTDF, &ConfigFileEntry.wmotd );

    ReadMessageFile( &ConfigFileEntry.motd );
    ReadMessageFile( &ConfigFileEntry.opermotd );
    ReadMessageFile( &ConfigFileEntry.wmotd );
  }

/*
 * write_pidfile
 *
 * inputs       - none
 * output       - none
 * side effects - write the pid of the ircd to PIDFILE
 */

static void write_pidfile(void)
{
  int fd;
  char buff[20];
  char ppath[256];

#ifndef PIDFILE  
  snprintf(ppath, 256, "%s/ircd.pid", ConfigFileEntry.varpath);
#else
  strncpy(ppath, PIDFILE, 256);
#endif
  
  if ((fd = open(ppath, O_CREAT|O_WRONLY, 0600))>=0)
    {
      ircsprintf(buff,"%d\n", (int)getpid());
      if (write(fd, buff, strlen(buff)) == -1)
        irclog(L_ERROR,"Error writing to pid file %s", ppath);
      close(fd);
      return;
    }
  else
    irclog(L_ERROR, "Error opening pid file %s", ppath);
}


/* check_logsize
 *
 * inputs       - nothing
 * output       - nothing
 * side effects - checks logfile if >100k log rotation will occur 
 *                
 * - Lamego
 */
static void check_logsize(void)
{
  struct stat finfo;
  char lpath[256], lpath1[256], lpath2[256], lpath3[256];
  snprintf(lpath, 256, "%s/log/ircd.log", ConfigFileEntry.varpath);
  snprintf(lpath1, 256, "%s/log/ircd.log.1", ConfigFileEntry.varpath);  
  snprintf(lpath2, 256, "%s/log/ircd.log.2", ConfigFileEntry.varpath);
  snprintf(lpath3, 256, "%s/log/ircd.log.3", ConfigFileEntry.varpath);    
  
  if(stat(lpath, &finfo)==0) /* successful */
	{
	  if (finfo.st_size > 100000) /* > 100k ? */
		{
		  printf("Rotating %s...\n", lpath);
		  rename(lpath2, lpath3);
		  rename(lpath1, lpath2);
		  rename(lpath, lpath1);		  
		}
	}
}  

/*
 * check_pidfile
 *
 * inputs       - nothing
 * output       - nothing
 * side effects - reads pid from pidfile and checks if ircd is in process
 *                list. if it is, gracefully exits
 * -kre
 */
static void check_pidfile(void)
{
  int fd;
  char buff[20];
  pid_t pidfromfile;
  char ppath[256];

#ifndef PIDFILE  
  snprintf(ppath, 256, "%s/ircd.pid", ConfigFileEntry.varpath);
#else
  strncpy(ppath, PIDFILE, 256);
#endif  
  if ((fd = open(ppath, O_RDONLY)) >= 0 )
  {
    if (read(fd, buff, sizeof(buff)) == -1)
    {
      /* printf("NOTICE: problem reading from %s (%s)\n", ppath,
          strerror(errno)); */
    }
    else
    {
      pidfromfile = atoi(buff);
      if (pidfromfile != (int)getpid() && !kill(pidfromfile, 0))
      {
        printf("ERROR: daemon is already running with pid=%i\n", pidfromfile);
        exit(-1);
      }
    }
    close(fd);
  }
  else if(errno != ENOENT)
  {
    printf("WARNING: problem opening %s: %s\n", ppath, strerror(errno));
  }

}

/*
 * setup_corefile
 *
 * inputs       - nothing
 * output       - nothing
 * side effects - setups corefile to system limits.
 * -kre
 */
static void setup_corefile(void)
{
  struct rlimit rlim; /* resource limits */

  /* Set corefilesize to maximum */
 if (!getrlimit(RLIMIT_CORE, &rlim))
  {
    rlim.rlim_cur = rlim.rlim_max;
    setrlimit(RLIMIT_CORE, &rlim);
  }
}


/* build multi-word items from dconf */
void build_multi_dconf(void)
{
  char tmp[256];
  int i;
  if(AutoJoinChan)
    {
      parse_multi(AutoJoinChan, ",", ajoin_chans, sizeof(ajoin_chans));  
      i = 0;
      tmp[0] = '\0';
      while(ajoin_chans[i])
        {
          if(i>0)
            strcat(tmp, ",");
          strcat(tmp, "#");
          strcat(tmp, ajoin_chans[i]);
          ++i;
        }
        free(AutoJoinChan);
        AutoJoinChan = strdup(tmp);
    }  
}


int main(int argc, char *argv[])
{
  uid_t       uid;
  uid_t       euid;
  time_t      delay = 0;
  aConfItem*  aconf;
  char        lpath[256];     

  /*
   * save server boot time right away, so getrusage works correctly
   */
  if ((CurrentTime = time(0)) == -1)
    {
      fprintf(stderr, "ERROR: Clock Failure: %s\n", strerror(errno));
      exit(errno);
    }
    
   srandom(CurrentTime); /* Set random seed */
   
  /*
   * Setup corefile size immediately after boot
   */
  setup_corefile();
  
  /* 
   * set initialVMTop before we allocate any memory
   */
  initialVMTop = get_vm_top();
  
  /*
   * Get ircd binary absolute path
   */
  ircdpath[0] = '\0';
  if (argv[0][0] != '/')
	{
	  if ( getcwd(ircdpath, 255)==NULL )
		{
		  fprintf(stderr, "ERROR getting current directory %s\n", strerror(errno));
    	  exit(errno);		
		}
	  strcat(ircdpath,"/");		
	}

  strncat(ircdpath, argv[0], 255-strlen(ircdpath));  
    
  ServerRunning = 0;
  memset(&me, 0, sizeof(me));
  GlobalClientList = &me;       /* Pointer to beginning of Client list */
  cold_start = YES;             /* set when server first starts up */

  memset(&Count, 0, sizeof(Count));
  Count.server = 1;     /* us */

  initialize_global_set_options();

#ifdef REJECT_HOLD
  reject_held_fds = 0;
#endif

/* this code by mika@cs.caltech.edu */
/* 
 * it is intended to keep the ircd from being swapped out. BSD swapping
 * criteria do not match the requirements of ircd 
 */
#ifdef SETUID_ROOT
  if (setuid(IRC_UID) < 0)
    exit(-1); /* blah.. this should be done better */
#endif


  uid = getuid();
  euid = geteuid();

  ConfigFileEntry.etcpath = ETCPATH;
  ConfigFileEntry.varpath = VARPATH;

#ifdef CHROOT_DIR
#ifdef HAVE_CHROOT
  if (chdir(CHROOT_DIR))
    {         
      fprintf(stderr,"ERROR: Cannot chdir\n");
      perror("chdir " CHROOT_DIR );
      exit(-1);
    }
  if (chroot(CHROOT_DIR))
    {         
      fprintf(stderr,"ERROR: Cannot chroot\n");
      perror("chroot " CHROOT_DIR );
      exit(-1);
    }
    
  /* lets override it to the root */
  ConfigFileEntry.etcpath = "/etc";
  ConfigFileEntry.varpath = "/var";
#else
#warning You specified chroot dir but chroot() was not found on your system
#endif /*HAVE_CHROOT*/
#endif /*CHROOT_DIR*/

  myargv = argv;
  umask(077);                /* better safe than sorry --SRB */

  setuid(uid);
  
#ifdef  ZIP_LINKS
  /* Make sure the include files match the library version number. */
  /* configure takes care of looking for zlib and zlibVersion(). */  
  if (strcmp(zlibVersion(), ZLIB_VERSION) != 0)
  {
    fprintf(stderr, "WARNING: zlib include files differ from library.\n");
    fprintf(stderr, "WARNING: ZIPLINKS may fail!\n");
    fprintf(stderr, "WARNING: library %s, include files %s\n",
            zlibVersion(), ZLIB_VERSION);
  }
#endif

  parse_command_line(argc, argv); 

  
  if (chdir(ConfigFileEntry.etcpath))
    {
      fprintf(stderr,"ERROR: Cannot chdir to %s\n", ConfigFileEntry.etcpath);
      perror("chdir");
      exit(-1);
    }
    
  /*
   * Just check dconf for errors
   */
  if(dconf_check_opt)
    {
      if ((dconf_read("save.dconf", 0) == -1) )	/* first check for saved settings */
	    {	  
		  if ((dconf_read("main.dconf", 0) == -1) )	/* read our local settings */
			{
				irclog(L_ERROR, "Error opening main.dconf");
		      return -1;
		    }  
	    }

      return !dconf_check(-1);
    }
    
  /* read the human verification code font */
  if(hvc_read_font() < 0)
  {
    fprintf(stderr, "Error reading the hvc font from "ETCPATH"/hvc.font");
    exit(2);
  }

  
  /*
   * Check if daemon is already running
   */
  check_pidfile();  

  /*
   * Check if log rotation is needed
   */  
  check_logsize();

#if !defined(IRC_UID) || !defined(IRC_GID)
  if ((uid != euid) && !euid)
    {
      fprintf(stderr,
              "ERROR: do not run ircd setuid root.\n" \
              "Make it setuid a normal user.\n" \
              "Or build it with --with-uid=uid, --with-gid=gid");
      exit(-1);
    }
#endif

#if !defined(CHROOT_DIR) || ((defined(IRC_UID) && defined(IRC_GID)))

  setuid(euid);

  if (getuid() == 0)
    {
# if defined(IRC_UID) && defined(IRC_GID)

      /* run as a specified user */
#if 0      
      fprintf(stderr,"WARNING: running ircd with uid = %d\n", IRC_UID);
      fprintf(stderr,"         changing to gid %d.\n",IRC_GID);
#endif
      /* setgid/setuid previous usage noted unsafe by ficus@neptho.net
       */

      if (setgid(IRC_GID) < 0)
        {
          fprintf(stderr,"ERROR: can't setgid(%d)\n", IRC_GID);
          exit(-1);
        }

      if(setuid(IRC_UID) < 0)
        {
          fprintf(stderr,"ERROR: can't setuid(%d)\n", IRC_UID);
          exit(-1);
        }

#else
#if !defined(IRC_UID)
      /* check for setuid root as usual */
      fprintf(stderr,
              "ERROR: do not run ircd setuid root. " \
              "Make it setuid a normal user.\n");
      return -1;
#endif
#endif 
            } 
#endif /*CHROOTDIR/UID/GID*/

  if(!silent_start)
	{
		printf("Starting PTlink IRCd version %s\n",ircdversion);
	}

  snprintf(lpath, 256, "%s/log/ircd.log", ConfigFileEntry.varpath);
  
  setup_signals();

  
  /* this code is just to check we can open the log file 
  we must reopen after init_sys() because it will close all open fd's -Lamego
  */
  init_log(lpath);  
  close_log();
  
  init_sys(bootDaemon);
  init_log(lpath);


  irclog(L_NOTICE, "Starting ircd from %s", ircdpath);
  
  initialize_message_files();

  dbuf_init();  /* set up some dbuf stuff to control paging */
  init_hash();

#ifdef HAVE_SSL 
  init_ssl();
#endif


  clear_scache_hash_table();    /* server cache name table */
  clear_ip_hash_table();        /* client host ip hash table */
  clear_Dline_table();          /* d line tree */
  initlists();
  initclass();
  initwhowas();
  init_stats();
  init_tree_parse(msgtab);      /* tree parse code (orabidoo) */

  fdlist_init();
  init_netio();

  if ((dconf_read("save.dconf", 0) == -1) )	/* first check for saved settings */
	{	  
		if ((dconf_read("main.dconf", 0) == -1) )	/* read our local settings */
			{
				irclog(L_ERROR, "Error opening main.dconf");
			return -1;
		}  
	}

  if( !dconf_check(0) )
    return -1;

  if(AutoEnableHVC)
    hvc_enabled = 1;
    
  /* build multi word items from dconf settings */
  build_multi_dconf();
  
  if (TimeZone) 
	{
#ifdef USE_SETENV
	  setenv("TZ",TimeZone,1);
  	  tzset();  
#else
	  irclog(L_WARN, "ignoring unsupported TimeZone setting");
#endif
	}

  if(CodePagePath && CodePages)
    LoadCodePages();
    
  read_conf_files(YES);         /* cold start init conf files */
  
  aconf = find_me();
  if (EmptyString(me.name))
    strncpy_irc(me.name, aconf->host, HOSTLEN);
  strncpy_irc(me.host, aconf->host, HOSTLEN);
  me.fd = -1;
  me.from = &me;
  me.servptr = &me;
  SetMe(&me);
  SetServer(&me);
  make_server(&me);
  me.serv->up = me.name;
  strncpy_irc(me.serv->version, ircdversion, HOSTLEN);
  me.lasttime = me.since = me.firsttime = CurrentTime;
  add_to_client_hash_table(me.name, &me);
  check_class();
  write_pidfile();

  spam_words_init(SpamWords);
  
  /**
   * default-chanmodes patch
   * 2005-09-18 mmarx
   */
   
  if(!DefaultChanMode)
  {
  	conf_change_item("DefaultChanMode", "+nt", 0);
  }
  
  init_new_cmodes();
  init_local_log_channels(&me);  
  
  help_load(UserHelpFile , &user_help);	
  help_load(OperHelpFile , &oper_help);	
  help_load(AdminHelpFile , &admin_help);	
      
  irclog(L_NOTICE, "Server Ready");

  ServerRunning = 1;
  while (ServerRunning)
    {
#ifdef USE_ADNS
  if(ReverseLookup)
  {
        usleep(100000);
        do_adns_io();
  }
#endif
      delay = io_loop(delay);
#ifdef USE_ADNS
  if(ReverseLookup)    
      do_adns_io();
#endif      
    } 
  return 0;
}

