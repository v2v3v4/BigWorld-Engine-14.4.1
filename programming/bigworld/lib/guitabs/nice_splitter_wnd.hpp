
#ifndef NICE_SPLITTER_WND_HPP
#define NICE_SPLITTER_WND_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This abstract class is inherited by other classes wanting to get notified
 *	when a spliter window's divisory line is moved.
 */
class SplitterEventHandler
{
public:
	virtual void resizeSplitter( int lastWidth, int lastHeight, int width, int height ) = 0;
};


/**
 *  This class extends CSplitterWnd to make a better-looking splitter window.
 */
class NiceSplitterWnd : public CSplitterWnd
{
public:
	NiceSplitterWnd();

	void setEventHandler( SplitterEventHandler* handler );
	void allowResize( bool allow );
	void setMinRowSize( int minSize );
	void setMinColSize( int minSize );

protected:
	SplitterEventHandler* eventHandler_;
	int lastWidth_;
	int lastHeight_;
	bool allowResize_;
	int minRowSize_;
	int minColSize_;

	// Overrrides
	virtual void StartTracking(int ht);
	virtual void SetSplitCursor(int ht);
	virtual void TrackRowSize(int y, int row);
	virtual void TrackColumnSize(int x, int col);
	virtual void OnDrawSplitter( CDC* pDC, ESplitType nType, const CRect& rectArg );

	// Messages
	afx_msg void OnSize( UINT nType, int cx, int cy );
	DECLARE_MESSAGE_MAP()
};

BW_END_NAMESPACE

#endif // NICE_SPLITTER_WND_HPP

