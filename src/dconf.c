/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2005    *
 *                   http://software.pt-link.net                 *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 
  $Id:
  File: dconf.c
  Desc: general .dconf parsing 
  Author: Lamego
*/

#include "common.h"
#include "s_log.h"
#include "irc_string.h"
#include "m_commands.h"
#include "ircd.h"
#include "send.h"
#include "client.h"
#include "numeric.h"
#include "dconf.h"
#include "dconf_vars.h"
#include "channel.h"

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>

extern void		spam_words_init(const char *);

char *ServicesServer = NULL;
char *NetworkAUP = NULL;
char *NetworkName = NULL;
char *RandomHost = NULL;
char *NetworkDesc = NULL;

char *OverrideNetsplitMessage = NULL;
int HideServerOnWhois = 0;

int CheckClones = 0;
int CheckClonesLimit = 0;
int CheckClonesPeriod = 0;
int CheckClonesDelay = 0;
int CheckTargetLimit = 0;
int CheckSpamOnTarget = 0;
int HideConnectInfo = 0;
int GLineOnExcessFlood = 0;
int GLineOnSVline = 0;
int SecureModes = 0;
int HideServicesServer = 0;
int DisableLinksForUsers = 0;
int WhoisExtension = 0;
int AutoAwayIdleTime = 0;

int AdminWithDot = 1;
int HostSpoofing = 1;
int SpoofMethod = 0;
int ChanFloodTime = 0;	
int IRCopsForAll = 0;
char *TimeZone = NULL;
int AllowSetNameToEveryone = 0;

char *NoCTCP_Msg = NULL;
char *Moderated_Msg = NULL;
char *NoExternal_Msg = NULL;
char *Banned_Msg = NULL;
char *NoSpam_Msg = NULL;
char *NoFlood_Msg = NULL;
char *NoDCCSend_Msg = NULL;
char *FloodLimit_Msg = NULL;
int RestrictedChans = 0;
int LockNickChange = 0;
char *RestrictedChansMsg = NULL;


/* Oper Privileges */
int OnlyRegisteredOper = 1;
int OperCanAlwaysJoin = 0;
int OperCanUseNewMask = 0;
int OperCanAlwaysSend = 0;
int OperKickProtection = 0;
char *OperByPass = NULL;
int EnableSelfKill = 0;
int MaxOperIdleTime = 0;

/* Line reasons */
char * GLineOthersReason = NULL;
char * KLineOthersReason = NULL;

/* Gline default */
int DefaultGLineTime = 0;
char* DefaultGLineReason = NULL;

/* host spoofing masks */
char *CryptKey = NULL; 
char* HostPrefix = NULL;
char* TechAdminMask = NULL;
char* NetAdminMask	= NULL;
char* SAdminMask = NULL;
char* AdminMask = NULL;
char* OperMask = NULL;
char* LocopMask = NULL;
char* HelperMask = NULL;
char* SpamWords = NULL;
int ForceServicesAlias = 1;
char *AntiSpamExitMsg = NULL;
int AntiSpamExitTime = 0;	
char *QuitPrefix = NULL;
char *NoQuitMsg = NULL;
char *NoSpamExitMsg = NULL;
char *NoColorsQuitMsg = NULL;
char *ZombieQuitMsg = NULL;

int	IPIdentifyMode = 0;
char *QModeMsg = NULL;

/* specific channels */
char* HelpChan= NULL;

/* server specific settings */
int ServicesUseCount = 0;
int ServicesInterval = 0;
int ReverseLookup = 1;
int CheckIdentd = 0;
int AllowChanCTCP = 0;
int MaxChansPerUser = 5;
int MaxChansPerRegUser = 20;
int UseIRCNTP = 0;
char *CodePagePath = NULL;
char *CodePages = NULL;
char *AutoJoinChan = NULL;

/**
 * default-chanmode patch
 * 2005-09-18 mmarx
 */
char* DefaultChanMode = NULL;

/* Help files */
char* UserHelpFile = NULL;
char* OperHelpFile = NULL;
char* AdminHelpFile = NULL;

