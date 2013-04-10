/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2000     *
 *                http://software.pt-link.net                     *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 *   $Id: spoof.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $ 
 
  File: spoof.c
  Desc: host spoofing routines
  Author: Lamego@PTlink.net
*/
#include <string.h>
#include <stdio.h>

#include "m_commands.h"
#include "client.h"
#include "ircd.h"
#include "numeric.h"
#include "s_log.h"
#include "s_serv.h"
#include "send.h"
#include "whowas.h"
#include "irc_string.h"
#include "spoof.h"
#include "s_user.h"
#include "crypt.h"
#include "hash.h"
#include "m_svsinfo.h"
#include "s_conf.h"
#include "dconf_vars.h"

int tagcount = 0;
#define MAXVIRTSIZE     (3 + 5 + 1)
#define HASHVAL_TOTAL   30011
#define HASHVAL_PARTIAL 211


void truncstring(char *stringvar, int firstlast, int amount);
int Maskchecksum (char *data, int len);
unsigned z(char *s);
void remove_dots_and_slashes(char* msg);
int str2array(char **pparv, char *string, char *delim);

/*
 * spoofed
 *
 * inputs       - client to be spoofed
 * output       - spoofed hostname
 * side effects - 
 */

char *spoofed(struct Client *acptr)
{
  switch(SpoofMethod)
   {
	  case 0:
	   return maskme(acptr->realhost);
    
	  case 1:
#ifdef IPV6
	   return maskme_myCrypt(acptr->realhost,(unsigned long)acptr->ip.s6_addr);
#else	   
	   return maskme_myCrypt(acptr->realhost, acptr->ip.s_addr);
#endif	   
   }
   return maskme(acptr->realhost);
}

#define B_BASE                  0x1000
void truncstring(char *stringvar, int firstlast, int amount){
   if (firstlast)
   {
    stringvar+=amount;
    *stringvar=0;
    stringvar-=amount;
   }
    else
   {
    stringvar+=strlen(stringvar);
    stringvar-=amount;
   }
}

int Maskchecksum (char *data, int len)
{
        int                     i;
        int                     j;

        j=0;
        for (i=0 ; i<len ; i++)
        {
          j += *data++ * (i < 16 ? (i+1)*(i+1) : i*(i-15));

        }

        return (j+1000)%0xffff;
}

unsigned z(char *s)
{
    unsigned int i;
        i = Maskchecksum (s, strlen(s));
    return i;
}

int str2array(char **pparv, char *string, char *delim)
{
    char *tok;
    int pparc = 0;

    tok = strtok(string, delim);
    while (tok != NULL) {
	pparv[pparc++] = tok;
	tok = strtok(NULL, delim);
    }

    return pparc;
}


