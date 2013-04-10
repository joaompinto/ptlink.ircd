/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2005    *
 *                http://software.pt-link.net                    *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 
  File: throttle.h
  Desc: throttling code
  Author: Lamego@PTlink.net
*/

struct aThrottle {		/* this is a throttle record */
    struct IN_ADDR ip;
    time_t last;
    struct aThrottle *next;
    char *connected;
    short count;
};

#define THROTTLE_HITLIMIT -1
#define THROTTLE_SENTWARN -2
extern int check_clones(struct Client * cptr, const char *remote);
extern void remove_clone_check(struct Client * cptr); 

