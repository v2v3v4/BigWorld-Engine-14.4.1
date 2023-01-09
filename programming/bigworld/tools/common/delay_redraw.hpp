#ifndef DELAY_REDRAW
#define DELAY_REDRAW

#include "cstdmf/bw_map.hpp"

BW_BEGIN_NAMESPACE

/**
 * A helper class that allows scoped disabling of updates
 */
class DelayRedraw
{
public:
	DelayRedraw( CWnd* wnd );
	~DelayRedraw();
private:
	CWnd* wnd_;
	static BW::map< CWnd*, int > s_counter_;
};

BW_END_NAMESPACE
#endif // DELAY_REDRAW
