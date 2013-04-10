/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2005    *
 *                http://software.pt-link.net                    *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
  $Id:
  File: zline.h
  Desc: zap line
  Author: Lamego@PTlink.net
*/
#include "struct.h"
extern aConfItem *zlines;
struct ConfItem* find_zline(char *host);
void send_all_zlines(struct Client *acptr);
void report_zlines(struct Client *sptr);
