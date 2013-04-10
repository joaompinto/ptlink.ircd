/***************************************************************** 
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2005	 * 
 *                  http://software.pt-link.net/                 *
 * 								 *
 * This program is distributed under GNU Public License		 * 
 * Please read the file COPYING for copyright information.	 * 
 *****************************************************************
* $Id: help.c,v 1.5 2005/08/27 16:23:49 jpinto Exp $           *

  File: help.c
  Desc: helpsys related routines
  Author: Lamego
*/ 
/* 
 Note: There are a lot of possible buffer overflows conditions
 on the help2html code, since this is just an add-on utility
 I will fix it later.
*/ 
#ifndef HTML
#include "config.h"
#include "m_commands.h"
#include "client.h"
#include "ircd.h"
#include "motd.h"
#include "msg.h"
#include "numeric.h"
#include "send.h"
#include "s_conf.h"
#include "s_log.h"
#include "dconf_vars.h"
#endif /* HTML */
#include "help.h"

#include <stdlib.h> 
#include <stdio.h>
#include <string.h>

HelpItem        *user_help, *oper_help, *admin_help;

#ifndef HTML
/*
 * m_functions execute protocol messages on this server:
 *
 *      cptr    is always NON-NULL, pointing to a *LOCAL* client
 *              structure (with an open socket connected!). This
 *              identifies the physical socket where the message
 *              originated (or which caused the m_function to be
 *              executed--some m_functions may call others...).
 *
 *      sptr    is the source of the message, defined by the
 *              prefix part of the message if present. If not
 *              or prefix not found, then sptr==cptr.
 *
 *              (!IsServer(cptr)) => (cptr == sptr), because
 *              prefixes are taken *only* from servers...
 *
 *              (IsServer(cptr))
 *                      (sptr == cptr) => the message didn't
 *                      have the prefix.
 *
 *                      (sptr != cptr && IsServer(sptr) means
 *                      the prefix specified servername. (?)
 *
 *                      (sptr != cptr && !IsServer(sptr) means
 *                      that message originated from a remote
 *                      user (not local).
 *
 *              combining
 *
 *              (!IsServer(sptr)) means that, sptr can safely
 *              taken as defining the target structure of the
 *              message in this server.
 *
 *      *Always* true (if 'parse' and others are working correct):
 *
 *      1)      sptr->from == cptr  (note: cptr->from == cptr)
 *
 *      2)      MyConnect(sptr) <=> sptr == cptr (e.g. sptr
 *              *cannot* be a local connection, unless it's
 *              actually cptr!). [MyConnect(x) should probably
 *              be defined as (x == x->from) --msa ]
 *
 *      parc    number of variable parameter strings (if zero,
 *              parv is allowed to be NULL)
 *
 *      parv    a NULL terminated list of parameter pointers,
 *
 *                      parv[0], sender (prefix string), if not present
 *                              this points to an empty string.
 *                      parv[1]...parv[parc-1]
 *                              pointers to additional parameters
 *                      parv[parc] == NULL, *always*
 *
 *              note:   it is guaranteed that parv[0]..parv[parc-1] are all
 *                      non-NULL pointers.
 */

/*
 * m_helpsys	by Lamego (May 2000)
 *	parv[0] = sender prefix
 *	parv[1] = help topic
 */ 
int m_helpsys(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  char** ht;
  int found=0;
  char *mename = me.name;
	  
  if(sptr->user && sptr->user->vlink)
    mename = sptr->user->vlink->name;

  
  if(	(ht=help_find(parc>1 ? parv[1] : "", user_help)) ) 
	{
	  found=1;
	  sendto_one(sptr, ":%s NOTICE %s :**************** \2USER HELP SYSTEM\2 ***************",
		  mename, sptr->name);        
		
	  while(*ht)
	    sendto_one(sptr, ":%s NOTICE %s :%s",
		   mename, sptr->name, *(ht++)); 
		   
	  sendto_one(sptr, ":%s NOTICE %s :*************************************************",
		   mename, sptr->name);
		   
	  if(HelpChan) 
	    sendto_one(sptr, ":%s NOTICE %s :For some human help please join %s", 
	 	mename, sptr->name, HelpChan);
    }
	
  if(IsAnOper(sptr) && (ht=help_find(parc>1 ? parv[1] : "",oper_help))) 
	{
	  found=1;    
	  sendto_one(sptr, ":%s NOTICE %s :**************** \2OPER HELP SYSTEM\2 ***************",
		mename, sptr->name);            
		
	  while(*ht)
		sendto_one(sptr, ":%s NOTICE %s :%s",
			mename, sptr->name, *(ht++));
			  
	  sendto_one(sptr, ":%s NOTICE %s :*************************************************",
		   mename, sptr->name);        		   		   
    } 

  if(IsAdmin(sptr) && (ht=help_find(parc>1 ? parv[1] : "",admin_help))) 
    {
  	  found=1;
	  sendto_one(sptr, ":%s NOTICE %s :**************** \2ADMIN HELP SYSTEM\2 ***************",
		   mename, sptr->name);            
		   
	  while(*ht)
	    sendto_one(sptr, ":%s NOTICE %s :%s",
		   mename, sptr->name, *(ht++));
		   
	  sendto_one(sptr, ":%s NOTICE %s :*************************************************",
		   mename, sptr->name);        
		   
    }        
	
  if(!found)
	sendto_one(sptr, ":%s NOTICE %s :Help topic not found",
         mename, sptr->name);
		 
  return 0;	    
}
#endif /* HTML */
/******************************************
    Loads a help system from a text file
 ******************************************/
