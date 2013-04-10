******************************************************************
*       http://www.ptlink.net/Coders/ - Coders at PTlink.net     *
******************************************************************
* $Id: 6.16.x,v 1.3 2005/08/27 16:23:49 jpinto Exp $           *

(.2) 11 Dec 2003

  What's new ?
  ---------------------
  tools/mkoline (easy oline generation)
  new dconf setting: GLineOnExcessFlood (openglx)

  What has been fixed ?
  ---------------------
  removed IRCOpsForAll check on /helpers
  opers can now message all clients with $* as target
  binary and pid paths on crontab/ircdchk script (repored bt semir)
  fixed some compile warnings
  MIRC_DCC_BUG_CHECK for DCCs not ending with " (reported by kil)
  MIRC_DCC_BUG_CHECK bug (reported by WarSoldier)
  zombie placed/zombie removed notice moved to imode +g
  updated the helpsys with lnotice
  updated the /info with FIZZER_CHECK/HALFOP/MIRC_DCC_BUG_CHECK
    for ircoperators
  fixed m_bots, it was showing the username and hostname in
    wrong positions.
  fixed "wrong prefix" error on /whois sent by a user on a vlink.
    this is an advice: NEVER use vlinks on remote messages! :)
  

(.1) 14 Oct 2003

  What's new ?
  ---------------------
  HideServicesServer on network.dconf (openglx)

  What has been fixed ?
  ---------------------
  added MIRC_DCC_BUG_CHECK (prevent recent mIRC DCC bug exploit)
  RAW 004 reply has been updated
  UMODE_SPY (umode +y) is back for WHOIS NOTICES (openglx)
  m_lost now count total lost users
  changed credits for openglx
  m_imode support for imodes propagation
  incorrect use of RPL_ENDOFLISTS on m_lost (fix by openglx)
  bug when using server/name matching on /whois
  imode query for other user was sent to target instead of source
  added OPER_DEFAULT_IMODES to include/config.h
  forced case match/control code striping on vlines checking
    can be disabled with DisableStrongVlines on main.dconf
  replace invalid user names with "invalid" instead of rejecting users


  PTlink6.16.0 (13 Sep 2003)
==================================================================
  
  What's new ?
  ---------------------
  GETTING_STARTED documentation (By: Rohin Koshi)
  New features increased TS_CURRENT to 7
  SVSGUEST (Idea from Unreal IRCd)
  HideConnectInfo on main.dconf
  m_lost (list lost users, can be handy for worms detection)
  real name match on m_whois (/whois :name)
  EnableSelfKill option (network.dconf)
  informantion modes separation with /imode (Unreal's snomask like)

  What has been fixed ?
  ---------------------
  Changed install directories to the standard
  Display short version for non-opers (security policy)
  Changed RPL_IRCOPS/ENDOF to a generic RPL_LISTS (CoolMaster's idea)
  m_newmask received from services

  To Do
  -----
  SETNAME (to allow a user to change it's real name after connect)
  Merge TKlines code from Hybrid6.3
  Fix RestrictedChans 
	(empty channels are not synchronized on netjoins)
