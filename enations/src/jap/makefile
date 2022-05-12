
all:	_japanese

clean:
	del *.res
	del *.obj
	del *.exp

_japanese: ENLang17.dll

ENLang17.dll: dllmain.obj ENLang17.res
	link /def:lang.def /out:ENLang17.dll dllmain.obj ENLang17.res
