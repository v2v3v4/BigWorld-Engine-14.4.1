#ifndef CHUNK_WATCH_CONTROL_HPP
#define CHUNK_WATCH_CONTROL_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "controls/dib_section32.hpp"
#include "controls/memdc.hpp"

BW_BEGIN_NAMESPACE

class ChunkWatchControl : public CWnd
{
public:
	enum DrawOptions
	{
		DRAW_PROJECTVIEW		=  1,
		DRAW_CHUNKS				=  2,
		DRAW_GRID				=  4,
		DRAW_USERPOS			=  8,
		DRAW_FRUSTUM			= 16,
		DRAW_UNLOADABLE			= 32,
		DRAW_WORKING			= 64,

		DRAW_ALL				= DRAW_PROJECTVIEW | DRAW_CHUNKS | DRAW_GRID
									| DRAW_USERPOS | DRAW_FRUSTUM
									| DRAW_UNLOADABLE | DRAW_WORKING
	};

	ChunkWatchControl();
	~ChunkWatchControl();

	void onNewSpace(float chunkSize, int32 minX, int32 minZ, int32 maxX, int32 maxZ);
	void onChangedChunk(int32 x, int32 z);

	BOOL Create(CRect const &extents, CWnd *parent);

	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC *dc);
	afx_msg void OnUpdateControl();
	afx_msg void OnLButtonUp(UINT flags, CPoint point);	
	afx_msg void OnMouseMove(UINT flags, CPoint point);	

	static COLORREF loadColour();
	static COLORREF unloadColour();
	static COLORREF dirtyColour();
	static COLORREF calcedColour();

	uint32 drawOptions() const;
	void enableDrawOptions(uint32 options, bool update = true);
	void disableDrawOptions(uint32 options, bool update = true);

	bool 
	getInfoText
	(
		CPoint			const &pos, 
		BW::string		&chunkText, 
		BW::string		&posText
	) const;

	DECLARE_MESSAGE_MAP()

protected:
	void getDrawConstants();

	void drawProject   (CDC &dc);
	void drawChunks    (CDC &dc);
	void drawUnloadable(CDC &dc);
	void drawGrid      (CDC &dc);
	void drawArrow     (CDC &dc);
	void drawFrustum   (CDC &dc);

	void worldToScreen(float *sx, float *sy, float wx, float wz) const;
	bool screenToWorld(CPoint const &pos, float *wx, float *wz) const;

	bool updateProjectDib();

	void redraw(bool force = false);

private:
	controls::DibSection32	projectDib_;
	controls::DibSection32	smallProjectDib_;
	controls::DibSection32	chunksDib_;
	uint32					projectMark_;
	Matrix					drawPos_;
	uint64					lastUpdateTime_;
	uint64					lastUpdateProjectTime_;
	CRect					extents_;
	float					chunkSize_;
	int32					minX_;
	int32					minZ_;
	uint32					chunksWide_;
	uint32					chunksHigh_;
	uint32					drawOptions_;
	controls::MemDC			memDC_;
};

BW_END_NAMESPACE

#endif // CHUNK_WATCH_CONTROL_HPP
