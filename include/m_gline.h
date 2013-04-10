/************************************************************************
 *   IRC - Internet Relay Chat, include/gline.h
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
 * "m_gline.h". - Headers file.
 *
 * $Id: m_gline.h,v 1.4 2005/08/27 16:23:49 jpinto Exp $
 *
 */

#ifndef INCLUDED_m_gline_h
#define INCLUDED_m_gline_h
#ifndef INCLUDED_config_h
#include "config.h"
#endif
#ifndef INCLUDED_ircd_defs_h
#include "ircd_defs.h"
#endif
#ifndef INCLUDED_sys_types_h
#include <sys/types.h>
#define INCLUDED_sys_types_h
#endif
struct Client;
struct ConfItem;

extern struct ConfItem *glines;

extern int m_gline(struct Client *,struct Client *,int,char **);
extern struct ConfItem* find_gkill(struct Client *, char *);
extern struct ConfItem* find_is_glined(const char *, const char *, const char*);
extern void   flush_glines(void);             
extern void   report_glines(struct Client *, char *mask); 
extern void   send_all_glines(struct Client *);
extern void   add_gline(struct ConfItem *aconf);
extern int apply_gline(char *host, char *user, char *reason);
#endif