char* WebPass = NULL;
int AutoEnableHVC = 0;

static 
    ConfItem conf_items[] = {
	/* Network Configuration */
	DCONF_DESC( "Network Configuration"),
	DCONF_STRING(NetworkAUP, 0),
	DCONF_STRING(HelpChan, CF_REQ),
	DCONF_STRING(NetworkName, CF_REQ),
	DCONF_STRING(RandomHost, 0),
	DCONF_STRING(NetworkDesc, 0),
	DCONF_STRING(OverrideNetsplitMessage, 0),
	DCONF_SET(HideServerOnWhois, 0),
	DCONF_SET(HostSpoofing, CF_REQ),    
	DCONF_INT(SpoofMethod, 0),
	DCONF_STRING(CryptKey, 0),
	DCONF_STRING(HostPrefix, CF_REQ),
	DCONF_STRING(TechAdminMask, 0),	
	DCONF_STRING(NetAdminMask, 0),	
	DCONF_STRING(SAdminMask, 0),		
	DCONF_STRING(AdminMask, 0),			
	DCONF_STRING(OperMask, 0),				
	DCONF_STRING(LocopMask, 0),
	DCONF_STRING(HelperMask, 0),
    DCONF_SET(AdminWithDot, CF_LOCKED),
	DCONF_STRING(SpamWords, CF_REQ | CF_LOCKED),
	DCONF_SET(IRCopsForAll, 0),
	DCONF_STRING(AntiSpamExitMsg, 0),
	DCONF_TIME(AntiSpamExitTime, 0),	
	DCONF_STRING(NoSpamExitMsg, 0),
	DCONF_STRING(NoColorsQuitMsg, 0),
	DCONF_STRING(QuitPrefix, 0),
	DCONF_STRING(NoQuitMsg, 0),
	DCONF_STRING(ZombieQuitMsg, 0),
	DCONF_SET(IPIdentifyMode, 0),
	DCONF_SET(LockNickChange, 0),	
	DCONF_SET(RestrictedChans, 0),
	DCONF_STRING(RestrictedChansMsg, 0),
	DCONF_SET(SecureModes, 0),
		
	/**
	 * default-chanmode patch
	 * 2005-09-18 mmarx
	 */
	 
	DCONF_STRING(DefaultChanMode, 0),
			
	/* Cannot send messages */
	DCONF_DESC("Cannot send messages"),
	DCONF_STRING(NoCTCP_Msg, CF_REQ),
	DCONF_STRING(Moderated_Msg, CF_REQ),
	DCONF_STRING(NoExternal_Msg, CF_REQ),
	DCONF_STRING(Banned_Msg, CF_REQ),
	DCONF_STRING(NoSpam_Msg, CF_REQ),
	DCONF_STRING(NoFlood_Msg, CF_REQ),
	DCONF_STRING(NoDCCSend_Msg, CF_REQ),
	DCONF_STRING(FloodLimit_Msg, CF_REQ),    
	DCONF_STRING(QModeMsg, 0),
    
	/* Oper Privileges */
	DCONF_DESC("Oper Privileges"),
	DCONF_SET(OnlyRegisteredOper, 0),
	DCONF_SET(OperCanAlwaysJoin, 0),
	DCONF_SET(OperCanUseNewMask, 0),
	DCONF_SET(OperCanAlwaysSend, 0),
	DCONF_SET(OperKickProtection, 0),
	DCONF_STRING(OperByPass, 0),
	DCONF_SET(EnableSelfKill, 0),
	DCONF_TIME(MaxOperIdleTime, 0),
	DCONF_STRING(GLineOthersReason, 0),
	DCONF_STRING(KLineOthersReason, 0),
	DCONF_TIME(DefaultGLineTime, 0),
	DCONF_STRING(DefaultGLineReason, 0), 
    
	/* Services configuration */
	DCONF_DESC("Services configuration"),
	DCONF_STRING(ServicesServer, CF_REQ),
	DCONF_SET(ForceServicesAlias, 0),
	DCONF_INT(ServicesUseCount, 0),
	DCONF_TIME(ServicesInterval, 0),
	/* Server configuration */
	DCONF_DESC("Server configuration"),
	DCONF_SET(ReverseLookup, 0),
	DCONF_SET(CheckIdentd, 0),
	DCONF_SET(AllowChanCTCP, 0),
	DCONF_TIME(ChanFloodTime, CF_REQ),
	DCONF_STRING(TimeZone, 0),
	DCONF_SET(CheckClones, 0),
	DCONF_INT(CheckClonesLimit, 0),
	DCONF_TIME(CheckClonesPeriod, 0),
	DCONF_TIME(CheckClonesDelay, 0),
 	DCONF_SET(CheckTargetLimit, 0),
    	DCONF_SET(CheckSpamOnTarget, 0),
    	DCONF_SET(HideServicesServer, 0),
    	DCONF_TIME(GLineOnExcessFlood, 0),
	DCONF_TIME(GLineOnSVline, 0),
    	DCONF_SET(AllowSetNameToEveryone, 0),
    	DCONF_SET(DisableLinksForUsers, 0),
    	DCONF_SET(WhoisExtension, 0),
    	DCONF_TIME(AutoAwayIdleTime, 0),
	DCONF_SET(HideConnectInfo, 0),
	DCONF_INT(MaxChansPerUser, CF_LOCKED),
	DCONF_INT(MaxChansPerRegUser, CF_LOCKED),
    	DCONF_SET(UseIRCNTP, 0),
    	DCONF_STRING(CodePagePath, CF_REQ | CF_LOCKED),
    	DCONF_STRING(CodePages, CF_REQ | CF_LOCKED),
    	DCONF_STRING(AutoJoinChan, CF_LOCKED),
	/* Help files */
	DCONF_DESC("Help files"),
	DCONF_STRING(UserHelpFile,  CF_REQ | CF_LOCKED),
	DCONF_STRING(OperHelpFile,  CF_REQ | CF_LOCKED),
	DCONF_STRING(AdminHelpFile, CF_REQ | CF_LOCKED),
	DCONF_DESC("Misc"),
	DCONF_STRING(WebPass, 0),	
	DCONF_SET(AutoEnableHVC, 0),
	{ NULL,	0,	NULL, 0 }
};

