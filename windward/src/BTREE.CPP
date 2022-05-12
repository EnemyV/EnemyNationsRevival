//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


//--------------------------------------------------------------
// btree.h		BTree class
//
// Copyright (c) 1995-1996 ChromeOcean Software - All Rights Reserved
// Reuse permission granted to Dave Thielen
//--------------------------------------------------------------


#include "stdafx.h"
#include "_windwrd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

//---------------------------------------------------------------------------
// BTreeNode::AssertValid
//---------------------------------------------------------------------------
#ifdef _DEBUG
void
BTreeNode::AssertValid() const
{
	CObject::AssertValid();

	ASSERT_STRICT( !m_pvData || AfxIsValidAddress( m_pvData, 1 ));

	ASSERT_STRICT( this != m_pnodeLeft );
	ASSERT_STRICT( this != m_pnodeRight );
	ASSERT_STRICT( this != m_pnodeParent );

	ASSERT_STRICT_VALID_OR_NULL( m_pnodeLeft   );
	ASSERT_STRICT_VALID_OR_NULL( m_pnodeRight  );
	ASSERT_STRICT_VALID_OR_NULL( m_pnodeParent );
}
#endif


