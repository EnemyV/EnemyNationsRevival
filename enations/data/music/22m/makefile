.SUFFIXES : .raw .wav .cmp

CMP = 	creditlp.cmp \
		credits1.cmp \
		cue120a.cmp \
		cue120b.cmp \
		cue130.cmp \
		cue135.cmp \
		cue145.cmp \
		losegame.cmp \
		mx01aif.cmp \
		wingame.cmp 

RAW = 	creditlp.raw \
		credits1.raw \
		cue120a.raw \
		cue120b.raw \
		cue130.raw \
		cue135.raw \
		cue145.raw \
		losegame.raw \
		mx01aif.raw \
		wingame.raw 


all:	$(RAW) $(CMP)

clean:
	del *.raw
	del *.cmp

.wav.raw:
	vtor $*.wav $@

.raw.cmp:
	codec16 -r $*.raw $@
