******************************************************************
*       http://www.ptlink.net/Coders - Coders@PTlink.net         *
******************************************************************

(.3) 25 Mar 2001
================
  What has been fixed ?
  ---------------------
  ALLOW_DOT_IN_IDENT is now defined by default
  server version propagation
  news sender prefix (reported by violence at ptlink.net)
  "not found gline" being propagated on ungline
  incorrect username change propagation
  cleaned some OLD_TS code
  exit notice for opers will now display the real host
  services kill messages from services not be sent to +k users
  
(.2) 13 Mar 2001
================
  fixed fatal bug crashing ircd on glines from non PTS4 servers
  fixed topic setting (again)
  added missing propagation on remote server introduction

(.1) 12 Mar 2001
================
  fixed bug suppressing topic propagation

  PTlink6.2.0 (12 Mar 2001)
==================================================================
  What's new ?
  ------------
  Added NoQuitMsg, QuitPrefix and AntiSpamExitMsg to dconf
  Oper HOST spoof settings replaced with MASK spoof settings
  added topic synchronization during server linking
  
  What has been fixed ?
  ---------------------
  for security reasons, the data directory permissions are set 700
  removed TS delta checking, this raw is now used for 
	protocol compatibility verification only
  fixed services exceeded usage jam  
  sendto_ops notices being sent to normal users

  To do
  -----
  chanmodes: +q
  channel modes on /list for Opers
  
==================================================================
