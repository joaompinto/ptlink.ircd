/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2001     *
 *                http://software.pt-link.net                     *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 *   $Id:
   
  File: svline.h
  Desc: services v-line (DCC block, filename based)
  Author: Lamego@PTlink.net
*/
#include "struct.h"

extern aConfItem *svlines;
struct ConfItem* find_svline(char *name);
struct ConfItem* is_svlined(char *name);
struct ConfItem* is_msgvlined(char *msg);
int is_supervlined(char *message, char *vline);
void send_all_svlines(struct Client *acptr);
void report_svlines(struct Client *sptr);
void clear_svlines(void);
