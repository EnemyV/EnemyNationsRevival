#ifndef RGBCOLOR_H
#define RGBCOLOR_H

#include "common.h"

class RGBColor
{
	private :

	//  CAUTION!  Currently, there is code which relies on
	//  the order of these three variables matching the RGB
	//  order in image.h!  If you change one, change both!
	byte	_blue, _green, _red;

	float	Value( float m1, float m2, float hue );

	public :

	RGBColor() : _red( 0 ), _green( 0 ), _blue( 0 ) {}
	RGBColor( byte red, byte green, byte blue ) :
		_red( red ), _green( green ), _blue( blue ) {}
	
	int operator ==( const RGBColor &col )
	{
		if ( _red == col._red && _green == col._green && _blue == col._blue )
			return 1;

		return 0;
	}

	void	Set( byte red, byte green, byte blue )
	{
		_red = red;
		_green = green;
		_blue = blue;
	}
	byte	Red() const { return _red; }
	byte	Green() const { return _green; }
	byte	Blue() const { return _blue; }

	word	To16() const
	{
		return ( word )( ( ( _red & 0xF8 ) << 8 ) |
			( ( _green & 0xFC ) << 3 ) |
			( ( _blue & 0xF8 ) >> 3 ) );
	}
	word	To15() const
	{
		return ( word )( ( ( _red & 0xF8 ) << 7 ) |
			( ( _green & 0xF8 ) << 2 ) |
			( ( _blue & 0xF8 ) >> 3 ) );
	}

	void	FromHLS( float h, float l, float s );
	void	ToHLS( float *ph, float *pl, float *ps );
};

#endif
