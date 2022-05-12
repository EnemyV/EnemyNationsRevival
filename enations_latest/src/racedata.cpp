//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


// racedata.cpp : implementation file
//

#include "stdafx.h"
#include "racedata.h"
#include "error.h"
#include "lastplnt.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

CRaceDef * ptheRaces;
int CRaceDef::m_iNumRaces = 0;

CRaceDef * CRaceDef::Init (CRaceDef * pRd)
{

	if (pRd != NULL)
		return (pRd);

	// read in the races
	CMmio *pMmio = theDataFile.OpenAsMMIO ("create", "CRAT");
	pMmio->DescendRiff ('C', 'R', 'A', 'T');
	pMmio->DescendList ('R', 'A', 'C', 'E');

	// read the supplies
	DWORD dwPos [NUM_START_TYPES] [num_supplies];
	pMmio->DescendChunk ('S', 'P', 'L', 'Y');
	for (int iTyp=0; iTyp<NUM_START_TYPES; iTyp++)
		for (int iOn=0; iOn<num_supplies; iOn++)
			dwPos [iTyp][iOn] = pMmio->ReadShort ();
	pMmio->AscendChunk ();

	// read the multipliers
	pMmio->DescendChunk ('N', 'U', 'M', 'P');
	CRaceDef::m_iNumRaces = pMmio->ReadShort ();
	pMmio->AscendChunk ();

	pRd = new CRaceDef [CRaceDef::m_iNumRaces];

	// read the data
	for (int iOn=0; iOn<CRaceDef::m_iNumRaces; iOn++)
		{
		memcpy (pRd[iOn].m_iPos, dwPos, sizeof (dwPos));
		pRd[iOn].InitData (pMmio);
		}
	pMmio->AscendList ();

	// read the pictures
	pMmio->DescendList  ( 'B', 'M', theApp.m_szOtherBPS[0], theApp.m_szOtherBPS[1] );
	for (iOn=0; iOn<CRaceDef::m_iNumRaces; iOn++)
		{
		pMmio->DescendChunk ('D', 'I', 'B', 'B');
		pRd[iOn].m_pdibPicture = new CDIB ( ptrthebltformat->GetColorFormat(), 
												CBLTFormat::DIB_MEMORY, ptrthebltformat->GetMemDirection() );
		pRd[iOn].m_pdibPicture->Load( *pMmio );
		pMmio->AscendChunk ();
		}
	pMmio->AscendList ();

	delete pMmio;

	// read in their text
	pMmio = theDataFile.OpenAsMMIO (NULL, "LANG");
	pMmio->DescendRiff ('L', 'A', 'N', 'G');
	pMmio->DescendList ('R', 'A', 'C', 'E');
	for (iOn=0; iOn<CRaceDef::m_iNumRaces; iOn++)
		pRd[iOn].InitText (pMmio);
	delete pMmio;

	return (pRd);
}

CRaceDef::~CRaceDef ()
{

	delete m_pdibPicture;
}

void CRaceDef::Close (CRaceDef * pRd)
{

	delete [] pRd;
}

CRaceDef::CRaceDef ()
{

	m_pdibPicture = NULL;

	for (int iOn=0; iOn<num_race; iOn++)
		m_fRace [iOn] = 1.0;

	memset (m_iPos, 0, sizeof (m_iPos));
}

void CRaceDef::InitData (CMmio * pMmio)
{

	pMmio->DescendChunk ('R', 'A', 'C', 'E');
	for (int iOn=0; iOn<num_race; iOn++)
		{
		m_fRace [iOn] = pMmio->ReadFloat ();
		if (m_fRace [iOn] < 0.1)
			m_fRace [iOn] = (float) (50 + RandNum (100)) / 100.0f;
		m_fRace [iOn] = (m_fRace [iOn] < 0.7f) ? 0.7f : (m_fRace [iOn] > 1.2f ? 1.2f : m_fRace [iOn]);
		}

	// read the island/ocean parts
	m_iPos [0] [island] = pMmio->ReadShort ();
	if ( m_iPos [0] [island] == -1 )
		m_iPos [0] [island] = RandNum (1);
	m_iPos [0] [ocean] = pMmio->ReadShort ();
	if ( m_iPos [0] [ocean] == -1 )
		m_iPos [0] [ocean] = RandNum (1);

	for (int iTyp=1; iTyp<NUM_START_TYPES; iTyp++)
		{
		m_iPos [iTyp] [island] = m_iPos [0] [island];
		m_iPos [iTyp] [ocean] = m_iPos [0] [ocean];
		}
	pMmio->AscendChunk ();
}

