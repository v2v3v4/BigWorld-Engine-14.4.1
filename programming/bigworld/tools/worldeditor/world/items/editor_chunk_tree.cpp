#include "pch.hpp"
#include "worldeditor/world/items/editor_chunk_tree.hpp"
#include "worldeditor/project/project_module.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/item_info_db.hpp"
#include "worldeditor/editor/item_properties.hpp"
#include "worldeditor/editor/item_editor.hpp"
#include "worldeditor/misc/cvswrapper.hpp"
#include "worldeditor/misc/options_helper.hpp"
#include "common/lighting_influence.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_light.hpp"
#include "chunk/geometry_mapping.hpp"
#include "appmgr/options.hpp"
#include "appmgr/module_manager.hpp"
#include "common/tools_common.hpp"
#include "speedtree/speedtree_config.hpp"
#include "speedtree/speedtree_collision.hpp"
#include "speedtree/billboard_optimiser.hpp"
#include "speedtree/speedtree_renderer.hpp"
#include "romp/fog_controller.hpp"
#include "model/super_model.hpp"
#include "physics2/bsp.hpp"
#include "resmgr/string_provider.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/resource_cache.hpp"
#include "cstdmf/debug.hpp"


DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )

BW_BEGIN_NAMESPACE

static AutoConfigString s_chunkTreeNotFoundModel( "system/notFoundModel" );	


namespace
{
	///The conversion rate from feet to metres (required for speed tree imports)
	const float FEET_TO_METRES = 0.3048f;
	///The conversion rate from feet to metres (required for speed tree imports)
	const float METRES_TO_FEET = 1.0f/FEET_TO_METRES;

	// Watcher helper class for turning on and off light debugging
	class LightDebugging
	{
	public:
		static bool enabled()
		{
			BW_GUARD;

			return instance().enabled_;
		}
	private:
		LightDebugging() : 
		   enabled_( false )
		{
			MF_WATCH( "SpeedTree/Debug Light Positions", enabled_, Watcher::WT_READ_WRITE,
				"Display debug lines from the selected tree to the lights affecting it" );
		}
		static LightDebugging& instance()
		{
			static LightDebugging s_inst;
			return s_inst;
		}
		bool enabled_;
	};

	/**
	 * Scales tree matrix to change feet to metres and metres to feet for Speed Tree
	 */
	class EditorTreeMatrix : public ChunkItemMatrix
	{
	public:
		EditorTreeMatrix( ChunkItemPtr pItem ) : ChunkItemMatrix( pItem )
		{
		}

		/**
		 * Overriding base class to change metres to feet
		 */
		void getMatrix( Matrix & m, bool world )
		{
			BW_GUARD;

			m = pItem_->edTransform();

			Matrix work = Matrix::identity;
			work.setScale( METRES_TO_FEET, METRES_TO_FEET, METRES_TO_FEET );
			m.preMultiply( work );

			if (world && pItem_->chunk() != NULL)
			{
				m.postMultiply( pItem_->chunk()->transform() );
			}
		}

		/**
		 *	Overriding base class to change feet to metres
		 */
		bool setMatrix( const Matrix & m )
		{
			BW_GUARD;

			Matrix newTransform = m;

			Matrix work = Matrix::identity;
			work.setScale( FEET_TO_METRES, FEET_TO_METRES, FEET_TO_METRES );
			newTransform.preMultiply( work );

			return ChunkItemMatrix::setMatrix(newTransform);
		}
	};

} // anonymous namespace

// -----------------------------------------------------------------------------
// Section: EditorChunkTree
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
EditorChunkTree::EditorChunkTree() :
	hasPostLoaded_( false ),
	bspBB_( BoundingBox::s_insideOut_ )
{
	BW_GUARD;

	// load missing model (static)
	missingTreeModel_ = Model::get( s_chunkTreeNotFoundModel.value() );
	ResourceCache::instance().addResource( missingTreeModel_ );
}


/**
 *	Destructor.
 */
EditorChunkTree::~EditorChunkTree()
{
}


/**
 *	overridden edShouldDraw method
 */
bool EditorChunkTree::edShouldDraw()
{
	BW_GUARD;

	if( !ChunkTree::edShouldDraw() )
		return false;
	
	return OptionsScenery::visible();
}

/**
 *	overridden draw method
 */