/*
 * conf_find_item
 *
 * inputs       - item name
 * output       - item index
 * side effects - looks for a specific item name on the conf items list
 */
int conf_find_item(char *name) {
    int i=0;
	
    while (conf_items[i].name && strcasecmp(conf_items[i].name, name))
	  ++i;
	  
    return conf_items[i].name ? i : -1;
}

/*
 * conf_item_str
 *
 * inputs       - item index
 * output       - item value (as string)
 * side effects - returns a string with the item value
 */
char* conf_item_str(int i) {
  static char tmpbuf[512];
  int tmpval;
  
  tmpbuf[0]='0';
  
  if(!conf_items[i].name)
	return tmpbuf;
	
  if(!(conf_items[i].flags & CF_DEFINED))	
	{
	  strcpy(tmpbuf,"Undefined");
	}
  else
	{
    switch(conf_items[i].type) 
	  {	
		case CT_DESC:
		break;		
		  
		case CT_STRING:	
		  ircsprintf(tmpbuf,"\"%s\"",*(char**) conf_items[i].vptr);
	  	break;
		
		case CT_SET:
		if (*(int*)conf_items[i].vptr == -1)
		  ircsprintf(tmpbuf,"Yes");
		else
		  ircsprintf(tmpbuf,"No");
		break;
			
		case CT_TIME:
		  tmpval = *(int*) conf_items[i].vptr;
		  if(tmpval>59)
			ircsprintf(tmpbuf,"%dm",tmpval / 60);
		  else
	  		ircsprintf(tmpbuf,"%ds",tmpval);
		break;
		
		case CT_INT:
		    tmpval = *(int*) conf_items[i].vptr;
	  		ircsprintf(tmpbuf,"%i",tmpval);
		break;
		
		default:
		  strcpy(tmpbuf,"Unknown format");
  	  }	
	  
    }
	
	return tmpbuf;	
}

