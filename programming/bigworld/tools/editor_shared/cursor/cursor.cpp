#include "cursor.hpp"

#include "cstdmf/bw_vector.hpp"
#include "cstdmf/singleton_manager.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	class CursorListeners
		: public BW::vector< Cursor::CursorExplicitlyChanged * >
	{
	public:
		CursorListeners()
		{
			REGISTER_SINGLETON( CursorListeners )
		}

		static CursorListeners & instance();
	};

	CursorListeners s_PosChangedListeners;

	CursorListeners & CursorListeners::instance()
	{
		SINGLETON_MANAGER_WRAPPER( CursorListeners );
		return s_PosChangedListeners;
	}
}

//==============================================================================
/*static */void Cursor::addPositionChangedListener(
	CursorExplicitlyChanged & listener )
{
	CursorListeners::instance().push_back( &listener );
}


//==============================================================================
/*static */void Cursor::emitCursorPosChanged( int x, int y )
{
	CursorListeners & listeners = CursorListeners::instance();
	for( CursorExplicitlyChanged * handler : listeners )
	{
		(*handler)( x, y );
	}
}

BW_END_NAMESPACE

