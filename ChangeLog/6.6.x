******************************************************************
*       http://www.ptlink.net/Coders - Coders@PTlink.net         *
******************************************************************
(.2) 3 Aug 2000

  What has been fixed ?
  ---------------------  
  added REALBAN to config.h (reported by Timelord)
  -= from Brasnet patch (fabulous) =-  
	can_send() will only be called on the client's server
    added time penalty on KICKed users 
	made closing link messages global
    added kill path to kill notice
	optimized services alias
    opers cannot use sqlined nicks
	fixed report_zlines output
	

(.1) 20 Jul 2001    

  What's new ?
  ---------------------
  stats z reporting last messages memory usage (chanmode +d)

  What has been fixed ?
  ---------------------
  bug on /oper crack protection (reported by Mauritz at Brasnet)
  access host on qlines were not correctly checked (reported by HuMPie)
    
  PTlink6.6.0 (16 Jul 2001)  
==================================================================
  What's new ?
  ---------------------
  added usermode +R from Dalnet Bahamut 
	(ignore non identified nicks privmsgs usermode
	Note: old umode +R for rejections was moved to +j)
  /DCONF SAVE to keep custom dconf settings
  Added MaxChansPerUser to main.dconf
  Network name on 005 raw (used by mIRC)
  T:Lines for host matched motd (from ircu)

  What has been fixed ?
  ---------------------
  added builtin crypt() for needing OSs 
	(#define OWN_CRYPT on setup.h)
  connection throttling will now use local zlines
  made spamwords detection case insensitive
  cleaned all BRASLINK code and all possible OLD_TS code
	(new protocol TS number is 5, supporting 4 for some OLD_TS)
  replaced blalloc with normal malloc/free functions

  reported by Ricardo Rodrigues <madinfo at montejunto.net>) 
  ----------------------------------------------------------
  newmask on opers will only bet set with OperCanUseNewMask YES
  updated RPL_MYINFO modes
