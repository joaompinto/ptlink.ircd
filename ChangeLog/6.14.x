******************************************************************
*       http://www.ptlink.net/Coders/ - Coders@PTlink.net        *
******************************************************************
(.5)
  What has been fixed ?
  ---------------------
  error showing ip number on throttling notice (bug #32)
  +B users could not be invited to +B channels (bug #30)
  wrong IP at the end of the server notice if a client quit (bug #24)
  removed repeated UNDLINE on msg.h (bug #23)
  added missing example for IPv6 addresses on example.conf (bug #19)
  removed +e from chanmodes helpsys (bug #18)
  fixed m_squit strange sentence (bug #15)
  IPv4 code changes should now compile on cygwin (bug #17)


(.4) 29 Mar 2003
  What's new ?
  ---------------------
  IPV6 support (./configure --enable-ipv6) based on dancer-maint5+IPv6.diff
  added "autoconf" rule to Makefile (from Hybrid7rc9)

  What has been fixed ?
  ---------------------
  fixed some compile warnins
  typo on a part of HIDE_SERVER_IP on s_serv.c (bug #6)
  Helpermask no longer overrides OperMask (bug #5)
  SQUIT for the own server crashing the ircd (bug #10)
  removed remaining +e code (bug #4)

(.3) 06 Feb 2003

  What's new ?
  ---------------------
  utility to generate html files from helpsys files (make help2html)

  What has been fixed ?
  ---------------------
  /KLINE nick will now put kline on *@host
  remote wallops were sent to normal users
  SVSMODE techadmin mask handling (^Stinger^) 
  completed and fixed services alias (^Stinger^)
  m_ircops not showing the vlink server (openglx)
  Removed '+' prefix option from /GLINE nick/chan
  WEB_TV option was not showing on /info (^Stinger^)

(.2) 07 Dec 2002
  What has been fixed ?
  ---------------------
  configure will now look for ar path
  HARD_FDLIMIT_ on config.h changed to 1000
  made some changes needed to compile on HP-UX11i
  propagation of chanadmin (+a) on SJOIN parsing
  invalid hack protection for users removing their own +a 
  when using nick/chan targets glines will now use the real host (semir)

(.1) 08 Oct 2002

  What's new ?
  ---------------------
  Added WEBTV_SUPPORT, include/config.h (suggested by Ediction IRC Network)

  What has been fixed ?
  ---------------------
  use vlink name on YOURHOST messages (openglx)
  corrupted pointers on clear_svlinks (openglx)
  buffer overflow on DCONF SET (^Stinger^)
  updated tools/makeconfig (^Stinger^)
  dconf files parsing for empty lines (Lamego)
  restricted m_operserv to opers (KayOssE)

  PTlink6.14.0 (22 Sep 2002)
==================================================================
  
  What's new ?
  ---------------------
  added GLINE_ON_POST (include/config.h)
  unicode support (adapted from forestnet ircd http://ircd.forestnet.org/)
  newmask can be used by services on any user
  virtual links (doc/vlinks.txt)
  LockNickChange: disable nick change after connect (suggested bt iMediax)

  What has been fixed ?
  ---------------------
  client password is now used to identify nick during connect
  removed RHC code (was just a dumb idea)
  message sent to stderr when resolv.conf is missing (suggested by openglx)
  updated info credits with openglx  	
  increased max. length on DCONF set options (reported by ^Stinger^)

  To Do
  -----
  Merge TKlines code from Hybrid6.3
  Fix RestrictedChans 
	(empty channels are not synchronized on netjoins)
