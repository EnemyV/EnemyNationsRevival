#include "rgbcolor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//  Given r, g, b in [ 0.0, 1.0 ], finds
//  h in [ 0, 360 ), s and l in [ 0, 1 ].
//  Note that if s == 0, h is undefined, but set
//  to 0 for numerical convenience.
void RGBColor::ToHLS( float *ph, float *pl, float *ps )
{
	assert( ph != NULL );
	assert( pl != NULL );
	assert( ps != NULL );

	float	r = _red / 255.0f;
	float	g = _green / 255.0f;
	float	b = _blue / 255.0f;

	float max = MAX( r, MAX( g, b ) );
	float min = MIN( r, MIN( g, b ) );

	*pl = ( max + min ) / 2.0f;

	//  If max == min, achromatic case.  s = 0, and h is 
	//  undefined ( set to 0 for convenience.
	if ( max == min )
	{
		*ps = 0.0f;
		*ph = 0.0f;
	}
	else
	{
		//  Calculate s.
		if ( *pl <= 0.5f )
			*ps = ( max - min ) / ( max + min );
		else 
			*ps = ( max - min ) / ( 2.0f - max - min );

		//  Calculate h.
		float delta = max - min;
		if ( r == max )
			*ph = ( g - b ) / delta;
		else if ( g == max )
			*ph = 2.0f + ( b - r ) / delta;
		else if ( b == max )
			*ph = 4.0f + ( r - g ) / delta;

		*ph = *ph * 60.0f;
		if ( *ph < 0.0f )
			*ph += 360.0f;
	}
}

float RGBColor::Value( float m1, float m2, float h )
{
	float v = 0.0f;
	
	//  These adjustments to h are necessary because the caller
	//  adds and subtracts 120 to h before calling this function,
	//  pushing h outside it's range sometimes.
	if ( h < 0.0f )
		h += 360.0f;
	if ( h > 360.0f )
		h -= 360.0f;
	
	if ( h < 60.0f )
		v = m1 + ( m2 - m1 ) * h / 60.0f;
	else if ( h < 180.0f )
		v = m2;
	else if ( h < 240.0f )
		v = m1 + ( m2 - m1 ) * ( 240 - h ) / 60.0f;
	else
		v = m1;

	return v;
}

//  Given h in [ 0.0, 360.0 ), and l and s in [ 0.0, 1.0 ],
//  finds r, g, and b in [ 0.0, 1.0 ].
void RGBColor::FromHLS( float h, float l, float s )
{
	assert( 0.0f <= h && h < 360.0f );
	assert( 0.0f <= l && l <= 1.0f );
	assert( 0.0f <= s && s <= 1.0f );

	float m2 = 0.0f;
	if ( l <= 0.5f )
		m2 = l * ( 1.0f + s );
	else
		m2 = l + s - l * s;

	float m1 = 2 * l - m2;

	if ( s == 0.0f )
		_red = _green = _blue = ( byte )( l * 255.0f );
	else
	{
		_red = ( byte )( Value( m1, m2, h + 120.0f ) * 255.0f );
		_green = ( byte )( Value( m1, m2, h ) * 255.0f );
		_blue = ( byte )( Value( m1, m2, h - 120.0f ) * 255.0f );
	}
}
