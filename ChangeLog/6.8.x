******************************************************************
*       http://www.ptlink.net/Coders - Coders@PTlink.net         *
******************************************************************
    
  PTlink6.8.0 (24 Oct 2001)
==================================================================

  What's new ?
  ---------------------
  check_target_limit from ircu ( dconf setting CheckTargetLimit )
	block messages being sent too fast for different targets
  merged HIDE_SERVERS_IPS path from hybrid6.2 (thanks to N1ghtW)
  added OnlyRegisteredOper dconf setting
	/oper is restricted to identified nicks when services 
	are available
  added SVSADMIN raw for remote server administration tasks  
  
  What has been fixed ?
  ---------------------
  extended default short .conf with default C/N/H lines  for services
  install_ircd will only restart ircd when ircd.pid is found
  channel operators cannot deop channel administrators
  removed limit messages from ircd startup
  changed log size rotation to 100K
  svsmode code optimization
  undefined IRC_UID on config.h
  sendto_ops_butone (was sending to all +w users)

  To Do
  -----
  Fix RestrictedChans 
	(empty channels are not synchronized on netjoins)
