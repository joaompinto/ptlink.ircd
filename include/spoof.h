/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2005    *
 *                http://software.pt-link.net                    *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 *   $Id: 
 
  File: spoof.h
  Desc: host spoofing routines
  Author: Lamego@PTlink.net
*/
char *maskme(char *curr);
char *maskme_myCrypt(char *curr, long ip);
char *spoofed();
