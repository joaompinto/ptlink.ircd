******************************************************************
*       http://www.ptlink.net/Coders - Coders@PTlink.net         *
******************************************************************

  (.3) 26 Jun 2001
  =====================
  
  What has been fixed ?
  ---------------------
  added NORES define to compile under CYGWIN
  bug on m_kick, breaking channel normal behavior
  changed MAX_MULTI_MESSAGES to 20

  (.2) 23 Jun 2001
  =====================

  What's new ?
  ------------
  channel (+K) knock mode, generate chanop notices on failed joins
  OPERKICKPROTECT option on
	( patch from mihaim@tfm.ro )
	
  What has been fixed ?
  ---------------------
  removed non CHW capability code
  added umode +p to /helpsys
  gline notice and wildcard based whois oper flood prevention
  private (+p) user mode was hidding channels from opers
  valid_username was not validating the first username char
	( reported by Mauritz@Brasnet )
    
  (.1) 16 Jun 2001
  =====================
  What's new ?
  ------------
  RestrictedChansMsg and RestrictedChansMsg settings for 
	restricted channel creation
  private (+p) user mode to hide joining channels from whois
  added REDIR numeric 10 on some rejections (from Hybrid7)
  
  What has been fixed ?
  ---------------------
  added "m" time sufix on dconf settings when needed
  case sense for | / nicks (reported by Mauritz@Brasnet)
  removed dummy sgline/svline expire checks
  added missing vline filename on DCCDENY message
  
  PTlink6.5.0 (13 Jun 2001)  
==================================================================
  What's new ?
  -----------
  NoColorsQuitMsg dconf setting to replace colored quits
  /stats X reporting missing links
  &Connects to log client connect/exit notices
  SGLINES (real name based glines)
  SVLINES (forbids filename patterns on DCC SEND)
  +N/+T set ability added on oline user modes
  services data synchronization
  
  What has been fixed ?
  ---------------------
  bug where no user could join an +k/+i channel
  (reported by Mauritz@Brasnet)
  /silence now blocks /whois notices for opers
  added zlines to the helpsys
  made some changes to ziplink settings 
	(suggested by Maurtiz@Brasnet)
	
