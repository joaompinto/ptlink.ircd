/************************************************************************
 *   IRC - Internet Relay Chat, src/oratime.c
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
 *   $Id: oratime.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $
 */
#include "oratime.h"
#ifdef ORATIMING

#include "client.h"
#include "send.h"

#include <sys/time.h>

static struct timeval tsdnow;
static struct timeval tsdthen;
static unsigned int   tsdms;

/*
 * Timing stuff (for performance measurements): compile with -DORATIMING
 *   and put a TMRESET where you want the counter of time spent set to 0,
 *  a TMPRINT where you want the accumulated results, and TMYES/TMNO pairs
 *  around the parts you want timed -orabidoo
 */
void orat_reset(void)
{
  tsdms = 0;
}

void orat_yes(void)
{
  gettimeofday(&tsdthen, 0);
}

void orat_no(void)
{
  gettimeofday(&tsdnow, 0); 
  if (tsdnow.tv_sec != tsdthen.tv_sec) 
    tsdms += 1000000 * (tsdnow.tv_sec - tsdthen.tv_sec); 
  tsdms += tsdnow.tv_usec;
  tsdms -= tsdthen.tv_usec;
}

void orat_report(void)
{
  sendto_realops("Time spent: %ld ms", tsdms);
}

#endif /* ORATIMING */

