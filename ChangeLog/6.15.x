******************************************************************
*       http://www.ptlink.net/Coders/ - Coders@PTlink.net        *
******************************************************************
* $Id: 6.15.x,v 1.3 2005/08/27 16:23:49 jpinto Exp $           *

(.2) 31 Aug 2003

  What has been fixed ?
  ---------------------
  IRC opers will see +H opers on /ircops
  AWAY is now propagated during netjoin (thanks to Avenger)
  NO_CHANOPS_ON_SPLIT m_join bug (reported by CoolMaster)
  remote /newmask misplaced "{" (reported by dev)
  bug #80: missing masked hostname with IPv6 
  /botserv forcealias now gives correct notice
  m_who.c not compiling with SERVERHIDE (reported by Avenger)

(.1) 12 Jul 2003

  What's new ?
  ---------------------
  half-op chanmode supported config.h(HALFOP) (patch from openglx)
  oper umode +H (Hide oper status on whois from normal users) 
  status notice for Tech/Net admins on operup
  glines usage on memory stats (stats z)
  local operator reply on /whois
 

  What has been fixed ?
  ---------------------
  m_topic will now avoid redundant topic changes
  vlinks for mode and stats
  install_ircd will now correctly check if the ircd is running
  corrected oper sentence (bug #72)
  when channel is +m nicks can be change, +N is for no nickchange
  umode +H opers now arn't on /ircops (bug #71) 
  opers can now change nickname on +N channel (bug #76)

  PTlink6.15.0 (26 May 2003)
==================================================================
  
  What's new ?
  ---------------------
  Secure Network mode
  Network Description on main.dconf
  No nickname change channel mode (+N)
  FIZZER_CHECK (Fizzer worm detection and uninstall)

  What has been fixed ?
  ---------------------
  only opers/services can kick on +O chans
  only admins/services can kick on +A chans
  rehash will now reopen log file (suggested by Duck at DuckCorp.org)
  on services_on mode should use IsServicesPath instead of TS compare (TOPIC)
  DoIdentd fix, some forgotten stuff wasn't used
  Changed to a more friendly services forced nick change penalty (bug #40)
  Cleanup Several things on m_info.h
  Removed local channel jupe (+j)
  SGLINES moved to SXLINES
  Fixed memory leak on helpsys load
  Avoid +v/+a mode sets for +S users (fab)
  Restrict mode /chan -a ownnick for oper users (bug #53)
  moved log() to irclog() 

  To Do
  -----
  Merge TKlines code from Hybrid6.3
  Fix RestrictedChans 
	(empty channels are not synchronized on netjoins)
