#include "pch.hpp"
#include "worldeditor/world/items/editor_chunk_point_link.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/misc/options_helper.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/geometry_mapping.hpp"
#include "resmgr/auto_config.hpp"
#include "moo/draw_context.hpp"

BW_BEGIN_NAMESPACE

namespace // anonymous
{
	AutoConfigString s_texture = "editor/chunkLinkTexture";
    AutoConfigString s_shader  = "editor/chunkLinkShader";


	/**
	 *	This local class implements a sorted channel object. in order to
	 *	properly draw transparent links.
	 */
	class SortedLinkItem : public Moo::DrawContext::UserDrawItem
	{
	public:
		/**
		 *  Constructor.
		 *
		 *  @param link		Link object to draw in the sorted channel.
		 */
		SortedLinkItem( EditorChunkPointLink* link )
		: link_( link )
		{
		}

		/**
		 *  This method calls the link's 'drawInternal' method.
		 */
		void draw()
		{
			BW_GUARD;

			link_->drawInternal();
		}

		/**
		 *  This method just deletes this object.
		 */
		void fini()
		{
			BW_GUARD;

			delete this;
		}

	private:
		EditorChunkPointLink* link_;
	};

} // anonymous namespace


/**
 *	Constructor: creates the texture for this type of links
 */
EditorChunkPointLink::EditorChunkPointLink() :
	endPoint_( Vector3::zero() )
{
	BW_GUARD;

    // Load the link texture:
	Moo::TextureManager::instance()->getDetailsManager()->setFormat( s_texture, D3DFMT_A8R8G8B8 );
	texture_ = Moo::TextureManager::instance()->get( s_texture );
	noDirectionTexture( texture_ );

    Moo::EffectMaterialPtr effect = new Moo::EffectMaterial();
    if ( effect->initFromEffect( s_shader ) )
		materialEffect( effect );
}


/**
 *	This method is called to define the end point of the link, in
 *	absolute coordinates.
 *
 *	@param endPoint		End point of the link in absolute coordinates.
 */
void EditorChunkPointLink::endPoint( const Vector3& endPoint, const BW::string& chunkId )
{
	endPoint_ = endPoint;
	chunkId_ = chunkId;
}


/**
 *	This method overrides the base class' implementation to return the
 *	appropriate end points for this kind of link, using the start item
 *	and the end point.
 *
 *	@param s				Return parameter for the start point position.
 *	@param index			Return parameter for the end point position.
 *	@param absoluteCoords	True to use absolute coordinates instead of local.
 *	@return					True if successful, false otherwise.
 */
/*virtual*/ bool EditorChunkPointLink::getEndPoints(
	Vector3 &s, Vector3 &e, bool absoluteCoords ) const
{
	BW_GUARD;

    EditorChunkItem *start  = (EditorChunkItem *)startItem().getObject();

    // Maybe it's still loading...
    if ( start == NULL || start->chunk() == NULL)
    {
        return false;
    }

    Vector3 lStartPt = start->edTransform().applyToOrigin();
    s = start->chunk()->transform().applyPoint( lStartPt );
	e = endPoint_;

	// Get the start height, and check if it's in the ground
	bool foundHeight;
	float sh  = heightAtPos(s.x, s.y + NEXT_HEIGHT_SAMPLE, s.z, &foundHeight);
	float sd = s.y - sh;
	if (!foundHeight)
		sd = 0.0f;

	bool inAir = fabs(sd) > AIR_THRESHOLD;
	if ( !inAir )
	{
		// It's in the ground, get height at the middle and interpolate it to
		// the end using the start height.
		Vector3 mid = (e - s) / 2.0f + s;
		float mh = heightAtPos(mid.x, mid.y + NEXT_HEIGHT_SAMPLE, mid.z);
		float h = (mh-sh) * 2.0f + sh;
		if ( h > e.y )
		{
			// It's not in the air, and the terrain occluding the link, so
			// make the end point as high as the terrain at that position.
			float oldLength = ( e - s ).length();
			e.y = h;
			// Preserve the length
			Vector3 dir = ( e - s );
			dir.normalise();
			e = dir * oldLength + s;
		}
	}

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
 *  This method overrides the base class' method to render to the sorted
 *	channel, to ensure transparency gets rendered properly.
 */
/*virtual*/ void EditorChunkPointLink::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;

    if (startItem() == NULL)
        return;

    if 
    (
        edShouldDraw()
		&&
		!WorldManager::instance().drawSelection()
        &&
        !Moo::rc().reflectionScene() 
        && 
        !Moo::rc().mirroredTransform()
        &&
        enableDraw()
    )
    {
		BW_GUARD;

		// The distance is calculated from the start item to the camera,
		// which is good enough for links.
		Vector3 linkPos = startItem()->edTransform().applyToOrigin();
		linkPos = startItem()->chunk()->transform().applyPoint( linkPos );
		Vector3 cameraPos = Moo::rc().invView().applyToOrigin();
		float distance = ( linkPos - cameraPos ).length();	

		drawContext.drawUserItem( new SortedLinkItem( this ),
			Moo::DrawContext::TRANSPARENT_CHANNEL_MASK, distance );
	}
}


/**
 *  This method is called by the sorted channel drawing item created in the
 *	'draw' method. It also checks to see if one of the end points of the link
 *	is in a read-only chunk. If so, it will set a shader constant to draw it
 *	in red.
 */
/*virtual*/ void EditorChunkPointLink::drawInternal()
{
	BW_GUARD;

	if ( materialEffect()->pEffect() && materialEffect()->pEffect()->pEffect() )
	{
		EditorChunkItem* start =
			static_cast<EditorChunkItem*>(startItem().getObject());

		int16 gridX;
		int16 gridZ;
		WorldManager::instance().geometryMapping()->gridFromChunkName( chunkId_, gridX, gridZ );

		bool drawRed = false;
		if (OptionsMisc::readOnlyVisible())
		{
			if ( start != NULL && !start->edIsEditable() )
				drawRed = true;
			else if ( !EditorChunk::outsideChunkWriteable( gridX, gridZ ) )
				drawRed = true;
		}

		if ( drawRed )
		{
			materialEffect()->pEffect()->pEffect()->SetBool("colourise", TRUE);
		}
		else
		{
			materialEffect()->pEffect()->pEffect()->SetBool("colourise", FALSE);
		}
	}
	EditorChunkLink::drawImmediate();
}


/**
 *  This method prevents collisions against this kind of links
 *
 *	@param source	Starting point of the collision ray
 *	@param dir		Direction of the collision ray
 *	@param wt		Triangle to test, in world coordinates
 *	@return			distance from 'source' to the collision point.
 */
/*virtual*/ float EditorChunkPointLink::collide(
    const Vector3& source, const Vector3& dir, WorldTriangle& wt ) const
{
	BW_GUARD;

	// return maximum distance to avoid collision.
	return std::numeric_limits<float>::max();
}

BW_END_NAMESPACE

