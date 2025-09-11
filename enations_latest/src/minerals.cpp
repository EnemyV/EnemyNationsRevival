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

void CMineralHex::InitHex (CHexCoord const & hex, int iType)
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

	iDensity = RandNum ( iDensity );
	iQuantity = RandNum ( iQuantity );

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

void SerializeElements (CArchive & ar, CMinerals * * ppMn, int nCount)
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
}


/////////////////////////////////////////////////////////////////////////////
// A mineral deposit

CString CMinerals::GetStatus ()
{

	ASSERT_VALID (this);

	return (CString (" - [" + CMaterialTypes::GetDesc (m_cType) + " (" + 
					IntToCString (m_cDensity) + "," + IntToCString (m_lQuantity) + ")]"));
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

