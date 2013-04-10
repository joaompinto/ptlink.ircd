******************************************************************
*       http://www.ptlink.net/Coders/ - Coders at PTlink.net     *
******************************************************************
* $Id: 6.17.x,v 1.3 2005/08/27 16:23:49 jpinto Exp $           *

(.1) 27 Apr 04
  What's new ?
  ---------------------
  parametric host spoofing (patch from dp)
  
  What has been fixed ?
  ---------------------
  Moved REHASH help from Admin to Oper helpsystem
  Fixed problem on SVSGUEST not chaning nick when it should (thanks to ^Stinger^)

  PTlink6.17.0 (26 Mar 04)
==================================================================
  
  What's new ?
  ---------------------
  SecureModes to disable modes for unregistered channels
  Added n - No Spoof (don't apply host spoof to users) listener option
  SETNAME (to allow a user to change it's real name after connect)
  OverrideNetsplitMessage: when enabled, will hide the real Netsplit
    message with anything that you want. Idea from VCHAT IRCd

  What has been fixed ?
  ---------------------
  StrongVlines checking now strips spaces (to catch "w w w .")
  Added a random factor to the default ptlink host masking (using my_tsinfo)
  m_newmask propagation when get a NEWMASK from Services
  Vlinks propagations on NetJoins (in fact, user->vlink propagation)
  O: samples on example.conf.* and oper pass on mkoline bug (report by Jeff)
  Stealth users can't message/notice to users
  makeconf tool updated with new options

  To Do
  -----
  Merge TKlines code from Hybrid6.3
  Fix RestrictedChans 
	(empty channels are not synchronized on netjoins)
