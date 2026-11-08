#include "creatmul.h"
#include "join.h"

// CDlgCreatePublish removed (Phase 2d) — its GetNew/GetLoad helpers went with it.

inline CCreateNewBase *	CJoinMulti::GetNew ()
												{ ASSERT (m_iTyp == CCreateBase::join_net);
													return (this); }
inline CCreateLoadBase * CJoinMulti::GetLoad ()
												{ ASSERT (m_iTyp == CCreateBase::load_join);
													return (this); }

