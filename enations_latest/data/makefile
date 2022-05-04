
all:    _mif enations.dat

_mif:   _misc _effect _units _music _research _create _lang _terrain \
	_bldg _vehicle _turret _muzzle _version

_rif = misc\misc.rif effect\effect.rif units\units.rif music\music.rif \
	research\research.rif create\create.rif language\9\9.rif \
	building\building.rif terrain\terrain.rif vehicle\vehicle.rif \
	turret\turret.rif muzzle\muzzle.rif

_version:
	cd version
	nmake
	cd ..

_turret:
	cd turret
	nmake
	cd ..

_misc:
	cd misc
	nmake
	cd ..

_effect:
	cd effect
	nmake
	cd ..

_units:
	cd units
	nmake
	cd ..

_music:
	cd music
	nmake
	cd ..

_research:
	cd research
	nmake
	cd ..

_create:
	cd create
	nmake
	cd ..

_lang:
	cd language
	nmake
	cd ..

_bldg:
	cd building
	nmake
	cd ..

_terrain:
	cd terrain
	nmake
	cd ..

_vehicle:
	cd vehicle
	nmake
	cd ..

_muzzle:
	cd muzzle
	nmake
	cd ..

ENations.dat : $(_rif) data.lst
	cdf data.lst d:\ENations.dat
	
