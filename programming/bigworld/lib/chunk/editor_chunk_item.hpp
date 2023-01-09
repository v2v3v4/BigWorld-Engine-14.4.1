#ifndef EDITOR_CHUNK_ITEM_HPP
#define EDITOR_CHUNK_ITEM_HPP

#ifndef EDITOR_ENABLED
#error EDITOR_ENABLED must be defined
#endif

#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_string.hpp"
#include "resmgr/string_provider.hpp"
#include "gizmo/meta_data.hpp"
#include "gizmo/meta_data_helper.hpp"
#include "gizmo/general_editor.hpp"
#include "../../tools/common/bw_message_info.hpp"
#include "umbra_draw_item.hpp"
#include "cstdmf/bw_functor.hpp"
#include "cstdmf/bw_stack_container.hpp"

#include "editor_chunk_common.hpp"
#include "invalidate_flags.hpp"


BW_BEGIN_NAMESPACE

class EditorGroup;

/**
 *	This class declares the extra data and methods that the editor requires all
 *	its chunk items to have.
 */
class EditorChunkItem : public ChunkItemBase, public EditorChunkCommonLoadSave
{
public:
	typedef BWBaseFunctor1< EditorChunkItem* > Callback;
	typedef SmartPointer< Callback > CallbackPtr;
	typedef BW::set< CallbackPtr > CallbackSet;

	explicit EditorChunkItem( WantFlags wantFlags = WANTS_NOTHING );
	virtual ~EditorChunkItem();

	/** Load function called after the chunk has been bound */
	virtual void edMainThreadLoad() {}
	/** Called when the chunk is bound, calls through to edMainThreadLoad() */
	void edChunkBind();


	virtual bool edCommonSave( DataSectionPtr pSection );
	virtual bool edCommonLoad( DataSectionPtr pSection );
	virtual bool edCommonEdit( GeneralEditor& editor );
	virtual void edCommonChanged();


	virtual InvalidateFlags edInvalidateFlags(){ return InvalidateFlags::FLAG_THUMBNAIL; }

	/**
	 *	Save to the given data section
	 *	May be called at any time (generally by the item onitself), not related
	 *	to the containing chunk being saved
	 */
	virtual bool edSave( DataSectionPtr pSection )	{ return false; }

	/**
	 *	Called when the parent chunk is saving itself
	 *
	 *	Any changed we've made to our DataSection will be automatically
	 *	save it any case, this is only needed to save external resources,
	 *	such as the static lighting data for EditorChunkModel
	 */
	virtual void edChunkSave() {}
	virtual void edChunkSaveCData(DataSectionPtr cData) {}


	virtual void toss( Chunk * pChunk );

	/**
	 *	Access and mutate matricies for items that have them
	 */
	virtual const Matrix & edTransform()	{ return Matrix::identity; }
	virtual bool edTransform( const Matrix & m, bool transient = false )
	{
		transient_ = transient;
		return false;
	}

	void edMove( Chunk* pOldChunk, Chunk* pNewChunk );

	/**
	 *	Is this item currently moving?
	 */
	virtual bool edIsTransient() { return transient_; }

	/**
	 *	Is this item a vlo
	 */
	virtual bool edIsVLO() const { return false; }

	/**
	 *	Get the local spec (in edTransform's space) coords of this item
	 */
	virtual void edBounds( BoundingBox & bbRet ) const { }

	/**
	*	Get bounding box of this item in world space
	*/
	virtual void edWorldBounds( BoundingBox & bbRet );

	/**
	 *	Get the local bounding box (in edTransform's space) to use when marking
	 *  as selected
	 */
	virtual void edSelectedBox( BoundingBox & bbRet ) const { return edBounds( bbRet ); }

	/**
	 *	Whether this item is editable according to locks in bwlockd ( it is always editable if bwlockd is not present )
	 */
	virtual bool edIsEditable() const;

	/**
	 *	Whether this item is further away than the threshold, allowing for
	 *	culling objects that are too far away.
	 */
	virtual bool edIsTooDistant();

	/**
	 *	Access the class name. Do NOT be tempted to use this in
	 *	switch statements... make a virtual function for it!
	 *	This should only be used for giving the user info about the item.
	 */
	virtual Name edClassName();

	/**
	 *	Get a nice description for this item. Most items will not need
	 *	to override this method.
	 */
	virtual Name edDescription();

	/**
	 *	Edit this item, by adding its properties to the given object
	 */
	virtual bool edEdit( class GeneralEditor & editor )
		{ return edCommonEdit( editor ); }

