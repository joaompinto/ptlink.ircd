******************************************************************
*       http://www.ptlink.net/Coders - Coders@PTlink.net         *
******************************************************************

(.1) (15 Jan 2002)

  What's new ?
  ---------------------    
  Added --disable-adns on configure to use the older resolver code
  Added CONFROOM_JAVA_PORT to config.h

  What has been fixed ?
  ---------------------
  /whois now shows the masked host even for opers
  /userhost <nick> will show the real host to opers
  add prefix '.' for channel admins on /names
  Fixed missing zero on RPL_REDIR (reported by Goblin)  
  adns/setup.c - fix from Hybrid6rc5
  src/parse.c - parameter count fix from Hybrid6rc5
  
  PTlink6.10.0 (10 Jan 2002)
==================================================================

  What's new ?
  ---------------------  
  extended m_svsadmin with noconn|conn|rehash|restart|die (fabulous)
  Merged from Hybrid6.3rc4: 
	add an extra field to the c line, it's now:
    c:ip:pass:host:port:class[:bindaddr], where bindaddr is the ip to use for
    outgoing connections, and is optional.
	/stats f
  
  What has been fixed ?
  ---------------------
  removed dummy rehash options
  LTRACE is now undefined on config.h
  avoid throttle triggering on server full (Mauritz)
  force lusers recount when whe have negative client count
  SERVER_HIDE bug on m_links
  DNS lookup is now using adns library
  system zlib will be used now
  Removed some CRYPT_LINKS code
  Merged from Hybrid6.3rc4:
	Added missing CLIENT_SERVER define to config.h
	extended the host check for "."'s to also check for ":" so we allow IPv6...
    Synced up mkpasswd with my latest version (MD5 support, cleanly compiles
    on Solaris)	
	/restart now requires the local server name
	HIDE_SERVERS_IPS is now applied on get_client_name()
	m_unkline can now remove tklines
	extended 005 (is supported) reply
	005 is now sent as /VERSION reply
	
  To Do
  -----
  Merge TKlines code from Hybrid6.3
  Fix RestrictedChans 
	(empty channels are not synchronized on netjoins)
