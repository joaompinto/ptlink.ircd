******************************************************************
*       http://www.ptlink.net/Coders - Coders@PTlink.net         *
******************************************************************
    
  PTlink6.7.0 (05 Oct 2001)
==================================================================

  What's new ?
  ---------------------
  added auto log rotation on ircd startup when log file size > 100K
  /ungline ALL to clear all glines
  
  What has been fixed ?
  ---------------------
  fixed fatal bug on remote /stats l 
	(reported by Robert Jameson <rj@open-net.org>)
  gline match on some cases (no ident / numeric ip)
  SJOIN '.' support for chan admin mode synchronization
  merged fixes from hybrid6.2rc1
  ircd now accepts -s (silent mode startup)