	virtual BW::vector<BW::string> edCommand( const BW::string& path ) const
		{	return BW::vector<BW::string>();	}
	virtual bool edExecuteCommand(  const BW::string& path , BW::vector<BW::string>::size_type index )
		{	return false;	}
	/**
	 *	Find which chunk this item has been dropped in if its local
	 *	position has changed to that given. Complains and returns
	 *	NULL if the drop chunk can't be found.
	 */
	virtual Chunk * edDropChunk( const Vector3 & lpos );


	/**
	 *	Access the group of the chunk item
	 */
	EditorGroup * edGroup()					{ return pGroup_; }
	void edGroup( EditorGroup * pGp );

	/**
	 * The DataSection of the chunk item, to enable copying
	 *
	 * NULL is a valid value to indicate that no datasection is exposed
	 */
	virtual DataSectionPtr pOwnSect()		{ return NULL; }
	virtual const DataSectionPtr pOwnSect()	const { return NULL; }

	/**
	 * If this ChunkItem is the interior mesh for it's chunk
	 */
	virtual bool isShellModel() const			{ return false; }

	/**
	 * If this ChunkItem is a portal
	 */
	virtual bool isPortal() const				{ return false; }

    /**
     * If this ChunkItem is an entity.
     */
    virtual bool isEditorEntity() const { return false; }
	 /**
     * If this ChunkItem is a User Data Object
     */
    virtual bool isEditorUserDataObject() const { return false; }

    /**
     * If this ChunkItem is a EditorChunkStationNode.
     */
    virtual bool isEditorChunkStationNode() const { return false; }

    /**
     * If this ChunkItem is a EditorChunkLink.
     */
    virtual bool isEditorChunkLink() const { return false; }

	/**
	 * Ask the item if we can snap other items to it, for example, when in
	 * obstacle mode.
	 */
	virtual bool edIsSnappable() { return edShouldDraw(); }

	/**
	 * Ask the item if we can delete it
	 */
	virtual bool edCanDelete() { return edIsEditable(); }

    /**
     * Can the item be added to the selection?
     */
    virtual bool edCanAddSelection() const { return true; }

    /**
     * Notifies the item has been added to the selection
     */
	virtual void edSelected( BW::vector<ChunkItemPtr>& selection );

	/**
	* Notifies the item has been removed from the selection
	*/
	virtual void edDeselected();

	/**
	* Notifies the item has been added to the selection
	*/
	bool edIsSelected() const;

	/**
	 *	Number of triangles
	 */
	virtual int edNumTriangles() const { return 0; }

	/**
	 *	Number of primitive groups / draw calls
	 */
	virtual int edNumPrimitives() const { return 0; }

	/**
	 *	Name of the asset.
	 */
	virtual BW::string edAssetName() const { return ""; }

	/**
	 *	Path of the asset's file on disk, if any.
	 */
	virtual BW::string edFilePath() const { return ""; }

	/**
	 * Tell the item we're about to delete it.
	 *
	 * Will only be called if edCanDelete() returned true
	 */
	virtual void edPreDelete()		
	{
		#if UMBRA_ENABLE
		bw_safe_delete(pUmbraDrawItem_);
		#endif	
	}

	/**
	 * Tell the item it was just cloned from srcItem
	 *
	 * srcItem will be NULL if they shell we were in was cloned, rather than
	 * us directly.
	 */
	virtual void edPostClone( EditorChunkItem* srcItem );

	/**
	 * get the DataSection for clone
	 */
	virtual void edCloneSection( Chunk* destChunk, const Matrix& destMatrixInChunk, DataSectionPtr destDS )
	{
		if( pOwnSect() )
		{
			destDS->copy( pOwnSect() );
			if (destDS->openSection( "transform" ))
				destDS->writeMatrix34( "transform", destMatrixInChunk );
			if (destDS->openSection( "position" ))
				destDS->writeVector3( "position", destMatrixInChunk.applyToOrigin() );
			if (destDS->openSection( "direction" ))
				destDS->writeVector3( "direction", destMatrixInChunk.applyToUnitAxisVector( 2 ) );
		}
	}

	/**
	 * refine the DataSection for chunk clone
	 */
	virtual bool edPreChunkClone( Chunk* srcChunk, const Matrix& destChunkMatrix, DataSectionPtr chunkDS )
	{
		return true;
	}

	/**
	 * return whether this item's position is relative to the chunk
	 */
	virtual bool edIsPositionRelativeToChunk()
	{
		return true;
	}

	/**
	 * return whether this item belongs to the chunk
	 */
	virtual bool edBelongToChunk()
	{
		return true;
	}

	virtual void edPostCreate();

	virtual void edPostModify();

	virtual bool edCheckMark(uint32 mark)
	{
		if (mark == selectionMark_)
			return false;
		else
		{
			selectionMark_ = mark;
			return true;
		}
	}

