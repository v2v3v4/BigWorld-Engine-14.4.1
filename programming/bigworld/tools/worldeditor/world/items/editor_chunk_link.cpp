#include "pch.hpp"
#include "worldeditor/world/items/editor_chunk_link.hpp"
#include "worldeditor/world/items/editor_chunk_substance.ipp"
#include "worldeditor/world/items/editor_chunk_entity.hpp"
#include "worldeditor/world/editor_chunk_link_manager.hpp"
#include "worldeditor/misc/selection_filter.hpp"
#include "worldeditor/undo_redo/station_link_operation.hpp"
#include "worldeditor/undo_redo/station_entity_link_operation.hpp"
#include "worldeditor/collisions/collision_terrain_only.hpp"
#include "worldeditor/import/terrain_utils.hpp"
#include "worldeditor/project/space_helpers.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/auto_config.hpp"
#include "model/super_model.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/umbra_chunk_item.hpp"
#include "scene/scene_object.hpp"
#include "math/mathdef.hpp"
#include "moo/texture_manager.hpp"
#include "resmgr/string_provider.hpp"
#include <limits>


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: <anonymous namespace>
// -----------------------------------------------------------------------------

namespace
{
    AutoConfigString    s_directionalTexture    = "editor/directionalTexture";
    AutoConfigString    s_noDirectionTexture    = "editor/noDirectionLinkTexture";
    AutoConfigString    s_linkShader            = "editor/linkShader";

    /**
     *  This is used to hit-test chunk links.
     */
    class EditChunkLinkObstacle : public ChunkObstacle
    {
    public:
        /**
         *  Constructor.
         *
         *  @param link         The link to hit test.
         *  @param transform    The local to world transform.
         *  @param bb           The bounding box.  WARNING  This bounding box's
         *                      address is used by ChunkObstacle later!
		 *	@param startPt		Start point of the link in absolute coords.
		 *	@param endPt		End point of the link in absolute coords.
         */
        EditChunkLinkObstacle
        (
            EditorChunkLinkPtr  link,
            Matrix              const &transform,
            BoundingBox         const *bb,
			Vector3				startPt,
			Vector3				endPt
        ) :
            ChunkObstacle(transform, bb, link),
            link_(link),
			startPt_(startPt),
			endPt_(endPt)
        {
        }

        /**
         *  Do we have a collision?
         *
         *  @param source       The source vector.
         *  @param extent       The extent vector.
         *  @param state        The collision state.
         *
         *  @return             True if more collisions should be tested.
         */
        /*virtual*/ bool
        collide
        (
            Vector3             const &source,
            Vector3             const &extent,
            CollisionState      &state
        ) const
        {
			BW_GUARD;

            Vector3 dir = extent - source;
            WorldTriangle wt;
            float dist = link_->collide(source, dir, wt);
            if (dist == std::numeric_limits<float>::max())
                return false;
            dist = state.sTravel_ + (state.eTravel_- state.sTravel_)*dist;
	        if (state.onlyLess_ && dist > state.dist_)
                return false;
	        if (state.onlyMore_ && dist < state.dist_)
                return false;

			CollisionObstacle ob( &transform_, &transformInverse_, 
				this->pItem()->sceneObject() );
            int say = state.cc_.visit(ob, wt, dist);
            if ((say & 3) != 3)
                state.dist_ = dist;
	        if (say == 0)
                return true;
	        state.onlyLess_ = !(say & 2);
	        state.onlyMore_ = !(say & 1);
	        return false;
        }

        /**
         *  Do we have a collision?
         *
         *  @param source       The source triangle.
         *  @param extent       The extent vector.
         *  @param state        The collision state.
         *
         *  @return             True if more collisions should be tested.
         */
        /*virtual*/ bool
        collide
        (
            WorldTriangle       const &source,
            Vector3             const &extent,
		    CollisionState      &state
        ) const
        {
			BW_GUARD;

            bool ok = false;
            ok |= collide(source.v0(), extent, state);
            ok |= collide(source.v1(), extent, state);
            ok |= collide(source.v2(), extent, state);
            return ok;
        }

        /**
         *  Returns the end points of the link.
         *
		 *	@param startPt	Returns start point of the link in absolute coords.
		 *	@param endPt	Returns end point of the link in absolute coords.
         */
		void endPoints( Vector3& startPt, Vector3& endPt )
		{
			startPt = startPt_;
			endPt = endPt_;
		}

    private:
        EditorChunkLinkPtr      link_;
		Vector3					startPt_;
		Vector3					endPt_;
    };


    /**
     *  This class adds ChunkObstacle objects to the space for the chunks
	 *	that this link intersects.
     */
	class ChunkLinkObstacle : public ChunkModelObstacle
	{
	public:
		ChunkLinkObstacle( Chunk & chunk ) :
			ChunkModelObstacle( chunk )
		{
		}

		~ChunkLinkObstacle()
		{
		}

		static Instance<ChunkLinkObstacle> instance;

	protected:

		typedef BW::set< ChunkSpace::Column * > ColumnSet;

		/**
		 *	This protected method inserts all relevant columns in the 'columns'
		 *	return parameter using the link's ChunkObstacle object 'cso'.
		 *
		 *	@param cso		The link's ChunkObstacle object
		 *	@param columns	Return paramenter, relevant columns
		 *	@param adding	True if adding to space, false otherwise
		 */
		void getColumns( ChunkObstacle & cso, ColumnSet& columns, bool adding )
		{
			BW_GUARD;

			// get the world-aligned bounding box that contains the link's BB.
			BoundingBox bbTr = cso.bb_;
			bbTr.transformBy( cso.transform_ );
			Vector3 startPt;
			Vector3 endPt;
			static_cast<EditChunkLinkObstacle&>( cso ).endPoints( startPt, endPt );

			// transform to grid coords
			const float gridSize = pChunk_->space()->gridSize();
			int minX = int( floorf( bbTr.minBounds().x / gridSize ) );
			int minZ = int( floorf( bbTr.minBounds().z / gridSize ) );
			int maxX = int( floorf( bbTr.maxBounds().x / gridSize ) );
			int maxZ = int( floorf( bbTr.maxBounds().z / gridSize ) );

			// for each grid area inside the BB, test to see if the link
			// touches it, and if so, add it to the columns.
			Vector2 line( endPt.x - startPt.x, endPt.z - startPt.z );

			// If the line is zero length, simply return
			if (line.lengthSquared() == 0.0f)
				return;

			for (int x = minX; x <= maxX; ++x)
			{
				for (int z = minZ; z <= maxZ; ++z)
				{
					// find out if the link line is inside the chunk in the
					// grid pos (x,y), by comparing the signs of the cross
					// products
					Vector2 wpos( x * gridSize, z * gridSize );
					Vector2 corners[4] = {
						wpos,
						wpos + Vector2( gridSize, 0.0f ),
						wpos + Vector2( 0.0f, gridSize ),
						wpos + Vector2( gridSize, gridSize )
					};

					bool linkIn = false;
					int lastSign = 0;
					for ( int i = 0; i < 4; ++i )
					{
						Vector2 cornerVec( corners[i].x - startPt.x, corners[i].y - startPt.z );
						float cross = line.crossProduct( cornerVec );
						if ( lastSign == 0 )
						{
							if ( cross >= 0 )
								lastSign = 1;
							else
								lastSign = -1;
						}
						else
						{
							if ( ( cross >= 0 && lastSign == -1 ) ||
								 ( cross < 0 && lastSign == 1 ) )
							{
								// sign changed, so should add this column
								linkIn = true;
								break;
							}
						}
					}

					if ( linkIn )
					{
						// Should add this column, so do it.
						Vector3 pt(
							(x + 0.5f) * gridSize,
							0.0f,
							(z + 0.5f) * gridSize );

						ChunkSpace::Column* pColumn =
							pChunk_->space()->column( pt, adding );

						if ( pColumn != NULL )
							columns.insert( pColumn );
					}
				}
			}

			if (columns.empty() && adding)
			{
				ERROR_MSG(
					"ChunkLinkObstacle::getColumns: Link is not inside the space - "
					"min = (%.1f, %.1f, %.1f). max = (%.1f, %.1f, %.1f)\n",
					bbTr.minBounds().x, bbTr.minBounds().y, bbTr.minBounds().z,
					bbTr.maxBounds().x, bbTr.maxBounds().y, bbTr.maxBounds().z );
			}
		}

		/**
		 *	This protected method adds the obstacle to all its relevant comumns
		 *
		 *	@param cso		The link's ChunkObstacle object
		 *	@return			Number of columns added to.
		 */
		virtual int addToSpace( ChunkObstacle & cso )
		{
			BW_GUARD;

			int colCount = 0;

			// find what columns we need to add to
			ColumnSet columns;

			getColumns( cso, columns, true );

			// and add it to all of them
			for (ColumnSet::iterator it = columns.begin(); it != columns.end(); it++)
			{
				MF_ASSERT( &**it );	// make sure we can reach all those we need to!
				if (*it)
				{
					(*it)->addObstacle( cso );
					++colCount;
				}
			}

			return colCount;
		}

