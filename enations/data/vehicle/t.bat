cd ..\%1
pkzip %1.zip *.tga
del *.tga
pkunzip q:%1.zip
