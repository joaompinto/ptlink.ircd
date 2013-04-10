/************************************************************************
 *   IRC - Internet Relay Chat, src/motd.c
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
 *   $Id: motd.c,v 1.4 2005/08/27 16:23:50 jpinto Exp $
 */
#include "motd.h"
#include "ircd.h"
#include "s_bsd.h"
#include "fileio.h"
#include "res.h"
#include "s_conf.h"
#include "class.h"
#include "send.h"
#include "s_conf.h"
#include "numeric.h"
#include "client.h"
#include "irc_string.h"
#include "s_serv.h"     /* hunt_server */

#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifndef PATH_MAX    
#define PATH_MAX    4096
#endif

extern ConfigFileEntryType ConfigFileEntry; /* defined in ircd.c */
ATline * tlines = NULL;	/* for T:lines storage */

/*
** m_motd
**      parv[0] = sender prefix
**      parv[1] = servername
*/
int m_motd(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  static time_t last_used = 0;
  MessageFile *motd;  
  char *mename = me.name;
  
  if(!IsPerson(sptr))
	  return 0;
	  
  if(sptr->user && sptr->user->vlink)
    mename = sptr->user->vlink->name;
    
  if(!IsAnOper(sptr))
    {
      if((last_used + PACE_WAIT) > CurrentTime)
        {
          /* safe enough to give this on a local connect only */
          if(MyClient(sptr))
            sendto_one(sptr,form_str(RPL_LOAD2HI),mename,sptr->name);
          return 0;
        }
      else
        last_used = CurrentTime;
    }

  if (hunt_server(cptr, sptr, ":%s MOTD :%s", 1,parc,parv)!=HUNTED_ISME)
    return 0;

  sendto_ops_imodes(IMODE_SPY, "motd requested by %s (%s@%s) [%s]",
                     sptr->name, sptr->username, sptr->host,
                     sptr->user->server);


  if (!IsWebchat(sptr))
      motd = FindMotd(sptr->host);
  else
      motd = &ConfigFileEntry.wmotd;
  
  return(SendMessageFile(sptr,motd));
}

/*
** InitMessageFile
**
*/
void InitMessageFile(MotdType motdType, char *fileName, MessageFile *motd)
  {
    strncpy_irc(motd->fileName, fileName, PATH_MAX);
    motd->fileName[PATH_MAX] = '\0';
    motd->motdType = motdType;
    motd->contentsOfFile = NULL;
    motd->lastChangedDate[0] = '\0';
  }

/*
** SendMessageFile
**
** This function split off so a server notice could be generated on a
** user requested motd, but not on each connecting client.
** -Dianora
*/

int SendMessageFile(struct Client *sptr, MessageFile *motdToPrint)
{
  MessageFileLine *linePointer;
  MotdType motdType;
  char *mename = me.name;

  if(motdToPrint)
    motdType = motdToPrint->motdType;
  else
    return -1;

  if(sptr->user && sptr->user->vlink)
    mename = sptr->user->vlink->name;

  switch(motdType)
    {
    case USER_MOTD:

      if (motdToPrint->contentsOfFile == (MessageFileLine *)NULL)
        {
          sendto_one(sptr, form_str(ERR_NOMOTD), mename, sptr->name);
          return 0;
        }

      sendto_one(sptr, form_str(RPL_MOTDSTART), mename, sptr->name, mename);

      for(linePointer = motdToPrint->contentsOfFile;linePointer;
          linePointer = linePointer->next)
        {
          sendto_one(sptr,
                     form_str(RPL_MOTD),
                     mename, sptr->name, linePointer->line);
        }
      sendto_one(sptr, form_str(RPL_ENDOFMOTD), mename, sptr->name);
      return 0;
      /* NOT REACHED */
      break;

    case OPER_MOTD:
      if (motdToPrint == (MessageFile *)NULL)
        {
          sendto_one(sptr, ":%s NOTICE %s :No OPER MOTD", mename, sptr->name);
          return 0;
        }
      sendto_one(sptr,":%s NOTICE %s :Start of OPER MOTD",mename,sptr->name);
      break;

    case WEB_MOTD:
      break;

    default:
      return 0;
      /* NOT REACHED */
    }

  sendto_one(sptr,":%s NOTICE %s :%s",mename,sptr->name,
             motdToPrint->lastChangedDate);


  for(linePointer = motdToPrint->contentsOfFile;linePointer;
      linePointer = linePointer->next)
    {
      sendto_one(sptr,
                 ":%s NOTICE %s :%s",
                 mename, sptr->name, linePointer->line);
    }
  sendto_one(sptr, ":%s NOTICE %s :End", mename, sptr->name);
  return 0;
}

/*
 * ReadMessageFile() - original From CoMSTuD, added Aug 29, 1996
 * modified by -Dianora
 */

int ReadMessageFile(MessageFile *MessageFileptr)
{
  struct stat sb;
  struct tm *local_tm;

  /* used to clear out old MessageFile entries */
  MessageFileLine *mptr = 0;
  MessageFileLine *next_mptr = 0;

  /* used to add new MessageFile entries */
  MessageFileLine *newMessageLine = 0;
  MessageFileLine *currentMessageLine = 0;

  char buffer[MESSAGELINELEN];
  char *p;
  FBFILE* file;

  if( stat(MessageFileptr->fileName, &sb) < 0 )
  /* file doesn't exist oh oh */
  /* might consider printing error message to all opers */
    return -1;

  local_tm = localtime(&sb.st_mtime);

  if (local_tm)
    ircsprintf(MessageFileptr->lastChangedDate,
               "%d/%d/%d %t:%t",
               local_tm->tm_mday,
               local_tm->tm_mon + 1,
               1900 + local_tm->tm_year,
               local_tm->tm_hour,
               local_tm->tm_min);

  /*
   * Clear out the old MOTD
   */
  for( mptr = MessageFileptr->contentsOfFile; mptr; mptr = next_mptr)
    {
      next_mptr = mptr->next;
      MyFree(mptr);
    }

  MessageFileptr->contentsOfFile = NULL;

  if ((file = fbopen(MessageFileptr->fileName, "r")) == 0)
    return(-1);

  while (fbgets(buffer, MESSAGELINELEN, file))
    {
      if ((p = strchr(buffer, '\n')))
        *p = '\0';
      newMessageLine = (MessageFileLine*) MyMalloc(sizeof(MessageFileLine));

      strncpy_irc(newMessageLine->line, buffer, MESSAGELINELEN);
      newMessageLine->line[MESSAGELINELEN] = '\0';
      newMessageLine->next = (MessageFileLine *)NULL;

      if (MessageFileptr->contentsOfFile)
        {
          if (currentMessageLine)
            currentMessageLine->next = newMessageLine;
          currentMessageLine = newMessageLine;
        }
      else
        {
          MessageFileptr->contentsOfFile = newMessageLine;
          currentMessageLine = newMessageLine;
        }
    }

  fbclose(file);
  return(0);
}

MessageFile *FindMotd(const char* host)
  {
	ATline *tline = tlines;
	
	while(tline && !match(tline->host, host))
	  tline = tline->next;
	  
	return (tline) ? &tline->motd : &ConfigFileEntry.motd;
	
  }
  
