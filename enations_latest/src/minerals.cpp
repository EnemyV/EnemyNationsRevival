//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// minerals.cpp - natural resources
//

#include "stdafx.h"
#include "base.h"
#include "minerals.inl"
#include "terrain.inl"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW


#ifdef _CHEAT
static int aiNumRes [CMaterialTypes::num_types];
#endif

/////////////////////////////////////////////////////////////////////////////
// A hash table of the entries

CMineralHex theMinerals;

CMineralHex::CMineralHex () 
{ 

	InitHashTable (409);

#ifdef _CHEAT
	memset (aiNumRes, 0, sizeof (aiNumRes));
#endif
}

void CMineralHex::InitHex( CHexCoord const& hex, int iType, int multiplier )
{

	int iDensity = MAX_MINERAL_DENSITY;			
	int iQuantity = MAX_MINERAL_QUANTITY;

	switch (iType)
	  {
		case CMaterialTypes::coal :
			iQuantity = MAX_MINERAL_COAL_QUANTITY;
			break;
		case CMaterialTypes::iron :
			iQuantity = MAX_MINERAL_IRON_QUANTITY;
			if (MyRand () & 0x0100)
				iQuantity = MAX_MINERAL_IRON_QUANTITY / 2;
			if (MyRand () & 0x0100)
				iDensity = MAX_MINERAL_DENSITY / 2;
			break;
		case CMaterialTypes::oil :
			iQuantity = MAX_MINERAL_OIL_QUANTITY;
			if (MyRand () & 0x0100)
				iDensity = MAX_MINERAL_DENSITY / 2;
			break;
		case CMaterialTypes::copper :
			if (MyRand () & 0x0100)
				iQuantity = MAX_MINERAL_XIL_QUANTITY / 4;
			else
				if (MyRand () & 0x0100)
					iQuantity = MAX_MINERAL_XIL_QUANTITY / 2;
				else
					iQuantity = MAX_MINERAL_XIL_QUANTITY;
			if (MyRand () & 0x0100)
				iDensity = MAX_MINERAL_DENSITY / 4;
			else
				if (MyRand () & 0x0100)
					iDensity = MAX_MINERAL_DENSITY / 2;
			break;
	  }

	// MP determinism: the two draws MUST be sequenced — as one `a + b*0.1`
	// expression their evaluation order is unspecified and differed across
	// MSVC/gcc/clang, swapping which draw lands in which role. When the swap
	// crosses the <=0 early-return below, the minerals flag differs per
	// platform and MakeMineral's iAvail branch then consumes a different
	// number of draws -> world-gen RAND MISMATCH (the missed 23rd site of the
	// arg-eval hoist wave). Integer /10 replaces the *0.1 double: this TU is
	// not /fp:precise, and the value is equivalent at the boundary.
	const int iDensDraw1 = RandNum( iDensity );
	const int iDensDraw2 = RandNum( iDensity );
	iDensity  = iDensDraw1 + ( iDensDraw2 * multiplier ) / 10;
      iQuantity = RandNum( iQuantity ) * multiplier;

	if ((iDensity <= 0) || (iQuantity <= 0))
		return;

	CMinerals *pMn = new CMinerals (iType, iDensity, iQuantity);
	theMinerals.SetAt (hex, pMn);
	theMap._GetHex (hex)->OrUnits (CHex::minerals);

#ifdef _CHEAT
	aiNumRes [iType] ++;
#endif
}

void CMineralHex::Close ()
{

	POSITION pos = GetStartPosition ();
	while (pos != NULL)
		{
		DWORD dw;
		CMinerals *pMn;
		GetNextAssoc (pos, dw, pMn);
		delete pMn;
		}

	RemoveAll ();
}

/* void SerializeElements( CArchive& ar, CMinerals** ppMn, int nCount )
{

	while (nCount--)
		{
		if (ar.IsStoring ())
			ar << *(*ppMn);
		else
			{
			*ppMn = new CMinerals ();
			ar >> *(*ppMn);
			}
		ppMn++;
		}
}*/

