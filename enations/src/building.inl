#ifndef __BUILDING_INL__
#define __BUILDING_INL__

#include "building.h"
#include "player.h"
#include "unit_wnd.h"


//   CStructure
inline CStructureData const * CStructure::GetData (int iIndex) const
			{ ASSERT_STRICT ((0 <= iIndex) && (iIndex < m_iNumBuildings));
				ASSERT ((0 <= iIndex) && (iIndex < m_iNumBuildings));
				ASSERT_STRICT_VALID (this); 
				return (* (m_ppData + iIndex)); }
inline CStructureData * CStructure::_GetData (int iIndex) const
			{ ASSERT_STRICT ((0 <= iIndex) && (iIndex < m_iNumBuildings));
				ASSERT ((0 <= iIndex) && (iIndex < m_iNumBuildings));
				ASSERT_STRICT_VALID (this); 
				return (* (m_ppData + iIndex)); }
inline int CStructure::GetNumBuildings () const { return (m_iNumBuildings); }
inline int CStructureData::GetPopHoused () const { ASSERT_STRICT_VALID (this); return (m_iMaxMaterials); }

//   CBuildMaterials
inline int CBuildMaterials::GetTime () const { ASSERT_STRICT_VALID (this); return (m_iTime); }
inline int CBuildMaterials::GetInput (int iInd) const { ASSERT_STRICT ((0 <= iInd) && (iInd < CMaterialTypes::num_types));
													ASSERT_STRICT_VALID (this); return (m_aiInput [iInd]); }
inline int CBuildMaterials::GetOutput (int iInd) const { ASSERT_STRICT ((0 <= iInd) && (iInd < CMaterialTypes::num_types));
													ASSERT_STRICT_VALID (this); return (m_aiOutput [iInd]); }

//   CBuildUnit
inline int CBuildUnit::GetTime () const { ASSERT_STRICT_VALID (this); return (m_iTime); }
inline int CBuildUnit::GetInput (int iInd) const { ASSERT_STRICT ((0 <= iInd) && (iInd < CMaterialTypes::num_build_types));
													ASSERT_STRICT_VALID (this); return (m_aiInput [iInd]); }
inline int CBuildUnit::GetVehType () const { ASSERT_STRICT_VALID (this); return (m_iVehType); }

//   CBuildVehicle
inline int CBuildVehicle::GetSize () const { ASSERT_STRICT_VALID (this); return (m_aBldUnits.GetSize ()); }
inline int CBuildVehicle::GetTotalInput (int iInd) const { ASSERT_STRICT ((0 <= iInd) && (iInd < CMaterialTypes::num_build_types));
													ASSERT_STRICT_VALID (this); return (m_aiTotalInput [iInd]); }
inline CBuildUnit const *	CBuildVehicle::GetUnit (int iInd) const { ASSERT_STRICT_VALID (this);
											ASSERT_STRICT ((0 <= iInd) && (iInd < m_aBldUnits.GetSize ()));
											return (m_aBldUnits[iInd]); }

//   CBuildRepair
inline int CBuildRepair::GetTotalRepair (int iInd) const { ASSERT_STRICT ((0 <= iInd) && (iInd < CMaterialTypes::num_build_types));
													ASSERT_STRICT_VALID (this); return (m_aiTotalRepair [iInd]); }
inline CBuildUnit const *	CBuildRepair::GetRepair (int iInd) const { ASSERT_STRICT_VALID (this);
											ASSERT_STRICT ((0 <= iInd) && (iInd < m_aBldUnits.GetSize ()));
											return (m_aRprUnits[iInd]); }

//   CBuildShipyard

//   CStructureData
inline CStructureData::BLDG_TYPE CStructureData::GetType () const { ASSERT_STRICT_VALID (this); return ( m_iType); }
inline CStructureData::BLDG_UNION_TYPE CStructureData::GetUnionType () const { ASSERT_STRICT_VALID (this); return ( m_iUnionType); }
inline int CStructureData::GetCat () const { ASSERT_STRICT_VALID (this); return ( m_iBldgCat); }
inline int CStructureData::GetTimeBuild () const { ASSERT_STRICT_VALID (this); return ( m_iTimeBuild); }
inline int CStructureData::GetMoraleEffect () const { ASSERT_STRICT_VALID (this); return ( m_iMoraleEffect); }
inline int CStructureData::GetBuild (int iInd) const { ASSERT_STRICT ((0 <= iInd) && (iInd < CMaterialTypes::num_build_types));
													ASSERT_STRICT_VALID (this); return ( m_aiBuild [iInd]); }
inline int CStructureData::GetPower () const { ASSERT_STRICT_VALID (this); return ( m_iPower); }
inline float CStructureData::GetNoPower () const { ASSERT_STRICT_VALID (this); return (m_fNoPower); }
inline CBuildHousing * CStructureData::GetBldHousing () const {
													ASSERT_STRICT_VALID (this); ASSERT_STRICT (m_iUnionType == UThousing);
													return ((CBuildHousing *) this); }
