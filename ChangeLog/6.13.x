******************************************************************
*       http://www.ptlink.net/Coders/ - Coders@PTlink.net        *
******************************************************************

  PTlink6.13.0 (12 Aug 2002)
==================================================================
  
  What's new ?
  ---------------------
  CheckIdentd dconf setting
  reason parameter on SVSPART
  IRCNTP (UseIRCNTP dconf setting)
  notice channel admin on failed kicks attempts (^Stinger^)
  tools/makeconfig dialog script (^Stinger^)
  
  What has been fixed ?
  ---------------------
  Added missing check for undefined options on short /gline syntax
  Services nicks will override normal users during nick collisions
  /whois @host was matching masked host AND real host instead of OR
  Removed IsHub setting
  ReversLookup dconf setting type changed to yes/no
  dconf related conf* functions renamed to dconf*
  Removed PTS4 capability (we only support PTS4 servers now)
  cosmetic fixes and code cleanup (Lamego & ^Stinger^)
  added ts propagation on SVS* messages
  added parsing penalty on check target limit triggering
  updated supported channel modes
  improved CYGWIN compatibility
  fatal bug parsing SpamWords
  	
  To Do
  -----
  RHC documentation
  Merge TKlines code from Hybrid6.3
  Fix RestrictedChans 
	(empty channels are not synchronized on netjoins)
