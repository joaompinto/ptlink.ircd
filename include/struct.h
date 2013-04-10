/* - Internet Relay Chat, include/struct.h
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
 * $Id: struct.h,v 1.3 2005/08/27 16:23:49 jpinto Exp $
 */
#ifndef INCLUDED_struct_h
#define INCLUDED_struct_h
#ifndef INCLUDED_config_h
#include "config.h"
#endif
#ifndef INCLUDED_watch_h
#include "watch.h"
#endif

#include <sys/types.h>       /* time_t */ 

struct Channel;
struct Ban;

typedef struct  ConfItem aConfItem;
typedef struct  Client  aClient;
typedef struct  User    anUser;
typedef struct  Server  aServer;
typedef struct  SLink   Link;
typedef struct  Mode    Mode;
typedef struct  Zdata   aZdata;
/* general link structure used for chains */

struct SLink
{
  struct        SLink   *next;
  union
  {
    struct Client   *cptr;
    struct Channel  *chptr;
    struct ConfItem *aconf;
#ifdef BAN_INFO
    struct Ban   *banptr;
#endif
    char      *cp;
	struct Watch*	wptr;
  } value;
  int   flags;
  int                   nmsg;           /* Number of msgs  (+f) */
  time_t                lastmsg;        /* Last message time (+f) */
  char    		*lmsg;          /* Last message sent to channel */
  int     		lsize;          /* Last message size */
  time_t  		lmsg_time;      /* Last message time */   
};

typedef struct	ListOptions	LOpts;
struct ListOptions {
	LOpts	*next;
	Link	*yeslist, *nolist;
	short int	showall;
	unsigned short	usermin;
	int	usermax;
	time_t	currenttime;
	time_t	chantimemin;
	time_t	chantimemax;
	time_t	topictimemin;
	time_t	topictimemax;
};

#define IsSendable(x)		(DBufLength(&x->sendQ) < 2048)

#endif /* INCLUDED_struct_h */