inline CBuildMaterials * CStructureData::GetBldMaterials () const {
													ASSERT_STRICT_VALID (this); ASSERT_STRICT (m_iUnionType == UTmaterials);
													return ((CBuildMaterials *) this); }
inline CBuildVehicle * CStructureData::GetBldVehicle () const {
													ASSERT_STRICT_VALID (this); ASSERT_STRICT (m_iUnionType == UTvehicle);
													return ((CBuildVehicle *) this); }
inline CBuildPower * CStructureData::GetBldPower () const {
													ASSERT_STRICT_VALID (this); ASSERT_STRICT (m_iUnionType == UTpower);
													return ((CBuildPower *) this); }
inline CBuildResearch * CStructureData::GetBldResearch () const {
													ASSERT_STRICT_VALID (this); ASSERT_STRICT (m_iUnionType == UTresearch);
													return ((CBuildResearch *) this); }
inline CBuildMine * CStructureData::GetBldMine () const {
													ASSERT_STRICT_VALID (this); ASSERT_STRICT (m_iUnionType == UTmine);
													return ((CBuildMine *) this); }
inline CBuildWarehouse * CStructureData::GetBldWarehouse () const {
													ASSERT_STRICT_VALID (this); ASSERT_STRICT (m_iUnionType == UTwarehouse);
													return ((CBuildWarehouse *) this); }
inline CBuildFarm * CStructureData::GetBldFarm () const {
													ASSERT_STRICT_VALID (this); ASSERT_STRICT (m_iUnionType == UTfarm);
													return ((CBuildFarm *) this); }
inline CBuildRepair * CStructureData::GetBldRepair () const {
													ASSERT_STRICT_VALID (this); ASSERT_STRICT (m_iUnionType == UTrepair);
													return ((CBuildRepair *) this); }
inline CBuildShipyard * CStructureData::GetBldShipyard () const {
													ASSERT_STRICT_VALID (this); ASSERT_STRICT (m_iUnionType == UTshipyard);
													return ((CBuildShipyard *) this); }
inline BOOL CStructureData::HasVehExit () const { ASSERT_STRICT_VALID (this); return (m_bFlags & FlVehExit); }
inline BOOL CStructureData::HasShipExit () const { ASSERT_STRICT_VALID (this); return (m_bFlags & FlShipExit); }
inline BOOL CStructureData::IsPartWater () const { ASSERT_STRICT_VALID (this); return (m_bFlags & FlPartWater); }

//   CBuilding
inline int CBuilding::GetProd (float fProd)
									{ float fInc = m_fDamPerfMult * 
																			GetOwner()->GetPplMult () * fProd / AVG_SPEED_MUL;
										if (GetOwner()->GetPwrMult () < 1)
											fInc *= GetData()->GetNoPower () + (1.0f - 
																GetData()->GetNoPower ()) * GetOwner()->GetPwrMult ();
										float fRtn = fInc * theGame.GetOpersElapsed () + m_fOperMod;
										int iRtn = (int) fRtn;
										m_fOperMod = fRtn - (float) iRtn;
										return ( iRtn ); }
inline int CBuilding::GetProdNoPeople (float fProd)
									{ float fInc = m_fDamPerfMult * fProd / AVG_SPEED_MUL;
										if (GetOwner()->GetPwrMult () < 1)
											fInc *= GetData()->GetNoPower () + (1.0f - 
																GetData()->GetNoPower ()) * GetOwner()->GetPwrMult ();
										float fRtn = fInc * theGame.GetOpersElapsed () + m_fOperMod;
										int iRtn = (int) fRtn;
										m_fOperMod = fRtn - (float) iRtn;
										return ( iRtn ); }
inline int CBuilding::GetProdNoDamage (float fProd)
									{ float fInc = GetOwner()->GetPplMult () * 
																			fProd / AVG_SPEED_MUL;
										if (GetOwner()->GetPwrMult () < 1)
											fInc *= GetData()->GetNoPower () + (1.0f - 
																GetData()->GetNoPower ()) * GetOwner()->GetPwrMult ();
										float fRtn = fInc * theGame.GetOpersElapsed () + m_fOperMod;
										int iRtn = (int) fRtn;
										m_fOperMod = fRtn - (float) iRtn;
										return ( iRtn ); }
inline float CBuilding::GetFrameProd (float fProd)
									{ float fInc = m_fDamPerfMult * GetOwner()->GetPplMult () * fProd;
										if (GetOwner()->GetPwrMult () < 1)
											fInc *= GetData()->GetNoPower () + (1.0f - 
																GetData()->GetNoPower ()) * GetOwner()->GetPwrMult ();
										return (fInc); }
inline float CBuilding::GetFrameProdNoPeople (float fProd)
									{ float fInc = m_fDamPerfMult * fProd;
										if (GetOwner()->GetPwrMult () < 1)
											fInc *= GetData()->GetNoPower () + (1.0f - 
																GetData()->GetNoPower ()) * GetOwner()->GetPwrMult ();
										return (fInc); }

