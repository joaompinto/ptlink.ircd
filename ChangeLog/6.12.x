******************************************************************
*       http://www.ptlink.net/Coders/ - Coders@PTlink.net        *
******************************************************************


(.6) 24 May 2002

  What's new ?
  ---------------------
  /LNOTICE to send notice to local users (suggested by ^Stinger^)
  OperKickProtection on dconf settings (^Stinger^)

  What has been fixed ?
  ---------------------
  Completed 005 info (based on http://www.irc.org/tech_docs/005.html
  /list will now hide +S users count (reported by Mauritz)
  /dconf rehash was not working (reported by ^Stinger^) 
  some code cleanups (^Stinger^)
  DO_IDENTD is now undefined as default
  Fixed missing SystemTime initializations in adns (fix from andosyn for hybrid)
  Typo on OperCanAlwaysJoin dconf setting
  MAX_MULTI_MESSAGES changed to 1 (die spammers)
  MAXTARGETS changed to 10 (one more spammer kiled)
  Changed default IsHub setting to know
  Compile error with jupe channels defined (^Stinger^)

(.5) 04 May 2002

  What's new ?
  ---------------------
  Added /gline list mask
  missing configuration added to /info (^Stinger^)
  ZombieQuitMsg dconf setting (^Stinger^)

  What has been fixed ?
  ---------------------
  some code cleanups (^Stinger^)
  fastdns lookup was disable 
    reported to cause problems with adns on >30K users
    (reported by Mauritz)
  rare bug on /map count when a cached server name was changed
  removed need for server admin on /rehash 
    (suggested by ^Stinger^)
  /stats G buffer overflow with long reason 
    (reported by Mauritz)
  allow newmask to local opers, needed for LocOp mask 
    (reported by ^Stinger^)
  +z was beeing removed on any /mode
    (reported by Daniel)
  disabled broken SSL support
  zombies can't change topic (^Stinger^)


(.4) 14 Apr 2002

  What has been fixed ?
  ---------------------
  wrong numeric for banned/moderated chan nick change
      (reported by fabulous)
  missing HLC reset
      (reported by Mauritz and Cord) 


  What's new ?
  ---------------------
  REGLIST option on config.h to hide non-registered channels from /list
      (suggested by Cord)

(.3) 11 Apr 2002

  What's new ?
  ---------------------
  default chan modes (+nt) are now defined on config.h
      (suggested by Cord)
  Added Korean chars support (./configure --enable-korean)
      patch from ircu2.10.04+.H1
      Cocoja for Hangul IRC. (cocoja at florida.sarang.net)
      submited by LW-CHAT

  What has been fixed ?
  ---------------------
  deny nick change for non-ops on moderated channels
  cmode +f max value changed from 9 to 99 (suggested by  Martin Matuska)

(.2)  3 Apr 2002

  What has been fixed ?
  ---------------------
  HLC is now described and defined on config.h
  memory leak on clear_sub_mtrie() (from dancer-ircd-1.0.31)
  = reported by Mauritz =
  bug introduced with 6.12.0, 'C:' on conf file was seen as 'c'    
  deny use of chars 8-31 on channel names 
  redirect non-SSL connections on SSL port to port 6667
  = reported by fabulous =
  +nt beeing forced on non empty channels during first local join
  only show HLC info if we are not an Hub
  is_supported channel modes +l and +f should be merged
  = from W. Campbell- posted on Hybrid ML =
  avoid core dump during FreeBSD gdb detach

(.1)  11 Mar 2002

  What's new ?
  ---------------------
  Added HLC info to /info for opers (suggested by Devastator)

  What has been fixed ?
  ---------------------
  Removed "non-SSL client" notice, was flooding opers
  DCONF max parameters count
  removed modes prefixes from nicks on PRIVMSGs

  PTlink6.12.0 (02 Mar 2002)
==================================================================
  
  What's new ?
  ---------------------
  --help to display command line options
  /REDIRECT server
    redirects all users to the specified server
  SSL support for client connections
  Recent Host Cache (will cache +z and +B hosts)
  
  What has been fixed ?
  ---------------------
  DO_IDENTD is now working
  Added SAFELIST and chanmode +f to is_supported (Mauritz)
  msg.h invalid parameters count (Mauritz)
  Moved WebChatPort to a P: option
  Protected services from kicks
	
  To Do
  -----
  RHC documentation
  Merge TKlines code from Hybrid6.3
  Fix RestrictedChans 
	(empty channels are not synchronized on netjoins)
  
