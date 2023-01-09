#pragma once

#include "ual/ual_callback.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"
#include "math/rectt.hpp"

class CRect;
class CPen;
class CWnd;

BW_BEGIN_NAMESPACE

// Forward declarations and typedefs
class	UalDropCallback;
typedef std::pair < HWND , SmartPointer < UalDropCallback > > DropType;
typedef BW::multimap < HWND , SmartPointer < UalDropCallback > > DropMap;
typedef DropMap::const_iterator DropMapIter;

/**
 *	This class keeps track of drop target areas and notifies registered
 *	callbacks when a successful drag and drop operation is executed.
 */
class UalDropManager
{
private:

	static const int DRAG_BORDER = 3;
	static const DWORD DRAG_COLOUR = RGB( 96,96,96 );

public:

	UalDropManager();
	~UalDropManager();

	void clear();
	
	void add( SmartPointer< UalDropCallback > dropping, bool useHighlighting = true );
	
	void start( const BW::string& ext);
	
	SmartPointer< UalDropCallback > test( HWND hwnd, UalItemInfo* ii );
		
	SmartPointer< UalDropCallback > test( UalItemInfo* ii );

	bool end( UalItemInfo* ii );

	static RectInt HIT_TEST_NONE;
	static RectInt HIT_TEST_MISS;
	static RectInt HIT_TEST_OK_NO_RECT;

private:

	DropMap droppings_;
	BW::set< HWND > dontHighlightHwnds_;
	BW::string ext_;
	CPen * pen_;
	RectInt highlightRect_;
	RectInt lastHighlightRect_;
	HWND lastHighlightWnd_;

	void drawHighlightRect( HWND hwnd, const CRect & rect );

	void highlight( HWND hwnd, const CRect & rect );
};


/**
 *	This abstract class serves as the base class for drop target callback
 *	objects.
 */
class UalDropCallback : public ReferenceCount
{
public:
	UalDropCallback() {}

	UalDropCallback(CWnd* wnd, const BW::string& ext, bool canAdd ):
		wnd_(wnd),
		id_(0),
		ext_(ext),
		canAdd_(canAdd)
	{
	}

	UalDropCallback(CWnd* wnd, uint32 id, const BW::string& ext, bool canAdd ):
		wnd_(wnd),
		id_(id),
		ext_(ext),
		canAdd_(canAdd)
	{
	}

	virtual bool execute( UalItemInfo* ii ) = 0;

	CWnd* cWnd();

	HWND hWnd();

	const BW::string& ext() { return ext_; }

	bool canAdd() { return canAdd_; }

	virtual RectInt test( UalItemInfo* ii );

protected:
	CWnd* wnd_;
	BW::string ext_;
	bool canAdd_;
	uint32 id_;
};


/**
 *	This templated class is used to create functor objects that can relay
 *	notifications to an arbitrary method in an arbitrary class, similar to the
 *	functors implemented in bw_functor.hpp.
 */
template< class C >
class UalDropFunctor: public UalDropCallback
{
public:
	typedef bool (C::*Method)( UalItemInfo* ii );
	typedef RectInt (C::*Method2)( UalItemInfo* ii );
	
	UalDropFunctor() {}
	
	UalDropFunctor( CWnd* wnd, const BW::string& ext, C* instance, Method method, bool canAdd = false, Method2 test = NULL ):
		UalDropCallback( wnd, ext, canAdd ),
		instance_(instance),
		method_(method),
		test_(test)
	{
		BW_GUARD;
	}

	UalDropFunctor( CWnd* wnd, uint32 id, const BW::string& ext, C* instance, Method method, bool canAdd = false, Method2 test = NULL ):
		UalDropCallback( wnd, id, ext, canAdd ),
		instance_(instance),
		method_(method),
		test_(test)
	{
		BW_GUARD;
	}

	bool execute( UalItemInfo* ii )
	{
		BW_GUARD;

		return (instance_->*method_)( ii );
	}

	RectInt test( UalItemInfo* ii )
	{
		BW_GUARD;

		if (test_ != NULL)
			return (instance_->*test_)( ii );
		else
			return UalDropManager::HIT_TEST_NONE;

	}
private:
	C* instance_;
	Method method_;
	Method2 test_;
};

BW_END_NAMESPACE