int help_load(char *fn, HelpItem **hi) {
    char *hl[1024]; /* buffer to store a full item */
    char line[256]; /* one line buffer */
    char topic[32]; /* current topic */
    char *tmp;
    int	 total;    /* total lines for the current item */
    FILE *helpfile;
    HelpItem *h,*last;   /* current,last help item */    
    *hi = NULL;
	
    if(!(helpfile = fopen(fn,"rt"))) 
	  {
#ifdef HTML
		fprintf(stderr,"Could not open help file: %s\n",fn);
#else	  
	  	irclog(L_ERROR,"Could not open help file: %s",fn);
#endif	  	
		return 1;
  	  }
	  
    h = (HelpItem *) malloc(sizeof(HelpItem));
    (*hi)=h;
    last=NULL;
    topic[0]='\0';
    total=0;
    while((total<1024) && fgets(line,sizeof(line),helpfile)) {
	if ((tmp = (char *) strchr(line, '\n')))
	    *tmp = '\0';
	if ((tmp = (char *) strchr(line, '\r')))
	    *tmp = '\0';	    
	if(line[0]=='*') {
	    if(total) {
		h->topic = strdup(topic);
		h->text = (char**) malloc(sizeof(char*) * (total+1));
		h->text[total]=NULL;	/* mark end of text*/
		while(total--) {
		    h->text[total] = hl[total];
		};
		h->next = (HelpItem *) malloc(sizeof(HelpItem));
		last=h;
		h=h->next;
		total=0;
	    }
	    strncpy(topic,&line[1],sizeof(topic)); /* save topic*/
	} else 
	    hl[total++]=strdup(line);
    }
    free(h); /* Last help is *EOF ... lets discard it */
    last->next=NULL; /* mark end of list*/
    fclose(helpfile);
    return 0;
}

/******************************************
    Looks for a topic on help system
    returns help message or NULL (not found)
 ******************************************/
char** help_find(char* topic, HelpItem *hi) {
    HelpItem *h = hi;
    while(h) { 
	if(!strcasecmp(h->topic,topic)) 
	    return h->text;
	else    
	    h=h->next;
    }	    
    return NULL;
}

/***************************************************
    Free all the memory alocated to an help item
 ***************************************************/
void help_free(HelpItem **hi) {
    char** ht; 
    HelpItem *h = *hi;
    HelpItem *tofree;
    while(h) {
	free(h->topic);
	ht=h->text;
	while(*ht) {
	    free(*(ht++));
	}
	free(h->text);
	tofree = h;
	h=h->next;
	free(tofree);
    }
    (*hi)=NULL;
}

#ifdef HTML
/*
 * Convert a text string to html
 */
char* txt2html(char* txt)
{
  static char html[512];
  char *c;
  int i,i2;

  char link[32];
  i= 0;  
  html[0]='\0'; /* init string */  
  c = txt;  
  while(*c && i<sizeof(html))
    {
    	switch(*c)
    	{
    	  case '\2': /* link */
     	    link[0]='\0';
     	    ++c;i2=0;
    	    while(*c && *c!='\2' && i2<sizeof(link))
    	      link[i2++]=*(c++);
    	    link[i2]='\0';    	  	
    	    if(*c=='\2' && *link) /* we found a link */
    	      {
    	        if(*link) /* append link */
    	    	  sprintf(&html[i],"<A HREF=\"%s.html\">%s</A>", link, link);
    	    	++c;
    	    	i = strlen(html);
    	      }
    	    break;     	 
	  case '>': /* lt */
	    sprintf(&html[i],"&gt;");
	    ++c; i+=4;
	  break;
	  case '<': /* gt */
	    sprintf(&html[i],"&lt;");
	    ++c; i+=4;
	  break;
	  case ' ': /* space */
	    sprintf(&html[i],"&nbsp;");
	    ++c; i+=6;	   
	  break;
    	  case '-': /* Check if we should use a ruler */
    	    if(*(c+1)=='-' && *(c+2)=='-')
    	      {
    	        sprintf(html,"<hr size=\"2\" width=\"50%%\" align=\"left\"><br>\n");
    	        return html;
    	      }
    	  default:
    	    html[i++]=*(c++);
    	}    	  
    }
  html[i] = '\0';  
  strcat(html,"<BR>\n");
  return html;
}

/******************************************
    Save a help system in HTML format
 ******************************************/
int help2html(char *destdir, HelpItem **hi) 
{
    HelpItem *h = *hi;
    char fn[100];
    FILE *f;
    char **ht; 
    while(h) {
    	ht = h->text;
    	snprintf(fn, sizeof(fn),"%s/%s.html",
    		destdir, (h->topic[0]!='\0') ? h->topic : "index");
    	f = fopen(fn, "w"); 
    	if(!f)
    	  {
    	    perror("Creating HTML output file");
    	    return 1;
    	  }
    	printf("Generating %s\n", fn);
    	fprintf(f,"<html>\n<head>\n<title>PTlink IRCD - %s</title>\n</head>\n", h->topic);
    	fprintf(f,"<body><center><H1>%s</H1></center>\n<br><br>\n", h->topic);
    	
    	while(*ht)
    	  {
    	    fprintf(f,"%s", txt2html(*(ht++)));
    	  }
    	if(h->topic[0]!='\0')
    	  fprintf(f,"<br><br><br><center><A HREF=\"index.html\">INDEX</A></center>");
    	fprintf(f,"</body></html>\n");
    	fclose(f);
    	h=h->next;    	
    }	    
  
  return 0;    	
}


int main(int argc, char **argv)
{
  HelpItem *hi;
  if(argc!=3)
  {
    printf("Usage: %s helpfile destdir\n", argv[0]);
    return 2;
  }
  help_load(argv[1], &hi);  
  help2html(argv[2], &hi);
  return 0;
}
#endif /* HTML */
