******************************************************************
*       http://www.ptlink.net/Coders - Coders@PTlink.net         *
******************************************************************

(.2) 23 Dec 2001

  What has been fixed ?
  ---------------------
  removed duplicated zlib
  -- Brasnet bug reports and suggestions:
  nick change rule can_send() replaced with is_banned() 
  can_send() rules changed:
	+c,+d and +S are only checked for local users
	other modes and bans are allways checked
  bug matching nonidented users
  made services nicknames match on privmsg filter case insensitive
  svline and sgline changed to include a reason parameter
  completed STEALTH user mode
  config.h cleanup
	
(.1) 08 Dec 2001

  What's new ?
  ---------------------  
  Hebrew support with ./configure --enable-hebrew
  (from shimi <shimi at hosterz.net>)

  What has been fixed ?
  ---------------------
  Fixed glines match for real hosts
  (bug reported by DURAKOVIC Ahmedin <durakovic at nonlimit.ch>)

  PTlink6.9.0 (06 Dec 2001)
==================================================================

  What's new ?
  ---------------------  
  Stealth user mode (+S) to hide channels presence
	(can only be set from services)
  Host Prefix info exchange via CAPAB PREFIX
  /UNDLINE  to remove D:Lines, patch from 
    Adam Herscher <xref@blackened.com> 
      (http://www.einride.org/ircd/)

  What has been fixed ?
  ---------------------
  bug that could crash ircd on ping timeouts
  IRC_UID typo on ircd.c (repored by VampireMan)
  undefined HIDE_SERVERS_IPS
  moved sample configuration files to /samples
  changed some stats letters and added /stats v
  suggestions from fabulous@t7ds.com.br:
    OWN_CRYPT defined with ./configure --enable-owncrypt    
    some code optimizations
    added reason field to sv:lines/sg:lines
  

  To Do
  -----
  Fix RestrictedChans 
	(empty channels are not synchronized on netjoins)