/*
 * conf_change_item
 *
 * inputs       - item name, new value, rehashing state
 * output       - change success
 * side effects - log any possible problem with the new setting
 */
int  conf_change_item(char *name, char* value, int rehashing) 
  {
    int tmpval;
	
    int i = conf_find_item(name);

	
    if(i<0)
	  {
		if(!rehashing)
		  irclog(L_WARN,"Ignoring unknown directive %s", name);
		return (UNKNOWN_ITEM);
	  }
	  	  
	while(IsSpace(*value)) 
		  ++value;

	/* Let's remove the surrounding "" */
	if ((*value) == '\"') 
		*(value++)=' ';	
			  
	if (*value && (value[strlen(value)-1] == '\"') )
		value[strlen(value)-1]='\0';
		  
	if ((rehashing<2) && (conf_items[i].flags & CF_REQ) && (value[0]=='\0')) /* cannot undefine */
		  return (LOCKED_ITEM);
	  	  
    if (conf_items[i].flags & CF_DEFINED) 
	  { /* this value is already defined  */
		if (rehashing) 
		  {
	  		if ((rehashing==1) && (conf_items[i].flags & CF_LOCKED)) /* item is locked */
			  return (LOCKED_ITEM);
	  		else 
			  {
				if (conf_items[i].type==CT_STRING) 
	  			  free( *(char**) conf_items[i].vptr);
	  		  }	    
		  }	
		else 
		  {
	  		irclog(L_WARN,"Ignoring duplicated directive %s", name);		
	  		return 0;
		  }
  	  }
	
    switch(conf_items[i].type) 
	  {	
		case CT_DESC:
		break;		
		  
		case CT_STRING:	
			
		  /* Let's remove the surrounding "" */
		  if ((*value) == '\"') 
			*(value++)=' ';	
				  
		  if (*value && (value[strlen(value)-1] == '\"') )
			value[strlen(value)-1]='\0';
			
		  if(value[0]=='\0')
			{
	  	  	  *(char **)conf_items[i].vptr = NULL;
			  conf_items[i].flags &= ~CF_DEFINED;
			  return 1;
			}
		  else /* save the new data */
	  		*(char **)conf_items[i].vptr = (value[0]) ? strdup(value) : NULL;
	  	break;
		
		case CT_SET:
		  if ( strcasecmp(value,"yes") == 0)
			*(int*) conf_items[i].vptr = -1;
		  else if ( strcasecmp(value,"no") == 0)
			*(int*) conf_items[i].vptr = 0;
		  else 
		  {
		  	if (!rehashing)
			  irclog(L_WARN,"Invalid value (%s) on .conf item %s, please use Yes/No", value, name);
		    return (INV_SET_ITEM);
		  }		  
		break;
			
		case CT_TIME:
	  	  tmpval = time_str(value);
	  	  if (tmpval<=0) 
			{ 
			  if (!rehashing)
		  		irclog(L_WARN,"Invalid time format on .conf item %s", name);
    		  return (INV_TIME_ITEM);
	  		} 			  
	  	  else *(int*) conf_items[i].vptr = tmpval;
		  conf_items[i].flags |= CF_DEFINED;
		break;
		  
		default:
		  *(int*) conf_items[i].vptr = atoi(value);
  	  }
	    	
	conf_items[i].flags |= CF_DEFINED;	
	
	/**
	 * default-chanmode patch
	 * 2005-09-18 mmarx
	 */
	
	if(!strcasecmp(name, "DefaultChanMode"))
	{
		init_new_cmodes();
	}
	
    return 1;
  }


/*
 * dconf_read
 *
 * inputs       - .conf filename and rehash mode
 * output       - number of errors during .conf reading
 * side effects - log any possible problems
 */
