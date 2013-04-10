/************************************************************************
 *   IRC - Internet Relay Chat, src/numeric.c
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
 *      I kind of modernized this code a bit. -Dianora
 *
 *   $Id: numeric.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $
 */
#include "numeric.h"
#include "irc_string.h"
#include "common.h"     /* NULL cripes */

#include <assert.h>
/* there is no CUSTOM_ERR option anymore, other message.tab option 
 * is removed, just like the other code pieces of it - ^Stinger^ 
 */ 
#include "messages.tab"


/*
 * The observant will note that err_str and rpl_str
 * could be replaced by one function now. 
 * -Dianora
 * ok. ;-)
 */

#if 0
static char numbuff[512];  /* ZZZ There is no reason this has to
                            * be so large
                            */
#endif

const char* form_str(int numeric)
{

  assert(-1 < numeric);
  assert(numeric < ERR_LAST_ERR_MSG);
  assert(0 != replies[numeric]);
  
  return replies[numeric];
#if 0
  char *nptr;
  if ((numeric < 0) || (numeric > ERR_LAST_ERR_MSG))
    {
      ircsprintf(numbuff, ":%%s %d %%s :INTERNAL ERROR: BAD NUMERIC! %d",
                 numeric, numeric);
      return numbuff;
    }

  if (!(nptr = replies[numeric]))
    {
      ircsprintf(numbuff, ":%%s %d %%s :NO ERROR FOR NUMERIC ERROR %d",
                 numeric, numeric);
      return numbuff;
    }
  else
    return nptr;
#endif
}


