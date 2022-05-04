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
//
//--------------------------------------------------------------

#ifndef BTREE_H
#define BTREE_H

//--------------------------- B T r e e N o d e ----------------

class BTreeNode
#ifdef _DEBUG
: public CObject
#endif
{
public:

	BTreeNode( void 		*pvData,
				  BTreeNode *pnodeParent = NULL,
				  BTreeNode *pnodeLeft   = NULL,
				  BTreeNode *pnodeRight  = NULL )
		:
			m_pvData     ( pvData      ),
			m_pnodeParent( pnodeParent ),
			m_pnodeLeft  ( pnodeLeft   ),
			m_pnodeRight ( pnodeRight  )
	{
		ASSERT_VALID( this );
	}

	void 		 *Data() const
				 { ASSERT_VALID( this ); return m_pvData; }

	BTreeNode *Left() const				 		{ 	ASSERT_VALID( this );
															return m_pnodeLeft; }

	BTreeNode *Left( BTreeNode *pnode );

	BTreeNode *Right() const	 		 		{ 	ASSERT_VALID( this );
				   										return m_pnodeRight; }

	BTreeNode *Right( BTreeNode *pnode );

	BTreeNode *Parent() const	 		 		{ 	ASSERT_VALID( this );
															return m_pnodeParent; }

	BTreeNode *Parent( BTreeNode *pnode );

	#ifdef _DEBUG
	virtual void AssertValid() const;
	#endif

	void 		 *m_pvData;
	BTreeNode *m_pnodeLeft;
	BTreeNode *m_pnodeRight;
	BTreeNode *m_pnodeParent;
};

//--------------------------------------------------------------
// BTreeNode::Left
//--------------------------------------------------------------
inline BTreeNode *
BTreeNode::Left(
	BTreeNode *pnode )
{
	ASSERT_VALID( this );

	m_pnodeLeft = pnode;

	if ( pnode )
		pnode->Parent( this );

	ASSERT_VALID( this );

	return pnode;
}

//--------------------------------------------------------------
// BTreeNode::Right
//--------------------------------------------------------------
inline BTreeNode *
BTreeNode::Right(
	BTreeNode *pnode )
{
	ASSERT_VALID( this );

	m_pnodeRight = pnode;

	if ( pnode )
		pnode->Parent( this );

	ASSERT_VALID( this );

	return pnode;
}

//--------------------------------------------------------------
// BTreeNode::Parent
//--------------------------------------------------------------
inline BTreeNode *
BTreeNode::Parent(
	BTreeNode *pnode )
{
	ASSERT_VALID( this );

  	m_pnodeParent = pnode;

	ASSERT_VALID( this );

	return m_pnodeParent;
}

//------------------------------- B T r e e --------------------

template <class T>
class BTree
#ifdef _DEBUG
: public CObject
#endif
{
public:

	BTree( BOOL bOwns = TRUE )
	:
		m_bOwns    ( bOwns ),
		m_pnodeRoot( NULL ),
		m_iCount   ( 0 )
	{
		ASSERT_VALID( this );
	}

  ~BTree();

	BTreeNode *	Insert( T * pt );
	BTreeNode *	Find  ( T * pt ) const;

	void			Purge();

	BTreeNode * Root()  const 		{ ASSERT_VALID( this );
											  return m_pnodeRoot; }

	int			Count() const 		{ ASSERT_VALID( this );
											  return m_iCount; }

	BOOL			Owns()  const 		{ ASSERT_VALID( this );
											  return m_bOwns; }

	#ifdef _DEBUG
	virtual void AssertValid() const;
	#endif

	friend class BTreeIter<T>;

protected:

	void	Root ( BTreeNode * pnode );
	T *	Data ( BTreeNode * pnode ) const;

	void	DeleteNode ( BTreeNode *pnode );
	void	DeleteValue( T * pt );

//private:

	BTreeNode *	m_pnodeRoot;
	BOOL			m_bOwns;
	int			m_iCount;
};

//--------------------------------------------------------------
// BTree::~BTree
//--------------------------------------------------------------
template <class T>
BTree<T>::~BTree()
{
	ASSERT_VALID( this );

	Purge();

	ASSERT_VALID( this );
}

