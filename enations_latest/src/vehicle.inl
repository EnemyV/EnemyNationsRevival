#ifndef __VEHICLE_INL__
#define __VEHICLE_INL__

#include "vehicle.h"
#include "terrain.h"
#include "player.h"


inline CTransportData const * CTransport::GetData (int iIndex) const
			{ ASSERT ((0 <= iIndex) && (iIndex < m_iNumTransports));
				ASSERT_STRICT (0 <= iIndex); if (iIndex >= m_iNumTransports) iIndex = 0;
				ASSERT_STRICT_VALID (this); 
				return (m_pData + iIndex); }
inline CTransportData * CTransport::_GetData (int iIndex) const
			{ ASSERT ((0 <= iIndex) && (iIndex < m_iNumTransports));
				ASSERT_STRICT (0 <= iIndex); if (iIndex >= m_iNumTransports) iIndex = 0;
				ASSERT_STRICT_VALID (this); 
				return (m_pData + iIndex); }
inline int CTransportData::GetMaxMaterials () const { ASSERT_STRICT_VALID (this); return (m_iMaxMaterials); }
inline int CTransport::GetNumTransports () const { return (m_iNumTransports); }


//   CTransportData
inline CTransportData::TRANS_TYPE CTransportData::GetType () const { ASSERT_STRICT_VALID (this); return ( m_iType); }
inline int CTransportData::GetSetupFire () const { ASSERT_STRICT_VALID (this); return ( m_iSetupFire); }
inline int CTransportData::GetPeopleCarry () const { ASSERT_STRICT_VALID (this); return ( m_iPeopleCarry); }
inline int CTransportData::GetWheelType () const { ASSERT_STRICT_VALID (this); return ( m_cWheelType); }
inline int CTransportData::GetSpeed () const { ASSERT_STRICT_VALID (this); return ( m_iSpeed); }
inline int CTransportData::GetWaterDepth () const { ASSERT_STRICT_VALID (this); return ( m_cWaterDepth); }
inline BOOL CTransportData::IsCrane () const { ASSERT_STRICT_VALID (this); return (m_transFlags & FLconstruction); }
inline BOOL CTransportData::IsBoat () const { ASSERT_STRICT_VALID (this); return (m_transFlags & FLboat); }
inline BOOL CTransportData::IsCarrier () const { ASSERT_STRICT_VALID (this); return (m_transFlags & FLcarrier); }
inline BOOL CTransportData::IsTransport () const { ASSERT_STRICT_VALID (this); return (m_transFlags & FLtransport); }
inline BOOL CTransportData::IsPeople () const { ASSERT_STRICT_VALID (this); return (m_transFlags & FLpeople); }
inline BOOL CTransportData::IsCarryable () const { ASSERT_STRICT_VALID (this); return (m_transFlags & FLcarryable); }
inline BOOL CTransportData::IsLcCarryable () const { ASSERT_STRICT_VALID (this); return (m_transFlags & FLlc_carryable); }
inline BOOL CTransportData::IsRepairable () const { ASSERT_STRICT_VALID (this); return (m_transFlags & FLrepairable); }


//   CVehicle
inline void CVehicle::SetLoadOn ( CVehicle * pCarrier ) { m_pVehLoadOn = pCarrier; }
inline int CVehicle::GetProd (float fProd)
										{ div_t dtProd = div ((int) (m_fDamPerfMult * fProd * 
																theGame.GetOpersElapsed ()) + m_lOperMod, AVG_SPEED_MUL);
											m_lOperMod = dtProd.rem;
											return (dtProd.quot); }

inline CVehicle::VEH_MODE CVehicle::GetRouteMode () const 	{ ASSERT_STRICT_VALID (this); return (m_cMode); }
inline CTransportData const *	CVehicle::GetData () const { ASSERT_STRICT_VALID (this); return ((CTransportData const *) m_pUnitData); }
inline void CVehicle::UpdateChoices () { if (m_pDlgStructure != NULL) m_pDlgStructure->UpdateChoices (); }
inline CSubHex CVehicle::Rotate (int iDir) { return (::Rotate (iDir, m_ptHead, m_ptTail)); }