	/**
	 * Return the binary data used by this item, if any.
	 *
	 * Used by terrain items to expose the terrain block data.
	 */
	virtual BinaryPtr edExportBinaryData() { return 0; }

	/**
	 * If the chunk item should be drawn
	 */
	virtual bool edShouldDraw() const;

	/**
	 * Always on minimum values this item can be moved by
	 */
	virtual Vector3 edMovementDeltaSnaps() { return Vector3( 0.f, 0.f, 0.f ); }
	/**
	 * Always on snap value for this item, in degrees
	 */
	virtual float edAngleSnaps() { return 0.f; }

	/**
	 * Returns a light container suitable for visualising this chunk item.
	 */
	virtual Moo::LightContainerPtr edVisualiseLightContainer() { return NULL; }

	void recordMessage( BWMessageInfo * message );
	void deleteMessage( BWMessageInfo * message );

	MetaData::MetaData& metaData() {	return metaData_;	}

	static void drawSelection( bool drawSelection );
	static bool drawSelection();

	static void hideAllOutside( bool hide );
	static bool hideAllOutside();

	static void addOnModifyCallback( Callback * onModifyCallback );
	static void delOnModifyCallback( Callback * onModifyCallback );
	static void addOnDeleteCallback( Callback * onDeleteCallback );
	static void delOnDeleteCallback( Callback * onDeleteCallback );

	static void updateSelectionMark() { currentSelectionMark_++; }
	static uint32 selectionMark() { return currentSelectionMark_; }

private:
	EditorChunkItem( const EditorChunkItem& );
	EditorChunkItem& operator=( const EditorChunkItem& );

	bool hasLoaded_;
	bool isSelected_;

	void doItemDeleted();
	void doItemRemoved();
	void doItemRestored();

	BW::set< BWMessageInfo *> linkedMessages_;

protected:

	bool groupMember_;
	bool transient_;
	uint32 selectionMark_;
	BW::string groupName_;
	EditorGroup * pGroup_;
	static bool s_drawSelection_;
	static bool s_hideAllOutside_;
	MetaData::MetaData metaData_;

	static CallbackSet s_onModifyCallback_;
	static CallbackSet s_onDeleteCallback_;
	static uint32 currentSelectionMark_;
};

/**
 *	SpecialChunkItem is a type definition that is the application specific base
 *	class of ChunkItem. When making the client, it is defined as
 *	ClientChunkItem.
 */
typedef EditorChunkItem SpecialChunkItem;


/**
 *	This macro should be used in place of DECLARE_CHUNK_ITEM for the editor
 *	versions of chunk item types
 *
 *	@see DECLARE_CHUNK_ITEM
 */
#define DECLARE_EDITOR_CHUNK_ITEM_CLASS_NAME( name )											\
	virtual Name edClassName()																	\
	{																							\
		static Name s_className( name );														\
		return s_className;																		\
	}																							\

#define DECLARE_EDITOR_CHUNK_ITEM_DESCRIPTION( token )											\
	virtual Name edDescription()																\
	{																							\
		STATIC_LOCALISE_NAME( s_desc, token );													\
		return s_desc;																			\
	}																							\

#define DECLARE_EDITOR_CHUNK_ITEM_WITHOUT_DESCRIPTION( CLASS )									\
	DECLARE_CHUNK_ITEM( CLASS )																	\
	virtual Name edClassName()																	\
	{																							\
		static Name s_className( 6+#CLASS );													\
		return s_className;																		\
	}																							\


#define DECLARE_EDITOR_CHUNK_ITEM( CLASS )														\
	DECLARE_EDITOR_CHUNK_ITEM_WITHOUT_DESCRIPTION( CLASS )										\
	public:																						\
	virtual Name edDescription()																\
	{																							\
		BW_GUARD;																				\
																								\
		STATIC_LOCALISE_WSTRING( s_descWithLabel, L"CHUNK/EDITOR/EDITOR_CHUNK_ITEM/ED_DESCRIPTION_WITH_LABEL" );	\
																								\
		const char* label = this->label();														\
		if (label != NULL && label[0])															\
		{																						\
			WStackString< 1024 > wStackString;													\
			formatString(																		\
				s_descWithLabel.c_str(), wStackString.container(), edClassName().c_str(), label );\
			StackString< 1024 > stackString;													\
			bw_wtoutf8( wStackString.container(), stackString.container() );					\
			return Name( stackString.container() );												\
		}																						\
																								\
		static Name s_desc( LocaliseUTF8( L"CHUNK/EDITOR/EDITOR_CHUNK_ITEM/ED_DESCRIPTION",		\
			edClassName().c_str() ) );															\
		return s_desc;																			\
	}																							\
	private:

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "editor_chunk_item.ipp"
#endif

#endif // EDITOR_CHUNK_ITEM_HPP