		/**
		 *	This protected method removes the obstacle from all its relevant
		 *	comumns
		 *
		 *	@param cso		The link's ChunkObstacle object
		 */
		virtual void delFromSpace( ChunkObstacle & cso )
		{
			BW_GUARD;

			ColumnSet columns;

			getColumns( cso, columns, false );

			// and add it to all of them
			for (ColumnSet::iterator it = columns.begin(); it != columns.end(); it++)
			{
				if ( *it )
					(*it)->stale();
			}
		}
	};
	ChunkCache::Instance<ChunkLinkObstacle> ChunkLinkObstacle::instance;


	/**
	 *  This class keeps a single set of textures and materials for the links.
	 */
	class LinkMaterialManager
	{
		SimpleMutex mutex_;
	public:
		LinkMaterialManager() : userCount_( 0 )
		{
		}

		void addUser()
		{
			BW_GUARD;

			SimpleMutexHolder smh( mutex_ );

			++userCount_;
			if (userCount_ == 1)
			{
				Moo::TextureManager::instance()->getDetailsManager()->setFormat( s_directionalTexture, D3DFMT_A8R8G8B8 );
				directionalTexture_ = Moo::TextureManager::instance()->get( s_directionalTexture );

				// Load the entity-node texture:
				Moo::TextureManager::instance()->getDetailsManager()->setFormat( s_noDirectionTexture, D3DFMT_A8R8G8B8 );
				noDirectionTexture_ = Moo::TextureManager::instance()->get( s_noDirectionTexture );

				meshEffect_ = new Moo::EffectMaterial();
				meshEffect_->initFromEffect(s_linkShader);
			}
		}

		void removeUser()
		{
			BW_GUARD;

			SimpleMutexHolder smh( mutex_ );

			MF_ASSERT( userCount_ > 0 );

			--userCount_;
			if (userCount_ == 0)
			{
				directionalTexture_ = NULL;
				noDirectionTexture_ = NULL;
				meshEffect_ = NULL;
			}
		}

		Moo::BaseTexturePtr directionalTexture() { return directionalTexture_; }
		Moo::BaseTexturePtr noDirectionTexture() { return noDirectionTexture_; }
		Moo::EffectMaterialPtr meshEffect() { return meshEffect_; }

	private:
		int userCount_;
		Moo::BaseTexturePtr directionalTexture_;	// texture used to directional links draw with
		Moo::BaseTexturePtr noDirectionTexture_;	// texture used to draw links without direction
		Moo::EffectMaterialPtr meshEffect_;			// effect file used to draw mesh
	};

	LinkMaterialManager s_linkMaterialManager;


	/**
	 *  This class manages the life of the batch rendering lists, so they get
	 *	cleaned up when all chunk link items are removed.
	 *	NOTE: Lists must be cleared when, for example, changing space, to avoid
	 *	circular references that can result in EditorChunkLink items leaking.
	 */
	class LinkBatcher
	{
	public:
		// List used in the LinkBatcher class
		typedef BW::vector< EditorChunkLinkPtr > LinkList;

		LinkBatcher() : userCount_( 0 )
		{
		}

		void addUser()
		{
			BW_GUARD;

			++userCount_;
			if (userCount_ == 1)
			{
				MF_ASSERT( !pDirLists_ );
				pDirLists_ = new LinkLists;
			}
		}

		void removeUser()
		{
			BW_GUARD;

			--userCount_;
			if (userCount_ == 0)
			{
				MF_ASSERT( pDirLists_ );
				bw_safe_delete(pDirLists_);
			}
		}

		bool hasUsers()
		{
			return userCount_ > 0;
		}

		struct LinkLists
		{
			static const int LINK_LIST_COUNT = 9;

			LinkList dirBoth;
			LinkList dirStartEnd;
			LinkList dirEndStart;
			LinkList dirNone;
			LinkList dirBothLocked;
			LinkList dirStartEndLocked;
			LinkList dirEndStartLocked;
			LinkList dirNoneLocked;

			LinkList dirFrozen;

		} * pDirLists_;

		struct LinkLists & lists()
		{
			MF_ASSERT( pDirLists_ );
			return *pDirLists_;
		}

	private:
		int userCount_;
	};

	LinkBatcher s_linkBatcher;

} // anonymous namespace



// -----------------------------------------------------------------------------
// Section: EditorChunkLink
// -----------------------------------------------------------------------------

/*static*/ const float EditorChunkLink::MAX_SEG_LENGTH          =       8.0f;   // largest space between segments
/*static*/ const float EditorChunkLink::MIN_SEG_LENGTH          =       0.4f;   // minimum segment length
/*static*/ const float EditorChunkLink::LINK_THICKNESS          =       0.1f;   // thickness of links
/*static*/ const float EditorChunkLink::SEGMENT_SPEED           =       0.5f;   // speed texture moves along the segment
/*static*/ const float EditorChunkLink::HEIGHT_BUFFER           =       0.5f;   // increase the link height by this amount to make sure it is above the terrain
/*static*/ const float EditorChunkLink::NEXT_HEIGHT_SAMPLE      =       1.0f;   // sample height beyond current height per metre
/*static*/ const float EditorChunkLink::NEXT_HEIGHT_SAMPLE_MID  =       2.0f;   // sample height beyond mid estimate for mid point
/*static*/ const float EditorChunkLink::MAX_SEARCH_HEIGHT       = 1000000.0f;   // max. search above height
/*static*/ const float EditorChunkLink::AIR_THRESHOLD           =       1.0f;   // nodes above this height above the ground are in the air
/*static*/ const float EditorChunkLink::BB_OFFSET               =       0.5f;   // amount to offset bounding box
/*static*/ const float EditorChunkLink::VERTICAL_LINK_EPSILON   =       0.001f; // adds a tiny variation to vertical links to avoid calculation errors

/*static*/ bool EditorChunkLink::s_linkCollide_ = true;
/*static*/ bool EditorChunkLink::s_enableDraw_ = true;

/*static*/ float EditorChunkLink::s_totalTime_ = 0.0f;


/**
 *  EditorChunkLink constructor.
 */
EditorChunkLink::EditorChunkLink() :
	pLastObstacleChunk_( NULL ),
    yOffset_(0.5f),
    minY_(0.0f),
    maxY_(0.0f),
    needRecalc_(false),
	highlight_(false),
    midY_(0.0f),
	vertexBufferSize_( 0 )
{
	BW_GUARD;

    lastStart_.x = lastStart_.y = lastStart_.z =
        lastEnd_.x = lastEnd_.y = lastEnd_.z =
        std::numeric_limits<float>::max();

    bbTransform_.setIdentity();
	bbInvTransform_.setIdentity();
    lineTransform_.setIdentity();
	lineInvTransform_.setIdentity();

    HRESULT hr = S_OK;

	s_linkBatcher.addUser();

    // Load the link texture
	s_linkMaterialManager.addUser();
	directionalTexture_ = s_linkMaterialManager.directionalTexture();
	noDirectionTexture_ = s_linkMaterialManager.noDirectionTexture();
	meshEffect_ = s_linkMaterialManager.meshEffect();

	this->wantFlags_ = WantFlags( this->wantFlags_ | WANTS_DRAW | WANTS_TICK );
}


/**
 *  EditorChunkLink destructor.
 */
EditorChunkLink::~EditorChunkLink()
{
	BW_GUARD;

	s_linkMaterialManager.removeUser();
	s_linkBatcher.removeUser();
}


/**
 *	This method adds a link to the appropriate batch list for drawing later.
 */
void EditorChunkLink::batch( bool colourise, bool frozen /* = false*/ )
{
	BW_GUARD;

	// We have 9 lists, 4 for the 4 ChunkLink::Direction values, and 4 for
	// the same direction values but wfor rendering red when in BWLockD.
	// and one extra for 'frozen' links
	if (colourise)
	{
		if (direction() == ChunkLink::DIR_BOTH)
		{
			s_linkBatcher.lists().dirBothLocked.push_back( this );
		}
		else if (direction() == ChunkLink::DIR_START_END)
		{
			s_linkBatcher.lists().dirStartEndLocked.push_back( this );
		}
		else if (direction() == ChunkLink::DIR_END_START)
		{
			s_linkBatcher.lists().dirEndStartLocked.push_back( this );
		}
		else
		{
			s_linkBatcher.lists().dirNoneLocked.push_back( this );
		}
	}
	else if (frozen)
	{
		s_linkBatcher.lists().dirFrozen.push_back( this );
	}
	else
	{
		if (direction() == ChunkLink::DIR_BOTH)
		{
			s_linkBatcher.lists().dirBoth.push_back( this );
		}
		else if (direction() == ChunkLink::DIR_START_END)
		{
			s_linkBatcher.lists().dirStartEnd.push_back( this );
		}
		else if (direction() == ChunkLink::DIR_END_START)
		{
			s_linkBatcher.lists().dirEndStart.push_back( this );
		}
		else
		{
			s_linkBatcher.lists().dirNone.push_back( this );
		}
	}
}


