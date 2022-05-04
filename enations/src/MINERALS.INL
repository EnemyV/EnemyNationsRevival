#ifndef __MINEARLS_INL__
#define __MINEARLS_INL__

#include "minerals.h"


inline CMinerals::CMinerals (CMinerals const & src) 
									{ ASSERT_STRICT_VALID (&src); 
									  m_lQuantity = src.m_lQuantity; 
									  m_cType = src.m_cType;
										m_cDensity = src.m_cDensity; }
inline const CMinerals & CMinerals::operator= (const CMinerals & src)
									{	ASSERT_STRICT_VALID (&src);
										m_lQuantity = src.m_lQuantity; 
									  m_cType = src.m_cType;
										m_cDensity = src.m_cDensity;
										return (* this); }


#endif
