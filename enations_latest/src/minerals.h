//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


#ifndef __MINEARLS_H__
#define __MINEARLS_H__

#include "base.h"


const int MAX_MINERAL_DENSITY = 255;
const int MAX_MINERAL_QUANTITY = 1000 * 180;
const int MAX_MINERAL_COAL_QUANTITY = 1000 * 120;
const int MAX_MINERAL_IRON_QUANTITY = 1000 * 180;
const int MAX_MINERAL_OIL_QUANTITY = 60 * 120;
const int MAX_MINERAL_XIL_QUANTITY = 30 * 120;


/////////////////////////////////////////////////////////////////////////////
// CMinerals - minerals in the ground

#ifdef _DEBUG
class CMinerals : public CObject
#else
class CMinerals
#endif
{
public:
#ifdef _DEBUG					
					CMinerals () {m_cType = (BYTE) -1;}
#else
					CMinerals () {}
#endif
					CMinerals (int iType, int iDensity, int iQuantity);
					CMinerals (CMinerals const & src);
					const CMinerals & operator= (const CMinerals & src);

		int			GetDensity () { ASSERT_STRICT_VALID (this); return (m_cDensity); }
		int			GetQuantity () { ASSERT_STRICT_VALID (this); return (m_lQuantity); }
		std::string	GetStatus ();
		int			GetType () { ASSERT_STRICT_VALID (this); return (m_cType); }
		void		SetDensity (int iNum) { m_cDensity = iNum; }
		void		SetQuantity (int iNum) 
											{ ASSERT_STRICT ((0 <= iNum) && (iNum <= 0xFFFF)); 
											m_lQuantity = iNum; ASSERT_STRICT_VALID (this); }

		static int		DensityDiv () { return (MAX_MINERAL_DENSITY / 4); }

		friend CArchive& operator<< (CArchive& ar, const CMinerals & mn);
		friend CArchive& operator>> (CArchive& ar, CMinerals & mn);

protected:
		LONG			m_lQuantity;
		BYTE			m_cType;
		BYTE			m_cDensity;

      public:
        BOOL IsValid( ) const;
#ifdef _DEBUG
      public:
        virtual void AssertValid( ) const;
#endif
};


// tracks hexes units are on
class CMineralHex : public CMap <DWORD, DWORD, CMinerals *, CMinerals *>
{
public:
	CMineralHex ();
	~CMineralHex () { Close (); }

	void		InitHex (CHexCoord const & hex, int iType, int multiplier);
	void		Close ();

	// Saved-game round-trip. MUST be declared here: the base CMap::Serialize in
	// the MFC-compat layer is a no-op (most CMaps don't persist), so without this
	// override theMinerals would never be written or read back — loaded games end
	// up with zero deposits and mines/oil wells can't be placed. CBridgeMap solves
	// the same problem the same way.
	void		Serialize (CArchive & ar);
};

extern CMineralHex theMinerals;
extern void SerializeElements(CArchive& ar, CMinerals** pMn, int nCount);

template<>
// nCount must be int to match the primary SerializeElements template in
// mfc_compat.h; INT_PTR differs from int on x64 and breaks the specialization.
void AFXAPI SerializeElements<CMinerals*>( CArchive& ar, CMinerals** ppMn, int nCount );


#endif