inline CVehicle * CVehicleMap::_GetVehicle (DWORD dwID) const
									{ CVehicle * pVeh;
										if (theVehicleMap.Lookup (dwID, pVeh) == NULL)
											return (NULL);
										ASSERT_STRICT_VALID (pVeh);
										return (pVeh);
									}
inline CVehicle * CVehicleMap::GetVehicle (DWORD dwID) const
									{ CVehicle * pVeh = _GetVehicle (dwID);
										if ((pVeh == NULL) || (pVeh->GetFlags () & CUnit::dying))
											return (NULL);
										return (pVeh);
									}

inline void CVehicleHex::GrabHex (int x, int y, CVehicle * pVehicle)
											{	pVehicle->AddSubOwned ( x, y );
												SetAt (ToArg (x,y), (CVehicle *) pVehicle); 
												theMap._GetHex (x/2, y/2)->OrUnits (1 << ((x & 1) + ((y & 1) * 2)));
											}
inline void CVehicleHex::ReleaseHex (int x, int y, CVehicle * pVeh)
											{	pVeh->RemoveSubOwned ( x, y );
												RemoveKey (ToArg (x,y));
												theMap._GetHex (x/2, y/2)->NandUnits (1 << ((x & 1) + ((y & 1) * 2)));
											}

inline CVehicle * CVehicleHex::GetVehicle (CSubHex const & pt) const
																			{ return (GetVehicle (pt.x, pt.y)); }
inline CVehicle * CVehicleHex::GetVehicle (int x, int y) const
											{ x = CSubHex::Wrap (x);
												y = CSubHex::Wrap (y);
												CVehicle * pVeh; if (Lookup (ToArg (x,y), pVeh)) return (pVeh); return (NULL); }
inline DWORD CVehicleHex::ToArg (int x, int y) const { return (((DWORD) CSubHex::Wrap (x) << 16) | (DWORD) CSubHex::Wrap (y)); }

inline CVehicle * CVehicleHex::_GetVehicle (CSubHex const & pt) const
																			{ return (_GetVehicle (pt.x, pt.y)); }
inline CVehicle * CVehicleHex::_GetVehicle (int x, int y) const
											{ CVehicle * pVeh; if (Lookup (ToArg (x,y), pVeh)) return (pVeh); return (NULL); }
inline DWORD CVehicleHex::_ToArg (int x, int y) const { return (((DWORD) x << 16) | (DWORD) y); }


// note [4] is no change
extern int aiBaseDir[9];
extern int aiDir[9];

_inline int GetDirIndex (CPoint const & ptHead, CPoint const & ptTail)
{
	
	int iDir = (CSubHex::Diff (ptHead.x - ptTail.x) + 1) * 3 + CSubHex::Diff (ptHead.y - ptTail.y) + 1;
	return (__minmax (0, 8, iDir));
}

_inline int GetAngle ( const CSubHex & _head1, const CSubHex & _tail1, const CSubHex & _head2, const CSubHex & _tail2)
{

	int _iDir = aiBaseDir [GetDirIndex (_head1, _tail1)] -
															aiBaseDir [GetDirIndex (_head2, _tail2)];
	_iDir += (_iDir > 4) ? - 8 : ((_iDir <= -4) ? 8 : 0);
	return (_iDir);
}

inline int _CalcBaseDir (CPoint const & ptHead, CPoint const & ptTail)
{

	return (aiBaseDir [GetDirIndex (ptHead, ptTail)]);
}

inline int CVehicle::CalcDir () const
{

	return (aiDir [GetDirIndex (m_ptHead, m_ptTail)]);
}

inline int CVehicle::CalcNextDir () const
{

	return (aiDir [GetDirIndex (m_ptNext, m_ptHead)]);
}

inline int CVehicle::CalcBaseDir () const
{

	return (aiBaseDir [GetDirIndex (m_ptHead, m_ptTail)]);
}

inline int CVehicle::CalcNextBaseDir () const
{

	return (aiBaseDir [GetDirIndex (m_ptNext, m_ptHead)]);
}

#endif
