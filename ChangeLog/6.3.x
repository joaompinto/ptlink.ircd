******************************************************************
*       http://www.ptlink.net/Coders - Coders@PTlink.net         *
******************************************************************

(.7) 02 Jun 2001
-----------------------
  What's new ?
  ------------
  ZLINES - Block new connections from host 
	(does not close existing connections)
  zombie user on oper hack attempt (too many failed /opers)

  What has been fixed ?
  ---------------------
  newmask source on svsmode (reported by HuMPie)
  removed SIGSEGV handler (causing top frame info loss on core)
  fixed dccdeny/zombie max parameters 
	(removing the client protocol need for : on the reason)
	

(.6) 31 May 2001
---------------------
  What has been fixed ?
  ---------------------
  detailed status for IRCOPS reply (from HuMPie)
  optimized glines and fixed a bug on the realhost/host matching
  added whowas realhost trace for global opers

(.5) 25 May 2001
---------------------
  What's new ?
  ------------
  no quit messages channel model (+q) with dconf QModeMsg setting
  sent rate on HTM info (fabulous@brasnet) 
  
  What has been fixed ?
  ---------------------
  added WALLCHOPS to is_supported raw (mIRC will use /notice @# for onotice)
	(suggested by Mauritz@Brasnet)
  fatal bug on remote BAD NICK notice
  removed extra parameter on unsqline (reported by fabulous@brasnet) 
  moved REALBAN to the correct place (reported by fabulous@brasnet) 
  changed max number of channel modes from client to 6
  not found helpsys message now displays the correct topic
  defined OLD_NON_RED for non redundant channel mode changes
  documentation cleanup

(.4) 20 May 2001
---------------------
  What has been fixed ?
  ---------------------
  silence will block invites
  Removed /who common channel restriction for IRC Opers
  completed "is an oper" whois reply description
  Show masked host to local opers
  /stats C showing the correct letter case
  /IRCOPS will now detail oper/admin (fabulous@brasnet) 
  removed AKILL compatibility code (fabulous@brasnet)
  zombies cannot invite (fabulous@brasnet)
  allow CTCP for services (can be usefull for users)
  fixed services messages destination check (fabulous@brasnet)
  added explanation on "Too many targets" message for notices (fabulous@brasnet)
  fixed whois notice for spy mode
  fixed TS bug on SVSNICK (fabulous@brasnet)
  global opers can see other user modes with /mode nick (idea from fabulous@brasnet)
  fixed bug on throttle release
  
(.3) 14 Apr 2001
---------------------
  Fixed m_squit leaving services server ghost on remote services squit
	(reported by OldHawk)
  Fixed version propagation for servers with > 1 hop
  Fixed K/G-lines match for non ident usernames
	(reported by Mauritz)
	
(.2) 10 Apr 2001
---------------------
  What's new ?
  ------------
  Added WebChatPort for handling webchat user identification
  Added IPIdentifyMode to use IP instead of real hostname
	(suggested by fabulous)
  Added SQLINE support

  What has been fixed ?    
  ---------------------
  adjusted some default settings on config.h/ircd_defs.h
  glines will always match numeric IP (suggested by Mauritz)
  GLOBOPS changed to the original GLOBOPS notice
  Changed Connect/Exit notices to report masked hosts
  fixed m_watch parameter parsing (was breaking notify)
  added "The" to the welcome message (suggested by fabulous)
  changed report_glines to a more user friendly output
  removed alphanumeric start restriction from valid_username()
  fixed bug on TRACE unknown nreply string (reported by Mauritz)
  Added RPL_ENDOFWHOIS after ERR_NOSUCHNICK (suggested by fabulous)
  Services SQUIT restricted to services admins
  allow CTCPs to be sent to services even with ForceServicesAlias on
	(suggested by Mauritz)
		
  Added Brasnet IRCd compatibility (BRASLINK)

(.1) 02 Apr 2001
-----------------
  made HelperMask being set on helper setting
  added m_identify as services identify alias
  added DCONF SET propagation for services
  fixed broken m_locops when SLAVE_SERVERS was defined
     (reported by OldHawk)

  PTlink6.3.0 (31 Mar 2001)  
==================================================================
  What's new ?
  -----------
  channel modes info on /list from opers
  improved +s channels visibility for opers on 
	/list, /names, /who
  /list now supports channel mask as argument
  extended whois wildcards search functionality
  (supporting nick, user/host or server match)
  added crypt() Spoof method
  clone connections throttling (ported from PTlink5)
  
  What has been fixed ?
  ---------------------
  services cannot be kicked (noticed by Violence@PTlink.net)
  users which cannot send messages to a channel
	are not allowed to change their nick 
  users cannot changed to a banned nickname (numeric from bahamut)
  install_ircd always installs new help files
  optimized del_banid (unban for non existent bans was being sent)
  oper can always use /msg service (useful for services ping)
  only normal users could set channel mode +O
  /newmask propagation
  cleaned some tabs from helpsys (reported by Mauritz)
  +O/+A channels are kept even when empty
	
==================================================================
  To do
  -----
  &Connects
