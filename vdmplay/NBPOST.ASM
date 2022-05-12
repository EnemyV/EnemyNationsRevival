
	.MODEL LARGE
	.286
	.CODE

	EXTRN NCBCompletion:FAR

	PUBLIC NetbiosPostRoutine

NetbiosPostRoutine PROC

	PUSHA
	
	PUSH ES
	PUSH BX

	call NCBCompletion

	POPA
	ret
NetbiosPostRoutine ENDP

	END