char *maskme(char *curr)
{
    char crcstring[HOSTLEN+34];
    static char mask[HOSTLEN + 1];
    unsigned hash_total, hash_partial;
    char     destroy[HOSTLEN+1], *parv[HOSTLEN+1], *ptr, charmask[2];
    
    int      parc=0, len=0, overflow=0;

	if(!HostPrefix)
	  {
		strncpy(mask, curr, HOSTLEN);
		return mask;
	  }
    if(CryptKey)
    {
	ircsprintf(crcstring, "%s%s", CryptKey, curr);
    }
    else
    {
	if(curr[0] % 1)
	    ircsprintf(crcstring, "%s%s", my_svsinfo_ts_S, curr);
	else
	    ircsprintf(crcstring, "%s%s", curr, my_svsinfo_ts_S);
    }
      
    hash_total =  z(crcstring) % HASHVAL_TOTAL;
    strncpy(destroy, curr, HOSTLEN);

    len = strlen(destroy);

    if ((len + MAXVIRTSIZE) > HOSTLEN)
    {
        overflow = (len + MAXVIRTSIZE) - HOSTLEN;

        ptr = &destroy[overflow];
    }
    else
        ptr = &destroy[0];
    if(strchr(curr,'.'))
	charmask[0]='.';
    else if(strchr(curr,':'))
	charmask[0]=':';
    else {
    	 strncpy(mask, curr, HOSTLEN);
    	 return mask;
    }
    charmask[1]='\0';
    parc = str2array(parv, ptr, charmask);
    if (parc == 4)
    {
        len = strlen(parv[3]);

        if (IsDigit(parv[3][len-1]))
        {
            hash_partial = (z(parv[0])+z(parv[1])+z(parv[2])) % HASHVAL_PARTIAL;
            ircsprintf(mask, "%s%c%s%c%u%c%s-%u",
                parv[0], charmask[0], parv[1], charmask[0], hash_partial,
		charmask[0], HostPrefix, hash_total);
            return mask;
        }
    }
    if (parc == 2)
    {
        ircsprintf(mask, "%s-%u%c%s%c%s",
            HostPrefix, hash_total, charmask[0], parv[parc-2], charmask[0], parv[parc-1]);
        return mask;
    }
    len = strlen(parv[parc-1]);
    if (len == 2)
    {
        ircsprintf(mask, "%s-%u%c%s%c%s", /* Fixes spoofing - Lamego 1999*/
            HostPrefix, hash_total, charmask[0], parv[parc-2], charmask[0], parv[parc-1]);	
        return mask;
    }
    ircsprintf(mask, "%s-%u%c%s%c%s", HostPrefix, hash_total, charmask[0], 
    			parv[parc-2], charmask[0], parv[parc-1]);
    return mask;    
}

/* BCRYPT */
void remove_dots_and_slashes(char* msg) {
  char temp[HOSTLEN+1];
  int i,j;
  strcpy(temp, msg);
  for(i = j = 0; i < strlen(temp); i++,j++) {
    if(temp[i] == '.') {
      msg[j++] = 'p'; msg[j] = 'p';
   } else if (temp[i] == '/') {
      msg[j++] = 'b'; msg[j] = 'b';
    } else
      msg[j] = temp[i];
  }
  msg[j] = 0;
}   

/*
 * Adapted from Brasnet IRCd - Lamego
 */
char* maskme_myCrypt(char* host, long IP) 
{
#ifndef OWN_CRYPT
  extern  char *crypt();
#endif
  static char hexa[HOSTLEN+1];
  static char ret[HOSTLEN+1];
  static char salt[2] = "br";
  static char pass[8] = "87654312";
  register int i, res, hostlen;

  ret[0] = 0;

  if(HostPrefix) {
        salt[0] = HostPrefix[0];
        salt[1] = HostPrefix[1];
        salt[2] = 0;
        strncpy(pass, HostPrefix+2, 8);
        pass[8] = 0;
  }
  ircsprintf(hexa, "%X", ntohl(IP));
  for(i = 0; i < 8 && pass[i]; i++)
    hexa[i] = (pass[i] == hexa[i])?hexa[i]:pass[i]^hexa[i];
#ifdef OWN_CRYPT
  strcpy(hexa, (char *)crypt_des(hexa, salt));
#else
  strcpy(hexa, (char *)crypt(hexa, salt));
#endif
  remove_dots_and_slashes(hexa);
  hostlen = strlen(host);
  
  if(!IsDigit(host[hostlen-1]))
	{
  	  for(i=0; host[i] && host[i] != '.'; i++);
  		strcpy(ret, hexa+2);
  		if(host[i]) 
  		  {
                    res=HOSTLEN-( hostlen - i )-strlen(ret);
  		    if(res<0) 
  		    	i-=res;
		  strcat(ret, host+i);
	} 
	} 
  else 
	{
  	  strcpy(ret, hexa+2);
  	  strcat(ret, ".");
  	  strcat(ret, host);
  	  for(i = hostlen+1; i && (host[i] != '.'); i--);
  	  ret[strlen(hexa+2)+i+1] = 0;
  	  strcat(ret, ".O");
	}
	return ret;
}
 