void EditorChunkTree::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;

	if( !edShouldDraw() )
		return;

	if (!hasPostLoaded_)
	{
		edPostLoad();
		hasPostLoaded_ = true;
	}

	if (Moo::rc().dynamicShadowsScene() && !castsShadow_)
	{
		return;
	}

	bool projectModule = ProjectModule::currentInstance() == ModuleManager::instance().currentModule();
	bool drawBsp = !projectModule &&
		WorldManager::instance().drawBSP() != 0 &&
		WorldManager::instance().drawBSPSpeedTrees() != 0;

#if SPEEDTREE_SUPPORT
	// Load up the bsp tree if needed
	if ((drawBsp || WorldManager::instance().drawSelection() ) && tree_.get())
	{
		if (!this->isBspCreated())
		{
			// no vertices loaded yet, create some
			const BSPTree * tree = this->bspTree();

			if (tree)
			{
				Moo::Colour colour;
				BW::vector<Moo::VertexXYZL> verts;
				this->getRandomBspColour( colour );
				Moo::BSPTreeHelper::createVertexList( *tree, verts, colour);

				// If BSP has collideable vertices
				if (!verts.empty())
				{
					this->addBsp( verts, this->filename() );
					MF_ASSERT( this->isBspCreated() );
				}
			}
		}
	}
#endif

	if ((drawBsp || WorldManager::instance().drawSelection() ))
	{
		if (this->isBspCreated())
		{
			//set the transforms
			Matrix transform;

			transform.multiply( edTransform(), chunk()->transform() );

			if (WorldManager::instance().drawSelection())
			{
				WorldManager::instance().registerDrawSelectionItem( this );

				this->drawBsp( Moo::rc(), transform, true );
			}
			else
			{
				this->batchBsp( transform );
			}
		}
	}
	else
	{
		if (!this->ChunkTree::loadFailed())
		{
			this->ChunkTree::draw( drawContext );

			BoundingBox bbox;
			Matrix boxTransform;
			if (this->edFrozen() && OptionsMisc::frozenVisible())
			{
				// display tree as frozen
				this->edBounds(bbox);
				boxTransform = pChunk_ ? pChunk_->transform() : Matrix::identity;
				boxTransform.preMultiply( this->transform() );
				WorldManager::instance().showFrozenRegion( bbox, boxTransform );
			}
		}
		else 
		{
			// draw missing model
			if (missingTreeModel_)
			{
				Moo::rc().push();
				Moo::rc().preMultiply(edTransform());
				missingTreeModel_->dress( Moo::rc().world() );
				missingTreeModel_->draw( drawContext, true );
				Moo::rc().pop();
			}
		}			
	}
}

struct ErrorCallback
{
	static void printError(
		const BW::string & fileName,
		const BW::string & errorMsg)
	{
		BW_GUARD;

		BW::string msg = fileName + ":" + errorMsg;
		WorldManager::instance().addError(
			chunk, self, "%s", msg.substr(0, 255).c_str());
	}
	
	static Chunk           * chunk;
	static EditorChunkTree * self;
};

Chunk * ErrorCallback::chunk = NULL;
EditorChunkTree * ErrorCallback::self = NULL;

/**
 *	This method saves the data section pointer before calling its
 *	base class's load method
 */
