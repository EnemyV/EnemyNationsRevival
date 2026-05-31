//--------------------------------------------------------------------------
// Copyright 1995-1996 ChromeOcean Software - All Rights Reserved
// Reuse permission granted to Dave Thielen
//
// 16.16 fixed point class
//--------------------------------------------------------------------------

#ifndef __FIXPOINT_H__
#define __FIXPOINT_H__

#include "thielen.h"

//---------------------------------------------------------------------------
//
// Copyright (c) 1995, 1996. Windward Studios, Inc.
// All Rights Reserved.
//
//---------------------------------------------------------------------------


//------------------------------- F i x e d --------------------------------
// In-place 16.16 fixed-point ops. These were x86 inline-asm macros (64-bit
// imul/idiv via edx:eax); rewritten as portable expressions over `long long`
// so they build on x64 (MSVC has no inline asm there). Every call site passes
// an lvalue as i1 and the result is stored back into it, matching the asm.
//   FIXMUL:    i1 = (i1 * i2) >> 16, round-half-up (the +0x8000 reproduces the
//              asm's shrd/adc carry rounding for both signs)
//   FIXDIV:    i1 = (i1 << 16) / i2          (idiv truncates toward zero == C++)
//   FIXMULDIV: i1 = (i1 * i2) / i3           (full 64-bit product, then divide)

#define FIXDIV( i1, i2 )        ( (i1) = (long)( ( (long long)(i1) << 16 ) / (long long)(i2) ) )

#define FIXMUL( i1, i2 )        ( (i1) = (long)( ( (long long)(i1) * (long long)(i2) + 0x8000 ) >> 16 ) )

#define FIXMULDIV( i1, i2, i3 ) ( (i1) = (long)( ( (long long)(i1) * (long long)(i2) ) / (long long)(i3) ) )

struct Fixed {

    Fixed();
    Fixed( const Fixed & );
    Fixed( short iWhole );
    Fixed( short iWhole, unsigned short uFrac );
    Fixed( double );
    Fixed( int );

    operator int () const;

    void operator = ( Fixed );
    void operator = ( short iWhole );
    void operator = ( double );
    void operator = ( int );

    void operator += ( Fixed );
    void  operator -= ( Fixed );
    void  operator *= ( Fixed );
    void  operator /= ( Fixed );

    void operator += ( int );
    void  operator -= ( int );
    void  operator *= ( int );
    void  operator /= ( int );

    Fixed operator <<  ( int );
    void operator <<= ( int );
    Fixed operator >>  ( int );
    void operator >>= ( int );

    short    Floor()   const;
    short    Ceil()   const;
    short    Round()  const;
    unsigned short Frac()    const;
    long    Value()   const;
    float    AsFloat() const;
    double   AsDouble() const;
    Fixed    Fraction () const;

    long &   Value();
    void    Value( long lValue );

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

    void operator = ( long );

private:

    union
    {
        long m_lFixed;

        struct
        {
            unsigned short  m_uFrac;
            short           m_iWhole;
        };
    };
};

//---------------------------------------------------------------------------
// Fixed::Fixed()
//---------------------------------------------------------------------------
__inline Fixed::Fixed() 
{ 
    m_lFixed = 0L;
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
        m_uFrac ( 0 ),
        m_iWhole( iWhole )
{
}

//---------------------------------------------------------------------------
// Fixed::Fixed()
//---------------------------------------------------------------------------
__inline Fixed::Fixed( short iWhole, unsigned short uFrac )
    :
        m_uFrac ( uFrac  ),
        m_iWhole( iWhole )
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
    // 16.16 fixed-point multiply. Was x86 inline asm (imul; shrd eax,edx,16;
    // adc eax,0) — a full 64-bit signed product shifted right 16 with round-
    // half-up. The portable form computes the same: the +0x8000 before the
    // arithmetic >>16 reproduces the asm's carry-bit rounding for both signs.
    long long llProduct = (long long)m_lFixed * (long long)fixRHS.m_lFixed;
    m_lFixed = (long)( ( llProduct + 0x8000 ) >> 16 );
}


//---------------------------------------------------------------------------
// Fixed::operator /= ()
//---------------------------------------------------------------------------
__inline void Fixed::operator /= ( Fixed fixRHS )
{
    // 16.16 fixed-point divide. Was x86 inline asm (cdq; shld/shl to form
    // LHS<<16 in edx:eax; idiv). idiv truncates toward zero, matching C++
    // integer division, so the 64-bit form is an exact translation.
    long long llNum = (long long)m_lFixed << 16;
    m_lFixed = (long)( llNum / (long long)fixRHS.m_lFixed );
}

#pragma warning( disable : 4244 )
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
#pragma warning( default : 4244 )

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
    return ((short) (m_iWhole + ( 0 != m_uFrac ) ? 1 : 0));
}

//---------------------------------------------------------------------------
// Fixed::Round - 
//---------------------------------------------------------------------------
__inline short Fixed::Round() const
{
    return ((short) (m_lFixed + 0x00008000L >> 16));
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
__inline void Fixed::Value( long lValue ) 
{
    m_lFixed = lValue;
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
    // 16.16 multiply — portable form of the x86 imul/shrd/adc asm (see
    // Fixed::operator*= for the derivation of the +0x8000 round-half-up).
    long long llProduct = (long long)fixLHS.m_lFixed * (long long)fixRHS.m_lFixed;
    return (long)( ( llProduct + 0x8000 ) >> 16 );
}

//---------------------------------------------------------------------------
// operator / ()
//---------------------------------------------------------------------------
__inline Fixed operator / ( Fixed fixLHS, Fixed fixRHS )
{
    // 16.16 divide — portable form of the x86 cdq/shld/shl/idiv asm (see
    // Fixed::operator/= ). idiv truncates toward zero, matching C++.
    long long llNum = (long long)fixLHS.m_lFixed << 16;
    return (long)( llNum / (long long)fixRHS.m_lFixed );
}

//---------------------------------------------------------------------------
// operator + () : Binary +
//---------------------------------------------------------------------------
#pragma warning( disable : 4244 )
__inline Fixed operator + ( Fixed fixL, int iR )
{ 
    return Fixed (fixL.m_iWhole + (int)(short) iR, fixL.m_uFrac);
}

//---------------------------------------------------------------------------
// operator - () : Binary -
//---------------------------------------------------------------------------
__inline Fixed operator - ( Fixed fixL, int iR )
{ 
    return Fixed (fixL.m_iWhole - (int)(short) iR, fixL.m_uFrac);
}
#pragma warning( default : 4244 )

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

#pragma warning ( disable : 4244 ) // I don't like this!!!
#endif