int dconf_read(char *fn, int rehashing) 
  {
    char line[512];	/* buffer to read .conf line */
    FILE *confile;
    char* directive;
    char* value;
    char* auxptr;
    int	i;
	int errors = 0; /* errors found reading conf */
	

	
    confile = fopen(fn,"rt");
    if(!confile) 
	  {
		return -1;
  	  }	
	  
  if(rehashing)
    irclog(L_NOTICE,"Reading configuration file %s (during rehash)", fn);
  else
    irclog(L_NOTICE,"Reading configuration file %s", fn);	  
    
    while (!feof(confile)) 
	  {
                line[0]='\0';
		if(fgets(line, sizeof(line), confile)==NULL)
		  break;
		  
		auxptr = line;
		
		while(IsSpace(*auxptr))
		  ++auxptr;

		directive = strtok(auxptr,"\t\n ");
		
		if( !directive || (directive[0] == '\0') || (directive[0] == '#'))
		  continue;
		  
		value = strtok(NULL,"\r\n");

		if (!value) 
		  {
	    	    irclog(L_WARN,"Missing value for directive %s reading %s", directive, fn);
			continue;
		  }
		
		while(IsSpace(*value)) 
		  ++value;
		
		if ( !strcmp(directive,".include") )
		  {
			if (dconf_read(value, rehashing) == -1)
			  {
				irclog(L_ERROR,"Error opening include file %s",value);
			  }
			  continue;
		  }
		  		  
		i = conf_change_item(directive, value, rehashing);
		

		if(i<0)
		  ++errors;

	  }
	  
	fclose(confile);
	
	if(errors)
	  {
		irclog(L_ERROR,"%s had %i errors", fn, errors);
	  }	  	
	
    return errors;	  
  }
/*
 * dconf_check
 *
 * inputs       - verbose if true errors will be printed to stderr
 * output       - 1 if valid conf, 0 otherwise
 * side effects - check if any required item is missing
 */

