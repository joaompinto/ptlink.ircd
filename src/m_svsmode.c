/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2005    *
 *                http://software.pt-link.net                    *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 
  File: m_svsmode.c
  Desc: services sent modes 
  Author: Lamego@PTlink.net
*/

#include <stdlib.h>
#include <string.h>
#include "m_commands.h" 
#include "common.h"
#include "struct.h"
#include "fdlist.h"
#include "client.h"
#include "ircd.h"
#include "channel.h"
#include "send.h"
#include "s_user.h"
#include "s_conf.h"
#include "irc_string.h"
#include "dconf_vars.h"

/*
 * m_svsmode()
 * parv[0] - sender
 * parv[1] - nick to change mode for
 * parv[2] - TS 	(optional, validated if numeric)
 * parv[3] - modes to change 
 * parv[4] - extra parameter for mode 
 */
int m_svsmode(aClient *cptr, aClient *sptr, int parc, char *parv[]) 
{
  aClient *acptr;
  int   what, setflags;
  int	flag;
  int 	extrapar;
  char 	*m;
  time_t ts=0;
  char newmask[HOSTLEN+USERLEN+2];
  
  if (!IsServer(cptr) || !IsService(sptr)) 
	{	
	  if (IsServer(cptr))
		{ 
		  ts_warn("Got SVSMODE from non-service: %s", 
			sptr->name);
		  sendto_one(cptr, ":%s WALLOPS :ignoring SVSMODE from non-service %s",
			me.name, sptr->name);
		}
	  return 0;
	}
  

  if ( parc < 4 || !IsDigit(*parv[2]) )
	{
	  int i = (++parc);
	  while ( i > 2 ) 
		{
		  parv[i] = parv[i-1];
		  --i;
		}
	  parv[2] = "0";
	}

	
  if ( parc < 4) /* missing arguments */
	{	  
	  ts_warn("Invalid SVSMODE (%s) from %s",
		(parc==2 ) ? parv[1]: "-", parv[0]);
  	  return 0;
	}

  if ((acptr = find_person(parv[1], NULL)) && MyClient(acptr)) /* person found connected here */
	{
	  
	  ts = atol(parv[2]);			  
	  if (ts && (ts != acptr->tsinfo))	/* This is not the person the mode was sent for */
		{
		  sendto_ops_imodes(IMODE_DEBUG, 
			"Unmatched SVSMODE tsinfo for %s", parv[1]); 
		  return 0;						/* just ignore it */
		}
	
	  extrapar = parc - 4; /* extra mode parameter(s) */
  	
	  setflags = acptr->umodes;	/* save current flags to generate mode change */
	  what = MODE_ADD;	
  
	  for (m = parv[3]; *m; m++)
	  switch(*m)
		{
  	  	  case '+' :
        	what = MODE_ADD;
      		break;
          case '-' :
            what = MODE_DEL;
          break;
	  case 'n':
	    if(extrapar>0 && acptr->user)
	      {
	        acptr->user->news_mask = strtoul(parv[3+extrapar],NULL,0);
		--extrapar;
	      }
	  break;
	  case 'l':
	     if(extrapar>0 && acptr->user)
	       {
	         acptr->user->lang = atoi(parv[3+extrapar]);
	         if(acptr->user->lang>MAX_LANGS || acptr->user->lang<0)
	           acptr->user->lang = 0;
	         --extrapar;
	       };
	  break;
          case 'O': case 'o' :
	        if(what == MODE_ADD)
              {
	            if(IsServer(cptr) && !IsOper(acptr))
  	              {
    	            ++Count.oper;
      	            SetOper(acptr);
        	      }
          	  }
        	else
          	  {
		      /* Only decrement the oper counts if an oper to begin with
               * found by Pat Szuta, Perly , perly@xnet.com 
               */

  	            if(!IsAnOper(acptr))
    	          break;

      	        acptr->umodes &= ~(UMODE_OPER|UMODE_LOCOP);
      	        acptr->imodes = 0;

      	    	Count.oper--;

                if (MyConnect(acptr))
	              {
  	                aClient *prev_cptr = (aClient *)NULL;
  		            aClient *cur_cptr = oper_cptr_list;

      	                fdlist_delete(acptr->fd, FDL_OPER | FDL_BUSY);
      	                if(acptr->confs && acptr->confs->value.aconf)
        	          detach_conf(acptr,acptr->confs->value.aconf);
          	        acptr->flags2 &= ~(FLAGS2_OPER_GLOBAL_KILL|
                                    FLAGS2_OPER_REMOTE|
                                    FLAGS2_OPER_UNKLINE|
                                    FLAGS2_OPER_GLINE|
                                    FLAGS2_OPER_K);
    	            while(cur_cptr)
      	              {
        	            if(acptr == cur_cptr) 
          	              {
            	            if(prev_cptr)
              	              prev_cptr->next_oper_client = cur_cptr->next_oper_client;
                	        else
                  	          oper_cptr_list = cur_cptr->next_oper_client;
                            cur_cptr->next_oper_client = (aClient *)NULL;
	                        break;
  	                      }
      	                else
        	              prev_cptr = cur_cptr;
          	            cur_cptr = cur_cptr->next_oper_client;
            	      }
                  }
	          }
  	        break;            
	        /* we may not get these,
  	         * but they shouldnt be in default
    	     */
          case ' ' :
          case '\n' :
          case '\r' :
	      case '\t' :
  	        break;
          default :
	        if( (flag = user_modes_from_c_to_bitmask[(unsigned char)*m]))
  	          {
    	        if (what == MODE_ADD)
      	          acptr->umodes |= flag;
          	    else
        	      acptr->umodes &= ~flag;  
              }
			break;
  	    } /* switch */		

     /*
	  * compare new flags with old flags and send string which
	  * will cause servers to update correctly.
  	  */
	  send_umode_out(acptr, acptr, setflags);
	  
	  newmask[0]='\0';
	  
	  if (IsNetAdmin(acptr) && NetAdminMask)
		strncpy(newmask, NetAdminMask, HOSTLEN+USERLEN+2);
	  else if (IsTechAdmin(acptr) && TechAdminMask)
		strncpy(newmask, TechAdminMask, HOSTLEN+USERLEN+2);
	  else if (IsSAdmin(acptr) && SAdminMask)
		strncpy(newmask, SAdminMask, HOSTLEN+USERLEN+2);
	  else if ((IsHelper(acptr)) && HelperMask && (!IsOper(acptr) || !OperMask))
		strncpy(newmask, HelperMask, HOSTLEN+USERLEN+2);
	  if(newmask[0])
		{
		  parv[0] = acptr->name;
		  parv[1] = newmask;
		  return m_newmask(&me, acptr, 2, parv);
		}
	  		  
	}
  else if (acptr) /* nick was found but is not our client */	
	{
  	  if ( (acptr->from != cptr)) /* this should never happen */
		sendto_one(acptr, 
		  ":%s SVSMODE %s %s %s :%s", 
			parv[0], parv[1], parv[2], parv[3],
		  (parc>4) ? parv[4] :"" );	  
	}

  return 0;
} /* m_svsmode */
