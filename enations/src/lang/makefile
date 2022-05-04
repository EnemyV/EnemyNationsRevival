
all:	_german
#all:	_german, _italian, _french, _spanish

clean:
	del *.res
	del *.obj
	del *.exp

_german: ENLang7.dll

ENLang7.dll: dllmain.obj ENLang7.res
	link /def:lang.def /out:ENLang7.dll dllmain.obj ENLang7.res
