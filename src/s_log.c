/************************************************************************
 *   IRC - Internet Relay Chat, src/s_log.c
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
 *   $Id: s_log.c,v 1.4 2005/08/27 16:23:50 jpinto Exp $
 */
#include "s_log.h"
#include "irc_string.h"
#include "ircd.h"
#include "s_misc.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>


#define LOG_BUFSIZE 2000 

#ifdef USE_LOGFILE
static int logFile = -1;
#endif /* USE_LOGFILE */

static int logLevel = INIT_LOG_LEVEL;

#ifdef USE_SYSLOG
static int sysLogLevel[] = {
  LOG_CRIT,
  LOG_ERR,
  LOG_WARNING,
  LOG_NOTICE,
  LOG_INFO,
  LOG_INFO,
  LOG_INFO
};
#endif /* USE_SYSLOG */

static const char *logLevelToString[] =
{ "L_CRIT",
  "L_ERROR",
  "L_WARN",
  "L_NOTICE",
  "L_TRACE",
  "L_INFO",
  "L_DEBUG"
};

void vlog(int priority, const char *fmt, va_list args);

/*
 * open_log - open ircd logging file
 * returns false (0) if successful, true (-1) otherwise
 */
#if defined(USE_LOGFILE) 
static int open_log(const char* filename)
{
  logFile = open(filename, 
                 O_WRONLY | O_APPEND | O_CREAT | O_NONBLOCK, 0644);
       
  if (-1 == logFile) {
#ifdef USE_SYSLOG
    /* If the user does not wish to use syslog, don't even send the log error
     * to syslog -Hwy
     */            
    syslog(LOG_ERR, "Unable to open log file: %s: %s",
           filename, strerror(errno));
#endif           
    return -1;
  }
  return 0;
}
#endif

void close_log(void)
{
#if defined(USE_LOGFILE) 
  if (-1 < logFile) {
    close(logFile);
    logFile = -1;
  }
#endif
#ifdef USE_SYSLOG  
  closelog();
#endif
}

#if defined(USE_LOGFILE) 
static void write_log(const char* message)
{
  char buf[LOG_BUFSIZE];
  sprintf(buf, "[%s] %s\n", smalldate(CurrentTime), message);
  write(logFile, buf, strlen(buf));
}
#endif
   
void vlog(int priority, const char *fmt, va_list args)
{
  char buf[LOG_BUFSIZE];
  assert(-1 < priority);
  assert(0 != fmt);

  if(priority > logLevel)
  	return;  
  vsprintf(buf, fmt, args);
#ifdef USE_SYSLOG
  if(priority <= L_DEBUG)
  	syslog(sysLogLevel[priority], "%s", buf);
#endif
#ifdef USE_LOGFILE
   write_log(buf);
#endif	  		
}

void irclog(int priority, const char* fmt, ...)
{
  char    buf[LOG_BUFSIZE];
  va_list args;
  assert(-1 < priority);
  assert(0 != fmt);

  if (priority > logLevel)
    return;

  va_start(args, fmt);
  vsprintf(buf, fmt, args);
  va_end(args);

#ifdef USE_SYSLOG  
  if (priority <= L_DEBUG)
    syslog(sysLogLevel[priority], "%s", buf);
#endif
#if defined(USE_LOGFILE) 
  write_log(buf);
#endif
}
  
void init_log(const char* filename)
{
#if defined(USE_LOGFILE) 
  if(open_log(filename)==-1)
    {
      fprintf(stderr,"ERROR: Could not open log file %s\n", filename);
      exit(-1);
    };
#endif
#ifdef USE_SYSLOG
  openlog("ircd", LOG_PID | LOG_NDELAY, LOG_FACILITY);
#endif
}

void set_log_level(int level)
{
  if (L_ERROR < level && level <= L_DEBUG)
    logLevel = level;
}

int get_log_level(void)
{
  return( logLevel );
}

const char *get_log_level_as_string(int level)
{
  if(level > L_DEBUG)
    level = L_DEBUG;
  else if(level < L_ERROR)
    level = L_ERROR;

  return(logLevelToString[level]);
}