int dconf_check(int verbose) 
  {
	int i = 0;
	int errors = 0;
	int flags;
	while (conf_items[i].name)
	  {
		flags = conf_items[i].flags;
		if ((flags & CF_REQ) && !(flags & CF_DEFINED))
		  {
            if(verbose)
              fprintf(stderr,"Missing required directive: %s\n", conf_items[i].name);
            else
			  irclog(L_ERROR,"Missing required directive: %s", conf_items[i].name);
			++errors;
		  }
		  ++i;
	  }	  
	  
	if(errors) 
	  {
		irclog(L_ERROR,"Terminating because of %i required item(s)", errors);
		return 0;
	  }	
	  
	return 1;
  }

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
** m_dconf - general conf handling routine
**      parv[0] = sender prefix
**      parv[1] = command (list/rehash/set)
**      parv[2] = setting item  (or mask for /list)
**      parv[3] = new value (only required for set)
*/
int	m_dconf(struct Client *cptr, struct Client *sptr, int parc, char *parv[])
{
  FILE *confile;
  int errors = 0;
  int res;
  char *cmask = NULL;
  char reply[200];
  int i;
  char *item, *newval;
  
  if ( !IsAdmin(sptr) && !IsService(sptr) ) 
	{	
      sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, parv[0]);
      return 0;
	}

  if (parc < 2 || *parv[1] == '\0')
    {
	  if(MyClient(sptr))
    	  sendto_one(sptr, form_str(ERR_NEEDMOREPARAMS),
                 me.name, parv[0], "DCONF");
      return -1;
    }

	
  if ( !strcasecmp(parv[1],"rehash") )
	{
	
  	  sendto_one(sptr, ":%s NOTICE %s :Rehashing main.conf",
           me.name,parv[0]);		  	
		   
	  if( (errors = dconf_read("main.dconf", 2)) )
		{
      	  sendto_one(sptr, ":%s NOTICE %s :*** (%i) errors found",
           me.name,parv[0], errors);		  
		}
	  else
	    sendto_one(sptr, ":%s NOTICE %s :*** Rehash terminated without errors",
           me.name,parv[0]);
           
	  /**
	   * default-chanmode patch
	   * 2005-09-18 mmarx
	   */
	   
	   init_new_cmodes();           
	}
  else if ( !strcasecmp(parv[1],"list") )
	{
	  if (parc>1)
		cmask = parv[2];
		
	
	  sendto_one(sptr, ":%s NOTICE %s :---------- Start of dconf list ----------",
		  me.name,parv[0]);
		  
	  i = 0;

  	  while (conf_items[i].name)
		{
		  if (!cmask || match(cmask, conf_items[i].name))
			{
			if(conf_items[i].type == CT_DESC) 
			  {
		  		if(i)
				  sendto_one(sptr, ":%s NOTICE %s : ",
					me.name,parv[0]);			
		  		sendto_one(sptr, ":%s NOTICE %s :[ %s ]",
				  me.name,parv[0], conf_items[i].name);				  
			  }
			else
			  sendto_one(sptr, ":%s NOTICE %s :%s = %s",
				  me.name,parv[0],
				  conf_items[i].name, conf_item_str(i));
			}
		  ++i;
		}	  
	  
  	  sendto_one(sptr, ":%s NOTICE %s :---------- End of dconf list ----------",		
		  me.name,parv[0]);	  
	}
  else if ( !strcasecmp(parv[1],"set") )
	{
	  if (parc<4) 
		{
		  sendto_one(sptr, ":%s NOTICE %s :Usage DCONF SET <option> <value>",
			me.name, parv[0]);
		  return -2;
		}
		
	  item = parv[2];
	  newval = parv[3];
	  
	  if(strlen(item) > 100)
		item[100]='\0';
  	  if(strlen(newval) > 100)
		newval[100]='\0';
		
	  res = conf_change_item(item, newval, 1);
	  reply[0]='\0';
	  
	  switch(res) 
		{
		  case UNKNOWN_ITEM: 
			ircsprintf(reply,"Unknown item \2%s\2",item);
			break;
		  case LOCKED_ITEM:
			ircsprintf(reply,"Item \2%s\2 cannot be changed", item);
			break;
		  case INV_SET_ITEM: 
			ircsprintf(reply,"Invalid value \2%s\2, please use Yes/No", newval);
		  	break;
		  case INV_TIME_ITEM: 
			ircsprintf(reply,"Invalid time value \2%s\2, please use NNd|NNm|NNs", newval);
		  	break;
		  default:
		    ircsprintf(reply,"%s = \2%s\2", item, newval);		  			
			break;
		}
			  
	  irclog(L_NOTICE,"DCONF SET from %s : %s", get_client_name(sptr, REAL_IP), reply);
		  
	  if(IsService(sptr))
		sendto_serv_butone(cptr, ":%s DCONF SET %s :%s", parv[0], item, newval);
	  else
		{
		  sendto_one(sptr, ":%s NOTICE %s :%s", me.name, parv[0], reply);				
		}
	}
  else if ( !strcasecmp(parv[1],"save") )
	{
	  irclog(L_NOTICE,"DCONF SAVE from %s", get_client_name(sptr, REAL_IP));

	  if(!IsService(sptr))
		sendto_one(sptr, ":%s NOTICE %s :--- Saving dconf settings to save.dconf ",
			me.name,parv[0]);
		  
	  confile = fopen("save.dconf","w");
	  if(!confile) 
		{
			sendto_one(sptr, ":%s NOTICE %s :--- Could not open save.conf ",
				me.name,parv[0]);
			return -1;
  		}	
	  i = 0;

  	  while (conf_items[i].name)
		{
			if(conf_items[i].type == CT_DESC) 
				fprintf(confile,"\n# %s\n",conf_items[i].name);
			else if(conf_items[i].flags & CF_DEFINED)
				{
					fprintf(confile,"%s\t %s\n", conf_items[i].name, conf_item_str(i));
				}
		  ++i;
		}	  
	fprintf(confile,"# End of save.dconf\n");
	  fclose(confile);

	  if(IsService(sptr))
		sendto_serv_butone(cptr, ":%s DCONF SAVE", parv[0]);
	  else
  		sendto_one(sptr, ":%s NOTICE %s :--- File saved",		
			me.name,parv[0]);	  		
	}
  else 
	sendto_one(sptr, ":%s NOTICE %s :Usage DCONF < REHASH | SAVE | LIST [mask] | SET option value] >",
	  me.name, parv[0]);
	  
  return 0;
}
