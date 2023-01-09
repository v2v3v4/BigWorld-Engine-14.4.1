#ifndef ITEM_VIEW_HPP
#define ITEM_VIEW_HPP

#include "gizmo/tool_view.hpp"
#include "gizmo/formatter.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class reveals a vector of chunk items
 */
class ChunkItemRevealer : public PyObjectPlus
{
	Py_Header( ChunkItemRevealer, PyObjectPlus )

public:
	ChunkItemRevealer( PyTypeObject * pType ) : PyObjectPlus( pType ) { }
	virtual ~ChunkItemRevealer() { }

	PY_SIZE_INQUIRY_METHOD(			pySeq_length )			// len(x)
	PY_BINARY_FUNC_METHOD(			pySeq_concat )			// x + y
	PY_INTARG_FUNC_METHOD(			pySeq_item )			// x[i]
	PY_INTINTARG_FUNC_METHOD(		pySeq_slice )			// x[i:j]
	PY_OBJOBJ_PROC_METHOD(			pySeq_contains )		// v in x

	typedef BW::vector<ChunkItemPtr> ChunkItems;
	virtual void reveal( ChunkItems & items ) = 0;
};
PY_SCRIPT_CONVERTERS_DECLARE( ChunkItemRevealer )


/**
 *	This class holds a vector of chunk items
 */
class ChunkItemGroup : public ChunkItemRevealer
{
	Py_Header( ChunkItemGroup, ChunkItemRevealer )
public:
	ChunkItemGroup( ChunkItems items = ChunkItems(), PyTypeObject * pType = &s_type_ );
	~ChunkItemGroup();

	bool			add( ChunkItemPtr pNewbie );
	ChunkItems		get() const	{	return items_; }

	PY_SIZE_INQUIRY_METHOD(				pySeq_length )			// len(x)
//	PY_BINARY_FUNC_METHOD(			pySeq_concat )			// x + y
//	PY_INTARG_FUNC_METHOD(			pySeq_repeat )			// x * n
//	PY_INTARG_FUNC_METHOD(			pySeq_item )			// x[i]
//	PY_INTINTARG_FUNC_METHOD(		pySeq_slice )			// x[i:j]
	PY_INTOBJARG_PROC_METHOD(		pySeq_ass_item )		// x[i] = v
	PY_INTINTOBJARG_PROC_METHOD(	pySeq_ass_slice )		// x[i:j] = v
//	PY_OBJOBJ_PROC_METHOD(			pySeq_contains )		// v in x
//	PY_BINARY_FUNC_METHOD(			pySeq_inplace_concat )	// x += y
//	PY_INTARG_FUNC_METHOD(			pySeq_inplace_repeat )	// x *= n

	PY_METHOD_DECLARE( py_add )
	PY_METHOD_DECLARE( py_rem )

	PY_RO_ATTRIBUTE_DECLARE( items_.size(), size )

	ChunkItemGroup* filterTypes( const BW::vector<BW::string>& typeNames );

	PY_AUTO_METHOD_DECLARE( RETOWN, filterTypes, ARG( BW::vector<BW::string>, END ) );

	PY_FACTORY_DECLARE()

private:
	virtual void reveal( BW::vector< ChunkItemPtr > & items );

	ChunkItems	items_;
};
PY_SCRIPT_CONVERTERS_DECLARE( ChunkItemGroup )

typedef SmartPointer<ChunkItemGroup> ChunkItemGroupPtr;

/**
 *	This class draws an item as selected. It calls a virtual function
 *	in a chunk item revealer to find the objects that it is to draw.
 */
namespace Moo
{
	class DrawContext;
}
class ChunkItemView : public ToolView
{
	Py_Header( ChunkItemView, ToolView )

public:
	ChunkItemView( PyTypeObject * pType = &s_type_ );
	~ChunkItemView();

	virtual void viewResource( const BW::string& resourceID )
		{ resourceID_ = resourceID; }
	void updateAnimations( const Tool& tool );
	virtual void render( Moo::DrawContext& drawContext, const Tool& tool );

	PY_RW_ATTRIBUTE_DECLARE( revealer_, revealer )

	PY_FACTORY_DECLARE()

protected:
	SmartPointer<ChunkItemRevealer>		revealer_;
	BW::vector< ChunkItemPtr > items_;

private:
	ChunkItemView( const ChunkItemView& );
	ChunkItemView& operator=( const ChunkItemView& );
};

/**
 *	This class draws the selected item's bounding box
 */
class ChunkItemBounds : public ChunkItemView
{
	Py_Header( ChunkItemBounds, ChunkItemView )
public:
	ChunkItemBounds( PyTypeObject * pType = &s_type_ );

	PY_RW_ATTRIBUTE_DECLARE( colour_, colour )

	static ChunkItemBounds * New(
		SmartPointer<ChunkItemRevealer> spRevealer, uint32 col,
		float growFactor, BW::string texture, float offset, float tile );
	PY_AUTO_FACTORY_DECLARE( ChunkItemBounds,
		OPTARG( SmartPointer<ChunkItemRevealer>, NULL,
		OPTARG( uint32, 0xffffffff,		// colour
		OPTARG( float, 0.0f,			// grow factor
		OPTARG( BW::string, "",		// texture
		OPTARG( float, 0.0f,			// offset
		OPTARG( float, 1.0f,			// tiling
		END ) ) ) ) ) ) )

	virtual void updateAnimations( const Tool& tool );
	virtual void render( Moo::DrawContext& drawContext, const Tool& tool );
private:
	int colour_;
	float growFactor_;
	Moo::BaseTexturePtr texture_;
	float offset_;
	float tile_;
};


/**
 * Utility method to get a vector of chunks from a vector of chunkItems
 */
BW::vector<Chunk*> extractChunks( BW::vector<ChunkItemPtr> chunkItems );


/**
 * Get a list of chunks from the items revealed by the revealer
 */
BW::vector<Chunk*> extractChunks( ChunkItemRevealer* revealer );


BW_END_NAMESPACE
#endif // ITEM_VIEW_HPP