/**
 *  This static method is called at the end of the rendering loop to 
 *	render all the batched links.
 *	NOTE: flush( 0, true ) has to be called when changing space to avoid leaks
 *	caused by circular references caused by a list with leftover items.
 */
/*static*/ void EditorChunkLink::flush( float dtime, bool clearOnly /*= false*/ )
{
	BW_GUARD;

	if (!s_linkBatcher.hasUsers())
	{
		return;
	}

	s_totalTime_ += dtime;
	// Array to iterate over each direction
	ChunkLink::Direction dir[ 4 ] = {
		ChunkLink::DIR_NONE,
		ChunkLink::DIR_START_END,
		ChunkLink::DIR_END_START,
		ChunkLink::DIR_BOTH };

	// Array to iterate over the lists. 0-3 are normal links and 4-7 are
	// colourised links. 8 is the frozen links list.
	LinkBatcher::LinkList * list[ LinkBatcher::LinkLists::LINK_LIST_COUNT ] = {
			&s_linkBatcher.lists().dirNone,
			&s_linkBatcher.lists().dirStartEnd,
			&s_linkBatcher.lists().dirEndStart,
			&s_linkBatcher.lists().dirBoth,
			&s_linkBatcher.lists().dirNoneLocked,
			&s_linkBatcher.lists().dirStartEndLocked,
			&s_linkBatcher.lists().dirEndStartLocked,
			&s_linkBatcher.lists().dirBothLocked, 
			&s_linkBatcher.lists().dirFrozen };


	s_linkMaterialManager.meshEffect()->pEffect()->pEffect()->SetVector( "colouriseColour", &Vector4(0.2f, 0.f, 0.f, 1.f) );

	for (int i = 0; i < LinkBatcher::LinkLists::LINK_LIST_COUNT; ++i)
	{
		if (!s_linkBatcher.hasUsers())
		{
			break;
		}

		if (list[i]->empty())
		{
			continue;
		}

		if (!clearOnly)
		{
			ChunkLink::Direction direction = dir[ i % 4 ];

			// Check if we are onto the BWLockD locked links lists.
			bool colourise = (i > 3);
			
			if (colourise)
			{
				if ( i == 8 )
				{
					s_linkMaterialManager.meshEffect()->pEffect()->pEffect()->SetVector( "colouriseColour", &Vector4(0.2f, 0.2f, 0.2f, 1.f) );
					direction = ChunkLink::DIR_BOTH;
				}
			}

			s_linkMaterialManager.meshEffect()->pEffect()->pEffect()->SetBool( "colourise", colourise ? TRUE : FALSE );
			s_linkMaterialManager.meshEffect()->pEffect()->pEffect()->SetBool( "highlight", FALSE );

			DX::BaseTexture * texture =
				direction != ChunkLink::DIR_NONE ?
					s_linkMaterialManager.directionalTexture()->pTexture() :
					s_linkMaterialManager.noDirectionTexture()->pTexture();

			float speed = (direction == ChunkLink::DIR_NONE ?
								0.0f : EditorChunkLink::SEGMENT_SPEED);

			if ( i == 8 )
			{
				speed = 0.0f;
			}

			// Setup materials once, and render all links that have same
			// material in one go.
			if (ChunkLinkSegment::beginDrawSegments(
					Moo::rc(),
					texture,
					s_linkMaterialManager.meshEffect(),
					s_totalTime_,
					speed,
					direction,
					Matrix::identity ))
			{
				for (LinkBatcher::LinkList::iterator it = list[i]->begin();
					it != list[i]->end(); ++it)
				{
					ChunkLinkSegment::draw(
						Moo::rc(),
						(*it)->vertexBuffer_,
						(*it)->indexBuffer_,
						(*it)->lineVertexBuffer_,
						(*it)->lineIndexBuffer_,
						static_cast<uint>((*it)->meshes_.size()) );
				}

				ChunkLinkSegment::endDrawSegments(
					Moo::rc(),
					texture,
					s_linkMaterialManager.meshEffect(),
					s_totalTime_,
					speed,
					direction );
			}
		}

		list[i]->clear();
	}
}


/**
 *  This draws a EditorChunkLink.
 */
/*virtual*/ void EditorChunkLink::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;

	if
    (
		edShouldDraw()
		&&
		!WorldManager::instance().drawSelection()
        &&
        !Moo::rc().reflectionScene()
        &&
        !Moo::rc().mirroredTransform()
    )
    {
        if (startItem() == NULL && endItem() == NULL)
		{
            return;
		}

		if (!meshEffect_ || !meshEffect_->pEffect() || !meshEffect_->pEffect()->pEffect())
		{
			return;
		}

		if (meshes_.empty())
		{
			return;
		}

		if (highlight_)
		{
			// Draw immediately. Only one highlighted at a time, so performance
			// impact is minimal.
			meshEffect_->pEffect()->pEffect()->SetBool("colourise", FALSE);
			meshEffect_->pEffect()->pEffect()->SetBool("highlight", TRUE);
			highlight_ = false;

			drawImmediate();

			meshEffect_->pEffect()->pEffect()->SetBool("colourise", FALSE);
			meshEffect_->pEffect()->pEffect()->SetBool("highlight", FALSE);
		}
		else if (OptionsMisc::readOnlyVisible() || OptionsMisc::frozenVisible())
		{
			// Check if chunk is locked by someone else. If so, colourise red.
			// Batch to improve performance.
			EditorChunkItem* start =
				static_cast<EditorChunkItem*>(startItem().getObject());
			EditorChunkItem* end =
				static_cast<EditorChunkItem*>(endItem().getObject());
			
			bool lockedColouring = OptionsMisc::readOnlyVisible() &&
				(!start || !end || !start->chunk() || !end->chunk() ||
				!EditorChunkCache::instance( *(start->chunk()) ).edIsWriteable() ||
				!EditorChunkCache::instance( *(end->chunk()) ).edIsWriteable());

			bool frozenColouring = false;
			if (!lockedColouring)
			{
				frozenColouring = OptionsMisc::frozenVisible() &&
					((start && !start->edIsEditable()) || (end && !end->edIsEditable()));
			}

			batch( lockedColouring, frozenColouring );
		}
		else
		{
			// Batch to improve performance.
			batch( false );
		}

        // The following is useful for debugging sometimes
        //for (size_t i = 0; i < meshes_.size(); ++i)
        //{
        //    meshes_[i]->drawTris(*rc);
        //}
    }
}

/**
 *	This method draws the link without batching.
 */
void EditorChunkLink::drawImmediate()
{
	BW_GUARD;

	DX::BaseTexture *texture = direction() != DIR_NONE ?
		directionalTexture_->pTexture() : noDirectionTexture_->pTexture();

	float speed = direction() == DIR_NONE ? 0.0f : SEGMENT_SPEED;

	if (ChunkLinkSegment::beginDrawSegments(
										Moo::rc(),
										texture,
										meshEffect_,
										s_totalTime_,
										speed,
										direction(),
										Matrix::identity ))
	{
		ChunkLinkSegment::draw(
			Moo::rc(),
			vertexBuffer_,
			indexBuffer_,
			lineVertexBuffer_,
			lineIndexBuffer_,
			static_cast<uint>(meshes_.size()) );

		ChunkLinkSegment::endDrawSegments(
			Moo::rc(),
			texture,
			meshEffect_,
			s_totalTime_,
			speed,
			direction() );
	}
}


/**
 *  This gets the section name of a link.
 *
 *  @return             "link".
 */
/*virtual*/ char const *EditorChunkLink::sectName() const
{
    return "link";
}


/**
 *  This method checks if the draw flag's parent category is visible.
 *
 *  @return true if the "render/gameObjects" category is visible,
 *		false otherwise.
 */
/*virtual*/ bool EditorChunkLink::isDrawFlagVisible() const
{
    return OptionsGameObjects::entitiesVisible();
}


/**
 *  This gets the draw flag for a link.
 *
 *  @return             "render/gameObjects/drawEntities".
 */
/*virtual*/ const char *EditorChunkLink::drawFlag() const
{
    return "render/gameObjects/drawEntities";
}


/**
 *  This gets the representative model (there is none).
 *
 *  @return             NULL.
 */
/*virtual*/ ModelPtr EditorChunkLink::reprModel() const
{
    return NULL;
}


/**
 *  This gets the local to world transform.
 *
 *  @return             The local to world transform.
 */
/*virtual*/ const Matrix & EditorChunkLink::edTransform()
{
	BW_GUARD;

	// The link's chunk could be a shell, but links should not affected by
	// the rotation of the shell, so the edTransform neutralises it.
	if ( chunk() && !chunk()->isOutsideChunk() )
	{
		// Its a shell, so remove the rotation by multiplying by the shell's
		// inverse transform, but first must remove the outside chunk
		// translation component.
		bbShellTransform_ = bbTransform_;
		Chunk* outChunk = outsideChunk();
		Matrix shellXForm = chunk()->transform();
		shellXForm.postMultiply( outChunk->transformInverse() );
		shellXForm.invert();
		bbShellTransform_.postMultiply( shellXForm );
	    return bbShellTransform_;
	}
    return bbTransform_;
}