bool EditorChunkTree::load( DataSectionPtr pSection, Chunk * pChunk )
{
	BW_GUARD;

	ErrorCallback::chunk = pChunk;
	ErrorCallback::self  = this;
#if SPEEDTREE_SUPPORT
	speedtree::setErrorCallback(&ErrorCallback::printError);
#endif

	edCommonLoad( pSection );

	this->pOwnSect_ = pSection;	
	if (this->ChunkTree::load(pSection, pChunk))
	{
		// File path used for locating BSP in cache
		fullPath_ = this->ChunkTree::filename();

		// Short name used for editor properties
		// Strip off path and extension
		BW::StringRef desc = BWResource::getFilename( this->ChunkTree::filename() );
		desc = BWResource::removeExtension( desc );
		this->desc_ = Name( desc.to_string() );
		this->hasPostLoaded_ = false;	
	}
	else
	{
		// File path used for locating BSP in cache
		fullPath_ = pSection->readString("spt");

		// Short name used for editor properties
		// Strip off path and extension
		BW::StringRef desc = BWResource::getFilename( fullPath_ );
		desc = BWResource::removeExtension( desc );
		this->desc_ = Name( desc.to_string() );

		// set BSP for missing model
		if (missingTreeModel_)
		{
			const BSPTree * bsp = missingTreeModel_->decompose();
			this->BaseChunkTree::setBoundingBox( missingTreeModel_->boundingBox() );
			this->BaseChunkTree::setBSPTree( bsp );

			if (!this->isBspCreated())
			{
				static const Moo::Colour colour(0.0f, 1.0f, 0.0f, 1.f);
				BW::vector<Moo::VertexXYZL> verts;

				Moo::BSPTreeHelper::createVertexList( *bsp, verts, colour);

				// If BSP has collideable vertices
				if (!verts.empty())
				{
					this->addBsp( verts, missingTreeModel_->resourceID() );
					MF_ASSERT( this->isBspCreated() );
				}
			}
		}
		
		BW::string msg = LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_TREE/ERROR_LOADING_TREE",
			this->ChunkTree::lastError() );
		WorldManager::instance().addError(
			pChunk, this, "%s",  
			msg.substr(0, 255).c_str());
	
		this->hasPostLoaded_ = true;
	}
	return true;
}

void EditorChunkTree::edPostLoad()
{
}

/**
 * We just loaded up with srcItems lighting data, create some new stuff of our
 * own
 */
void EditorChunkTree::edPostClone( EditorChunkItem* srcItem )
{
	BW_GUARD;

	EditorChunkItem::edPostClone( srcItem );
	ChunkBspHolder::postClone();
}

void EditorChunkTree::edPostCreate()
{
	BW_GUARD;

	EditorChunkItem::edPostCreate();
}

/**
*	This method return the invalidateFlag 
*	that should notify the chunk to update 
*	when manupulating this item
*
*/
InvalidateFlags EditorChunkTree::edInvalidateFlags()
{
	return InvalidateFlags::FLAG_SHADOW_MAP |
		InvalidateFlags::FLAG_NAV_MESH |
		InvalidateFlags::FLAG_THUMBNAIL;
}


/**
 *	This method does extra stuff when this item is tossed between chunks.
 *
 *	It updates its datasection in that chunk.
 */
void EditorChunkTree::toss( Chunk * pChunk )
{
	BW_GUARD;

	if (pChunk_ != NULL && pOwnSect_)
	{
		EditorChunkCache::instance( *pChunk_ ).
			pChunkSection()->delChild( pOwnSect_ );
		pOwnSect_ = NULL;
	}

	this->ChunkTree::toss( pChunk );

	if (pChunk_ != NULL && !pOwnSect_)
	{
		pOwnSect_ = EditorChunkCache::instance( *pChunk_ ).
			pChunkSection()->newSection( "speedtree" );
		this->edSave( pOwnSect_ );
	}

	// Update the database
	ItemInfoDB::instance().toss( this, pChunk != NULL );
}


/**
 *	Save to the given section
 */
bool EditorChunkTree::edSave( DataSectionPtr pSection )
{
	BW_GUARD;

	if (!edCommonSave( pSection ))
		return false;

	pSection->writeString( "spt", this->ChunkTree::filename() );
	pSection->writeInt( "seed", this->ChunkTree::seed() );
	pSection->writeMatrix34( "transform", this->transform() );

	pSection->writeBool( "reflectionVisible", reflectionVisible_ );

	pSection->delChild( "editorOnly/castsShadow" );
	pSection->writeBool( "castsShadow", castsShadow_ );

	return true;
}

/**
 *	Called when our containing chunk is saved
 */
void EditorChunkTree::edChunkSave()
{
}

/**
 *	Called when our containing chunk is saved; save the lighting info
 */
void EditorChunkTree::edChunkSaveCData(DataSectionPtr cData)
{
}

/**
 *	This method sets this item's transform for the editor
 *	It takes care of moving it into the right chunk and recreating the
 *	collision scene and all that
 */
