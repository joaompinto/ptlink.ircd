/************************************************************************
 *   IRC - Internet Relay Chat, src/hash.c
 *   Copyright (C) 1991 Darren Reed
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  $Id: hash.c,v 1.3 2005/08/27 16:23:49 jpinto Exp $
 */
#include "s_conf.h"
#include "channel.h"
#include "client.h"
#include "common.h"
#include "hash.h"
#include "irc_string.h"
#include "ircd.h"
#include "numeric.h"
#include "send.h"
#include "struct.h"
#include "s_debug.h"
#include "watch.h"
#include "list.h"
#include "path.h"
#include "m_commands.h"

#include <assert.h>
#include <fcntl.h>     /* O_RDWR ... */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

/* New hash code */
/*
 * Contributed by James L. Davis
 */

#ifdef  DEBUGMODE
static struct HashEntry* clientTable = NULL;
static struct HashEntry* channelTable = NULL;
static int clhits;
static int clmiss;
static int chhits;
static int chmiss;
#else

static struct HashEntry clientTable[U_MAX];
static struct HashEntry channelTable[CH_MAX];

#endif

struct HashEntry hash_get_channel_block(int i)
{
  return channelTable[i];
}

size_t hash_get_channel_table_size(void)
{
  return sizeof(struct HashEntry) * CH_MAX;
}

size_t hash_get_client_table_size(void)
{
  return sizeof(struct HashEntry) * U_MAX;
}

/*
 *
 * look in whowas.c for the missing ...[WW_MAX]; entry
 *   - Dianora
 */

/*
 * Hashing.
 *
 *   The server uses a chained hash table to provide quick and efficient
 * hash table mantainence (providing the hash function works evenly over
 * the input range).  The hash table is thus not susceptible to problems
 * of filling all the buckets or the need to rehash.
 *    It is expected that the hash table would look somehting like this
 * during use:
 *                   +-----+    +-----+    +-----+   +-----+
 *                ---| 224 |----| 225 |----| 226 |---| 227 |---
 *                   +-----+    +-----+    +-----+   +-----+
 *                      |          |          |
 *                   +-----+    +-----+    +-----+
 *                   |  A  |    |  C  |    |  D  |
 *                   +-----+    +-----+    +-----+
 *                      |
 *                   +-----+
 *                   |  B  |
 *                   +-----+
 *
 * A - GOPbot, B - chang, C - hanuaway, D - *.mu.OZ.AU
 *
 * The order shown above is just one instant of the server.  Each time a
 * lookup is made on an entry in the hash table and it is found, the entry
 * is moved to the top of the chain.
 *
 *    ^^^^^^^^^^^^^^^^ **** Not anymore - Dianora
 *
 */

unsigned int hash_nick_name(const char* name)
{
  unsigned int h = 0;

  while (*name)
    {
      h = (h << 4) - (h + (unsigned char)ToLower(*name++));
    }

  return(h & (U_MAX - 1));
}

/*
 * hash_channel_name
 *
 * calculate a hash value on at most the first 30 characters of the channel
 * name. Most names are short than this or dissimilar in this range. There
 * is little or no point hashing on a full channel name which maybe 255 chars
 * long.
 */
unsigned int hash_channel_name(const char* name)
{
  register int i = 30;
  unsigned int h = 0;

  while (*name && --i)
    {
      h = (h << 4) - (h + (unsigned char)ToLower(*name++));
    }

  return (h & (CH_MAX - 1));
}

/*
 * clear_client_hash_table
 *
 * Nullify the hashtable and its contents so it is completely empty.
 */
void clear_client_hash_table()
{
#ifdef        DEBUGMODE
  clhits = 0;
  clmiss = 0;
  if(!clientTable)
    clientTable = (struct HashEntry*) MyMalloc(U_MAX * 
                                               sizeof(struct HashEntry));
#endif
  memset(clientTable, 0, sizeof(struct HashEntry) * U_MAX);
}

static void clear_channel_hash_table()
{
#ifdef        DEBUGMODE
  chmiss = 0;
  chhits = 0;
  if (!channelTable)
    channelTable = (struct HashEntry*) MyMalloc(CH_MAX *
                                                sizeof(struct HashEntry));
#endif
  memset(channelTable, 0, sizeof(struct HashEntry) * CH_MAX);
}

void init_hash(void)
{
  clear_client_hash_table();
  clear_channel_hash_table();
}

/*
 * add_to_client_hash_table
 */
void add_to_client_hash_table(const char* name, struct Client* cptr)
{
  unsigned int hashv;
  assert(0 != name);
  assert(0 != cptr);

  hashv = hash_nick_name(name);
  cptr->hnext = (struct Client*) clientTable[hashv].list;
  clientTable[hashv].list = (void*) cptr;
  ++clientTable[hashv].links;
  ++clientTable[hashv].hits;
}

