/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2005    *
 *                http://software.pt-link.net                    *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 *   $Id: 
   
  File: sxline.h
  Desc: zap line
  Author: Lamego@PTlink.net
*/
#include "struct.h"
extern aConfItem *sxlines;
struct ConfItem* find_sxline(char *name);
struct ConfItem* is_sxlined(char *name);
void send_all_sxlines(struct Client *acptr);
void report_sxlines(struct Client *sptr);
void clear_sxlines(void);
