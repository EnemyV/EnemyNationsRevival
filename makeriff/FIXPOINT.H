//--------------------------------------------------------------------------
// Copyright 1995-1996 Geoff Goldberg - All Rights Reserved
// Reuse permission granted to Dave Thielen
//
//	16.16 fixed point class
//--------------------------------------------------------------------------

#ifndef __FIXPOINT_H__
#define __FIXPOINT_H__

//------------------------------- F i x e d --------------------------------
// Last resort, when compiler refuses to inline

#define FIXDIV( i1, i2 ) 		\
	_asm mov 	eax, [i1]		\
	_asm cdq				  			\
	_asm shld	edx, eax, 16	\
	_asm shl		eax, 16			\
	_asm idiv	[i2]				\
	_asm mov		[i1], eax

#define FIXMUL( i1, i2 )		\
	_asm mov 	eax, [i1]		\
	_asm imul 	[i2]				\
	_asm shrd 	eax, edx, 16	\
	_asm adc		eax, 0			\
	_asm mov		[i1], eax

struct Fixed {

	Fixed();
	Fixed( const Fixed & );
	Fixed( short iWhole );
	Fixed( short iWhole, unsigned short uFrac );
	Fixed( double );
	Fixed( int );

	operator int () const;

	void	operator = ( Fixed );
	void	operator = ( short iWhole );
	void	operator = ( double );
	void	operator = ( int );

	void	operator += ( Fixed );
	void 	operator -= ( Fixed );
	void 	operator *= ( Fixed );
	void 	operator /= ( Fixed );

	void	operator += ( int );
	void 	operator -= ( int );
	void 	operator *= ( int );
	void 	operator /= ( int );

	Fixed	operator <<  ( int );
	void	operator <<= ( int );
	Fixed	operator >>  ( int );
	void	operator >>= ( int );

	short				Floor() 		const;
	short				Ceil() 		const;
	short				Round()		const;
	unsigned	short	Frac()  		const;
	long				Value() 		const;
	float				AsFloat()	const;
	double			AsDouble()	const;
	Fixed				Fraction () const;

	long &			Value();

	friend Fixed operator +  ( Fixed, Fixed );
	friend Fixed operator -  ( Fixed, Fixed );
	friend Fixed operator -  ( Fixed );
	friend Fixed operator *  ( Fixed, Fixed );
	friend Fixed operator /  ( Fixed, Fixed );

	friend Fixed operator +  ( Fixed, int );
	friend Fixed operator -  ( Fixed, int );
	friend Fixed operator *  ( Fixed, int );
	friend Fixed operator /  ( Fixed, int );

	friend BOOL  operator == ( Fixed, Fixed );
	friend BOOL  operator != ( Fixed, Fixed );
	friend BOOL  operator >= ( Fixed, Fixed );
	friend BOOL  operator <= ( Fixed, Fixed );
	friend BOOL  operator >  ( Fixed, Fixed );
	friend BOOL  operator <  ( Fixed, Fixed );

	friend BOOL  operator == ( Fixed, int );
	friend BOOL  operator != ( Fixed, int );
	friend BOOL  operator >= ( Fixed, int );
	friend BOOL  operator <= ( Fixed, int );
	friend BOOL  operator >  ( Fixed, int );
	friend BOOL  operator <  ( Fixed, int );

	friend CArchive& operator<< (CArchive& ar, const Fixed src);
	friend CArchive& operator>> (CArchive& ar, Fixed & dest);

protected:

	Fixed( long );

	void	operator = ( long );

private:

	union
	{
		long	m_lFixed;

		struct
		{
			unsigned short	m_uFrac;
			short				m_iWhole;
		};
	};
};

//---------------------------------------------------------------------------
// Fixed::Fixed()
//---------------------------------------------------------------------------
__inline Fixed::Fixed() 
{ 
}

//---------------------------------------------------------------------------
// Fixed::Fixed()
//---------------------------------------------------------------------------
__inline Fixed::Fixed( const Fixed &rfix ) 
{
	m_lFixed = rfix.m_lFixed;
}

//---------------------------------------------------------------------------
// Fixed::Fixed()
//---------------------------------------------------------------------------
__inline Fixed::Fixed( short iWhole )
	:
		m_iWhole( iWhole ),
		m_uFrac ( 0 )
{
}