/*
 * add_to_channel_hash_table
 */
void add_to_channel_hash_table(const char* name, struct Channel* chptr)
{
  unsigned int hashv;
  assert(0 != name);
  assert(0 != chptr);

  hashv = hash_channel_name(name);
  chptr->hnextch = (struct Channel*) channelTable[hashv].list;
  channelTable[hashv].list = (void*) chptr;
  ++channelTable[hashv].links;
  ++channelTable[hashv].hits;
}

/*
 * del_from_client_hash_table - remove a client/server from the client
 * hash table
 */
void del_from_client_hash_table(const char* name, struct Client* cptr)
{
  struct Client* tmp;
  struct Client* prev = NULL;
  unsigned int   hashv;
  assert(0 != name);
  assert(0 != cptr);

  hashv = hash_nick_name(name);
  tmp = (struct Client*) clientTable[hashv].list;

  for ( ; tmp; tmp = tmp->hnext)
    {
      if (tmp == cptr)
        {
          if (prev)
            prev->hnext = tmp->hnext;
          else
            clientTable[hashv].list = (void*) tmp->hnext;
          tmp->hnext = NULL;

          assert(clientTable[hashv].links > 0);
          if (clientTable[hashv].links > 0)
            --clientTable[hashv].links;
          return;
        }
      prev = tmp;
    }
  Debug((DEBUG_ERROR, "%#x !in tab %s[%s] %#x %#x %#x %d %d %#x",
         cptr, cptr->name, cptr->from ? cptr->from->host : "??host",
         cptr->from, cptr->next, cptr->prev, cptr->fd, 
         cptr->status, cptr->user));
}

/*
 * del_from_channel_hash_table
 */
void del_from_channel_hash_table(const char* name, struct Channel* chptr)
{
  struct Channel* tmp;
  struct Channel* prev = NULL;
  unsigned int    hashv;
  assert(0 != name);
  assert(0 != chptr);

  hashv = hash_channel_name(name);
  tmp = (struct Channel*) channelTable[hashv].list;

  for ( ; tmp; tmp = tmp->hnextch)
    {
      if (tmp == chptr)
        {
          if (prev)
            prev->hnextch = tmp->hnextch;
          else
            channelTable[hashv].list = (void*) tmp->hnextch;
          tmp->hnextch = NULL;

          assert(channelTable[hashv].links > 0);
          if (channelTable[hashv].links > 0)
            --channelTable[hashv].links;
          return;
        }
      prev = tmp;
    }
}


/*
 * hash_find_client
 */
struct Client* hash_find_client(const char* name, struct Client* cptr)
{
  struct Client* tmp;
  unsigned int   hashv;
  assert(0 != name);

  hashv = hash_nick_name(name);
  tmp = (struct Client*) clientTable[hashv].list;
  /*
   * Got the bucket, now search the chain.
   */
  for ( ; tmp; tmp = tmp->hnext)
    if (irccmp(name, tmp->name) == 0)
      {
#ifdef        DEBUGMODE
        ++clhits;
#endif
        return tmp;
      }
#ifdef        DEBUGMODE
  ++clmiss;
#endif
return cptr;

  /*
   * If the member of the hashtable we found isnt at the top of its
   * chain, put it there.  This builds a most-frequently used order into
   * the chains of the hash table, giving speedier lookups on those nicks
   * which are being used currently.  This same block of code is also
   * used for channels and servers for the same performance reasons.
   *
   * I don't believe it does.. it only wastes CPU, lets try it and
   * see....
   *
   * - Dianora
   */
}

/*
 * Whats happening in this next loop ? Well, it takes a name like
 * foo.bar.edu and proceeds to earch for *.edu and then *.bar.edu.
 * This is for checking full server names against masks although
 * it isnt often done this way in lieu of using matches().
 *
 * Rewrote to do *.bar.edu first, which is the most likely case,
 * also made const correct
 * --Bleep
 */
static struct Client* hash_find_masked_server(const char* name)
{
  char           buf[HOSTLEN + 1];
  char*          p = buf;
  char*          s;
  struct Client* server;

  if ('*' == *name || '.' == *name)
    return 0;

  /*
   * copy the damn thing and be done with it
   */
  strncpy_irc(buf, name, HOSTLEN);
  buf[HOSTLEN] = '\0';

  while ((s = strchr(p, '.')) != 0)
    {
       *--s = '*';
      /*
       * Dont need to check IsServer() here since nicknames cant
       * have *'s in them anyway.
       */
      if ((server = hash_find_client(s, 0)))
        return server;
      p = s + 2;
    }
  return 0;
}

/*
 * hash_find_server
 */
struct Client* hash_find_server(const char* name)
{
  struct Client* tmp;
  unsigned int   hashv;

