/************************************************************************
 *   IRC - Internet Relay Chat, include/h.h
 *   Copyright (C) 1992 Darren Reed
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
 * "list.h". - Headers file.
 *
 *
 * $Id: list.h,v 1.3 2005/08/27 16:23:49 jpinto Exp $
 *
 */

#ifndef INCLUDED_list_h
#define INCLUDED_list_h

struct SLink;
struct Client;
struct Class;
struct User;
struct Channel;

extern void count_user_memory(int *, int *);
extern void count_links_memory(int *, int *);
extern void count_flud_memory(int *, int *);
extern void     outofmemory(void);
extern  void    free_link (struct SLink *);
extern  void    free_user (struct User *, struct Client *);
extern  struct SLink    *make_link (void);
extern  struct User     *make_user (struct Client *);
extern  struct Class* make_class(void);
extern  struct Server   *make_server (struct Client *);
extern  struct SLink    *find_user_link (struct SLink *, struct Client *);
extern  void    initlists (void);
extern  void    block_garbage_collect(void);
extern  void    block_destroy(void);
extern struct SLink *find_channel_link(struct SLink *, struct Channel *);
extern void send_list(struct Client *cptr);
#endif
