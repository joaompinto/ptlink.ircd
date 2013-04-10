/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink IRC Software 1999-2005    *
 *                http://software.pt-link.net                    *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************
 
  File: conf_vars.h
  Desc: network.conf variables
  Author: Lamego@PTlink.net
  Date: Mon 15 Jan 2001 06:49:00 PM WET
*/
#ifndef include_conf_vars_h_

#define include_conf_vars_h_

#define E extern
E int ReverseLookup;
E int CheckIdentd;
E int CheckTargetLimit;
E int CheckSpamOnTarget;
E int HideConnectInfo;
E int GLineOnExcessFlood;
E int GLineOnSVline;
E int DisableLinksForUsers;

E char *ServicesServer;
E char *NetworkName;
E char *RandomHost;
E char *NetworkDesc;

E int CheckClones;
E int	CheckClonesLimit;
E int CheckClonesPeriod;
E int CheckClonesDelay;

E int AllowChanCTCP;
E int ForceServicesAlias;
E int AdminWithDot;
E char *SpamWords;
E char *CryptKey;
E int HostSpoofing;
E int SpoofMethod;
E char *NoCTCP_Msg;
E char *Moderated_Msg;
E char *NoExternal_Msg;
E char *Banned_Msg;
E char *NoSpam_Msg;
E char *NoFlood_Msg;
E char *FloodLimit_Msg;
E char *NoDCCSend_Msg;
E int LockNickChange;
E int RestrictedChans;
E char *RestrictedChansMsg;


E int ChanFloodTime;
E int IRCopsForAll;
E char *AntiSpamExitMsg;
E char *NoColorsQuitMsg;
E int AntiSpamExitTime;	
E char *NoSpamExitMsg;
E char *QuitPrefix;
E char *NoQuitMsg;
E char *ZombieQuitMsg;
E int IPIdentifyMode;
E char *TimeZone;
E int MaxChansPerUser;
E int MaxChansPerRegUser;
E int UseIRCNTP;
E char *CodePagePath;
E char *CodePages;
E char *AutoJoinChan;
E int AllowSetNameToEveryone;

/* Oper Privileges */
E int OnlyRegisteredOper;
E int OperCanAlwaysJoin;
E int OperCanUseNewMask;
E int OperCanAlwaysSend;
E int OperKickProtection;
E char* OperByPass;
E int EnableSelfKill;
E int MaxOperIdleTime;

/* Line reason */
E char* GLineOthersReason;
E char* KLineOthersReason;

/* gline defaults */
E int DefaultGLineTime;
E char *DefaultGLineReason;

/* host spoofing masks */
E char* HostPrefix;
E char* TechAdminMask;
E char* NetAdminMask;
E char* SAdminMask;
E char* AdminMask;
E char* OperMask;
E char* LocopMask;
E char* HelperMask;
E char *QModeMsg;

/* Services Protection */
E int 	ServicesUseCount;
E int	ServicesInterval;
E int	HideServicesServer;

E int 	SecureModes;
E char* NetworkAUP;
E char* OverrideNetsplitMessage;
E int	HideServerOnWhois;
E int   WhoisExtension;
E int   AutoAwayIdleTime;

/* specific channels */
E char* HelpChan;

/* Help files */
E char* UserHelpFile;
E char* OperHelpFile;
E char* AdminHelpFile;

/* Misc */
E char* WebPass;
E int AutoEnableHVC;

/**
 * default-chanmode patch
 * 2005-09-18 mmarx
 */

E char* DefaultChanMode;

#undef E

#endif /* conf_vars_h_ */