  assert(0 != name);
  hashv = hash_nick_name(name);
  tmp = (struct Client*) clientTable[hashv].list;

  for ( ; tmp; tmp = tmp->hnext)
    {
      if (!IsServer(tmp) && !IsMe(tmp))
        continue;
      if (irccmp(name, tmp->name) == 0)
        {
#ifdef        DEBUGMODE
          ++clhits;
#endif
          return tmp;
        }
    }
  
#ifndef DEBUGMODE
  return hash_find_masked_server(name);

#else /* DEBUGMODE */
  if (!(tmp = hash_find_masked_server(name)))
    ++clmiss;
  return tmp;
#endif
}

/*
 * find_channel
 */
struct Channel* hash_find_channel(const char* name, struct Channel* chptr)
{
  struct Channel*    tmp;
  unsigned int hashv;
  
  assert(0 != name);
  hashv = hash_channel_name(name);
  tmp = (struct Channel*) channelTable[hashv].list;

  for ( ; tmp; tmp = tmp->hnextch)
    if (irccmp(name, tmp->chname) == 0)
      {
#ifdef        DEBUGMODE
        ++chhits;
#endif
        return tmp;
      }
#ifdef        DEBUGMODE
  ++chmiss;
#endif
  return chptr;
}


/*
 * Rough figure of the datastructures for notify:
 *
 * NOTIFY HASH      cptr1
 *   |                |- nick1
 * nick1-|- cptr1     |- nick2
 *   |   |- cptr2                cptr3
 *   |   |- cptr3   cptr2          |- nick1
 *   |                |- nick1
 * nick2-|- cptr2     |- nick2
 *       |- cptr1
 *
 * add-to-notify-hash-table:
 * del-from-notify-hash-table:
 * hash-del-notify-list:
 * hash-check-notify:
 * hash-get-notify:
 */

static   struct Watch  *watchTable[WATCHHASHSIZE];

void  count_watch_memory(int *count, u_long *memory)
{
	int   i = WATCHHASHSIZE;
	struct Watch  *anptr;
	
	
	while (i--) {
		anptr = watchTable[i];
		while (anptr) {
			(*count)++;
			(*memory) += sizeof(struct Watch)+strlen(anptr->nick);
			anptr = anptr->hnext;
		}
	}
}

void  clear_watch_hash_table(void)
{
	   memset((char *)watchTable, '\0', sizeof(watchTable));
}


/*
 * add_to_watch_hash_table
 */
int   add_to_watch_hash_table(char* nick, struct Client  *cptr)
{
	int   hashv;
	struct Watch* anptr;
	struct SLink  *lp;
	
	
	/* Get the right bucket... */
	hashv = hash_nick_name(nick)%WATCHHASHSIZE;
	
	/* Find the right nick (header) in the bucket, or NULL... */
	if ((anptr = (struct Watch *)watchTable[hashv]))
	  while (anptr && strcasecmp(anptr->nick, nick))
		 anptr = anptr->hnext;
	
	/* If found NULL (no header for this nick), make one... */
	if (!anptr) {
		anptr = (struct Watch *)MyMalloc(sizeof(struct Watch)+strlen(nick));
		anptr->lasttime = CurrentTime;
		strcpy(anptr->nick, nick);
		
		anptr->watch = NULL;
		
		anptr->hnext = watchTable[hashv];
		watchTable[hashv] = anptr;
	}
	/* Is this client already on the watch-list? */
	if ((lp = anptr->watch))
	  while (lp && (lp->value.cptr != cptr))
		 lp = lp->next;
	
	/* No it isn't, so add it in the bucket and client addint it */
	if (!lp) {
		lp = anptr->watch;
		anptr->watch = make_link();
		anptr->watch->value.cptr = cptr;
		anptr->watch->next = lp;
				
		lp = make_link();
		lp->next = cptr->watch;
		lp->value.wptr = anptr;
		cptr->watch = lp;
		cptr->watches++;
	}
	
	return 0;
}

/*
 *  hash_check_watch
 */
int   hash_check_watch(struct Client *cptr, int reply)
{
	int   hashv;
	struct Watch  *anptr;
	struct SLink  *lp;
	
	
	/* Get us the right bucket */
	hashv = hash_nick_name(cptr->name)%WATCHHASHSIZE;
	
	/* Find the right header in this bucket */
	if ((anptr = (struct Watch *)watchTable[hashv]))
	  while (anptr && strcasecmp(anptr->nick, cptr->name))
		 anptr = anptr->hnext;
	if (!anptr)
	  return 0;   /* This nick isn't on watch */
	
	/* Update the time of last change to item */
	anptr->lasttime = CurrentTime;
	
	/* Send notifies out to everybody on the list in header */
	for (lp = anptr->watch; lp; lp = lp->next)
	  sendto_one(lp->value.cptr, form_str(reply), me.name,
					 lp->value.cptr->name, cptr->name,
					 (IsPerson(cptr)?cptr->username:"<N/A>"),
					 (IsPerson(cptr)?cptr->host:"<N/A>"),
					 anptr->lasttime, cptr->info);
	
	return 0;
}

