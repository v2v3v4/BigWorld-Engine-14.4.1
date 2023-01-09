#ifndef FLORA_BLOCK_HPP
#define FLORA_BLOCK_HPP

#include "math/boundbox.hpp"


BW_BEGIN_NAMESPACE

struct FloraVertex;
class Ecotype;
class Vector2;

/**
 * TODO: to be documented.
 */
class FloraBlock
{
public:
	FloraBlock();
	void init( const Vector2& pos, uint32 offset );
	uint32 offset() const { return offset_; }	
	void invalidate();
	bool needsRefill() const		{ return bRefill_; }

	const Vector2&	center() const	{ return center_; }
	void center( const Vector2& c );

	void cull();
	bool culled() const		{ return culled_; }
	void culled( bool c )	{ culled_ = c; }

	int	 blockID() const	{ return blockID_; }
	const BoundingBox& bounds() const	{ return bb_; }
	bool withinBounds( const AABB& bounds ) const;

private:
	friend class Flora;
	void fill( Flora * owner, uint32 numVertsAllowed );
	bool nextTransform( Flora * owner, const Vector2 & center, Matrix & ret,
		Vector2 & retEcotypeSamplePt );

	Vector2	center_;	
	BoundingBox	bb_;
	bool	culled_;
	bool	bRefill_;
	BW::vector<Ecotype*>	ecotypes_;
	int		blockID_;
	uint32	offset_; // number of vertexes offset from the start of the VB

#ifdef EDITOR_ENABLED
	// The following is for high lighting flora visuals, only supported in tool.
	#define MAX_RECORDED_VISUAL_COPY 1024
	//void* should be literally VisualsEcotype::VisualCopy*
	//uint32 is the offset of the vertices buffer.
	struct  RecordVisual
	{
		void*	pVisualCopy_;
		uint32	offset_;
	};
	RecordVisual recordedVisualCopy_[ MAX_RECORDED_VISUAL_COPY ];
	int numRecordedVisualCopy_;
public:
	void resetRecordedVisualCopy() { numRecordedVisualCopy_ = 0; }
	bool recordAVisualCopy( void* pVisualCopy, uint32 offset );
	void findVisualCopyVerticeOffset( void* pVisualCopy, BW::vector< uint32 >& retOffsets );
#endif

};

typedef FloraBlock*	pFloraBlock;

BW_END_NAMESPACE

#endif
