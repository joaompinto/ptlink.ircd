/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2004    *
 *                http://software.pt-link.net                    *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 *   $Id: m_imode.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $
  
 File: m_imode.c
 Desc: change information modes
 Author: Lamego@PTlink.net

*/

#include "m_commands.h"
#include "common.h"
#include "client.h"
#include "channel.h"
#include "hash.h"
#include "struct.h"
#include "ircd.h"
#include "numeric.h"
#include "s_serv.h"
#include "send.h"
#include "m_silence.h"
#include "list.h"
#include "s_user.h"
#include "irc_string.h"
#include "s_conf.h"
#include "dconf_vars.h"

/* table of ascii char letters to corresponding bitmask */
                                                                                
FLAG_ITEM imodes_map[] =
{
  {IMODE_BOTS, 		'b'},
  {IMODE_CLIENTS, 	'c' },
  {IMODE_DEBUG, 	'd' },
  {IMODE_EXTERNAL, 	'e'},
  {IMODE_FULL, 		'f' },
  {IMODE_GENERIC, 	'g' },  
  {IMODE_KILLS, 	'k' },
  {IMODE_REJECTS, 	'r' },
  {IMODE_TARGET, 	't' },
  {IMODE_VLINES, 	'v' },
  {IMODE_SPY, 		'y' },
  { 0 , 0}
};

/*
 * returns the IMODE change string 
 */
char* imode_change(int old, int new)
{
  int   i;
  int flag;
  char  *m;
  int   what = MODE_NULL;
  static char umode_buf[128];

  /*
   * build a string in umode_buf to represent the change in the user's
   * mode between the new (sptr->flag) and 'old'.
   */
  m = umode_buf;
  *m = '\0';

  for (i = 0; imodes_map[i].letter; i++ )
    {
      flag = imodes_map[i].mode;

      if ((flag & old) && !(new & flag))
        {
          if (what == MODE_DEL)
            *m++ = imodes_map[i].letter;
          else
            {
              what = MODE_DEL;
              *m++ = '-';
              *m++ = imodes_map[i].letter;
            }
        }
      else if (!(flag & old) && (new & flag))
        {
          if (what == MODE_ADD)
            *m++ = imodes_map[i].letter;
          else
            {
              what = MODE_ADD;
              *m++ = '+';
              *m++ = imodes_map[i].letter;
            }
        }
    }
  *m = '\0';
  return umode_buf;
}

/*
** m_imode() 
** parv[0] = sender
** parv[1] = target
** parv[2] = imode change
**
*/
int m_imode(struct Client* cptr, struct Client* sptr, int parc, char* parv[])
{
  struct Client *acptr;
  char buf[BUFSIZE];
  unsigned int old_imodes;
  unsigned int new_imodes;
  char *c;
  int add = 1;
  int i;
  char *m;

  if (!IsPrivileged(cptr))
    {
      sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, parv[0]);
      return 0;
    }

  if (parc < 2)
    {
      sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
                 me.name, parv[0], "IMODE");
      return 0;
    }
                                                                                
  if (!(acptr = find_person(parv[1], NULL)))
    {
      if (MyConnect(sptr))
        sendto_one(sptr, form_str(ERR_NOSUCHNICK),
                   me.name, parv[0], parv[1]);
      return 0;
    }
  
  if(!MyClient(acptr) && parc<3)
    {
      sendto_one(acptr,":%s IMODE %s", parv[0], parv[1]);
      return 0;
    }
    
  if (parc < 3)
    {
      m = buf;
      *m++ = '+';
                                                                                
      for (i = 0; imodes_map[i].letter && (m - buf < BUFSIZE - 4);i++)
        if (sptr->imodes & imodes_map[i].mode)
          *m++ = imodes_map[i].letter;
      *m = '\0';
      sendto_one(sptr, ":%s NOTICE %s :Current information modes: %s"
        , me.name, parv[0], buf);
      return 0;
    }
   
  old_imodes = sptr->imodes;
  new_imodes = old_imodes;
  c=parv[2];
  while(*c)
    {
      switch(*c) 
        {      
          case '+': add = 1; break;
          case '-': add = 0; break;
          default:
            i = 0;
            while((imodes_map[i].letter) && (imodes_map[i].letter!=*c))
              ++i;
            if(imodes_map[i].letter)
              {
                if(add)
                  new_imodes |= imodes_map[i].mode;
                else
                  new_imodes &= ~imodes_map[i].mode;
              }
          break;
        }
      c++;
    }
  sptr->imodes = new_imodes;
  if(MyConnect(sptr))
    sendto_one(sptr,":%s NOTICE %s :Information modes change: %s", 
      me.name, sptr->name, imode_change(old_imodes, new_imodes));
  return 0;
}
