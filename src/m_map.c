/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2000     *
 *                http://software.pt-link.net                     *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 
 File: m_map.c
 Desc: network map
 Author: Lamego@PTlink.net
  
*/

#include "m_commands.h"
#include "client.h"
#include "common.h"
#include "ircd.h"
#include "numeric.h"
#include "s_serv.h"
#include "irc_string.h"
#include "send.h"

#include <string.h> 	/* for strchr() */


void dump_map(struct Client * cptr,  struct Client * server, int prompt_length, int length);

/*
 * dump_map (used by m_map)
 * version info included - Lamego
 */
void dump_map(struct Client * cptr,  struct Client * server, int prompt_length, int length)
{
    static char prompt[64];
    char *p = &prompt[prompt_length];
    int cnt = 0, localu = 0;
	register struct Client *acptr;
    char *tmp =NULL;

    *p = '\0';

    if (prompt_length > 60)
	  sendto_one(cptr, form_str(RPL_MAPMORE), me.name, cptr->name, prompt, server->name);
    else 
	  {
	    for(acptr = GlobalClientList; acptr; acptr = acptr->next)
		  {
	  		if (IsPerson(acptr)) 
			  {
			  
				if (!irccmp(acptr->user->server, server->name))
		  		  ++localu;
				  
				++cnt;
				
	  		  }
		  }
		  
		tmp=strchr(server->serv->version,'_'); /* Lets strip the ugly bit mask -Lamego */
		if(tmp) 
		  *tmp='\0';
		  
		sendto_one(cptr, form_str(RPL_MAP), me.name, cptr->name, prompt, length, 
	  		server->name, localu, (localu * 100) / cnt,
	  		server->serv->version);
	
		if (tmp) *tmp='_';
	
  	  }

  	if (prompt_length > 0) 
	  {
		p[-1] = ' ';
		if (p[-2] == '`')
	  	  p[-2] = ' ';
  	  }
	
    if (prompt_length > 60)
	  return;


	
	cnt = 0;
	
	/* Count all servers connected to this server to determine the prompt type |- or ` */
    for(acptr = GlobalClientList; acptr; acptr = acptr->next)
  	  {
		if (!IsServer(acptr) || (acptr==&me) || strcmp(acptr->serv->up, server->name))
	  	  continue;
		  ++cnt;
	  }
	
	strcpy(p, "|-");
	
	/* Yes I am going to scan the full client list again,
	   m_map is oper only so no big deal - Lamego */
	   
    for(acptr = GlobalClientList; acptr; acptr = acptr->next)
  	  {
		if (!IsServer(acptr) || (acptr==&me) || strcmp(acptr->serv->up, server->name))
	  	  continue;
	    		
		
		if (--cnt == 0)
	  	  *p = '`';
		  
		dump_map(cptr, acptr, prompt_length + 2, length - 2);
	}

    if (prompt_length > 0)
	p[-1] = '-';
}

/*
   ** m_map
   **
   **      parv[0] = sender prefix
   * */
int m_map(struct Client * cptr, struct Client * sptr, int parc, char *parv[])
{

    struct Client *acptr;
    int longest = strlen(me.name);

    if (!IsPrivileged(sptr)) 
	  {
		sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, parv[0]);
		return 0;
  	  }

  	if (parc < 2)
	  parv[1] = "*";


	for(acptr = GlobalClientList; acptr; acptr = acptr->next)
	
	if (IsServer(acptr) && (strlen(acptr->name) + acptr->hopcount * 2) > longest)
	    longest = strlen(acptr->name) + acptr->hopcount * 2;

    if (longest > 60)
	longest = 60;
    longest += 2;
    dump_map(sptr, &me, 0, longest);
    sendto_one(sptr, form_str(RPL_MAPEND), me.name, parv[0]);

    return 0;
}