//--------------------------------------------------------------
// BTree::Purge
//--------------------------------------------------------------
template <class T>
void
BTree<T>::Purge()
{
	ASSERT_VALID( this );

	BTreeNode	* pnode = Root();

	if ( !pnode )
		return;
	else
		while ( pnode->Left() )
			pnode = pnode->Left();

	for ( ;; )
	{
		if ( pnode->Right() )
		{
			pnode = pnode->Right();

			while ( pnode->Left() )
				pnode = pnode->Left();
		}
		else
		{
			while ( pnode->Parent() &&
					  pnode->Parent()->Right() == pnode )
			{
				pnode = pnode->Parent();

				DeleteNode( pnode->Right() );

				pnode->Right( NULL );
			}

			if ( !pnode->Parent() )
			{
				DeleteNode( pnode );

				Root( NULL );

				return;
			}

			ASSERT( pnode->Parent() );

			pnode = pnode->Parent();

			DeleteNode( pnode->Left() );

			pnode->Left( NULL );
		}
	}
}

//--------------------------------------------------------------
// BTree::Root
//--------------------------------------------------------------
template <class T>
void
BTree<T>::Root(
	BTreeNode *pnode )
{
	ASSERT_VALID( this );

	m_pnodeRoot = pnode;
	
	if ( pnode )
		pnode->Parent( NULL );

	ASSERT_VALID( this );
}

//--------------------------------------------------------------
// BTree::Data
//--------------------------------------------------------------
template <class T>
T *
BTree<T>::Data(
	BTreeNode *pnode ) const
{
	ASSERT_VALID( this );

	if ( pnode )
		return ( T * )pnode->Data();
	else
		return NULL;
}

//--------------------------------------------------------------
// BTree::DeleteNode
//--------------------------------------------------------------
template <class T>
void
BTree<T>::DeleteNode(
	BTreeNode *pnode )
{
	ASSERT_VALID( this );

	ASSERT( pnode );

	DeleteValue( Data( pnode ));

	delete pnode;

	ASSERT( m_iCount > 0 );

	--m_iCount;
}

//--------------------------------------------------------------
// BTree::DeleteValue
//--------------------------------------------------------------
template <class T>
void
BTree<T>::DeleteValue(
	T * pt )
{
	ASSERT_VALID( this );

	if ( Owns() )
		delete pt;

	ASSERT_VALID( this );
}

//--------------------------------------------------------------
// BTree::Insert
//--------------------------------------------------------------
template <class T>
BTreeNode *
BTree<T>::Insert(
	T *pt )
{
	ASSERT_VALID( this );
	ASSERT( pt );

	BTreeNode *pnode 		  = Root();
	BTreeNode *pnodeParent = NULL;

	if ( !pnode )
	{
		Root( new BTreeNode( pt, NULL ));

		m_iCount++;

		return Root();
	}

	while ( pnode )
	{
		pnodeParent = pnode;

		if ( *pt < *( T * )pnode->Data() )
			pnode = pnode->Left();
		else if ( *( T * )pnode->Data() < *pt )
			pnode = pnode->Right();
		else
		{
			if ( Owns() )
				delete pt;

			return pnode;
		}
	}

	pnode = new BTreeNode( pt, pnodeParent );
	
	if ( *pt < *( T * )pnodeParent->Data() )
		pnodeParent->Left( pnode );
	else
		pnodeParent->Right( pnode );

	m_iCount++;

	ASSERT_VALID( this );

	return pnode;
}

//------------------------- B T r e e I t e r ------------------

template <class T>
class BTreeIter
#ifdef _DEBUG
: public CObject
#endif
{

public:

	BTreeIter( BTree<T> * );
	BTreeIter( BTree<T> *, BTreeNode * );

	T * 	Value()	{	ASSERT_VALID( this );
							return m_pnode ? ( T * )m_pnode->Data() : NULL; }

	void 	ToNext();
	T *	ToLast();
	void	Reset();
	void	Reset( BTreeNode * );
	void	Delete();

	#ifdef _DEBUG
	virtual void AssertValid() const;
	#endif

//private:

	BTree<T>   *m_pbtree;
	BTreeNode  *m_pnode;
};

//--------------------------------------------------------------
// BTree::Find
//--------------------------------------------------------------
template <class T>
BTreeNode *
BTree<T>::Find(
	T *pt ) const
{
	ASSERT_VALID( this );
	ASSERT( pt );

	BTreeNode *pnode = Root();

	while ( pnode )
		if ( *pt == *( T * )pnode->Data() )
			return pnode;
		else if ( *pt < *( T * )pnode->Data() )
			pnode = pnode->Left();
		else
			pnode = pnode->Right();

	return NULL;
}

//--------------------------------------------------------------
// BTreeIter::BTreeIter
//--------------------------------------------------------------
template <class T>
BTreeIter<T>::BTreeIter(
	BTree<T> *pbtree )
	:
		m_pbtree( pbtree ),
		m_pnode ( NULL )
{
	Reset();

	ASSERT_VALID( this );
}

