/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2000     *
 *                http://software.pt-link.net                     *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 $Id: watch.h,v 1.3 2005/08/27 16:23:49 jpinto Exp $
 
 File: watch.h
 Desc: watch structure
 Author: Lamego@PTlink.net  
*/
#ifndef INCLUDE_watch_h
#define INCLUDE_watch_h

#ifndef INCLUDED_sys_types_h
#include <sys/types.h>       /* time_t */
#define INCLUDED_sys_types_h
#endif

struct Watch {
	struct Watch  *hnext;
    time_t lasttime;
    struct SLink  *watch;
    char  nick[1];
}; 

#endif