inline CHexCoord & CBuilding::GetHex () { ASSERT_STRICT_VALID (this); return (m_hex); }
inline CHexCoord const & CBuilding::GetHex () const { ASSERT_STRICT_VALID (this); return (m_hex); }
inline BOOL CBuilding::IsConstructing () const { ASSERT_STRICT_VALID (this); return ((BOOL) (m_iConstDone != -1)); }
inline CStructureData const * CBuilding::GetData () const { ASSERT_STRICT_VALID (this); return ((CStructureData const *) m_pUnitData); }
inline int CBuilding::NeedToBuild (int iInd, int iNum) const 	{ ASSERT_STRICT_VALID (this);
												return ((GetData()->GetBuild (iInd) * iNum) / 100 - 
												(GetData()->GetBuild (iInd) * m_iLastMaterialTake) / 100); }

inline int CBuilding::NeedToRepair (int iInd, int iPts) const 	
												{ ASSERT_STRICT_VALID (this);
													int iMax = ( GetData()->GetTimeBuild () * 
																			( GetData()->GetDamagePoints () - m_iDamagePoints ) ) /
																			( 2 * GetData()->GetDamagePoints () );
													iPts = __min ( iPts, iMax );
													return ((GetData()->GetBuild (iInd) * iPts) / 
																	GetData()->GetTimeBuild ()); }
inline int CBuilding::PointsRepaired (int iInd, int iMat) const 
												{ return ( ( iMat * GetData()->GetTimeBuild () ) / 
																	GetData()->GetBuild (iInd) ); }

inline int CBuilding::GetPerHave () const { ASSERT_STRICT_VALID (this); return (m_iMatTo); }

//   CVehicleBuilding
inline CBuildUnit const * CVehicleBuilding::GetBldUnt () const { ASSERT_STRICT_VALID (this); return (m_pBldUnt); }
inline void CVehicleBuilding::UpdateChoices () { if (m_pDlgTransport != NULL) m_pDlgTransport->UpdateChoices (); }

//   CBuildRepair
inline CBuildRepair const *	CRepairBuilding::GetData () const 
																{ ASSERT_STRICT_VALID (this);
																	return ((CBuildRepair const *) m_pUnitData); }

//   CBuildShipyard
inline CBuildShipyard const *	CShipyardBuilding::GetData () const 
																{ ASSERT_STRICT_VALID (this);
																	return ((CBuildShipyard const *) m_pUnitData); }

inline CBuilding * CBuildingMap::_GetBldg (DWORD dwID) const
									{ CBuilding * pBldg;
										if (Lookup (dwID, pBldg) == NULL)
											return (NULL);
										ASSERT_STRICT_VALID (pBldg);
										return (pBldg);
									}
inline CBuilding * CBuildingMap::GetBldg (DWORD dwID) const
									{ CBuilding * pBldg = _GetBldg (dwID);
										if ((pBldg == NULL) || (pBldg->GetFlags () & CUnit::dying))
											return (NULL);
										return (pBldg);
									}

inline void CBuildingHex::_GrabHex (int x, int y, CBuilding const * pBldg)
											{ SetAt (ToArg (x,y), (CBuilding *) pBldg); 
												theMap._GetHex (x, y)->OrUnits (CHex::bldg);
											}
inline void CBuildingHex::_ReleaseHex (int x, int y)
											{ RemoveKey (ToArg (x,y));
												theMap._GetHex (x, y)->NandUnits (CHex::bldg);
											}
inline CBuilding * CBuildingHex::GetBuilding (CHexCoord const & hex) const
											{ return (GetBuilding (hex.X (), hex.Y ())); }
inline CBuilding * CBuildingHex::GetBuilding (CSubHex const & sub) const
											{ return (GetBuilding (sub.x / 2, sub.y / 2)); }
inline CBuilding * CBuildingHex::GetBuilding (int x, int y) const
											{ x = CHexCoord::Wrap (x);
												y = CHexCoord::Wrap (y);
												CBuilding * pBldg; if (Lookup (ToArg (x,y), pBldg)) return (pBldg); return (NULL); }
inline DWORD CBuildingHex::ToArg (int x, int y) const { return (((DWORD) CHexCoord::Wrap (x) << 16) | (DWORD) CHexCoord::Wrap (y)); }

inline CBuilding * CBuildingHex::_GetBuilding (CHexCoord const & hex) const
											{ return (_GetBuilding (hex.X (), hex.Y ())); }
inline CBuilding * CBuildingHex::_GetBuilding (CSubHex const & sub) const
											{ return (_GetBuilding (sub.x / 2, sub.y / 2)); }
inline CBuilding * CBuildingHex::_GetBuilding (int x, int y) const
											{ CBuilding * pBldg; if (Lookup (_ToArg (x,y), pBldg)) return (pBldg); return (NULL); }
inline DWORD CBuildingHex::_ToArg (int x, int y) const { return (((DWORD) x << 16) | (DWORD) y); }


#endif