/**
 *  This gets called when the link is moved to a new chunk.
 *
 *  @param newChunk     The new chunk.  NULL denotes removal from the old
 *                      chunk.
 */
/*virtual*/ void EditorChunkLink::toss(Chunk *newChunk)
{
	BW_GUARD;

    if (pLastObstacleChunk_)
	{
		if (pLastObstacleChunk_->loaded())
		{
	        ChunkLinkObstacle::instance( *pLastObstacleChunk_ ).delObstacles(this);
		}
		pLastObstacleChunk_ = NULL;
	}

    removeFromLentChunks();
	ChunkLink::toss(newChunk);

	if (newChunk)
	{
		EditorChunkLinkManager::instance().registerLink(this);
		addToLentChunks();
	    addAsObstacle();
	}
	else
	{
		EditorChunkLinkManager::instance().unregisterLink(this);
	}
}


/**
 *  This is called to update the link.
 *
 *  @param dtim         The time since this function was last called.
 */
/*virtual*/ void EditorChunkLink::tick(float dtime)
{
	BW_GUARD;

    Vector3 startPt, endPt;
    bool ok = getEndPoints(startPt, endPt, true);
    if (!ok)
        return;

    startPt.y += yOffset_;
    endPt  .y += yOffset_;

	// If one of the links end points has changed, recalculate
	if ((startPt != lastStart_ || endPt != lastEnd_) && edShouldDraw())
	{
		recalcMesh(startPt, endPt, !isEitherEndTransient());
		return;
	}

	// Only recalculate the link mesh if we are drawing the link and the link
	// manager allows us to recalculate
	bool recalc =
		needRecalc_ &&
		EditorChunkLinkManager::instance().canRecalc() &&
		edShouldDraw();
	if (recalc)
    {
        recalcMesh(startPt, endPt);
    }
}


/**
 *  This function adds the link as an obstacle.
 */
/*virtual*/ void EditorChunkLink::addAsObstacle()
{
	BW_GUARD;

	if (!edShouldDraw())
		return;

    Vector3 pt1, pt2;
    getEndPoints(pt1, pt2, false);

	// To minimise the size of the bounding box we align it with the x-axis.  A
	// transformation matrix must be constructed to transform the pt1 and pt2
	// points.
	alignLineWithXAxis(bbInvTransform_, bbTransform_, pt1, pt2);

	Matrix world = outsideChunk()->transform();
	world.preMultiply(bbTransform_);

	// Transform the points so that they are aligned with the x-axis the same
	// as the bb geometry
	pt1 = bbInvTransform_.applyPoint(pt1);
	pt2 = bbInvTransform_.applyPoint(pt2);

    bb_ = BoundingBox
		(
	        Vector3
            (
                std::min(pt1.x, pt2.x) - BB_OFFSET,
                minY_                  - BB_OFFSET,
                std::min(pt1.z, pt2.z) - BB_OFFSET
            ),
		    Vector3
            (
                std::max(pt1.x, pt2.x) + BB_OFFSET,
                maxY_                  + BB_OFFSET,
                std::max(pt1.z, pt2.z) + BB_OFFSET
            )
		);

	// the chunk obstacle needs the absolute end points
    getEndPoints(pt1, pt2, true);
	pLastObstacleChunk_ = outsideChunk();
	ChunkLinkObstacle::instance( *pLastObstacleChunk_ ).addObstacle(
					new EditChunkLinkObstacle( this, world, &bb_, pt1, pt2 ) );
}


/**
 *  This function gets the list of right-click menu options.
 *
 *  @param path         The path of the item.
 *
 *  @return             The list of right-click menu options.
 */
