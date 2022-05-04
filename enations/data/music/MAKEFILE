.SUFFIXES : .raw .wav .xmi .mid


XMI =	dtmx.xmi \
		130.xmi \
		135.xmi \
		145.xmi

all:	dir11 dir22 dir22m music.rif

dir11:
	cd 11
	nmake
	cd ..

dir22:
	cd 22
	nmake
	cd ..

dir22m:
	cd 22m
	nmake
	cd ..

music.rif:	music.mif $(RAW) $(XMI)
	makeriff music.mif

.wav.raw:
	vtor $*.wav $@

.mid.xmi:
	midiform $@ $*.mid
