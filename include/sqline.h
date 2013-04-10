/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2005    *
 *                http://software.pt-link.net                    *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 *   $Id: 
 
  File: sqline.h
  Desc: services based qline
  Author: Lamego@PTlink.net
*/
#include "struct.h"
extern aConfItem *sqlines;
struct ConfItem* find_sqline(char *nick);
void send_all_sqlines(struct Client *acptr);
void report_sqlines(struct Client *sptr);
void clear_sqlines();
