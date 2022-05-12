#ifndef __BRIDGE_H__
#define __BRIDGE_H__


class CBridge;



/////////////////////////////////////////////////////////////////////////////
// CBridgeHex - a piece of bridge occupying 1 hex

class CBridgeUnit : public CUnitTile
{
friend class CBridge;
public:
			CBridgeUnit (CHexCoord const & hex, CBridge * pPar);

			CHexCoord const &		GetHex () const { return m_hex; }
			int									GetDir () const { return m_iDir; }
			BOOL								IsExit () const { if (m_iExit == -1) return FALSE; return TRUE; }
			int									GetExit () const { return m_iExit; }
			CBridge *						GetParent () { return m_pParBridge; }
			int									GetAlt () const { return m_iAlt; }
			int									GetPer () const { return m_iPerBuilt; }
			BOOL								IsBuilt () const { return m_iPerBuilt >= 100; }

			CStructureSprite *GetSprite() const
									{
										#ifdef _DEBUG
										(( CStructureSprite * )m_psprite )->CheckValid();
										#endif
										return ( CStructureSprite * )m_psprite;
									}

			BOOL						IsTwoPiece() const;
			CPoint					GetDrawCorner( const CHexCoord & hexcoord ) const;
			CRect						Draw			 (	const CHexCoord & hexcoord );
			BOOL						IsHit			 ( CHexCoord, CPoint ) const;
         void                 AssignSprite();
			Fixed						GetSurfaceAlt ( CMapLoc const & ) const;

protected:
			CHexCoord		m_hex;
			int					m_iDir;					// 0 == y dir, 1 == x dir (no 2,3)
			int					m_iExit;				// -1 == no exit, else 0 == -y, 1 == +x, 2 == +y, 3 == -x (direction of exit)
			int					m_iAlt;
			int					m_iPerBuilt;		// 0 - 100 of amount built
			CBridge *		m_pParBridge;		// the bridge this is a piece of

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CBridge - a bridge over the water

class CBridge : public CObject
{
friend class CBridgeMap;
public:
			CBridge (CHexCoord const & hexStart, CHexCoord const & hexEnd, DWORD dwID, int iAlt);
			~CBridge ();
			static CBridge * Create (CHexCoord const & hexStart, CHexCoord const & hexEnd, DWORD dwID, int iAlt);

			DWORD								GetID () const { return m_dwID; }
			CHexCoord const &		GetHexStart () const { return m_hexStart; }
			CHexCoord const &		GetHexEnd () const { return m_hexEnd; }
			CMapLoc const &			GetMapLoc () const { return m_mlMiddle; }
			CBridgeUnit *				GetUnitStart () { return m_lstUnits.GetHead (); }
			CBridgeUnit *				GetUnitEnd () { return m_lstUnits.GetTail (); }

			int									GetDir () const { return m_iDir; }
			int									GetAlt () const { return m_iAlt; }
			int									GetPer () const { return m_iPerBuilt; }
			int									GetConstTotal () const { return m_iConstTotal; }
			BOOL								IsBuilt () const { return m_iPerBuilt >= 100; }

			void								BridgeBuilt () { __SetPer (100); }
			void								_SetPer (int iPer) { __SetPer (__minmax (0, 99, iPer)); }

protected:
			void								__SetPer (int iPer);

			DWORD				m_dwID;

			CMapLoc			m_mlMiddle;
			CHexCoord		m_hexStart;
			CHexCoord		m_hexEnd;
			int					m_iDir;					// 0 == y dir, 1 == x dir (no 2,3)
			int					m_iAlt;

			// building bridge
			int					m_iConstTotal;	// when iConstDone >= iConstTotal -> it's built
			int					m_iPerBuilt;		// 0 - 100 of amount built

			CList <CBridgeUnit *, CBridgeUnit *>		m_lstUnits;

#ifdef _DEBUG
public:
	virtual void AssertValid() const;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// unit maps - we store a map of all instances so we can walk all members and
// find one quickly by dwID

class CBridgeMap : public CMap <DWORD, DWORD, CBridge *, CBridge *>
{
public:
	CBridgeMap () { InitHashTable (HASH_INIT); }

	void					Add (CBridge * pBrdg) { SetAt (pBrdg->GetID (), pBrdg); }
	void					DeleteAll ();
	void					Serialize( CArchive & ar );

	CBridge *		GetBldg (DWORD dwID) const
									{ CBridge * pBrdg;
										if (Lookup (dwID, pBrdg) == NULL)
											return (NULL);
										ASSERT_STRICT_VALID (pBrdg);
										return (pBrdg); }
};

// tracks hexes units are on
class CBridgeHex : public CMap <DWORD, DWORD, CBridgeUnit *, CBridgeUnit *>
{
public:
	CBridgeHex () { InitHashTable (HASH_INIT); }

	void					GrabHex (CBridgeUnit * pBrdg);

	CBridgeUnit *		GetBridge (CHexCoord const & hex) const
											{ return (GetBridge (hex.X (), hex.Y ())); }
	CBridgeUnit *		GetBridge (CSubHex const & sub) const
											{ return (GetBridge (sub.x / 2, sub.y / 2)); }
	CBridgeUnit *		GetBridge (int x, int y) const
											{ CBridgeUnit * pBrdg; if (Lookup (ToArg (x,y), pBrdg)) return (pBrdg); return (NULL); }

	CBridgeUnit *		_GetBridge (CHexCoord const & hex) const
											{ return (_GetBridge (hex.X (), hex.Y ())); }
	CBridgeUnit *		_GetBridge (CSubHex const & sub) const
											{ return (_GetBridge (sub.x / 2, sub.y / 2)); }
	CBridgeUnit *		_GetBridge (int x, int y) const
											{ CBridgeUnit * pBrdg; if (Lookup (_ToArg (x,y), pBrdg)) return (pBrdg); return (NULL); }

protected:
	DWORD					ToArg (int x, int y) const
											{ return (((DWORD) CHexCoord::Wrap (x) << 16) | (DWORD) CHexCoord::Wrap (y)); }
	DWORD					_ToArg (int x, int y) const
											{ return (((DWORD) x << 16) | (DWORD) y); }
};


extern CBridgeMap theBridgeMap;
extern CBridgeHex theBridgeHex;


#endif