// explicit specialization fixes loaded game mineral bug
template<>
void AFXAPI SerializeElements<CMinerals*>( CArchive& ar, CMinerals** ppMn, int nCount )
{
    {
        while ( nCount-- )
        {
            if ( ar.IsStoring( ) )
                ar << *( *ppMn );
            else
            {
                *ppMn = new CMinerals( );
                ar >> *( *ppMn );
            }
            ppMn++;
        }
    }
}

// Round-trip every deposit through the archive. The base CMap::Serialize in the
// MFC-compat layer is a no-op, so this override is what actually persists
// theMinerals — without it, loaded games have no deposits and mines/oil wells
// report a "useless" (0,0) location everywhere. The key is the packed hex DWORD
// (CHexCoord <-> unsigned long); each value uses CMinerals' own operator<</>>.
void CMineralHex::Serialize( CArchive& ar )
{

    if ( ar.IsStoring( ) )
    {
        ar << (DWORD)GetCount( );
        POSITION pos = GetStartPosition( );
        while ( pos != NULL )
        {
            DWORD      dwKey;
            CMinerals* pMn;
            GetNextAssoc( pos, dwKey, pMn );
            ar << dwKey;
            ar << *pMn;
        }
    }
    else
    {
        // drop any stale deposits before reading the saved set
        Close( );

        DWORD nCount = 0;
        ar >> nCount;
        while ( nCount-- )
        {
            DWORD dwKey = 0;
            ar >> dwKey;
            CMinerals* pMn = new CMinerals( );
            ar >> *pMn;
            SetAt( dwKey, pMn );

            // Re-flag the hex so the toolbar hover and any GetUnits() checks see
            // the deposit, matching what InitHex does when generating a new map.
            theMap._GetHex( CHexCoord( (unsigned long)dwKey ) )->OrUnits( CHex::minerals );
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// A mineral deposit

std::string CMinerals::GetStatus ()
{

	ASSERT_VALID (this);

	return " - [" + CMaterialTypes::GetDesc (m_cType) + " (" +
		   IntToStr (m_cDensity) + "," + IntToStr (m_lQuantity) + ")]";
}

CMinerals::CMinerals (int iType, int iDensity, int iQuantity)
{

	// random # can do a 0
	iDensity = __minmax (1, MAX_MINERAL_DENSITY, iDensity);
	iQuantity = __minmax (1, MAX_MINERAL_QUANTITY, iQuantity);

	m_cType = (char) iType;
	m_cDensity = (char) iDensity;
	m_lQuantity = iQuantity;


	ASSERT_VALID (this);
}

CArchive& operator<< (CArchive& ar, const CMinerals & mn)
{

	ASSERT_VALID (&mn);

	ar << mn.m_lQuantity << mn.m_cType << mn.m_cDensity;
	return (ar);
}

CArchive& operator>> (CArchive& ar, CMinerals & mn)
{

	ar >> mn.m_lQuantity >> mn.m_cType >> mn.m_cDensity;
	return (ar);
}

#ifdef _DEBUG
void CMinerals::AssertValid( ) const
{

    CObject::AssertValid( );

    ASSERT( ( m_cType == CMaterialTypes::oil ) || ( m_cType == CMaterialTypes::coal ) ||
            ( m_cType == CMaterialTypes::iron ) || ( m_cType == CMaterialTypes::copper ) );
    ASSERT( ( 0 < m_cDensity ) && ( m_cDensity <= MAX_MINERAL_DENSITY ) );
    ASSERT( ( 0 < m_lQuantity ) && ( m_lQuantity <= MAX_MINERAL_QUANTITY ) );
}

#endif
BOOL CMinerals::IsValid( ) const
{
#ifdef _DEBUG
    try

    {
        return ( ( ( m_cType == CMaterialTypes::oil ) || ( m_cType == CMaterialTypes::coal ) ||
                   ( m_cType == CMaterialTypes::iron ) || ( m_cType == CMaterialTypes::copper ) ) &&
                 ( ( 0 < m_cDensity ) && ( m_cDensity <= MAX_MINERAL_DENSITY ) ) &&
                 ( ( 0 < m_lQuantity ) && ( m_lQuantity <= MAX_MINERAL_QUANTITY ) ) );
    }
    catch ( ... )
    {
    
			return FALSE;
    }

#else
    return TRUE;
#endif
}

