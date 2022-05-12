//--------------------------------------------------------------
// ptr.h		Ptr class
//
// Copyright 1995-1996 ChromeOcean Software - All Rights Reserved
// Reuse permission granted to Dave Thielen
//--------------------------------------------------------------

#ifndef PTR_H
#define PTR_H


//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


//---------------------------------- R e f ---------------------

class Ref
{

public:

	Ref()
	:
		_uCount( 1 )
	{}

	void	operator ++ ()
			{ ++_uCount; }

	unsigned	operator -- ()
			{ ASSERT_STRICT( _uCount > 0 ); return --_uCount; }

private:

	unsigned	_uCount;
};

//----------------------------- P t r B a s e ------------------

class PtrBase
{

public:

	PtrBase()
		{ Create( NULL ); }

	PtrBase( void *pv )
		{ Create( pv ); }

	PtrBase( const PtrBase &rptrbase )
		{ Set( rptrbase ); }

	PtrBase & operator = ( const PtrBase &rptrbase )
	{ Set( rptrbase ); return *this; }

	void Assign( const PtrBase &rptrbase )
	{
		Cleanup();
		Set( rptrbase );
	}

	void Assign( void *pv )
	{
		Cleanup();
		Create( pv );
	}

	void *Value() const
			{ return m_pv; }

protected:

	void *Release()
			{
				void	*pv = m_pv;

				m_pv = NULL;

				return pv;
			}

	virtual void Destroy() PURE_FUNC

	void 	Create( void *pv )
			{
				m_pref = NULL;
				m_pv   = pv;
			}

	void	Cleanup()
			{ 
				if ( !m_pref || 0 == --*m_pref )
				{
					delete m_pref;
					m_pref = NULL;
					Destroy();
					m_pv = NULL;
				}
			}

	void 	Set( const PtrBase &ptrbase )
			{
				if ( this != &ptrbase )
				{
					if ( !ptrbase.m_pref )
						(( PtrBase * )&ptrbase )->m_pref = new Ref;

					++*( m_pref = ptrbase.m_pref );
					m_pv = ptrbase.m_pv;
				}
			}

private:

	Ref	* m_pref;
	void	* m_pv;
};

//------------------------------- P t r <T> --------------------
//
// Reference-counting smart-pointer.
//
// You can assign it either the address of single object on the heap or
// an array of objects of a class with no destructor, also on the heap

template <class T>
class Ptr : public PtrBase
{

public:

	Ptr( T *pt = NULL )
	:
		PtrBase( pt )
		{}

	Ptr( const Ptr &rptr )
		:
			PtrBase( rptr )
		{}

  ~Ptr()												{ Cleanup(); }

	void 	operator = ( const Ptr &rptr )	{ Assign( rptr ); }

	void 	operator = ( T *pt )					{ Assign( pt ); }

	T*	operator -> ()	const						{ return Value(); }

	T& operator *  ()	const						{ return *Value(); }

	T&	operator [] ( unsigned uIndex )		{ return Value()[ uIndex ]; }

	T* Value() const								{ return ( T * )PtrBase::Value(); }

	T* Release()									{ return ( T * )PtrBase::Release();}

protected:

	void 	Destroy()								{ delete Value();	}
};

//-------------------------- S i m p l e P t r <T> -------------------------
//
// Reference-counting smart-pointer for built-in types (char, int, etc.).

template <class T>
class SimplePtr : public PtrBase
{

public:

	SimplePtr( T *pt = NULL )
	:
		PtrBase( pt )
		{}

	SimplePtr( const SimplePtr &rptr )
		:
			PtrBase( rptr )
		{}

  ~SimplePtr()												{ Cleanup(); }

	void 	operator = ( const SimplePtr &rptr )	{ Assign( rptr ); }

	void 	operator = ( T *pt )							{ Assign( pt ); }

	T& operator *  ()	const								{ return *Value(); }

	T&	operator [] ( unsigned uIndex )				{ return Value()[ uIndex ]; }

	T* Value() const										{ return ( T * )PtrBase::Value(); }

	T* Release()											{ return ( T * )PtrBase::Release();}

protected:

	void 	Destroy()										{ delete Value();	}
};

#endif