//---------------------------------------------------------------------------
// Fixed::Fixed()
//---------------------------------------------------------------------------
__inline Fixed::Fixed( short iWhole, unsigned short uFrac )
	:
		m_iWhole( iWhole ),
		m_uFrac ( uFrac  )
{
}

//---------------------------------------------------------------------------
// Fixed::Fixed()
//---------------------------------------------------------------------------
__inline Fixed::Fixed( int iSource )
	:
		m_iWhole( (short) iSource ),
		m_uFrac ( 0 )
{
}

//---------------------------------------------------------------------------
// Fixed::Fixed()
//---------------------------------------------------------------------------
__inline Fixed::Fixed( long lSource )
	:
		m_lFixed( lSource )
{
}

//---------------------------------------------------------------------------
// Fixed::Fixed()
//---------------------------------------------------------------------------
__inline Fixed::Fixed( double dSrc )
{ 
	m_lFixed = long( dSrc * 65536. ); 
}

//---------------------------------------------------------------------------
// Fixed::operator int ()
//---------------------------------------------------------------------------
__inline Fixed::operator int ( ) const
{ 
	return (m_iWhole);
}

//---------------------------------------------------------------------------
// Fixed::operator = ()
//---------------------------------------------------------------------------
__inline void Fixed::operator = ( Fixed fix )
{ 
	m_lFixed = fix.m_lFixed;
}

//---------------------------------------------------------------------------
// Fixed::operator = ()
//---------------------------------------------------------------------------
__inline void Fixed::operator = ( short iWhole )
{ 
	m_iWhole = iWhole;
	m_uFrac  = 0;
}

//---------------------------------------------------------------------------
// Fixed::operator = ()
//---------------------------------------------------------------------------
__inline void Fixed::operator = ( double d )
{ 
	m_lFixed = long( d * 65536. ); 
}

//---------------------------------------------------------------------------
// Fixed::operator = ()
//---------------------------------------------------------------------------
__inline void Fixed::operator = ( int iWhole )
{ 
	m_iWhole = (short) iWhole;
	m_uFrac  = 0;
}

//---------------------------------------------------------------------------
// Fixed::operator = ()
//---------------------------------------------------------------------------
__inline void Fixed::operator = ( long lValue )
{ 
	m_lFixed = lValue;
}

//---------------------------------------------------------------------------
// Fixed::operator += ()
//---------------------------------------------------------------------------
__inline void Fixed::operator += ( Fixed fixSrc )
{ 
	m_lFixed += fixSrc.m_lFixed; 
}

//---------------------------------------------------------------------------
// Fixed::operator -= ()
//---------------------------------------------------------------------------
__inline void Fixed::operator -= ( Fixed fixSrc )
{ 
	m_lFixed -= fixSrc.m_lFixed; 
}

//---------------------------------------------------------------------------
// Fixed::operator *= ()
//---------------------------------------------------------------------------
__inline void Fixed::operator *= ( Fixed fixRHS )
{
	long	lFixedLHS = m_lFixed;

	_asm 
	{
		mov 	eax, [lFixedLHS]
		imul 	[fixRHS]
		shrd 	eax, edx, 16
		adc	eax, 0
		mov	[lFixedLHS], eax
	}

	m_lFixed = lFixedLHS;
}


//---------------------------------------------------------------------------
// Fixed::operator /= ()
//---------------------------------------------------------------------------
__inline void Fixed::operator /= ( Fixed fixRHS )
{
	long	lFixedLHS = m_lFixed;

	_asm {
		
		mov 	eax, [lFixedLHS]
		cdq
		shld	edx, eax, 16
		shl	eax, 16
		idiv	[fixRHS]
		mov	[lFixedLHS], eax
	}

	m_lFixed = lFixedLHS;
}

//---------------------------------------------------------------------------
// Fixed::operator += ()
//---------------------------------------------------------------------------
__inline void Fixed::operator += ( int iSrc )
{ 
	m_iWhole += (short) iSrc;
}

//---------------------------------------------------------------------------
// Fixed::operator -= ()
//---------------------------------------------------------------------------
__inline void Fixed::operator -= ( int iSrc )
{ 
	m_iWhole -= (short) iSrc;
}

//---------------------------------------------------------------------------
// Fixed::operator *= ()
//---------------------------------------------------------------------------
__inline void Fixed::operator *= ( int iSrc )
{
	this->operator*= (Fixed (iSrc));
}


