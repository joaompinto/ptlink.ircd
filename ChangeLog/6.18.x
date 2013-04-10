******************************************************************
*       http://www.ptlink.net/Coders/ - Coders at PTlink.net     *
******************************************************************
* $Id: 6.18.x,v 1.3 2005/08/27 16:23:49 jpinto Exp $           *

(.2) 03 Oct 2004
  What's new ?
  ---------------------
  DisableLinksForUsers so admins can disable the entire LINKS command
  flood exception mode +f and /floodex (patch from dp)
  WhoisExtension so admins can make a choice if you want the WhoisExtension

  What has been fixed ?
  ---------------------
  bug #114: ForceServicesAlias can be bypassed by using nick@server
  bug #122: Able to hack ops on chans with exact channellen length
  bug #119: Compiling fails when LIMIT_UH is defined in config.h
  bug #91: Win32-cygwin must shutdown socket before close (thanks to Myzar)
  bug #95: Whois extension for realhost/usermodes, user can see his own realhost
    IRCops can see realhost and usersmodes on whois
  bug #96: HideServerOnWhois logic is swapped
  bug #89: HideServicesServer is not beeing used on /links
  bug #87: IPv4 hosts fails to resolve on IPv6 enabled IRCD
  Updated the makeconfig with the new options
  if HALFOP defined also strip the % if client does not support it
  bug #117: cleaned up some unused #ifdef things
  
(.1) 10 June 2004
  What's new ?
  ---------------------
  configure --bindir, --sysconfdir and  --localstatedir options are used now
  added --with-pidfile , --with-uid and  with-gid configure options

  What has been fixed ?
  ---------------------
  bug #85, Local operators were incorrectly propagated as global operators
  bug #84, Operators can set +s (ssl mode) from oline
  configure script will now add -lresolv when needed (suggested by duck)
  03_hurd_pathmax.patch defines PATH_MAX to compile on hurd

  PTlink6.18.0 (30 May 2004)
==================================================================

  What's new ?
  ---------------------
  Command line options -e,-l to specify etc path and var path
  Configure option (--with-chrootdir=) to setup chroot dir for ircd
  RNOTICE sends a LNOTICE to a given server (idea from Kefren)
  Language setting support (SVSMODE nick +l langid)
  Reenabled SSL support (patch from dp and common)
  HideServerOnWhois dconf option to hide server from whois reply
  Netjoin messages are now distinct by using NJOIN/NNICK
  TS_CURRENT increased to 9 (NJOIN/NNICK)
  Add_listener() logs
                                                                                
  What has been fixed ?
  ---------------------
  Updated the helpsys info
  IPv6 reverse resolution and host mask spoofing (patch from Kefren)
  Vlined words are now blocked on the quit messages
  Added improved ircd_SSL* (code from Unreal3.2/ssl.c)
  Removed obsolete command line options
  Major path options cleanup
  Decreased MAX_JOIN_LEAVE_COUNT to 5 (make sure we cath more bots)
 
  To Do
  -----
  Merge TKlines code from Hybrid6.3
  Fix RestrictedChans 
	(empty channels are not synchronized on netjoins)
