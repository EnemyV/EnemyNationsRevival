
	.MODEL LARGE
	.286
	.CODE

	EXTRN ECBCompletion:FAR

	PUBLIC ECBPostRoutine

ECBPostRoutine PROC

	PUSHA
	
	PUSH ES
	PUSH SI

	call ECBCompletion

	POPA
	ret
ECBPostRoutine ENDP

	END
