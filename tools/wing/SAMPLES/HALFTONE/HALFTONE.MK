DEBUG=NO

!if "$(DEBUG)" == "YES"
DEF = -DDEBUG -DSTRICT -DPROFILE
CC  = cl -c -W3 -AS -G2 -GA -Zi -Od $(DEF)
LINK= link /NOE/NOD/MAP/AL:16/CO 
!else
DEF = -DSTRICT
CC  = cl -c -W3 -AS -G3 -GA -Oxwt /FPi $(DEF)
LINK= link /NOE/NOD/MAP/AL:16
!endif

NAME= halftone
RC  = rc

OBJDIR = .
OBJ  = $(OBJDIR)\$(NAME).obj $(OBJDIR)\httables.obj $(OBJDIR)\utils.obj
LIBS = libw slibcew commdlg wing mmsystem

.c{$(OBJDIR)}.obj:
  $(CC) -Fo$*.obj $<

goal: $(NAME).exe

$(NAME).exe: $(OBJ) $(NAME).res $(NAME).def $(NAME).mk
  $(LINK) @<<
    $(OBJ) $(UTL), $(NAME), $(NAME),$(LIBS), $(NAME).def
<<
    
  rc $(NAME).res
  -mapsym $(NAME).map

$(NAME).res: $(NAME).rc $(NAME).ico
  $(RC) -r $(NAME).rc

clean:
	if exist $(NAME).exe  del $(NAME).exe
	if exist *.res  del *.res
	if exist *.err  del *.err
	if exist $(OBJDIR)\*.obj  del $(OBJDIR)\*.obj
	if exist *.map  del *.map
	if exist *.sym  del *.sym
	if exist *.cod  del *.cod
	if exist *.pdb  del *.pdb


$(OBJDIR)\utils.obj: ..\utils\utils.c ..\utils\utils.h
	$(CC) -Fo$*.obj ..\utils\utils.c
	