/*
** m_newmask - Ported to PTlink6 
**      parv[0] = sender prefix
**      parv[1] = new mask (if no @ host is assumed)
**      parv[2] = target nick (optional, restricted to services)
*/
int	m_newmask(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  char    *newhost = NULL, *newuser = NULL;	
  struct Client *acptr = sptr;
  char *mename = me.name;
  static char paramhost[HOSTLEN];

  if(acptr->user && acptr->user->vlink)
    mename = acptr->user->vlink->name;
	
  if (MyClient(sptr)) 
    {
      if (!IsAnOper(sptr) && !IsAdmin(sptr) && cptr!=&me)
        {
    	  sendto_one(sptr, form_str(ERR_NOPRIVILEGES), mename, parv[0]);
    	  return -1; 
	}		
      if(!OperCanUseNewMask && cptr!=&me)
	{
	  sendto_one(sptr, form_str(ERR_NOPRIVILEGES), mename, parv[0]);
	  return -1;
	}
    } 
  else
    {
	if(IsService(sptr) && parc>2)
	  {
	    if((acptr = find_client(parv[2],NULL)) == NULL)
	      return -1;
          }
    }
    
  if (parc<2 || EmptyString(parv[1]))
    {
      if(MyClient(sptr))
        sendto_one(sptr, ":%s NOTICE %s :*** Syntax: /NEWMASK <user@host|user@|host>", mename, parv[0]);
      return 0;
    }
      
  newhost = strchr(parv[1],'@');
	  
  if(newhost)
    {
      (*newhost++) = '\0';
      newuser = parv[1];
    } 
  else 
    newhost = parv[1];
  		
  if (!EmptyString(newhost)) 
    {
/*
 parametric host spoof patch by dp:
 if the user supplies a host in the form of
   +staff.network.org
 prepend "nick." to the host.
 works also with the dconfig entries.
 if you have any questions, i can be
 reached at ptlink.10.dpx@spamgourmet.com
*/
		/* prepend "nick."? */
		if( *newhost == '+' )
		{
			if( (strlen(newhost) + strlen(parv[0])) > HOSTLEN )
			{
				sendto_one(sptr, ":%s NOTICE %s :*** Error: Hostname too long", 
					mename, parv[0]);
				return -1;
			}
			else
			{
				snprintf(paramhost, HOSTLEN, "%s.%s", parv[0],
					newhost + 1);
				
				newhost = paramhost;
			}
		}

      if (strlen(newhost) > (HOSTLEN)) 
        {
  	  sendto_one(sptr, ":%s NOTICE %s :*** Error: Hostname too long", 
	    mename, parv[0]);
    	  return -1;
      	}
  
      if (!valid_hostname(newhost)) 
	{
      	  sendto_one(sptr, ":%s NOTICE %s :*** Error: Invalid hostname", 
	    mename, parv[0]);
            return -1;
      	}
		
      strcpy(acptr->host, newhost);
    }

  if (!EmptyString(newuser)) 
    {

      if (strlen(newuser) > (USERLEN)) 
        {
  	  sendto_one(sptr, ":%s NOTICE %s :*** Error: username too long", mename, parv[0]);
    	  return -1;
      	}
      if (!valid_username(newuser)) 
	{
      	  sendto_one(sptr, ":%s NOTICE %s :*** Error: Invalid username",
	    mename, parv[0]);
            return -1;
      	}		  
       strcpy(acptr->username, newuser);
    }
    
  if(IsService(sptr) && parc>2) /* this was a remote newmask */
    sendto_serv_butone(cptr, ":%s NEWMASK %s@%s %s", sptr->name, 
      newuser ? newuser :"", newhost ? newhost :"", acptr->name);
  else
    sendto_serv_butone(cptr, ":%s NEWMASK %s@%s", parv[0], 
       newuser ? newuser :"", newhost ? newhost :"");
	  
  if (MyClient(acptr)) /* We will only send new mask notices if its a local user */
    sendto_one(acptr, ":%s NOTICE %s :*** New mask: \2%s (%s@%s)\2", mename, acptr->name,
      acptr->name, acptr->username, acptr->host);
  return 0;	    
}
  
