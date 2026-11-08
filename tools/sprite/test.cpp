
Old Format
----------

type
w
h
bitsperpixel
# views
<views>
<view indices>

<view>
------
w
h
bitsperpixel
<anchor>
# hotspots
<hotspots>
# base
<base bitmaps>
#overlay
<overlay bitmaps>
#anim front 1
<anim front 1 bitmaps>	4 zooms
#anim front 2
<anim front 2 bitmaps>	4 zooms
#anim back 1
<anim back 1 bitmaps>	4 zooms
#anim back 2
<anim back 2 bitmaps>	4 zooms

<bitmap>
--------
id
time
x
y
w
h
len
<pixel data>


<view indices>
--------------
Same as in New Format

<hotspot>
---------
type
x
y
index

-----------------------------------------------------------------------

New format
----------

<sprite>	variable-size			
----------------------
id							
total len
<-----------.SPR starts here
type
hdr len

compression type
# views per zoom
<# superviews per zoom>

<zoom layouts>						4 zooms
<superview layouts>				# superviews per zoom x 4 zooms
<view offsets>						# views per zoom
<views>								# views per zoom
<view indices>						variable

data len
<superviews>						4 zooms (3 to 0) x # superviews per zoom

<view> variable-size									
--------------------
reserved - 128 bytes
<anchor point>						
superview index
# hotspots							
# base bitmaps						
# overlay bitmaps
# anim front 1 bitmaps
# anim front 2 bitmaps
# anim back 1 bitmaps
# anim back 2 bitmaps
<hotspots>							# hotspots
<base bitmap infos>				# base bitmaps
<overlay bitmap infos>			# overlay bitmaps
<anim front 1 bitmap infos>	# anim front 1 bitmaps
<anim front 2 bitmap infos>	# anim front 2 bitmaps
<anim back 1 bitmap infos>		# anim back 1 bitmaps
<anim back 2 bitmap infos>		# anim back 2 bitmaps

<# superviews per zoom>
-----------------------
4 for BUILDING, 1 for others

<hotspot> fixed-size	
--------------------
<hotspot points>					4 zooms
index			
type

<point>
------------
x
y

<bitmap info> fixed-size
------------------------
<reserved 8>
type
bytes-per-pixel
hold time
<rects>								4 zooms
<bitmap layouts>					4 zooms

<view indices>
--------------
<building view indices> OR
<vehicle  view indices> OR
<terrain  view indices>

<building view indices> - views for a structure sprite
-----------------------
dir x stage x layer x damage

<vehicle view indices> - views for a vehicle sprite
----------------------
dir x tilt x damage

<terrain view indices> - views for a terrain sprite (as indices into the array of actual views) 
---------------------- - -1 = this view not present
dir x damage

<zoom layout>
-------------
offset 	(from start of zoom layouts)
length

<bitmap layout>	Offsets are from the start of a superview
---------------
compressed offset
compressed length
uncompressed offset - 1-bpp
uncompressed length - 1-bpp
uncompressed offset - 2-bpp
uncompressed length - 2-bpp
uncompressed offset - 3-bpp
uncompressed length - 3-bpp
uncompressed offset - 4-bpp
uncompressed length - 4-bpp

<superview layout>
------------------
compressed offset			Offset from start of first superview
compressed length
decompressed length

<superview>
-----------
block of compressed dibs (data only)

*/

