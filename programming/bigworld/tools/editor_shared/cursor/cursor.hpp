#ifndef CURSOR_HPP
#define CURSOR_HPP

#include <functional>
#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class Cursor
{
public:
	typedef std::function< void ( int, int ) >	CursorExplicitlyChanged;

	static void setPosition( int x, int y );
	static void addPositionChangedListener( CursorExplicitlyChanged & listener );

private:
	static void emitCursorPosChanged( int x, int y );
};

BW_END_NAMESPACE

#endif //CURSOR_HPP