void CRaceDef::InitText (CMmio * pMmio)
{

	pMmio->DescendChunk ('D', 'E', 'S', 'C');
	pMmio->ReadString (m_sLine);
	pMmio->ReadString (m_sDesc);
	pMmio->AscendChunk ();
}

#ifdef _DEBUG

void CRaceDef::AssertValid() const
{

	CObject::AssertValid ();

	for (int iOn=0; iOn<num_race; iOn++)
		ASSERT ((0.5 <= m_fRace [iOn]) && (m_fRace[iOn] <= 2.0));

	for (int iTyp=0; iTyp<NUM_START_TYPES; iTyp++)
		for (int iOn=0; iOn<num_supplies; iOn++)
			ASSERT (m_iPos [iTyp][iOn] <= 32000);
}

#endif

CInitData::CInitData ()
{

	for (int iOn=0; iOn<CRaceDef::num_race; iOn++)
		m_fRace [iOn] = 1.0;

	memset (m_iPos, 0, sizeof (m_iPos));
}

void CInitData::Set (CRaceDef const * pRd, int iTyp)
{

	memcpy (m_fRace, pRd->m_fRace, sizeof (m_fRace));
	memcpy (m_iPos, &(pRd->m_iPos[iTyp]), sizeof (m_iPos));
}

void CInitData::Serialize(CArchive& ar)
{

	if (ar.IsStoring ())
		{
		for (int iOn=0; iOn<CRaceDef::num_race; iOn++)
			ar << m_fRace [iOn];

		for (iOn=0; iOn<CRaceDef::num_supplies; iOn++)
			ar << m_iPos [iOn];
		}
	else
		{
		for (int iOn=0; iOn<CRaceDef::num_race; iOn++)
			ar >> m_fRace [iOn];

		for (iOn=0; iOn<CRaceDef::num_supplies; iOn++)
			ar >> m_iPos [iOn];
		}
}

const CInitData & CInitData::operator= (CInitData const & src)
{

	memcpy (m_fRace, src.m_fRace, sizeof (m_fRace));
	memcpy (m_iPos, src.m_iPos, sizeof (m_iPos));
	return (* this);
}

#ifdef BUGBUG
IMPLEMENT_SERIAL (CRaceDefinition, CObject, 0);

#define new DEBUG_NEW


void CInitData::Init ()
{

	// read in the RIF data
	CMmio *pMmio = theDataFile.OpenAsMMIO ("create");

	pMmio->DescendRiff ('C', 'R', 'A', 'T');
	pMmio->DescendList ('R', 'A', 'C', 'E');
	for (int iOn=0; iOn<NUM_RACE_CHAR; iOn++)
		m_raceAttrib[iOn].InitData (*pMmio);
	for (iOn=0; iOn<NUM_INIT_SPLYS; iOn++)
		m_posAttrib[iOn].InitData (*pMmio);

	delete pMmio;

	// read in the text (language.rif)
	pMmio = theDataFile.OpenAsMMIO (NULL);

	pMmio->DescendRiff ('L', 'A', 'N', 'G');
	pMmio->DescendList ('R', 'A', 'C', 'E');
	for (iOn=0; iOn<NUM_RACE_CHAR; iOn++)
		m_raceAttrib[iOn].InitText (*pMmio);
	for (iOn=0; iOn<NUM_INIT_SPLYS; iOn++)
		m_posAttrib[iOn].InitText (*pMmio);
	delete pMmio;

	ASSERT_VALID (this);
}

#ifdef _DEBUG
void CInitData::AssertValid () const
{

	// assert base object
	CObject::AssertValid ();

	for (int iOn=0; iOn<NUM_RACE_CHAR; iOn++)
		ASSERT_VALID (&m_raceAttrib[iOn]);
	for (iOn=0; iOn<NUM_INIT_SPLYS; iOn++)
		ASSERT_VALID (&m_posAttrib[iOn]);
}
#endif


void CInitAttrib::InitData (CMmio & mmio)
{

	mmio.DescendChunk ('D', 'A', 'T', 'A');
	m_iTyp = mmio.ReadShort ();
	m_iValStd = mmio.ReadShort ();

	switch (m_iTyp)
		{
		case ATRIB_TYP_RANGE :
			m_iMinPer = mmio.ReadShort ();
			m_iMaxPer = mmio.ReadShort ();
			m_iMaxPts = mmio.ReadShort ();
			break;

		case ATRIB_TYP_UNITS :
			m_iMinPer = mmio.ReadShort ();
			m_iValStd *= m_iMinPer;
			break;

		case ATRIB_TYP_SPLYS :
		case ATRIB_TYP_FEE :
			m_iMinPer = mmio.ReadShort ();
			break;
		}

	mmio.AscendChunk ();
}