/*virtual*/ BW::vector<BW::string>
EditorChunkLink::edCommand( BW::string const &/*path*/ ) const
{
	BW_GUARD;

    BW::vector<BW::string> commands;

    EditorChunkStationNodePtr start = startNode();
    EditorChunkStationNodePtr end   = endNode();
    // Nodes at each end?
    if (start != NULL && end != NULL)
    {
        commands.push_back(LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_LINK/DISCONNECT"));
        commands.push_back(LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_LINK/SWAP_DIRECTION_THIS_LINK"));
        commands.push_back(LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_LINK/BOTH_DIRECTION_THIS_LINK"));
        commands.push_back(LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_LINK/SWAP_DIRECTION_RUN_LINK"));
        commands.push_back(LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_LINK/BOTH_DIRECTION_RUN_LINK"));
        commands.push_back(LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_LINK/SPLIT"));
#ifdef _DEBUG
        commands.push_back(LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_LINK/VALIDATE"));
#endif
    }
    // A node at one end and an entity at the other?
    else if (start != NULL || end != NULL)
    {
        commands.push_back(LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_LINK/DELETE"));
    }
    return commands;
}


/**
 *  This is called when the user does a right-click command.
 *
 *  @param path         The path of the object.
 *  @param index        The index of the command.
 *
 *  @return             True if the command was done.
 */
/*virtual*/ bool
EditorChunkLink::edExecuteCommand
(
    BW::string     const &path,
    BW::vector<BW::string>::size_type index
)
{
	BW_GUARD;

    ScopedSyncMode scopedSyncMode;

    loadGraph(); // force graph's chunks to be in memory

    EditorChunkStationNodePtr start = startNode();
    EditorChunkStationNodePtr end   = endNode  ();

    // Node at each end case:
    if (start != NULL && end != NULL)
    {
        switch (index)
        {
        case 0: deleteCommand    (); return true;
        case 1: swapCommand      (); return true;
        case 2: bothDirCommand   (); return true;
        case 3: swapRunCommand   (); return true;
        case 4: bothDirRunCommand(); return true;
        case 5: splitCommand     (); return true;
        case 6: validateCommand  (); return true;
        default:                     return false;
        }
    }
    // Node, entity case:
    else if (start != NULL || end != NULL)
    {
        switch (index)
        {
        case 0: deleteLinkCommand(); return true;
        default:                     return false;
        }
    }
    return false;
}


/*virtual*/ bool EditorChunkLink::edShouldDraw()
{
	BW_GUARD;

    if (startItem() == NULL && endItem() == NULL)
        return false;

	return
	(
		s_enableDraw_ &&
		startItem()->edShouldDraw() &&
		(endItem() == NULL || endItem()->edShouldDraw())
	);
}


/**
 *  This function is the collision test for EditorChunkLink.
 *
 *  @param source       The source vector.
 *  @param dir          The direction.
 *  @param wt           The collision triangle.
 *
 *  @return             std::numeric_limits<float>::max() for no collision,
 *                      the t-value of the collision otherwise.
 */
/*virtual*/ float
EditorChunkLink::collide
(
    Vector3         const &source,
    Vector3         const &dir,
    WorldTriangle   &wt
) const
{
	BW_GUARD;

	if ( !s_linkCollide_ || !const_cast<EditorChunkLink*>( this )->edShouldDraw() )
		return std::numeric_limits<float>::max();

    // Apply the chunk transform to the source and dir so "intersects" works
	Vector3 tsource = lineTransform_.applyPoint( outsideChunk()->transform().applyPoint( source ) );
	Vector3 tdir = lineTransform_.applyVector( outsideChunk()->transform().applyVector( dir ) );

    float dist = std::numeric_limits<float>::max();

	for (size_t i = 0; i < meshes_.size(); ++i)
    {
        meshes_[i]->intersects(tsource, tdir, dist, wt);
    }

	// Bring it back to world coords
	wt = WorldTriangle(
			outsideChunk()->transformInverse().applyPoint( lineInvTransform_.applyPoint( wt.v0() ) ),
			outsideChunk()->transformInverse().applyPoint( lineInvTransform_.applyPoint( wt.v1() ) ),
			outsideChunk()->transformInverse().applyPoint( lineInvTransform_.applyPoint( wt.v2() ) ) );

    return dist;
}


/**
 *  This function returns the midpoint of the link, taking into account the
 *  height of the terrain.
 *
 *  @param chunk            The chunk that the midpoint is in.
 *
 *  @return                 The position of the midpoint.
 */
Vector3 EditorChunkLink::midPoint(Chunk *&chunk) const
{
	BW_GUARD;

    Vector3 startPt, endPt;
    getEndPoints(startPt, endPt, true);
    Vector3 midPt = 0.5f*(startPt + endPt);
    bool found = true;
    midPt.y =
        heightAtPos(midPt.x, midY_ + NEXT_HEIGHT_SAMPLE_MID, midPt.z, &found);
    if (!found)
        midPt.y = 0.5f*(startPt.y + endPt.y);
	const float gridSize = chunk->space()->gridSize();
    int wgx = (int)floor(midPt.x/gridSize);
    int wgy = (int)floor(midPt.z/gridSize);
    chunk = getChunk(wgx, wgy);
    midPt = chunk->transformInverse().applyPoint(midPt);
    return midPt;
}


/**
 *  This function returns the frozen state of this link.
 *  A link is considered frozen if either of it's ends is
 *  frozen.
 *
 *  @return true if the link is frozen.
 */
/*virtual*/ bool EditorChunkLink::edFrozen() const
{
	BW_GUARD;

	EditorChunkItem* start =
		static_cast<EditorChunkItem*>(startItem().getObject());
	EditorChunkItem* end =
		static_cast<EditorChunkItem*>(endItem().getObject());
	return (start && start->edFrozen()) || (end && end->edFrozen());
}


/**
 *  This function gets the bounds of the link.
 *
 *  @param bb               This is set to the bounds.
 */
/*virtual*/ void EditorChunkLink::edBounds(BoundingBox &bb) const
{
    bb = bb_;
}


/**
 *	Get the local bounding box (in edTransform's space) to use when marking
 *  as selected
 */
/*virtual*/ void EditorChunkLink::edSelectedBox( BoundingBox & bbRet ) const
{
	BW_GUARD;

    highlight_ = true;
	return edBounds( bbRet );
}



/**
 *  Yes, this is an EditorChunkLink.
 *
 *  @return                 True.
 */
/*virtual*/ bool EditorChunkLink::isEditorChunkLink() const
{
    return true;
}


/**
 *  EditorChunkLink's are not selectable, though you can right-click on them.
 *
 *  @return                 False.
 */
/*virtual*/ bool EditorChunkLink::edCanAddSelection() const
{
    return false;
}


/**
 *  This checks to topological consistency.
 *
 *  @param failureMsg       A failure message if not consistent.
 *
 *  @return                 True if consistent, false otherwise.
 */
/*virtual*/ bool EditorChunkLink::isValid(BW::string &failureMsg) const
{
	BW_GUARD;

    if (!ChunkLink::isValid(failureMsg))
        return false;

    return true;
}


/**
 *  This function makes the link dirty, as well as the nodes it is linked to.
 */
/*virtual*/ void EditorChunkLink::makeDirty()
{
	BW_GUARD;

    Chunk *c = chunk();
    if (c != NULL)
    {
        c->delStaticItem(this);
        c->addStaticItem(this);
        WorldManager::instance().changedChunk(c);
    }
    EditorChunkStationNodePtr start = startNode();
    if (start != NULL)
        start->makeDirty();
    EditorChunkStationNodePtr end = endNode();
    if (end != NULL)
        end->makeDirty();
}


/**
 *  This function enables/disables drawing of chunk links.
 */
/*static*/ void EditorChunkLink::enableDraw(bool enable)
{
    s_enableDraw_ = enable;
}


/**
 *  Called when a new chunk is loaded.
 *
 *	@param	chunkId		The id of the newly loaded chunk.
 */
void EditorChunkLink::chunkLoaded(const BW::string& chunkId)
{
	BW_GUARD;

	// We may be waiting to recalculate the link already
	if (needRecalc_)
		return;

	BW::set<BW::string>::iterator it;
	for (it = unlentChunks_.begin(); it != unlentChunks_.end(); ++it)
	{
		if (*it == chunkId)
		{
			// Only reset the total timer on the initial chunk load
			EditorChunkLinkManager::instance().coveringLoadedChunk();
			needRecalc_ = true;
			return;
		}
	}
}


/**
 *  Called when a chunk has been tossed.
 *
 *	@param	chunkId		The id of the tossed chunk.
 */
void EditorChunkLink::chunkTossed(const BW::string& chunkId)
{
	BW_GUARD;

	BW::vector<ChunkPtr>::iterator it;
	for (it = lentChunks_.begin(); it != lentChunks_.end(); ++it)
	{
		if ((*it)->identifier() == chunkId)
		{
			unlentChunks_.insert(chunkId);
			return;
		}
	}
}


/**
 *  This method returns the outside chunk the link belongs to.
 *
 *  @return		Outside chunk the link belongs to
 */
Chunk* EditorChunkLink::outsideChunk() const
{
	BW_GUARD;

	if ( chunk() && !chunk()->isOutsideChunk() &&
		ChunkManager::instance().cameraSpace() )
	{
		ChunkSpace::Column* col =
			ChunkManager::instance().cameraSpace()->column(
				chunk()->transform().applyToOrigin() );
		if ( col && col->pOutsideChunk() )
			return col->pOutsideChunk();
	}
	return chunk();
}


/**
 *  This function recalculates the EditorChunkLink's mesh.
 *
 *  @param s            The start point.
 *  @param e            The end point.
 */
void EditorChunkLink::recalcMesh(Vector3 const &s, Vector3 const &e, bool updateCollisions)
{
	BW_GUARD;

	bool foundHeight = true; ; // is the height found?

    // Get the start and add it to the polyline_.
    polyline_.clear();
    polyline_.push_back(s);  // Add the start

    // Find the difference between the heights at the start/end and the
    // ground:
    float sd, ed;
    sd  = s.y - heightAtPos(s.x, s.y + NEXT_HEIGHT_SAMPLE, s.z, &foundHeight);
    if (!foundHeight)
        sd = 0.0f;
    ed  = e.y - heightAtPos(e.x, e.y + NEXT_HEIGHT_SAMPLE, e.z, &foundHeight);
    if (!foundHeight)
        ed = 0.0f;

    bool  underground = sd < 0.0f || ed < 0.0f;

    // Choose a segment length that will not make the second last point in
    // the polyline_ end on the segment.
    float len = (e - s).length();
    float segLength = std::max(len - 2*MAX_SEG_LENGTH, MAX_SEG_LENGTH);
    while (segLength >= MAX_SEG_LENGTH && segLength > MIN_SEG_LENGTH)
        segLength *= 0.5f;
    bool ok          = true;

    // We interpolate between the x,z coordinates of the start and end points
    // and do the following:
    //  1.  Get the height at the interpolated point (h) and interpolate the
    //      height at that point by adding it to the interpolated height
    //      between the start and end (y).
    //  2.  Find the closest point to the mid point and get it's y
    //      coordinate in case the segment is split (midY_).  This helps give a
    ///     good point to split the link.
    //  3.  Keep track of the minimum and maximum y-value (minY_, maxY_).  This
    //      helps with the collision detection.
    float lastY      = s.y;
    float midDist    = std::numeric_limits<float>::max();
    float midX = 0.5f*(s.x + e.x);
    float midY = 0.5f*(s.y + e.y);
    midY_ = midY;
    for (float t = MAX_SEG_LENGTH; t < len; t += segLength)
    {
        float x = Math::lerp(t, 0.0f, len, s.x, e.x);
        float y = Math::lerp(t, 0.0f, len, s.y, e.y);
        float z = Math::lerp(t, 0.0f, len, s.z, e.z);

        if (!underground)
        {
			float h =
				heightAtPos
				(
					x,
					lastY + segLength*NEXT_HEIGHT_SAMPLE,
					z,
					&foundHeight
				);
			// If the height was not found then linearly interpolate between
			// the last good point and the end:
			if (!foundHeight)
				h = Math::lerp(t, t -  segLength, len, lastY, e.y);

			if (h - y > 0.0f)
				y += HEIGHT_BUFFER + h - y;

			ok &= foundHeight;
		}

		Vector3 thisPos(x, y, z);
        polyline_.push_back(thisPos);
        lastY = y;
        float thisMidDist = (x - midX)*(x - midX) + (y - midY)*(y - midY);
        if (thisMidDist < midDist)
        {
            midDist = thisMidDist;
            midY_ = lastY;
        }
    }

    polyline_.push_back(e);  // Add the end

    lastStart_   = s;       // Update so we don't recalculate on every draw
    lastEnd_     = e;

	needRecalc_ = false;

	// To minimise the size of the bounding box, we align the line geometry with
	// the x-axis.  A transformation matrix must be constructed to transform the
	// polyline.  The inverse of this matrix is used to transform the line back
	// to its original position.
	alignLineWithXAxis(
		lineInvTransform_, lineTransform_, lastStart_, lastEnd_);

	// Create the index and vertex buffers:
    meshes_.clear();
    if (polyline_.size() - 1 > 0)
    {
		int newVertexBufferSize = static_cast<int>( (polyline_.size() - 1)*
			sizeof(ChunkLinkSegment::VertexType)*ChunkLinkSegment::numberVertices());

		bool needsNewBuffers =
			(vertexBufferSize_ < newVertexBufferSize || // regenerate if the new size is bigger
			vertexBufferSize_ > newVertexBufferSize * 2); // or if it's less than half of what we've got (to save memory)
		if (needsNewBuffers)
		{
			vertexBuffer_.release();
			vertexBufferSize_ = newVertexBufferSize;

			HRESULT hr = vertexBuffer_.create
				(
					newVertexBufferSize,
					D3DUSAGE_WRITEONLY,
					ChunkLinkSegment::VertexType::fvf(),
					D3DPOOL_MANAGED
				);
			ASSERT(SUCCEEDED(hr));

			lineVertexBuffer_.release();

			hr = lineVertexBuffer_.create
				(
					(uint32)(polyline_.size() - 1)*sizeof(ChunkLinkSegment::VertexType)*2,
					D3DUSAGE_WRITEONLY,
					ChunkLinkSegment::VertexType::fvf(),
					D3DPOOL_MANAGED
				);
			ASSERT(SUCCEEDED(hr));

			hr = indexBuffer_.create(
				(int)(polyline_.size() - 1)*ChunkLinkSegment::numberIndices(),
				D3DFMT_INDEX16,
				D3DUSAGE_WRITEONLY,
				D3DPOOL_MANAGED );
			ASSERT(SUCCEEDED(hr));

			hr = lineIndexBuffer_.create(
				static_cast<int>((polyline_.size() - 1)*2),
				D3DFMT_INDEX16,
				D3DUSAGE_WRITEONLY,
				D3DPOOL_MANAGED );
			ASSERT(SUCCEEDED(hr));
		}

		// Set minY_/maxY_ to the start point y-value
		Vector3 const &startp = lineInvTransform_.applyPoint(polyline_[0]);
		minY_ = startp.y;
		maxY_ = startp.y;

		// Turn the polyline_ into a mesh:
        float dist = 0.0;
        for (size_t i = 0; i < polyline_.size() - 1; ++i)
        {
			// Align the points with the x-axis
			Vector3 const &sp = polyline_[i    ];
            Vector3 const &ep = polyline_[i + 1];

			// Update minY_/maxY_
			minY_ = std::min(minY_, lineInvTransform_.applyPoint(ep).y);
			maxY_ = std::max(maxY_, lineInvTransform_.applyPoint(ep).y);

			float thisDist = (ep - sp).length();
            ChunkLinkSegmentPtr mesh =
                new ChunkLinkSegment
                (
                    sp,
                    ep,
                    MAX_SEG_LENGTH*dist,
                    MAX_SEG_LENGTH*(dist + thisDist),
                    LINK_THICKNESS,
                    vertexBuffer_,
                    indexBuffer_,
                    (uint16)(i*ChunkLinkSegment::numberVertices()),
                    (uint16)(i*ChunkLinkSegment::numberIndices()),
                    lineVertexBuffer_,
                    lineIndexBuffer_,
                    (uint16)(i*2),
                    (uint16)(i*2)
                );
            meshes_.push_back(mesh);
            dist += thisDist;
        }
    }

	// Update the lent chunks
    removeFromLentChunks();
	addToLentChunks();

	// Update collision information:
    if (chunk())
    {
		if (updateCollisions)
		{
			if (pLastObstacleChunk_)
			{
				ChunkLinkObstacle::instance( *pLastObstacleChunk_ ).delObstacles(
																			this );
				pLastObstacleChunk_ = NULL;
			}
			addAsObstacle();
		}
		this->syncInit();
    }
}


/**
 *  Method calculates the transformation needed to align a line with the x-axis
 *	by rotating the line around its start point.
 *
 *	@param	align		The transformation matrix to align points along the line
 *						with the x-axis.
 *	@param	invAlign	The inverse of align.
 *	@param	startPoint	The start point of the line.
 *	@param	endPoint	The end point of the line.
 */
void EditorChunkLink::alignLineWithXAxis(
	Matrix& align, Matrix& invAlign, Vector3 startPoint,
	Vector3 endPoint) const
{
	BW_GUARD;

	// First test the projection of the line on the xz-plane.  If the length of
	// the projection is less than a certain tolerance, return the identity
	// matrix for both transformation matrices, since we can not be certain that
	// the up vector of the line will match the y-axis after aligning.
	// 1)	Simply discard the y-component
	Vector2 projection( endPoint.x - startPoint.x, endPoint.z - startPoint.z );
	if ( projection.lengthSquared() < 0.001f )
	{
		align.setIdentity();
		invAlign.setIdentity();
		return;
	}

	// 2)	Okay, we're good to go. Normalise the projection matrix so that the
	//		dot product below returns the correct value
	projection.normalise();

	// 3)	Calculate the arc-cosine of the dot product between the projection
	//		vector and the positive x-axis to get the angle between the vectors
	float theta = acos(
		Math::clamp(-1.0f, projection.dotProduct( Vector2(1.0, 0.0) ), +1.0f) );

	// 4)	Determine if the projection has a positive or negative z-component
	//		(this is projection.y since we are using a 2D vector), this affects
	//		the sign of the angle
	if ( projection.y < 0.0 )
		theta = -theta;

	// 5)	We now have the rotation about the y-axis
	Matrix		Ry;			Ry.setRotateY(theta);

	// 6)	Translate the line so that its start point is at the origin and
	//		rotate about the y-axis using Ry
	Matrix		invT;
	invT.setTranslate(-startPoint);

	// 7)	Apply to align matrix
	align.setIdentity();
	align.postMultiply(invT);
	align.postMultiply(Ry);
	align.applyPoint(endPoint, endPoint);

	// 8)	Now that the line is in the xy-plane, we need to rotate the end
	//		point about the z-axis to align it with the x-axis.  Perform similar
	//		step to above
	projection = Vector2(endPoint.x, endPoint.y);
	projection.normalise();
	theta = acos(
		Math::clamp(-1.0f, projection.dotProduct(Vector2(1.0, 0.0)), +1.0f));
	if (projection.y > 0.0)
		theta = -theta;

	// 9)	Apply to align matrix
	Matrix		Rz;
	Rz.setRotateZ(theta);
	align.postMultiply(Rz);

	// 10)	Tranlate the line back to its original position
	Matrix		T;
	T.setTranslate(startPoint);
	align.postMultiply(T);

	// 11)	Finally, calculate the inverse
	invAlign = align;
	invAlign.invert();
}


/**
 *  This function gets the end points.
 *
 *  @param s        The start point.
 *  @param e        The end point.
 *  @param absoluteCoords   If true then the points are world coordinates,
 *                  if false then they are wrt the chunk's coordinates.
 *
 *  @return         False if the end points don't yet exist.
 */
bool
EditorChunkLink::getEndPoints
(
    Vector3         &s,
    Vector3         &e,
    bool            absoluteCoords
) const
{
	BW_GUARD;

    EditorChunkItem *start  = (EditorChunkItem *)startItem().getObject();
    EditorChunkItem *end    = (EditorChunkItem *)endItem  ().getObject();

    // Maybe it's still loading...
    if
    (
        start == NULL || end == NULL
        ||
        start->chunk() == NULL || end->chunk() == NULL
    )
    {
        return false;
    }

    Vector3 lStartPt        = start->edTransform().applyToOrigin();
    Vector3 lEndPt          = end  ->edTransform().applyToOrigin();

    s   = start->chunk()->transform().applyPoint(lStartPt);
    e   = end  ->chunk()->transform().applyPoint(lEndPt  );

	// avoid perfectly vertical links, as they won't get calculated properly.
	if ( s.x == e.x && s.z == e.z )
		s.x += VERTICAL_LINK_EPSILON;

    if (!absoluteCoords)
    {
        Matrix m = outsideChunk()->transform();
        m.invert();
        s = m.applyPoint(s);
        e = m.applyPoint(e);
    }

    return true;
}


/**
 *  This function determines if either end of the link is currently transient.
 *
 *  @return	True is an end point is transient, false otherwise.
 */
bool EditorChunkLink::isEitherEndTransient()
{
	BW_GUARD;

    EditorChunkItem *start  = (EditorChunkItem *)startItem().getObject();
    EditorChunkItem *end    = (EditorChunkItem *)endItem  ().getObject();

	return (
		start &&
		end &&
		(start->edIsTransient() || end->edIsTransient()) );
}


/**
 *  This function returns the height at the given position.
 *
 *  @param x        The x coordinate.
 *  @param y        The y coordinate to search down from.
 *  @param z        The z coordinate.
 *  @param found    If not NULL this will be set to true/false if something
 *                  was found.
 *
 *  @return         The height at the given position.
 */
float EditorChunkLink::heightAtPos(float x, float y, float z, bool *found) const
{
	BW_GUARD;

	// avoid colliding with the links (very expensive)
	s_linkCollide_ = false;

    // Look downwards from an estimated position:
    Vector3 srcPos(x, y, z);
    Vector3 dstPos(x, -MAX_SEARCH_HEIGHT, z);
    float dist =
        ChunkManager::instance().cameraSpace()->collide
        (
            srcPos,
            dstPos,
            CollisionTerrainOnly()
        );
    float result = 0.0f;
    if (dist > 0.0f)
    {
        result = y - dist;
        if (found != NULL)
            *found = true;
    }
    else
    {
        // That didn't work, look upwards:
        srcPos = Vector3(x, y, z);
        dstPos = Vector3(x, +MAX_SEARCH_HEIGHT, z);
        dist =
            ChunkManager::instance().cameraSpace()->collide
            (
                srcPos,
                dstPos,
                CollisionTerrainOnly()
            );
        if (dist > 0.0f)
        {
            result = y + dist;
            if (found != NULL)
                *found = true;
        }
        // No height found, give up.  This can occur if, for example, a
        // chunk's terrain has not yet loaded.
        else
        {
            if (found != NULL)
                *found = false;
        }
    }

	// turn on link collisions again
	s_linkCollide_ = true;

    return result;
}


/**
 *  This function gets the chunk at the given position.
 *
 *  @param x        The x coordinate.
 *  @param y        The y coordinate.
 *
 *  @return         The chunk at the given position.
 */
Chunk *EditorChunkLink::getChunk(int x, int y) const
{
	BW_GUARD;

    BW::string chunkName;
    int16 wgx = (int16)x;
    int16 wgy = (int16)y;
    chunkID(chunkName, wgx, wgy);
    Chunk *chunk =
        ChunkManager::instance().findChunkByName
        (
            chunkName,
            WorldManager::instance().geometryMapping(),
            false // don't create chunk if one doesn't exist
        );
    return chunk;
}


/**
 *  This function adds the link to the lent-out chunks.
 */
void EditorChunkLink::addToLentChunks()
{
	BW_GUARD;

	unlentChunks_.clear();

	const float gridSize = pChunk_->space()->gridSize();
	// Generate a list of the chunks that the link passes through and add them:
    int lastx = std::numeric_limits<int>::max();
    int lasty = std::numeric_limits<int>::max();
    for (size_t i = 0; i < polyline_.size(); ++i)
    {
        // No need to transform the polyline since it is already in world space
		Vector3 const &pt = polyline_[i];
        int wgx = (int)floor(float(pt.x)/gridSize);
        int wgy = (int)floor(float(pt.z)/gridSize);
        if (wgx != lastx || wgy != lasty)
        {
            Chunk *loanChunk = getChunk(wgx, wgy);
            if (loanChunk != NULL && loanChunk->loaded() && !loanChunk->loading())
            {
                if (loanChunk->addLoanItem(this))
                    lentChunks_.push_back(loanChunk);
            }
            else
            {
				// Add the chunk id to the list of chunks needing lending
				BW::string chunkName;
				chunkID(chunkName, (int16)wgx, (int16)wgy);
				if (chunkName != "")
					unlentChunks_.insert(chunkName);
            }

			lastx = wgx;
            lasty = wgy;
        }
    }
}


/**
 *  This function removes the link from the lent-out chunks.
 */
void EditorChunkLink::removeFromLentChunks()
{
	BW_GUARD;

    // Remove from the list of loaned chunks:
    for (size_t i = 0; i < lentChunks_.size(); ++i)
    {
        if (lentChunks_[i]->isLoanItem(this))
            lentChunks_[i]->delLoanItem(this);
    }
    lentChunks_.clear();
}


/**
 *  This function is useful to cast the start as an EditorChunkStationNodePtr.
 *
 *  @return         The start as an EditorChunkStationNodePtr.
 */
EditorChunkStationNodePtr EditorChunkLink::startNode() const
{
	BW_GUARD;

    if (startItem() != NULL && startItem()->isEditorChunkStationNode())
        return static_cast<EditorChunkStationNode *>(startItem().getObject());
    else
        return EditorChunkStationNodePtr();
}


/**
 *  This function is useful to cast the end as an EditorChunkStationNodePtr.
 *
 *  @return         The end as an EditorChunkStationNodePtr.
 */
EditorChunkStationNodePtr EditorChunkLink::endNode() const
{
	BW_GUARD;

    if (endItem() != NULL && endItem()->isEditorChunkStationNode())
        return static_cast<EditorChunkStationNode *>(endItem().getObject());
    else
        return EditorChunkStationNodePtr();
}


/**
 *  This function finds neighbourhood links.  These are links that are
 *  connected to this go through nodes that only have two links.
 */
void EditorChunkLink::neighborhoodLinks(BW::vector<Neighbor> &neighbors) const
{
	BW_GUARD;

    EditorChunkStationNodePtr start = startNode();
    EditorChunkStationNodePtr end   = endNode();

    // Handle the degenerate case:
    if (start == NULL || end == NULL)
        return;

    // This link is a neighbour to itself, which may seem a little odd!
    Neighbor myself;
    myself.first  = const_cast<EditorChunkLink *>(this);
    myself.second = true;
    neighbors.push_back(myself);

    // Find all links in the start direction:
    EditorChunkStationNodePtr node         = start;
    EditorChunkLinkPtr        link         = const_cast<EditorChunkLink *>(this);
    bool                      done         = false;
    bool                      sameDirAs1st = true;
    while (node && node->numberLinks() == 2 && !done)
    {
        done = true;
        for (size_t i = 0; i < node->numberLinks() && done; ++i)
        {
            EditorChunkLinkPtr nextLink = node->getLink(i);
            if (nextLink != NULL && nextLink != this && nextLink != link)
            {
                EditorChunkStationNodePtr nextNode = NULL;
                // Get the next node:
                if (nextLink->startNode() != node)
                    nextNode = nextLink->startNode();
                else
                    nextNode = nextLink->endNode();
                // Have we added the new link yet?  If we have then we've gone
                // in a cycle and we are done:
                for (size_t k = 0; k < neighbors.size() && done; ++k)
                {
                    if (neighbors[k].first == nextLink.getObject())
                    {
                        done = true;
                    }
                    else
                    {
                        // Are they in opposite directions?
                        bool sameDir = true;
                        if
                        (
                            nextLink->startNode() == link->startNode()
                            ||
                            nextLink->endNode() == link->endNode()
                        )
                        {
                            sameDir = false;
                        }
                        if (!sameDir)
                            sameDirAs1st = !sameDirAs1st;
                        Neighbor neighbor;
                        neighbor.first  = nextLink.getObject();
                        neighbor.second = sameDirAs1st;
                        // Add to the neighbor list:
                        neighbors.push_back(neighbor);
                        // There is more to do.
                        done = false;
                    }
                }
                node = nextNode;
                link = nextLink;
            }
        }
    }

    // Find all links in the end direction:
    node         = end;
    link         = const_cast<EditorChunkLink *>(this);
    done         = false;
    sameDirAs1st = true;
    while (node && node->numberLinks() == 2 && !done)
    {
        done = true;
        for (size_t i = 0; i < node->numberLinks() && done; ++i)
        {
            EditorChunkLinkPtr nextLink = node->getLink(i);
            if (nextLink != NULL && nextLink != this && nextLink != link)
            {
                EditorChunkStationNodePtr nextNode = NULL;
                // Get the next node:
                if (nextLink->startNode() != node)
                    nextNode = nextLink->startNode();
                else
                    nextNode = nextLink->endNode();
                // Have we added the new link yet?  If we have then we've gone
                // in a cycle and we are done:
                for (size_t k = 0; k < neighbors.size() && done; ++k)
                {
                    if (neighbors[k].first == nextLink.getObject())
                    {
                        done = true;
                    }
                    else
                    {
                        // Are they in opposite directions?
                        bool sameDir = true;
                        if
                        (
                            nextLink->startNode() == link->startNode()
                            ||
                            nextLink->endNode() == link->endNode()
                        )
                        {
                            sameDir = false;
                        }
                        if (!sameDir)
                            sameDirAs1st = !sameDirAs1st;
                        Neighbor neighbor;
                        neighbor.first  = nextLink.getObject();
                        neighbor.second = sameDirAs1st;
                        // Add to the neighbor list:
                        neighbors.push_back(neighbor);
                        // There is more to do.
                        done = false;
                    }
                }
                node = nextNode;
                link = nextLink;
            }
        }
    }
}


/**
 *  This is called to delete a link.
 */
void EditorChunkLink::deleteCommand()
{
	BW_GUARD;

    EditorChunkStationNodePtr start = startNode();
    EditorChunkStationNodePtr end   = endNode();

	UndoRedo::instance().add
    (
        new StationLinkOperation
        (
            start,
            end,
            start->getLinkDirection(end.getObject())
        )
    );
    UndoRedo::instance().barrier(LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_LINK/DELETE_LINK"), false);
    start->removeLink(end->id());
    start->makeDirty();
    end  ->makeDirty();
    BW::vector<ChunkItemPtr> emptyList;
    WorldManager::instance().setSelection(emptyList, true);
}


/**
 *  This is called to swap the direction of a link.
 */
void EditorChunkLink::swapCommand()
{
	BW_GUARD;

    EditorChunkStationNodePtr start = startNode();
    EditorChunkStationNodePtr end   = endNode();

	UndoRedo::instance().add
    (
        new StationLinkOperation
        (
            start,
            end,
            start->getLinkDirection(end.getObject())
        )
    );
    UndoRedo::instance().barrier(LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_LINK/SWAP_LINK_DIRECTION"), false);
    Direction newDir = direction();
    if (direction() == ChunkLink::DIR_START_END)
        newDir = ChunkLink::DIR_END_START;
    else if (direction() == ChunkLink::DIR_END_START)
        newDir = ChunkLink::DIR_START_END;
    else
        newDir = ChunkLink::DIR_START_END;
    start->setLink(end  .getObject(), ((newDir & DIR_START_END) != 0));
    end  ->setLink(start.getObject(), ((newDir & DIR_END_START) != 0));
    makeDirty();
}


/**
 *  This is used to set the direction of a link to both directions.
 */
void EditorChunkLink::bothDirCommand()
{
	BW_GUARD;

    EditorChunkStationNodePtr start = startNode();
    EditorChunkStationNodePtr end   = endNode();

	UndoRedo::instance().add
    (
        new StationLinkOperation
        (
            start,
            end,
            start->getLinkDirection(end.getObject())
        )
    );
    UndoRedo::instance().barrier(LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_LINK/BOTH_LINK_DIRECTION"), false);
    direction(ChunkLink::DIR_BOTH);
    start->setLink(end  .getObject(), true);
    end  ->setLink(start.getObject(), true);
    makeDirty();
}


/**
 *  This is used to swap the direction of a run of links.
 */
void EditorChunkLink::swapRunCommand()
{
	BW_GUARD;

    EditorChunkStationNodePtr start = startNode();
    EditorChunkStationNodePtr end   = endNode();

    BW::vector<Neighbor> links;
    neighborhoodLinks(links);
    Direction newDir, oppositeDir;
    if (direction() == ChunkLink::DIR_START_END)
    {
        newDir      = ChunkLink::DIR_END_START;
        oppositeDir = ChunkLink::DIR_START_END;
    }
    else if (direction() == ChunkLink::DIR_END_START)
    {
        newDir      = ChunkLink::DIR_START_END;
        oppositeDir = ChunkLink::DIR_END_START;
    }
    else
    {
        newDir      = ChunkLink::DIR_START_END;
        oppositeDir = ChunkLink::DIR_END_START;
    }
    for (size_t i = 0; i < links.size(); ++i)
    {
        start = links[i].first->startNode();
        end   = links[i].first->endNode();
        UndoRedo::instance().add
        (
            new StationLinkOperation
            (
                start,
                end,
                start->getLinkDirection(end.getObject())
            )
        );
        if (links[i].second)
        {
            start->setLink(end  .getObject(), (newDir & DIR_START_END) != 0);
            end  ->setLink(start.getObject(), (newDir & DIR_END_START) != 0);
        }
        else
        {
            start->setLink(end  .getObject(), (oppositeDir & DIR_START_END) != 0);
            end  ->setLink(start.getObject(), (oppositeDir & DIR_END_START) != 0);
        }
        links[i].first->makeDirty();
    }
    UndoRedo::instance().barrier(LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_LINK/SWAP_LINK_DIRECTION"), false);
}


/**
 *  This is used to set the direction of a run of links to both.
 */
void EditorChunkLink::bothDirRunCommand()
{
	BW_GUARD;

    EditorChunkStationNodePtr start = startNode();
    EditorChunkStationNodePtr end   = endNode();

    BW::vector<Neighbor> links;
    neighborhoodLinks(links);
    Direction newDir = direction();
    if (newDir == ChunkLink::DIR_START_END)
        newDir = ChunkLink::DIR_END_START;
    else if (newDir == ChunkLink::DIR_END_START)
        newDir = ChunkLink::DIR_START_END;
    else
        newDir = ChunkLink::DIR_START_END;
    for (size_t i = 0; i < links.size(); ++i)
    {
        start = links[i].first->startNode();
        end   = links[i].first->endNode();
        UndoRedo::instance().add
        (
            new StationLinkOperation
            (
                start,
                end,
                start->getLinkDirection(end.getObject())
            )
        );
        start->setLink(end  .getObject(), true);
        end  ->setLink(start.getObject(), true);
        links[i].first->makeDirty();
    }
    UndoRedo::instance().barrier(LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_LINK/BOTH_LINK_DIRECTION"), false);
}


/**
 *  This is called to split a link.
 */
void EditorChunkLink::splitCommand()
{
	BW_GUARD;

    EditorChunkStationNodePtr start = startNode();
    EditorChunkStationNodePtr end   = endNode();

    start->split(*end);
    makeDirty();
}


/**
 *  This is used to validate all of the graphs.  It displays an error message
 *  if something is not valid and does nothing if it is.
 */
void EditorChunkLink::validateCommand()
{
	BW_GUARD;

    bool ok = true;
    BW::string failureMsg;
    BW::vector<StationGraph*> graphs = StationGraph::getAllGraphs();
    for (size_t i = 0; i < graphs.size() && ok; ++i)
    {
        if (!graphs[i]->isValid(failureMsg))
            ok = false;
    }
    if (!ok)
    {
		BW::wstring wfailureMsg;
		bw_utf8tow( failureMsg, wfailureMsg );
        AfxMessageBox(wfailureMsg.c_str());
    }
}


/**
 *  This is called to delete a link with an entity.
 */
void EditorChunkLink::deleteLinkCommand()
{
	BW_GUARD;

    EditorChunkStationNodePtr start = startNode();
    EditorChunkStationNodePtr end   = endNode();

    // Sort out which is the node and which is the entity:
    EditorChunkStationNodePtr node = start;
    EditorChunkEntity *entity =
        static_cast<EditorChunkEntity *>(endItem().getObject());
    if (end != NULL)
    {
        node = end;
        entity = static_cast<EditorChunkEntity *>(startItem().getObject());
    }
    // Set an undo point:
	UndoRedo::instance().add
    (
        new StationEntityLinkOperation
        (
            entity
        )
    );
    UndoRedo::instance().barrier(LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_LINK/DELETE_LINK"), false);
    // Do the link deletion:
    entity->disconnectFromPatrolList();
    makeDirty();
}


/**
 *  This is called to force the graph to load of the chunks that it's nodes
 *  are in.
 */
void EditorChunkLink::loadGraph()
{
	BW_GUARD;

    EditorChunkStationNodePtr start = startNode();
    EditorChunkStationNodePtr end   = endNode();

    if (start != NULL && start->graph() != NULL)
        start->graph()->loadAllChunks(WorldManager::instance().geometryMapping());
    else if (end != NULL && end->graph() != NULL)
        end->graph()->loadAllChunks(WorldManager::instance().geometryMapping());
}


/**
 *  This method allows derived classes to change the directional texture
 *
 *	@param texture	New directional texture
 */
void EditorChunkLink::directionalTexture( Moo::BaseTexturePtr texture )
{
	directionalTexture_ = texture;
}


/**
 *  This method allows derived classes to change the texture for links with
 *	no direction.
 *
 *	@param texture	New no-direction texture
 */
void EditorChunkLink::noDirectionTexture( Moo::BaseTexturePtr texture )
{
	noDirectionTexture_ = texture;
}


/**
 *  This method allows derived classes to change the shader for links.
 *
 *	@param effect	New shader
 */
void EditorChunkLink::materialEffect( Moo::EffectMaterialPtr effect )
{
	if ( !effect )
		return;

    meshEffect_ = NULL;
	meshEffect_ = effect;
}


/**
 *  This method allows derived classes to get the shader for links.
 *
 *	@return		Current shader
 */
Moo::EffectMaterialPtr EditorChunkLink::materialEffect()
{
	return meshEffect_;
}


/**
 *  This static method allows derived classes to know the state of the
 *	static s_enableDraw_ variable.
 *
 *	@return		True if drawing links is enabled, false otherwise.
 */
/*static*/ bool EditorChunkLink::enableDraw()
{
	return s_enableDraw_;
}


void EditorChunkLink::syncInit()
{
	#if UMBRA_ENABLE
	BW_GUARD;

	bw_safe_delete(pUmbraDrawItem_);
	Vector3 pt1, pt2;
    getEndPoints(pt1, pt2, true);    
	BoundingBox bb
		(
	        Vector3
            (
                std::min(pt1.x, pt2.x) - LINK_THICKNESS,
				std::min(pt1.y, pt2.y) - LINK_THICKNESS,
                std::min(pt1.z, pt2.z) - LINK_THICKNESS
            ),
		    Vector3
            (
                std::max(pt1.x, pt2.x) + LINK_THICKNESS,
				std::max(pt1.y, pt2.y) + LINK_THICKNESS + yOffset_,
                std::max(pt1.z, pt2.z) + LINK_THICKNESS
            )
		);		
	Matrix bbInv = this->edTransform();
	bbInv.invert();
	bb.transformBy( bbInv );

	Matrix m = this->edTransform();

	// Create the umbra chunk item
	UmbraChunkItem* pUmbraChunkItem = new UmbraChunkItem();
	pUmbraChunkItem->init( this, bb, m, pChunk_->getUmbraCell());
	pUmbraDrawItem_ = pUmbraChunkItem;

	this->updateUmbraLenders();
	#endif
}

BW_END_NAMESPACE