//---------------------------------------------------------------------------
// Fixed::operator /= ()
//---------------------------------------------------------------------------
__inline void Fixed::operator /= ( int iSrc )
{
	this->operator/= (Fixed (iSrc));
}

//---------------------------------------------------------------------------
// Fixed::operator <<
//---------------------------------------------------------------------------
__inline Fixed Fixed::operator << ( int iShift )
{
	return m_lFixed << iShift;
}

//---------------------------------------------------------------------------
// Fixed::operator <<=
//---------------------------------------------------------------------------
__inline void Fixed::operator <<= ( int iShift )
{
	m_lFixed <<= iShift;
}

//---------------------------------------------------------------------------
// Fixed::operator >>
//---------------------------------------------------------------------------
__inline Fixed Fixed::operator >> ( int iShift )
{
	return m_lFixed >> iShift;
}

//---------------------------------------------------------------------------
// Fixed::operator >>=
//---------------------------------------------------------------------------
__inline void Fixed::operator >>= ( int iShift )
{
	m_lFixed >>= iShift;
}

//---------------------------------------------------------------------------
// Fixed::Floor - 
//---------------------------------------------------------------------------
__inline short Fixed::Floor() const
{
	return m_iWhole;
}

//---------------------------------------------------------------------------
// Fixed::Ceil - 
//---------------------------------------------------------------------------
__inline short Fixed::Ceil() const
{
	return ((short) (m_iWhole + ( 0 != m_uFrac )));
}

//---------------------------------------------------------------------------
// Fixed::Round - 
//---------------------------------------------------------------------------
__inline short Fixed::Round() const
{
	return m_lFixed + 0x00008000L >> 16;
}

//---------------------------------------------------------------------------
// Fixed::Frac
//---------------------------------------------------------------------------
__inline unsigned short Fixed::Frac() const
{
	return m_uFrac;
}

//---------------------------------------------------------------------------
// Fixed::Fraction
//---------------------------------------------------------------------------
__inline Fixed Fixed::Fraction() const
{
	return Fixed (0, m_uFrac);
}

//---------------------------------------------------------------------------
// Fixed::Value
//---------------------------------------------------------------------------
__inline long Fixed::Value() const
{
	return m_lFixed;
}

//---------------------------------------------------------------------------
// Fixed::Value
//---------------------------------------------------------------------------
__inline long & Fixed::Value()
{
	return m_lFixed;
}

//---------------------------------------------------------------------------
// AsDouble()
//---------------------------------------------------------------------------
__inline double Fixed::AsDouble() const
{
	return (double) m_lFixed / 65536.0;
}

//---------------------------------------------------------------------------
// AsFloat()
//---------------------------------------------------------------------------
__inline float Fixed::AsFloat() const
{
	return (float) m_lFixed / (float) 65536.0;
}

//---------------------------------------------------------------------------
// operator + () : Binary +
//---------------------------------------------------------------------------
__inline Fixed operator + ( Fixed fixL, Fixed fixR )
{ 
	return fixL.m_lFixed + fixR.m_lFixed; 
}

//---------------------------------------------------------------------------
// operator - () : Binary -
//---------------------------------------------------------------------------
__inline Fixed operator - ( Fixed fixL, Fixed fixR )
{ 
	return fixL.m_lFixed - fixR.m_lFixed; 
}

//---------------------------------------------------------------------------
// operator - () : Unary -
//---------------------------------------------------------------------------
__inline Fixed operator - ( Fixed fix )
{ 
	return -fix.m_lFixed;
}

//---------------------------------------------------------------------------
// operator * ()
//---------------------------------------------------------------------------
__inline Fixed operator * ( Fixed fixLHS, Fixed fixRHS )
{
	long	lResult;

	_asm 
	{
		mov 	eax, [fixLHS]
		imul 	[fixRHS]
		shrd 	eax, edx, 16
		adc	eax, 0
		mov	[lResult], eax
	}

	return lResult;
}

//---------------------------------------------------------------------------
// operator / ()
//---------------------------------------------------------------------------
__inline Fixed operator / ( Fixed fixLHS, Fixed fixRHS )
{
	long	lResult;

	_asm {
		
		mov 	eax, [fixLHS]
		cdq
		shld	edx, eax, 16
		shl	eax, 16
		idiv	[fixRHS]
		mov	[lResult], eax
	}

	return lResult;
}