bool EditorChunkTree::edTransform( const Matrix & m, bool transient )
{
	BW_GUARD;

	// it's permanent, so find out where we belong now
	BoundingBox lbb( Vector3(0.f,0.f,0.f), Vector3(1.f,1.f,1.f) );
	Chunk * pOldChunk = pChunk_;
	Chunk * pNewChunk = this->edDropChunk( m.applyPoint(
		(lbb.minBounds() + lbb.maxBounds()) * 0.5f ) );
	if (pNewChunk == NULL) return false;

	// if this is only a temporary change, keep it in the same chunk
	if (transient)
	{
		this->setTransform( m );
		this->syncInit();
		return true;
	}

	// make sure the chunks aren't readonly
	if (!EditorChunkCache::instance( *pOldChunk ).edIsWriteable() 
		|| !EditorChunkCache::instance( *pNewChunk ).edIsWriteable())
		return false;


	Matrix transform = this->transform();
	transform.multiply( m, pOldChunk->transform() );
	transform.postMultiply( pNewChunk->transformInverse() );
	this->setTransform( transform );




	// note that both affected chunks have seen changes
	edMove( pOldChunk, pNewChunk );

	if (pOldChunk != pNewChunk )
	{
		edSave( pOwnSect() );
	}
	this->syncInit();
	return true;
}


void EditorChunkTree::edPreDelete()
{
	BW_GUARD;

	EditorChunkItem::edPreDelete();
}


/**
 *	Get the bounding box
 */
void EditorChunkTree::edBounds( BoundingBox& bbRet ) const
{
	BW_GUARD;

	bbRet = this->boundingBox();
}


/**                                                                             
 *  Get the bounding box used for showing the selection                         
 */                                                                             
void EditorChunkTree::edSelectedBox( BoundingBox& bbRet ) const                 
{
	BW_GUARD;

    if ( bspTree() && bspTree()->numTriangles() > 0 &&                                  
        Options::getOptionInt( "bspBoundingBox", 1 ) )                          
    {                                                                           
        if ( bspBB_ == BoundingBox::s_insideOut_ )                              
        {     
            // bsp bounding box uninitialised, so initialise.                   
			bspBB_ = bspTree()->boundingBox();
        }                                                                       
        bbRet = bspBB_;                                                         
    }                                                                           
    else                                                                        
    {                                                                           
        this->edBounds( bbRet );                                                
    }                                                                           
}                                                                               


/**
 *	This method adds this item's properties to the given editor
 */