//--------------------------------------------------------------
// BTreeIter::BTreeIter
//--------------------------------------------------------------
template <class T>
BTreeIter<T>::BTreeIter(
	BTree<T> 	*pbtree,
	BTreeNode	*pnode )
	:
		m_pbtree( pbtree ),
		m_pnode ( NULL   )
{
	Reset( pnode );

	ASSERT_VALID( this );
}

//--------------------------------------------------------------
// BTreeIter::Reset
//--------------------------------------------------------------
template <class T>
void
BTreeIter<T>::Reset()
{
	ASSERT_VALID( this );

	m_pnode = m_pbtree->Root();

	if ( m_pnode )
		while ( m_pnode->Left() )
			m_pnode = m_pnode->Left();

	ASSERT_VALID( this );
}

//--------------------------------------------------------------
// BTreeIter::Reset
//--------------------------------------------------------------
template <class T>
void
BTreeIter<T>::Reset(
	BTreeNode *pnode )
{
	#ifdef _DEBUG
	if ( m_pnode )	// 1st time is construction
		ASSERT_VALID( this );
	#endif

	m_pnode = pnode;

	ASSERT_VALID( this );
}

//--------------------------------------------------------------
// BTreeIter::ToNext
//--------------------------------------------------------------
template <class T>
void
BTreeIter<T>::ToNext()
{
	ASSERT_VALID( this );

	BTreeNode	*pnodePrev = m_pnode;

	if ( m_pnode->Right() )
	{
		m_pnode = m_pnode->Right();

		while ( m_pnode->Left() )
			m_pnode = m_pnode->Left();
	}
	else
	{
		while ( m_pnode->Parent() &&
				  m_pnode->Parent()->Right() == m_pnode )

			m_pnode = m_pnode->Parent();

		m_pnode = m_pnode->Parent();
	}

	ASSERT( m_pnode != pnodePrev );
	ASSERT_VALID( this );
}

//--------------------------------------------------------------
// BTreeIter::ToLast
//--------------------------------------------------------------
template <class T>
T *
BTreeIter<T>::ToLast()
{
	ASSERT_VALID( this );

	if ( m_pnode )
	{
		while ( m_pnode->Parent() )
			m_pnode = m_pnode->Parent();

		while ( m_pnode && m_pnode->Right() )
			m_pnode = m_pnode->Right();
	}

	ASSERT_VALID( this );

	return Value();
}

//--------------------------------------------------------------
// BTreeIter::Delete
//--------------------------------------------------------------
template <class T>
void
BTreeIter<T>::Delete()
{
	ASSERT_VALID( this );

	BTreeNode	*pnode 			= m_pnode;
	BTreeNode	*pnodeOriginal = pnode;
	BTreeNode	*pnodeParent   = pnode->Parent();

	ToNext();

	if ( !pnode->Right() )
		pnode = pnode->Left();
	else if ( pnode->Right()->Left() )
	{
		BTreeNode *pnodeCurrent = pnode->Right();

		while ( pnodeCurrent->Left()->Left() )
			pnodeCurrent = pnodeCurrent->Left();

		pnode = pnodeCurrent->Left();

		pnodeCurrent->Left ( pnode->Right() );
		pnode			->Left ( pnodeOriginal->Left()  );
		pnode			->Right( pnodeOriginal->Right() );
	}
	else
	{
		pnode = pnode->Right();

		pnode->Left( pnodeOriginal->Left() );
	}

	if ( !pnodeParent )
		m_pbtree->Root( pnode );
	else if ( pnodeOriginal == pnodeOriginal->Parent()->Left() )
		pnodeParent->Left( pnode );
	else
		pnodeParent->Right( pnode );

	m_pbtree->DeleteNode( pnodeOriginal );

	ASSERT_VALID( this );
}

#ifdef _DEBUG

//---------------------------------------------------------------------------
// BTree::AssertValid
//---------------------------------------------------------------------------
template <class T>
void
BTree<T>::AssertValid() const
{
	CObject::AssertValid();

//	ASSERT_VALID_OR_NULL( m_pnodeRoot );

//	ASSERT( 0 <= m_iCount && m_iCount < 10000 );	// Arbitrary limit - change at will
//	ASSERT( !m_pnodeRoot || 0 < m_iCount );
}

//---------------------------------------------------------------------------
// BTreeIter::AssertValid
//---------------------------------------------------------------------------
template <class T>
void
BTreeIter<T>::AssertValid() const
{
	CObject::AssertValid();

	ASSERT_VALID( m_pbtree );
	ASSERT_VALID_OR_NULL( m_pnode  );
}

#endif
#endif

