/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2005    *
 * http://www.ptlink.net/Coders/ - coders@PTlink.net             *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 *   $Id: 
 
  File: vlinks.h
  Desc: services based qline
  Author: Lamego@PTlink.net
*/
#include "struct.h"
extern aConfItem *vlinks;
struct ConfItem* find_vlink(char *nick);
void send_all_vlinks(struct Client *acptr);
void report_vlinks(struct Client *sptr);
void dump_vlinks(char* dest, struct Client *sptr);
void clear_vlinks();
