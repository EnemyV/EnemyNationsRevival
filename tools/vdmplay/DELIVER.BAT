pkzip -aP delivery\vdmplay.zip *.cpp *.h *.mak *.rc *.def *.ico
pkzip -aP delivery\vdmplay.zip WinDebug\*.lib WinDebug\*.dll WinRel\*.lib WinRel\*.dll
pkzip -aP delivery\vdmplay.zip ut\*.cpp ut\*.h ut\*.mak ut\*.rc ut\*.def ut\*.lib ut\*.dll ut\*.ico
pkzip -aP delivery\vdmplay.zip ut\WinDebug\*.lib ut\WinDebug\*.dll
pkzip -aP delivery\vdmplay.zip ut\WinRel\*.lib ut\WinRel\*.dll
pkzip -aPr delivery\vdmplay.zip vdmplui\*.cpp vdmplui\*.h vdmplui\*.mak vdmplui\*.rc 
pkzip -aPr delivery\vdmplay.zip vdmplui\*.rc2 vdmplui\*.def vdmplui\*.lib vdmplui\*.dll
pkzip -aPr delivery\vdmplay.zip iserve\*.cpp iserve\*.h iserve\*.mak iserve\*.rc iserve\
pkzip -aPr delivery\vdmplay.zip iserve\*.rc2 iserve\*.def iserve\*.lib iserve\*.exe\*.ico
pkzip -aPr delivery\vdmplay.zip iserve\res\*.*

pkzip -aPr delivery\vdmplay.zip vpdemo\*.cpp vpdemo\*.h vpdemo\*.mak vpdemo\*.rc vpdemo\*.ico

pkzip -aPr delivery\vdmplay.zip vpdemo\*.rc2 vpdemo\*.def vpdemo\*.lib vpdemo\*.exe
pkzip -aPr delivery\vdmplay.zip vpdemo\res\*.*



pkzip -aPr delivery\vdmplay.zip vptest\vptst16\*.cpp vptest\vptst16\*.h vptest\vptst16\*.mak vptest\vptst16\*.rc 
pkzip -aPr delivery\vdmplay.zip vptest\vptst16\*.rc2 vptest\vptst16\*.def vptest\vptst16\*.lib vptest\vptst16\
pkzip -aPr delivery\vdmplay.zip vptest\vptst16\res\*.*

pkzip -aPr delivery\vdmplay.zip *.mdp
pkzip -a delivery\vdmplay.zip deliver.bat

pkzip -aPr delivery\vdmplay.zip vpdiag\*.cpp vpdiag\*.h vpdiag\*.mak vpdiag\*.rc vpdiag\*.ico

pkzip -aPr delivery\vdmplay.zip vpdiag\*.rc2 vpdiag\*.def vpdiag\*.lib vpdiag\*.exe
pkzip -aPr delivery\vdmplay.zip vpdiag\res\*.*







