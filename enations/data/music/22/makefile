.SUFFIXES : .raw .wav .cmp

CMP = 	gen03.cmp \
		gen04.cmp \
		gen05.cmp \
		gen06.cmp \
		gen07.cmp \
		gen08.cmp \
		gen09.cmp \
		gen10.cmp \
		gen11.cmp \
		bld01.cmp \
		bld02.cmp \
		bld03.cmp \
		bld04.cmp \
		bld05.cmp \
		bld06.cmp \
		bld07.cmp \
		bld08.cmp \
		bld09.cmp \
		bld10.cmp \
		bld11.cmp \
		bld12.cmp \
		bld13.cmp \
		bld14.cmp \
		bld15.cmp \
		bld16.cmp \
		bld17.cmp \
		bld18.cmp \
		bld19.cmp \
		bld20.cmp \
		bld21.cmp \
		bld22.cmp \
		veh01.cmp \
		veh02.cmp \
		veh03.cmp \
		veh04.cmp \
		veh05.cmp \
		veh06.cmp \
		veh07.cmp \
		veh08.cmp \
		veh09.cmp \
		veh10.cmp \
		veh11.cmp \
		veh12.cmp \
		veh13.cmp \
		veh14.cmp \
		veh15.cmp \
		veh16.cmp \
		veh17.cmp \
		veh18.cmp \
		mgn03.cmp \
		rov05.cmp \
		mgn01.cmp \
		gun07.cmp \
		gun16.cmp \
		gun11.cmp \
		gun12.cmp \
		gun14.cmp \
		rkt02.cmp \
		gun08.cmp \
		gun13.cmp \
		gun15.cmp \
		mgn01.cmp \
		gun02.cmp \
		mgn02.cmp \
		sht02.cmp \
		sht03.cmp \
		sht07.cmp \
		sht06.cmp \
		sht05.cmp \
		sht04.cmp \
		sht09.cmp \
		sht10.cmp \
		sht08.cmp \
		exp13.cmp \
		exp09.cmp \
		exp11.cmp \
		exp14.cmp \
		exp05.cmp \
		spc01.cmp \
		16synth1.cmp

RAW = 	gen01.raw \
		gen02.raw \
		gen03.raw \
		gen04.raw \
		gen05.raw \
		gen06.raw \
		gen07.raw \
		gen08.raw \
		gen09.raw \
		gen10.raw \
		gen11.raw \
		bld01.raw \
		bld02.raw \
		bld03.raw \
		bld04.raw \
		bld05.raw \
		bld06.raw \
		bld07.raw \
		bld08.raw \
		bld09.raw \
		bld10.raw \
		bld11.raw \
		bld12.raw \
		bld13.raw \
		bld14.raw \
		bld15.raw \
		bld16.raw \
		bld17.raw \
		bld18.raw \
		bld19.raw \
		bld20.raw \
		bld21.raw \
		bld22.raw \
		veh01.raw \
		veh02.raw \
		veh03.raw \
		veh04.raw \
		veh05.raw \
		veh06.raw \
		veh07.raw \
		veh08.raw \
		veh09.raw \
		veh10.raw \
		veh11.raw \
		veh12.raw \
		veh13.raw \
		veh14.raw \
		veh15.raw \
		veh16.raw \
		veh17.raw \
		veh18.raw \
		mgn03.raw \
		rov05.raw \
		mgn01.raw \
		gun07.raw \
		gun16.raw \
		gun11.raw \
		gun12.raw \
		gun14.raw \
		rkt02.raw \
		gun08.raw \
		gun13.raw \
		gun15.raw \
		mgn01.raw \
		gun02.raw \
		mgn02.raw \
		sht02.raw \
		sht03.raw \
		sht07.raw \
		sht06.raw \
		sht05.raw \
		sht04.raw \
		sht09.raw \
		sht10.raw \
		sht08.raw \
		exp13.raw \
		exp09.raw \
		exp11.raw \
		exp14.raw \
		exp05.raw \
		spc01.raw \
		16synth1.raw

all:	$(RAW) $(CMP)

clean:
	del *.raw
	del *.cmp

.wav.raw:
	vtor $*.wav $@

.raw.cmp:
	codec16 -r -m $*.raw $@
