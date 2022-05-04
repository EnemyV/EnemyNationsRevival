# If no CPUTYPE variable is defined, then we are running on a DOS system
# so lets whack in some flags and switches to match the NTWIN32.MAK
# settings:
hcopts = -n
cc = cl
cdebug = -Zipe -Oas
cflags = -c -AM -Gsw -W3 $(cdebug)
cvars = -D$(ENV)
linkdebug = 
link = link $(linkdebug)
guiflags = /NOE /NOD
guilibs = ,,libw mlibcew,
