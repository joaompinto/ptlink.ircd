******************************************************************
*       http://www.ptlink.net/Coders - Coders@PTlink.net         *
******************************************************************

  PTlink6.1.x (04 Mar 2001)
================================================================== 
channel.c
  fixed AllowChanCTCP (was blocking ACTIONs)	
dconf.c
  added DCONF SET <option> set
m_map.c
  fixed m_map layout  
network.dconf
  added missing " to LocopHost on sampe conf
	(report by ^Stinger^ <stinger at kippesoep.nl.com>)  
s_services.c
  added SVSPART
spoof.c
  NEWHOST changed to NEWMASK, now supporting username change
  (subsequently CanUseNewHost changed to CanUseNewMask)

 TO DO
------------------------------------------------------------------
  chanmodes :	+a, +q
