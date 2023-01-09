#ifndef EDITOR_CHUNK_BINDING_HPP
#define EDITOR_CHUNK_BINDING_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/items/editor_chunk_substance.hpp"
#include "chunk/chunk_marker.hpp"
#include "resmgr/string_provider.hpp"

BW_BEGIN_NAMESPACE

/** NOTE **
 *
 *	This class is not being utilised (and is not finished).
 *
 *	Binding is currently implemented as a property and is a one way attribute
 *	(i.e. only the origin item is awear of the binding)
 *
 */


/**
 *	This class implements two way bindings, where both the bindee and binder
 *	is awear of the binding.  A binding provides a way for the engine to 
 *	send messages between items.
 */
class EditorChunkBinding : public EditorChunkSubstance<ChunkBinding>
{
	DECLARE_EDITOR_CHUNK_ITEM_WITHOUT_DESCRIPTION( EditorChunkBinding )
public:
	EditorChunkBinding();
	~EditorChunkBinding();

	virtual void draw( Moo::DrawContext& drawContext );

	bool load( DataSectionPtr pSection );

	virtual bool edSave( DataSectionPtr pSection );

	virtual const Matrix & edTransform();
	virtual bool edTransform( const Matrix & m, bool transient );

	virtual bool edCanDelete();
	virtual void edPreDelete();
	virtual void edPostClone( EditorChunkItem* srcItem );

	DECLARE_EDITOR_CHUNK_ITEM_DESCRIPTION( "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_BINDING/ED_DESCRIPTION" );

	void calculateTransform(bool transient = false);

	virtual bool autoAddToSceneBrowser() const { return false; }

private:
	EditorChunkBinding( const EditorChunkBinding& );
	EditorChunkBinding& operator=( const EditorChunkBinding& );

	virtual const char * sectName() const { return "binding"; }
	virtual bool isDrawFlagVisible() const
		{ return OptionsGameObjects::entitiesVisible(); }
	virtual const char * drawFlag() const
		{ return "render/gameObjects/drawEntities"; }

	virtual ModelPtr reprModel() const;

	ModelPtr model_;

	Matrix transform_;
};

BW_END_NAMESPACE

#endif // EDITOR_CHUNK_BINDING_HPP