bool EditorChunkTree::edEdit( GeneralEditor & editor )
{
	BW_GUARD;

	if (this->edFrozen())
		return false;

	if (!edCommonEdit( editor ))
	{
		return false;
	}

	STATIC_LOCALISE_NAME( s_position, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_TREE/POSITION" );
	STATIC_LOCALISE_NAME( s_rotation, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_TREE/ROTATION" );
	STATIC_LOCALISE_NAME( s_scale, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_TREE/SCALE" );
	STATIC_LOCALISE_NAME( s_castShadow, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_TREE/CASTS_SHADOW" );
	STATIC_LOCALISE_NAME( s_filename, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_TREE/FILENAME" );
	STATIC_LOCALISE_NAME( s_changeInSptCad, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_TREE/CHANGE_IN_SPT_CAD" );
	STATIC_LOCALISE_NAME( s_seed, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_TREE/SEED" );
	STATIC_LOCALISE_NAME( s_reflectionVisible, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_TREE/REFLECTION_VISIBLE" );

	MatrixProxy * pMP = new EditorTreeMatrix( this );
	editor.addProperty( new ChunkItemPositionProperty( s_position, pMP, this ) );
	editor.addProperty( new GenRotationProperty( s_rotation, pMP ) );
	editor.addProperty( new GenScaleProperty( s_scale, pMP ) );

	// can affect shadow?
	editor.addProperty( new GenBoolProperty( s_castShadow,
		new AccessorDataProxy< EditorChunkTree, BoolProxy >(
			this, "castsShadow", 
			&EditorChunkTree::getCastsShadow, 
			&EditorChunkTree::setCastsShadow ) ) );

	StaticTextProperty * pProp = new StaticTextProperty( s_filename,
		new ConstantDataProxy< StringProxy >( this->ChunkTree::filename() ) );
	editor.addProperty(pProp);
		
	BW::stringstream seed;
	seed << EditorChunkTree::getSeed() << s_changeInSptCad.c_str();
	editor.addProperty( new StaticTextProperty( s_seed, new ConstantDataProxy<StringProxy>(seed.str()) ) );

	editor.addProperty( new GenBoolProperty( s_reflectionVisible,
		new AccessorDataProxy< EditorChunkTree, BoolProxy >(
			this, "reflectionVisible",
			&EditorChunkTree::getReflectionVis,
			&EditorChunkTree::setReflectionVis ) ) );

	return true;
}


/**
 *	Find the drop chunk for this item
 */
Chunk * EditorChunkTree::edDropChunk( const Vector3 & lpos )
{
	BW_GUARD;

	Vector3 npos = pChunk_->transform().applyPoint( lpos );

	Chunk * pNewChunk = NULL;

	pNewChunk = pChunk_->space()->findChunkFromPointExact( npos );

	if (pNewChunk == NULL)
	{
		ERROR_MSG( "Cannot move %s to (%f,%f,%f) "
			"because it is not in any loaded chunk!\n",
			this->edDescription().c_str(), npos.x, npos.y, npos.z );
		return NULL;
	}

	return pNewChunk;
}

Name EditorChunkTree::edDescription()
{
	return desc_;
}


/**
 *	This returns the number of triangles of this tree.
 */
int EditorChunkTree::edNumTriangles() const
{
	BW_GUARD;

#if SPEEDTREE_SUPPORT
	return tree_.get() ? tree_->numTris() : 0;
#else
	return 0;
#endif // SPEEDTREE_SUPPORT
}


/**
 *	This returns the number of primitive groups of this tree.
 */
int EditorChunkTree::edNumPrimitives() const
{
	BW_GUARD;

#if SPEEDTREE_SUPPORT
	return tree_.get() ? tree_->numPrims() : 0;
#else
	return 0;
#endif // SPEEDTREE_SUPPORT
}


/**
 *	This returns the file for the asset of this tree.
 */
BW::string EditorChunkTree::edAssetName() const
{
	BW_GUARD;

	return BWResource::getFilename( filename() ).to_string();
}


/**
 *	This returns the file for the asset of this tree.
 */
BW::string EditorChunkTree::edFilePath() const
{
	return filename();
}


Vector3 EditorChunkTree::edMovementDeltaSnaps()
{
	BW_GUARD;

	return EditorChunkItem::edMovementDeltaSnaps();
}

float EditorChunkTree::edAngleSnaps()
{
	BW_GUARD;

	return EditorChunkItem::edAngleSnaps();
}

unsigned long EditorChunkTree::getSeed() const
{
	BW_GUARD;

	return this->ChunkTree::seed();
}

bool EditorChunkTree::setSeed( const unsigned long & seed )
{
	BW_GUARD;

	bool success = this->ChunkTree::seed(seed);
	if (!success)
	{
		BW::string msg = LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_TREE/COULD_NOT_CHANGE_TREE_SEED",
			this->ChunkTree::lastError() );
		WorldManager::instance().addCommentaryMsg(msg.substr(0,255));
	}	
	return success;
}

bool EditorChunkTree::getCastsShadow() const
{
	return castsShadow_;
}

bool EditorChunkTree::setCastsShadow( const bool & castsShadow )
{
	BW_GUARD;

	if ( castsShadow_ != castsShadow )
	{
		castsShadow_ = castsShadow;

		MF_ASSERT( pChunk_ != NULL );


		BoundingBox bb = BoundingBox::s_insideOut_;
		this->edBounds( bb );
		bb.transformBy( this->edTransform() );
		bb.transformBy( chunk()->transform() );

		BW::set<Chunk*> chunks;
		WorldManager::instance().collectOccupiedChunks( bb, pChunk_, chunks );
		WorldManager::instance().changedChunks( chunks, 
			InvalidateFlags::FLAG_SHADOW_MAP |
			InvalidateFlags::FLAG_THUMBNAIL,
			*this );
	}	
	return true;
}

Moo::LightContainerPtr EditorChunkTree::edVisualiseLightContainer()
{
	if (!chunk())
		return NULL;

	Moo::LightContainerPtr lc = new Moo::LightContainer;

	BoundingBox bb = this->boundingBox();
	bb.transformBy( this->transform() );
	bb.transformBy( this->chunk()->transform() );

	lc->init( ChunkLightCache::instance( *chunk() ).pAllLights(), bb, false );

	return lc;
}


/// Write the factory statics stuff
/// Write the factory statics stuff
#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection, pChunk)
IMPLEMENT_CHUNK_ITEM( EditorChunkTree, speedtree, 1 )
BW_END_NAMESPACE