/*
 * hash_get_watch
 */
struct Watch  *hash_get_watch(char  *name)
{
	int   hashv;
	struct Watch  *anptr;
	
	
	hashv = hash_nick_name(name)%WATCHHASHSIZE;
	
	if ((anptr = (struct Watch *)watchTable[hashv]))
	  while (anptr && strcasecmp(anptr->nick, name))
		 anptr = anptr->hnext;
	
	return anptr;
}

/*
 * del_from_watch_hash_table
 */
int   del_from_watch_hash_table(char  *nick, struct Client  *cptr)
{
	int   hashv;
	struct Watch  *anptr, *nlast = NULL;
	Link  *lp, *last = NULL;
	
	
	/* Get the bucket for this nick... */
	hashv = hash_nick_name(nick)%WATCHHASHSIZE;
	
	/* Find the right header, maintaining last-link pointer... */
	if ((anptr = (struct Watch *)watchTable[hashv]))
	  while (anptr && strcasecmp(anptr->nick, nick)) {
		  nlast = anptr;
		  anptr = anptr->hnext;
	  }
	if (!anptr)
	  return 0;   /* No such watch */
	
	/* Find this client from the list of notifies... with last-ptr. */
	if ((lp = anptr->watch))
	  while (lp && (lp->value.cptr != cptr)) {
		  last = lp;
		  lp = lp->next;
	  }
	if (!lp)
	  return 0;   /* No such client to watch */
	
	/* Fix the linked list under header, then remove the watch entry */
	if (!last)
	  anptr->watch = lp->next;
	else
	  last->next = lp->next;
	free_link(lp);
	
	/* Do the same regarding the links in client-record... */
	last = NULL;
	if ((lp = cptr->watch))
	  while (lp && (lp->value.wptr != anptr)) {
		  last = lp;
		  lp = lp->next;
	  }
	
	/*
	 * Give error on the odd case... probobly not even neccessary
	 * No error checking in ircd is unneccessary ;) -Cabal95
	 */
	if (!lp)
	  sendto_ops("WATCH debug error: del_from_watch_hash_table "
					 "found a watch entry with no client "
					 "counterpoint processing nick %s on client %s!",
					 nick, cptr->user);
	else {
		if (!last) /* First one matched */
		  cptr->watch = lp->next;
		else
		  last->next = lp->next;
		free_link(lp);
	}
	/* In case this header is now empty of notices, remove it */
	if (!anptr->watch) {
		if (!nlast)
		  watchTable[hashv] = anptr->hnext;
		else
		  nlast->hnext = anptr->hnext;
		MyFree(anptr);
	}
	
	/* Update count of notifies on nick */
	cptr->watches--;
	
	return 0;
}

/*
 * hash_del_watch_list
 */
int   hash_del_watch_list(aClient  *cptr)
{
	int   hashv;
	struct Watch  *anptr;
	Link  *np, *lp, *last;
	
	
	if (!(np = cptr->watch))
	  return 0;   /* Nothing to do */
	
	cptr->watch = NULL; /* Break the watch-list for client */
	while (np) {
		/* Find the watch-record from hash-table... */
		anptr = np->value.wptr;
		last = NULL;
		for (lp = anptr->watch; lp && (lp->value.cptr != cptr);
			  lp = lp->next)
		  last = lp;
		
		/* Not found, another "worst case" debug error */
		if (!lp)
		  sendto_ops("WATCH Debug error: hash_del_watch_list "
						 "found a WATCH entry with no table "
						 "counterpoint processing client %s!",
						 cptr->name);
		else {
			/* Fix the watch-list and remove entry */
			if (!last)
			  anptr->watch = lp->next;
			else
			  last->next = lp->next;
			free_link(lp);
			
			/*
			 * If this leaves a header without notifies,
			 * remove it. Need to find the last-pointer!
			 */
			if (!anptr->watch) {
				struct Watch  *np2, *nl;
				
				hashv = hash_nick_name(anptr->nick)%WATCHHASHSIZE;
				
				nl = NULL;
				np2 = watchTable[hashv];
				while (np2 != anptr) {
					nl = np2;
					np2 = np2->hnext;
				}
				
				if (nl)
				  nl->hnext = anptr->hnext;
				else
				  watchTable[hashv] = anptr->hnext;
				MyFree(anptr);
			}
		}
		
		lp = np; /* Save last pointer processed */
		np = np->next; /* Jump to the next pointer */
		free_link(lp); /* Free the previous */
	}
	
	cptr->watches = 0;
	
	return 0;
}

