
Vdmplay.ini variables and default values


[TCP]
; Activate fragmentation and simulator
SimFrag = 0

; Simulate Flow control
SimFlow = 0

; TCP socket output buffer size in K
OBufSize=16

; How much packets we send at each time tick when trying to resend postponed data  	
MaxSends=10

; Well known socket number
WellKnownPort=1707


;Registration server address
RegistrationAddress=iserve.windward.net

;Address of the game server machine
ServerAddress=


; Address index to use on multihomed machines
AddressIndex=0



[COMM]
; Send Window size
WinSize=4

; How much times we trying to resend a non acked packet
ResendLimit=10


;BaudRate for comm/modem divided by 100
BaudRate=192


 

[VDMPLAY]
; Enable logging to the vdmplay.log file
UseLogFile=0

; Use special character encoding on comm/modem/tapi
UseEncoder=1

;Log Winsock notifications
LogWinsock=0


[NETBIOS]
; String to use as Netbios name for this station
StationName=

; Lan Adapter to use for netbios traffic
LANA=0
	

 