//---------------------------------------------------------------------------
// operator + () : Binary +
//---------------------------------------------------------------------------
__inline Fixed operator + ( Fixed fixL, int iR )
{ 
	return Fixed (fixL.m_iWhole + (short) iR, fixL.m_uFrac);
}

//---------------------------------------------------------------------------
// operator - () : Binary -
//---------------------------------------------------------------------------
__inline Fixed operator - ( Fixed fixL, int iR )
{ 
	return Fixed (fixL.m_iWhole - (short) iR, fixL.m_uFrac);
}

//---------------------------------------------------------------------------
// operator * ()
//---------------------------------------------------------------------------
__inline Fixed operator * ( Fixed fixLHS, int iRHS )
{
	return (fixLHS * Fixed (iRHS));
}

//---------------------------------------------------------------------------
// operator / ()
//---------------------------------------------------------------------------
__inline Fixed operator / ( Fixed fixLHS, int iRHS )
{
	return (fixLHS / Fixed (iRHS));
}

//---------------------------------------------------------------------------
// operator == ()
//---------------------------------------------------------------------------
__inline BOOL operator == ( Fixed fixLHS, Fixed fixRHS )
{
	return fixLHS.m_lFixed == fixRHS.m_lFixed;
}

//---------------------------------------------------------------------------
// operator != ()
//---------------------------------------------------------------------------
__inline BOOL operator != ( Fixed fixLHS, Fixed fixRHS )
{
	return fixLHS.m_lFixed != fixRHS.m_lFixed;
}

//---------------------------------------------------------------------------
// operator >= ()
//---------------------------------------------------------------------------
__inline BOOL operator >= ( Fixed fixLHS, Fixed fixRHS )
{
	return fixLHS.m_lFixed >= fixRHS.m_lFixed;
}

//---------------------------------------------------------------------------
// operator <= ()
//---------------------------------------------------------------------------
__inline BOOL operator <= ( Fixed fixLHS, Fixed fixRHS )
{
	return fixLHS.m_lFixed <= fixRHS.m_lFixed;
}

//---------------------------------------------------------------------------
// operator > ()
//---------------------------------------------------------------------------
__inline BOOL operator >  ( Fixed fixLHS, Fixed fixRHS )
{
	return fixLHS.m_lFixed > fixRHS.m_lFixed;
}

//---------------------------------------------------------------------------
// operator < ()
//---------------------------------------------------------------------------
__inline BOOL operator <  ( Fixed fixLHS, Fixed fixRHS )
{
	return fixLHS.m_lFixed < fixRHS.m_lFixed;
}

//---------------------------------------------------------------------------
// operator == ()
//---------------------------------------------------------------------------
__inline BOOL operator == ( Fixed fixLHS, int iRHS )
{
	return (fixLHS.m_iWhole == (short) iRHS) && (fixLHS.m_uFrac == 0);
}

//---------------------------------------------------------------------------
// operator != ()
//---------------------------------------------------------------------------
__inline BOOL operator != ( Fixed fixLHS, int iRHS )
{
	return (fixLHS.m_iWhole != (short) iRHS) || (fixLHS.m_uFrac != 0);
}

//---------------------------------------------------------------------------
// operator >= ()
//---------------------------------------------------------------------------
__inline BOOL operator >= ( Fixed fixLHS, int iRHS )
{
	return fixLHS >= Fixed ((short) iRHS, 0);
}

//---------------------------------------------------------------------------
// operator <= ()
//---------------------------------------------------------------------------
__inline BOOL operator <= ( Fixed fixLHS, int iRHS )
{
	return fixLHS <= Fixed ((short) iRHS, 0);
}

//---------------------------------------------------------------------------
// operator > ()
//---------------------------------------------------------------------------
__inline BOOL operator >  ( Fixed fixLHS, int iRHS )
{
	return fixLHS > Fixed ((short) iRHS, 0);
}

//---------------------------------------------------------------------------
// operator < ()
//---------------------------------------------------------------------------
__inline BOOL operator <  ( Fixed fixLHS, int iRHS )
{
	return fixLHS < Fixed ((short) iRHS, 0);
}


inline CArchive& operator<< (CArchive& ar, const Fixed src)
													{ ar << (LONG) src.m_lFixed;
														return ar; }

inline CArchive& operator>> (CArchive& ar, Fixed & dest)
													{ LONG l; ar >> l; dest.m_lFixed = l; 
														return ar; }

extern const Fixed xfixSqrtTwo;
extern const Fixed xfixPI;

#endif
