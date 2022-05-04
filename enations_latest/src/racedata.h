#ifndef __RACEDATA_H__
#define __RACEDATA_H__


const int NUM_START_TYPES = 5;

class CInitData;


class CRaceDef : public CObject
{
friend CInitData;
public:
					CRaceDef ();
					~CRaceDef ();
	static CRaceDef *		Init (CRaceDef * pRd);
	static void					Close (CRaceDef * pRd);
	static int	GetNumRaces () { return m_iNumRaces; }


	enum RACE {
				build_bldgs,
				manf_materials,
				manf_vehicles,
				mine_prod,
				farm_prod,
				research,
				pop_grow,
				pop_die,
				pop_eat,
				attack,
				defense,
				num_race };

	enum SUPPLIES {
				food,
				lumber,
				steel,
				copper,
				gas,
				coal,
				iron,
				oil,
				people,

				crane,
				heavy_truck,
				light_scout,
				med_scout,
				infantry_carrier,
				light_tank,
				med_tank,
				light_art,
				infantry,

				island,
				ocean,
				num_supplies };

	float						GetRace (int iInd) const { return m_fRace[iInd]; }
	int							GetSupplies (int iTyp, int iInd) const { return m_iPos [iTyp][iInd]; }
	char const *		GetLine () const { return m_sLine; }
	char const *		GetDesc () const { return m_sDesc; }
	CDIB *					GetPicture () { return m_pdibPicture; }

private :
	void		InitData (CMmio * pMmio);
	void		InitText (CMmio * pMmio);

	static int	m_iNumRaces;


	float			m_fRace [num_race];
	DWORD			m_iPos [NUM_START_TYPES] [num_supplies];
	CString		m_sLine;
	CString		m_sDesc;
	CDIB *		m_pdibPicture;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};

class CInitData
{
public:
						CInitData ();
	void			Set (CRaceDef const * pRd, int iTyp);

	const CInitData & operator= (CInitData const & src);

	void 				Serialize(CArchive& ar);

	float						GetRace (int iInd) const { return m_fRace[iInd]; }
	int							GetSupplies (int iInd) const { return m_iPos [iInd]; }

	void						SetRace (int iInd, float fVal) { m_fRace[iInd] = fVal; }
	void						SetSupplies (int iInd, int iVal) { m_iPos [iInd] = iVal; }

private :
	float			m_fRace [CRaceDef::num_race];
	DWORD			m_iPos [CRaceDef::num_supplies];
};


extern CRaceDef * ptheRaces;


#endif
