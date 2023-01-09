#ifndef ITEM_LOCATOR_HPP
#define ITEM_LOCATOR_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "gizmo/tool_locator.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is a locator that finds a chunk item under the cursor.
 *	Its matrix is determined by a supplemental locator set into the
 *	class.
 */
class ChunkItemLocator : public ToolLocator
{
	Py_Header( ChunkItemLocator, ToolLocator )
public:
	ChunkItemLocator( ToolLocatorPtr pSubLoc = NULL,
		PyTypeObject * pType = &s_type_ );
	~ChunkItemLocator();

	virtual void calculatePosition( const Vector3& worldRay, Tool& tool );

	virtual bool positionValid() const { return positionValid_; }

	ChunkItemPtr	chunkItem();

	PY_RW_ATTRIBUTE_DECLARE( subLocator_, subLocator )

	PyObject * pyGet_revealer();
	PY_RO_ATTRIBUTE_SET( revealer )

	PY_RW_ATTRIBUTE_DECLARE( enabled_, enabled )

	PY_FACTORY_DECLARE()

private:
	ChunkItemLocator( const ChunkItemLocator& );
	ChunkItemLocator& operator=( const ChunkItemLocator& );

	ToolLocatorPtr	subLocator_;
	ChunkItemPtr	chunkItem_;
	bool			enabled_;
	bool			positionValid_;
};


/**
 *	This class implements a tool locator that sits on the xz plane,
 *	with the y value being set by the last item interaction.
 *
 *	Additionally, XZ snaps are taken into account.
 */
class ItemToolLocator : public ToolLocator
{
	Py_Header( ItemToolLocator, ToolLocator )
public:
	ItemToolLocator( PyTypeObject * pType = &s_type_ );
	virtual void calculatePosition( const Vector3& worldRay, Tool& tool );

	virtual bool positionValid() const { return positionValid_; }
	/** If the position can not be set
	  * because the raycast from the camera to the world
	  * does not hit anything.
	  * Keep the raycast direction, as we can use this when placing
	  * the objects relate to the camera 
	  */
	virtual Vector3 direction() const { return direction_; }

	PY_FACTORY_DECLARE()
private:
	bool			positionValid_;
	Vector3			direction_;

	LOCATOR_FACTORY_DECLARE( ItemToolLocator() )
};


BW_END_NAMESPACE

#endif // ITEM_LOCATOR_HPP