void CInitAttrib::InitText (CMmio & mmio)
{

	mmio.DescendChunk ('L', 'I', 'N', 'E');
	mmio.ReadString (m_sLine);
	mmio.AscendChunk ();

	mmio.DescendChunk ('D', 'E', 'S', 'C');
	mmio.ReadString (m_sDesc);
	mmio.AscendChunk ();

#ifdef _DEBUG
	m_bInited = TRUE;
#endif
}

int CInitAttrib::GetNumUnits (int iVal) const
{

	ASSERT_VALID (this);

	switch (m_iTyp)
		{
		case ATRIB_TYP_RANGE :
		case ATRIB_TYP_SPLYS :
		case ATRIB_TYP_FEE :
			return (iVal);
		case ATRIB_TYP_UNITS :
			return (iVal / m_iMinPer);
		}

	return (0);
}

int CInitAttrib::ComputePts (int iVal) const
{

	switch (m_iTyp)
		{
		case ATRIB_TYP_RANGE :
			return (((iVal - m_iMinPer) * m_iMaxPts) / (m_iMaxPer - m_iMinPer));

		case ATRIB_TYP_UNITS : {
			int iMod = iVal % m_iMinPer;
			if (iMod != 0)
				iVal += (m_iMinPer - iMod);
			return (iVal / m_iMinPer); }

		case ATRIB_TYP_SPLYS :
			return (iVal * m_iMinPer);

		case ATRIB_TYP_FEE :
			return (iVal ? m_iMinPer : 0);
		}

	ASSERT_VALID (this);
	return (0);
}

CString CInitAttrib::GetLine (int iVal) const
{

	ASSERT_VALID (this);
	CString sRtn;
	int iPts = ComputePts (iVal);

	switch (m_iTyp)
		{
		case ATRIB_TYP_RANGE :
			sRtn = m_sLine + "\t" + IntToCString (iVal) +
																	"%\t(" + IntToCString (iPts) + ")";
			break;
		case ATRIB_TYP_UNITS :
			sRtn = m_sLine + "\t" + IntToCString (iVal) +
																	"\t(" + IntToCString (iPts) + ")";
			break;
		case ATRIB_TYP_SPLYS :
			sRtn = m_sLine + "\t" + IntToCString (iVal) +
																	"\t(" + IntToCString (iPts) + ")";
			break;
		case ATRIB_TYP_FEE :
			if (iVal)
				sRtn = m_sLine + "\tYes\t(" + IntToCString (iPts) + ")";
			else
				sRtn = m_sLine + "\tNo\t(0)";
			break;
		}

	return (sRtn);
}

#ifdef _DEBUG
void CInitAttrib::AssertValid () const
{

	// assert base object
	CObject::AssertValid ();

	if (! m_bInited)
		return;

	ASSERT ((0 <= m_iTyp) && (m_iTyp <= ATRIB_TYP_MAX));

	// BUGBUG - do the rest

	ASSERT_VALID_CSTRING (&m_sLine);
	ASSERT_VALID_CSTRING (&m_sDesc);
}
#endif


static CString sHdr ("RaceData file\n\032");

void CRaceDefinition::Serialize(CArchive& ar)
{

	if (ar.IsStoring ())
		{
		ASSERT_VALID (this);

		ar << sHdr;

		for (int iOn=0; iOn<NUM_RACE_CHAR; iOn++)
			ar << m_iRace[iOn];
		for (int iTyp=0; iTyp<NUM_START_TYPES; iTyp++)
			for (iOn=0; iOn<NUM_INIT_SPLYS; iOn++)
				ar << m_iPos[iTyp][iOn];

		ar << m_sLine;
		ar << m_sDesc;
		}

	else
	  {
		CString sTmp;
		ar >> sTmp;
		if (sTmp != sHdr)
			ThrowError (ERR_TLP_BAD_RCE_FILE);

		for (int iOn=0; iOn<NUM_RACE_CHAR; iOn++)
			ar >> m_iRace[iOn];
		for (int iTyp=0; iTyp<NUM_START_TYPES; iTyp++)
			for (iOn=0; iOn<NUM_INIT_SPLYS; iOn++)
				ar >> m_iPos[iTyp][iOn];

		ar >> m_sLine;
		ar >> m_sDesc;
	  
		ASSERT_VALID (this);
	  }
}

#ifdef _DEBUG
void CRaceDefinition::AssertValid () const
{

	CObject::AssertValid ();

	// BUGBUG - check ranges
}
#endif


#endif
