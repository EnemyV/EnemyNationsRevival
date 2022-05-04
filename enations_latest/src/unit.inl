#ifndef __UNIT_INL__
#define __UNIT_INL__

#include "unit.h"
#include "player.h"

// player.h
inline void CGame::FreeQueueElem ( CNetCmd * pCmd )
													{ if ( pCmd->m_bMemPool ) MemFreeFS ( pCmd );
														  else MemFreeFS ( pCmd ); }

// unit.h
//   CUnitData
inline int CUnitData::GetRsrchReq (int iInd) const { ASSERT ((0 <= iInd) && (iInd < 4)); return (m_iRsrchReq[iInd]); }
inline int CUnitData::GetPeople () const { ASSERT_STRICT_VALID (this); return (m_iPeople); }
inline int CUnitData::GetDamagePoints () const { ASSERT_STRICT_VALID (this); return (m_iDamagePoints); }
inline int CUnitData::GetTargetType () const { ASSERT_STRICT_VALID (this); return (m_iTargetType); }
inline int CUnitData::_GetFireRate () const { ASSERT_STRICT_VALID (this); return (m_iFireRate); }
inline CString const & CUnitData::GetDesc () const { ASSERT_STRICT_VALID (this); return (m_sDesc); }
inline CString const & CUnitData::GetText () const { ASSERT_STRICT_VALID (this); return (m_sText); }
inline int CUnitData::GetCX () const { ASSERT_STRICT_VALID (this); return (m_cx); }
inline int CUnitData::GetCY () const { ASSERT_STRICT_VALID (this); return (m_cy); }

inline int CUnitData::_GetSpottingRange () const { ASSERT_STRICT_VALID (this); return (m_iSpottingRange); }
inline int CUnitData::_GetRange () const { ASSERT_STRICT_VALID (this); return (m_iRange); }
inline int CUnitData::_GetAccuracy () const { ASSERT_STRICT_VALID (this); return (m_iAccuracy); }

//   CUnit
inline void CUnit::SetStore (int iInd, int iNum) 
																{ m_iUpdateMat = 2;
																	m_aiStore[iInd] = iNum; }
inline void CUnit::AddToStore (int iInd, int iNum)
																{ if (iNum < 0)
																		m_iUpdateMat = 2;
																	else
																		if ( (iNum > 0) && (m_iUpdateMat < 1) )
																			m_iUpdateMat = 1;
																	m_aiStore[iInd] += iNum; }
inline int CUnit::GetSpottingRange () const { ASSERT_STRICT_VALID (this); return (m_iSpottingRange); }
inline int CUnit::GetRange () const { ASSERT_STRICT_VALID (this); return (m_iRange); }
inline int CUnit::GetAccuracy () const { ASSERT_STRICT_VALID (this); return (m_iAccuracy); }
inline int CUnit::GetFireRate () const { ASSERT_STRICT_VALID (this); return (m_iFireRate); }

inline BOOL CUnit::IsPaused () const { ASSERT_STRICT_VALID (this); return ((BOOL) (m_unitFlags & stopped)); }
inline BOOL CUnit::IsWaiting () const { ASSERT_STRICT_VALID (this); return ((BOOL) (m_unitFlags & event)); }

inline int CUnit::GetDamagePoints () const { return (m_iDamagePoints); }
inline int CUnit::GetDamagePer () const { return (m_iDamagePer); }
inline int CUnit::GetLastShowDamagePer () const { return (m_iLastShowDamagePer); }
inline float CUnit::GetDamageMult () const { return (m_fDamageMult); }
inline float CUnit::GetDamPerfMult () const { return (m_fDamPerfMult); }
inline CUnitData const * CUnit::GetData () const { ASSERT_STRICT_VALID (this); return (m_pUnitData); }
inline DWORD CUnit::GetID () const { ASSERT_STRICT_VALID (this); return (m_dwID); }
inline CPlayer * CUnit::GetOwner () const { ASSERT_STRICT_VALID (this); return (m_pOwner); }
inline int CUnit::GetStore (int iInd) const { ASSERT_STRICT_VALID (this);
															ASSERT_STRICT ((0 <= iInd) && (iInd < CMaterialTypes::GetNumTypes ()));
															return ( m_aiStore[iInd]); }
inline CUnit::UNIT_TYPE CUnit::GetUnitType () const { ASSERT_STRICT_VALID (this); return ( m_iUnitType); }
inline void CUnit::DecSee (int iPlyrNum) { ASSERT (m_pdwPlyrsSee[iPlyrNum] > 0); m_pdwPlyrsSee[iPlyrNum]--; }
inline BOOL CUnit::GetSee (CUnit * pSpotter) { return ( m_pdwPlyrsSee [pSpotter->GetOwner()->GetPlyrNum ()] != 0); }


inline void * CProjBase::operator new ( size_t iSiz ) 
											{ TRAP ( iSiz > PROJ_BASE_ALLOC_SIZE ); 
												return MemAllocFS ( m_memPool ); }
inline void *	CProjBase::operator new ( size_t iSiz, const char *, int ) 
											{	return MemAllocFS ( m_memPool ); }
inline void CProjBase::operator delete ( void * pBuf ) 
											{ MemFreeFS ( pBuf ); }
inline CInitProjMem::CInitProjMem () 
											{ CProjBase::m_memPool = MemPoolInitFS ( PROJ_BASE_ALLOC_SIZE, 64, MEM_POOL_SERIALIZE ); }

inline void CProjMap::Remove (CProjBase * pProj)
											{	Remove (CHexCoord (pProj->GetMapLoc ()), pProj); }
inline DWORD CProjMap::ToArg (CMapLoc const & _ml) const 
											{ return (((DWORD) (_ml.x >> HEX_HT_PWR) << 16) | (DWORD) (_ml.y >> HEX_HT_PWR)); }
inline DWORD CProjMap::ToArg (CHexCoord const & _hex) const 
											{ return (((DWORD) _hex.X () << 16) | (DWORD) _hex.Y ()); }
inline void CProjMap::Move (CHexCoord const & _hex, CProjBase * pProj)
											{ if (_hex.SameHex (pProj->GetMapLoc ()))
													return;
												Remove (_hex, pProj);
												Add (pProj); }
inline CProjBase * CProjMap::GetFirst ( CHexCoord _hex ) const
											{ CProjBase * pPb;
												if (Lookup ( ToArg (_hex), pPb ) != NULL)
													return (pPb);
												return (NULL); }
inline CProjBase * CProjMap::GetNext ( CProjBase * pOn )
											{ return pOn->m_pNext; }


#endif
