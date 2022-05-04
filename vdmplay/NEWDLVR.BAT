pkzip -aP delivery\vdmplay.zip *.* -x@zipexcl.txt 
pkzip -aPr delivery\vdmplay.zip ut\*.* vptest\*.*  vpdemo\*.* iserve\*.* vpdiag\*.*  -x@zipexcl.txt 
pkzip -aP delivery\vdmplay.zip WinDebug\*.lib WinDebug\*.dll WinRel\*.lib WinRel\*.dll
delete vpdemo\vdmplay.dll
pkzip -aPr delivery\vdmplay.zip ut\*.dll  vpdemo\*.exe  iserve\*.exe vpdiag\*.exe
pkzip -aP delivery\vdmplay.zip *.obj ut\*.lib










