/**********************************************************************
 * PTlink IRC Services is (C) CopyRight PTlink IRC Software 1999-2006 *
 *                     http://software.pt-link.net                    *
 * This program is distributed under GNU Public License               *
 * Please read the file COPYING for copyright information.            *
 **********************************************************************
*/
/*
  hvc stands for human verification code
  It protects the network againts bots/trojans or any type of automated login
  bug requesting prompting for a code which is shown using ascii art  
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "s_log.h"
#include "path.h"
#include "struct.h"
#include "client.h"
#include "common.h"
#include "ircd.h"

#define V_SIZE 7

char* hvc_font[10][V_SIZE];
int hvc_read_font(void)
{
  FILE *f;
  int num = 0;
  int row = 0;
  
  f = fopen(ETCPATH"/hvc.font", "rt");
  if(f == NULL)
    return -1;
  
  while(!feof(f))
  {
    char line[256];
    fgets(line, sizeof(line), f);
    strip_rn(line);
    if(line[0] != '\003') 
      continue; /* skip empty lines */    
    hvc_font[num][row++] = strdup(line);
    if(row >= V_SIZE) /* jump to next digit */
    {
      ++num;
      row = 0;
    }
  }
  fclose(f);
}

void hvc_send_code(aClient* acptr)
{
  char output[V_SIZE][512];
  int dig_array[10];
  int dig_count;
  int i;
  int rem;
  char *c;
  int x;
  rem = acptr->hvc;
  dig_count = 0;
  
  /* build digit array */  
  while(rem)
  {
    int digit = rem % 10;
    rem /=  10;
    dig_array[dig_count++] = digit;
  }

  /* init output string*/
  for(i = 0; i< V_SIZE; ++i)
    output[i][0] = '\0';
    
  /* build output string */
  while(dig_count--)
  {
    for(i = 0; i < V_SIZE; ++i)
      strcat(output[i], hvc_font[dig_array[dig_count]][i]);
  }

  /* send on motd */
  for(i = 0; i< V_SIZE; ++i)
    sendto_one(acptr, ":%s 006 %s :%s", me.name, acptr->name, output[i]);  

  sendto_one(acptr, ":%s 006 %s :To connect to this server you need to type:",
    me.name, acptr->name);
  sendto_one(acptr, ":%s 006 %s :\002  /login number",
    me.name, acptr->name);    

}

int m_hvc(aClient *cptr, aClient *sptr, int parc, char *parv[])
{
  if(!IsPrivileged(sptr))
    return;
  if(parc < 2)
  { 
    if(MyClient(sptr))
      sendto_one(sptr, "NOTICE %s :Human verification code is \002%s\002", 
        sptr->name, hvc_enabled ? "enabled" :"disabled");
  }
  else
  if(strcasecmp(parv[1], "on") == 0)
  {
    hvc_enabled = 1;
    if(MyClient(sptr))
      sendto_one(sptr, "NOTICE %s :Human verification code is now \002enabled\002",
        sptr->name);
    sendto_serv_butone(cptr,":%s HVC on", sptr->name);
  }
  else
  if(strcasecmp(parv[1], "off") == 0)
  {
    hvc_enabled = 0;
    if(MyClient(sptr))
      sendto_one(sptr, "NOTICE %s :Human verification code is now \002disabled\002",
        sptr->name);
    sendto_serv_butone(cptr,":%s HVC off", sptr->name);
  }  
  else
  {
   if(MyClient(sptr))
     sendto_one(sptr, "NOTICE %s :Invalid value, possible values are on/off",
       sptr->name);
  }
}
