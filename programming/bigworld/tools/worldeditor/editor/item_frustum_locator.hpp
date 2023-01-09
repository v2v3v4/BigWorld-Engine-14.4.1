#ifndef ITEM_FRUSTUM_LOCATOR_HPP
#define ITEM_FRUSTUM_LOCATOR_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "gizmo/tool_locator.hpp"
#include "gizmo/tool_view.hpp"
#include "chunk/chunk_obstacle.hpp"

BW_BEGIN_NAMESPACE

/**
 * This class is a locator that finds chunk items in a frustum
 * defined by the positions the mouse cursor has been in.
 *
 * It will generate a frustum aligned with the camera, with the same
 * near and far planes, and the corners being the extents the mouse
 * has been moved to.
 *
 * This is used to do drag selecting.
 *
 * Its matrix is determined by a supplemental locator set into the
 * class.
 */
class ChunkItemFrustumLocator : public ToolLocator
{
	Py_Header( ChunkItemFrustumLocator, ToolLocator )
public:
	ChunkItemFrustumLocator( ToolLocatorPtr pSubLoc = NULL,
		PyTypeObject * pType = &s_type_ );
	~ChunkItemFrustumLocator();

	static void fini();

	virtual void calculatePosition( const Vector3& worldRay, Tool& tool );

	PY_RW_ATTRIBUTE_DECLARE( subLocator_, subLocator )

	PyObject * pyGet_revealer();
	PY_RO_ATTRIBUTE_SET( revealer )

	PY_RW_ATTRIBUTE_DECLARE( enabled_, enabled )

	PY_FACTORY_DECLARE()

	BW::vector<ChunkItemPtr> items;
	
private:
	ChunkItemFrustumLocator( const ChunkItemFrustumLocator& );
	ChunkItemFrustumLocator& operator=( const ChunkItemFrustumLocator& );

	void enterSelectionMode();
	void leaveSelectionMode();

	ToolLocatorPtr	subLocator_;
	bool			enabled_;

	POINT startPosition_;
	POINT currentPosition_;

	Matrix oldView_;
	Matrix oldProjection_;

	bool oldDrawReflection_;

	friend class DragBoxView;
};

typedef SmartPointer<ChunkItemFrustumLocator> ChunkItemFrustumLocatorPtr;

PY_SCRIPT_CONVERTERS_DECLARE( ChunkItemFrustumLocator )


class DragBoxView : public ToolView
{
	Py_Header( DragBoxView, ToolView )

public:
	DragBoxView( ChunkItemFrustumLocatorPtr locator, Moo::Colour colour,
		PyTypeObject * pType = &s_type_ );
	~DragBoxView();

	virtual void viewResource( const BW::string& resourceID )
		{ resourceID_ = resourceID; }
	virtual void updateAnimations( const Tool& tool );
	virtual void render( Moo::DrawContext& drawContext, const Tool& tool );

	PY_AUTO_CONSTRUCTOR_FACTORY_DECLARE( DragBoxView,
		NZARG( ChunkItemFrustumLocatorPtr, ARG( uint, END ) ) )
private:
	DragBoxView( const DragBoxView& );
	DragBoxView& operator=( const DragBoxView& );

	ChunkItemFrustumLocatorPtr locator_;
	Moo::Colour colour_;
};

BW_END_NAMESPACE

#endif // ITEM_FRUSTUM_LOCATOR_HPP
