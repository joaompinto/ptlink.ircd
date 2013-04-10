/* - Internet Relay Chat, include/s_log.h
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
 * $Id: s_log.h,v 1.4 2005/08/27 16:23:49 jpinto Exp $
 */
#ifndef INCLUDED_s_log_h
#define INCLUDED_s_log_h

#define L_CRIT    0
#define L_ERROR   1
#define L_WARN    2
#define L_NOTICE  3
#define L_TRACE   4
#define L_INFO    5
#define L_DEBUG   6

extern void init_log(const char* filename);
extern void close_log(void);
extern void set_log_level(int level);
extern int  get_log_level(void);
extern void irclog(int priority, const char* fmt, ...);
extern const char *get_log_level_as_string(int level);

#endif /* INCLUDED_s_log_h */
