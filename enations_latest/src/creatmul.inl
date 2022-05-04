#include "creatmul.h"
#include "join.h"


inline CCreateMulti * CDlgCreatePublish::GetNew () 
												{ ASSERT (m_pCm->m_iTyp == CCreateBase::create_net); 
													return ((CCreateMulti *) m_pCm); }
inline CCreateLoadMulti *	CDlgCreatePublish::GetLoad () 
												{ ASSERT (m_pCm->m_iTyp == CCreateBase::load_multi); 
													return ((CCreateLoadMulti *) m_pCm); }

inline CCreateNewBase *	CJoinMulti::GetNew ()
												{ ASSERT (m_iTyp == CCreateBase::join_net);
													return (this); }
inline CCreateLoadBase * CJoinMulti::GetLoad ()
												{ ASSERT (m_iTyp == CCreateBase::load_join);
													return (this); }

