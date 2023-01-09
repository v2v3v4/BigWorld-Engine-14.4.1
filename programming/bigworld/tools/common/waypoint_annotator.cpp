#include "pch.hpp"

#include "waypoint_annotator.hpp"

#include "math/lineeq.hpp"
#include "moo/render_context.hpp"
#include "moo/render_target.hpp"
#include "moo/texture_exposer.hpp"
#include "waypoint_generator/waypoint_view.hpp"
#include "physics2/worldtri.hpp"
#include "chunk/chunk_space.hpp"
#include "romp/font_manager.hpp"
#include "physics2/collision_callback.hpp"
#include "physics2/collision_obstacle.hpp"

DECLARE_DEBUG_COMPONENT2( "WPGen", 0 )


BW_BEGIN_NAMESPACE

namespace
{
	bool processMessages( DWORD interval )
	{
		BW_GUARD;

		HANDLE h = GetCurrentThread();
		MSG msg;
		switch (MsgWaitForMultipleObjects( 1, &h, 0, interval, QS_ALLEVENTS ))
		{
		case WAIT_OBJECT_0:
		case WAIT_TIMEOUT:
			return false;
		default:
			while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
			{
				if (msg.message == WM_QUIT)
				{
					break;
				}
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
		}
		return true;
	}
} // anonymous

// -----------------------------------------------------------------------------
// Section: WaypointAnnotator
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
WaypointAnnotator::WaypointAnnotator( IWaypointView * pView,
		ChunkSpace * pSpace ) :
	pView_( pView ),
	pSpace_( pSpace )
{
	BW_GUARD;

	// temporary surface used by annotate function. taken out of main loop for 
	// speed and because we were running out of video memory ( is the problem
	// anything to do with this ??? ).
	Moo::rc().device()->CreateOffscreenPlainSurface( 64, 64, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &pSurface_, NULL );
	if ( !pSurface_.hasComObject() )
	{
		ERROR_MSG( "WaypointAnnotator: Could not create image surface.\n" );
	}

	// make and push render target 64 by 64 pixels
	pTarget_ = new Moo::RenderTarget( "WPAnnotator" );
	if ( !pTarget_->create( 64, 64 ) )
	{
		ERROR_MSG( "WaypointAnnotator: Could not create render target.\n" );
	}
	else 
	{
		if ( !pTarget_->push() )
		{
			ERROR_MSG( "WaypointAnnotator: Could not push render target.\n" );
		}
		else
		{
			((DX::Texture&)(*pTarget_->pTexture())).
				GetSurfaceLevel( 0, &pRTSurface_ );
			if ( !pRTSurface_.hasComObject() )
			{
				ERROR_MSG( "WaypointAnnotator: Could not access render target.\n" );
			}
		}
	}
}


WaypointAnnotator::~WaypointAnnotator()
{
	BW_GUARD;

	// final pop of render target
	pTarget_->pop();
}


/**
 *	This function does the actual work of annotating the graph
 */
void WaypointAnnotator::annotate( bool slowly, float girth )
{
	BW_GUARD;

	if ( !pSurface_.hasComObject() || !pTarget_ || !pRTSurface_.hasComObject() )
	{
		ERROR_MSG( "WaypointAnnotator::annotate: Not annotating - "
			"not properly initialised.\n" );
		return;
	}

	int ccount = 0, ecount = 0, acount = 0;

	int npolys = pView_->getPolygonCount();

	for ( int p = 0; p < npolys; ++p )
	{
		if ( p%100 == 0 )
		{
			INFO_MSG( "annotated %d/%d polys...\n", p, npolys );
		}

		if ( girth > 0.f && !pView_->equivGirth( p, girth ) )
		{
			continue;
		}

		IWaypointView::PolygonRef poly( *pView_, p );

		for ( uint v = 0; v < poly.size(); ++v )
		{
			if ( poly[v].adjNavPoly() != 0 || poly[v].adjToAnotherChunk() )
			{
				continue;
			}

			int res = this->annotate( p, v, slowly );
			if ( res >= 1 )
			{
				++ecount;

				if (res == 2)
				{
					++acount;
				}
			}
			++ccount;
		}
	}

	INFO_MSG( "Considered %d edges, examined %d, and annotated %d of them\n",
		ccount, ecount, acount );
}


#include "resmgr/xml_section.hpp"
#include "romp/font.hpp"


/**
 *	Debug method to draw this target
 */
static int drawTarget( Moo::RenderTargetPtr pTarget, int p, int v, int a, uint32 flags )
{
	BW_GUARD;

	static Moo::Material targetMat;
	static FontPtr xfont = FontManager::instance().get( "default_small.font" );
	if ( !xfont )
	{
		ERROR_MSG( "Could not create font 'default_small.font'.\n" );
	}

	static bool targetMatInited = false;
	if (!targetMatInited)
	{
		targetMatInited = true;

		DataSectionPtr pDS = new XMLSection("root");
		pDS->writeString( "identifier", "render target" );
		pDS->writeString( "texture", "maps/system/notfound.bmp" );
		targetMat.load( pDS );
		targetMat.doubleSided( true );
		targetMat.fogged( false );

		//xfont->setFontScale( FONT_SCALE_FIXED );
		//TODO: find out how to do this with new fonts
	}
	targetMat.textureStage(0).pTexture( pTarget );

	const int axmap[3] = { 1, 0, 2 };
	const float ax = float(axmap[a>>1]);
	const float ay = float(1 - (a&1));

	pTarget->pop();

	Moo::rc().beginScene();

	if ( a == 0 )
	{
		Moo::rc().device()->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
			0x000000aa, 1, 0 );

		char desc[256];
		bw_snprintf( desc, sizeof(desc), "Waypoint %d, vertex %d:", p+1, v );

		if ( xfont && FontManager::instance().begin( *xfont ) )
		{
			xfont->colour( 0xffffff00 );			
			xfont->drawConsoleString( BW::string(desc), 0, 0 );

			xfont->drawConsoleString(
				"Press space to continue. When doing so, hold down:", 0, 26 );
			xfont->drawConsoleString(
				"Escape to not wait for space", 1, 27 );
			xfont->drawConsoleString(
				"0-5 to save the collision scene info for that area", 1, 28 );
			FontManager::instance().end();
		}
	}

	targetMat.set();
	Moo::rc().setVertexShader( NULL );
	Moo::rc().setFVF( Moo::VertexTLUV::fvf() );

	Moo::VertexTLUV quad[4];
	const float xK = 220;
	const float yK = 240;
	const float xOff = 40;
	quad[0].pos_.set( xOff + ax*xK, 45 + ay*yK, 0, 1 );
	quad[0].colour_ = 0x7f7f7f7f;
	quad[0].uv_.set( 0.f + 0.5f/64.f, 0.f + 0.5f/64.f );
	quad[1].pos_.set( xOff + 128 + ax*xK, 45 + ay*yK, 0, 1 );
	quad[1].colour_ = 0x7f7f7f7f;
	quad[1].uv_.set( 1.f + 0.5f/64.f, 0.f + 0.5f/64.f );
	quad[2].pos_.set( xOff + 0 + ax*xK, 173 + ay*yK, 0, 1 );
	quad[2].colour_ = 0x7f7f7f7f;
	quad[2].uv_.set( 0.f + 0.5f/64.f, 1.f + 0.5f/64.f );
	quad[3].pos_.set( xOff + 128 + ax*xK, 173 + ay*yK, 0, 1 );
	quad[3].colour_ = 0x7f7f7f7f;
	quad[3].uv_.set( 1.f + 0.5f/64.f, 1.f + 0.5f/64.f );

	Moo::rc().device()->DrawPrimitiveUP(
		D3DPT_TRIANGLESTRIP, 2, &quad[0], sizeof(quad[0]) );

	if ( xfont && FontManager::instance().begin(*xfont) )
	{
		xfont->colour( 0xffffffff );
		char * ahead[4] = { "Solid", "Mixed", "Reserved", "Clear" };
		char * ourAhead = ahead[flags];
		size_t col = 6 + int(ax)*13 - strlen(ourAhead)/2;
		MF_ASSERT( col <= INT_MAX );
		size_t row = 12 + int(ay)*11;
		MF_ASSERT( row <= INT_MAX );
		xfont->drawConsoleString( BW::string(ourAhead), ( int ) col, ( int ) row );
		FontManager::instance().end();
	}

	Moo::rc().endScene();

	int ret = -1;
	if ( a == 5 )
	{
		Moo::rc().device()->Present( NULL, NULL, NULL, NULL );

		if ((GetAsyncKeyState( VK_ESCAPE ) & 0x8000) == 0)
		{
			while ((GetAsyncKeyState( VK_SPACE ) & 0x8000) == 0 &&
				(GetAsyncKeyState( VK_ESCAPE ) & 0x8000) == 0)
			{
				processMessages( 50 );
			}
			while ((GetAsyncKeyState( VK_SPACE ) & 0x8000) != 0 &&
				(GetAsyncKeyState( VK_ESCAPE ) & 0x8000) == 0) 
			{
				processMessages( 50 );
			}
			for ( int a = 0; a < 6; ++a )
			{
				if ( (GetAsyncKeyState( '0'+a ) & 0x8000) != 0 )
				{
					ret = a;
					break;
				}
			}
		}
	}

	pTarget->push();
	return ret;
}


/**
 *	Helper class to accumulate triangles from the collision scene
 */
class TriangleAccumulator : public CollisionCallback
{
public:
	TriangleAccumulator()
	{
		BW_GUARD;

		tris.clear();
	}

	virtual int operator()( const CollisionObstacle & obstacle,
		const WorldTriangle & tri, float dist )
	{
		BW_GUARD;

		if ( !(tri.flags() & TRIANGLE_NOCOLLIDE) )
		{
			tris.push_back( obstacle.transform().applyPoint( tri.v0() ) );
			tris.push_back( obstacle.transform().applyPoint( tri.v1() ) );
			tris.push_back( obstacle.transform().applyPoint( tri.v2() ) );
		}

		return COLLIDE_ALL;
	}

	static BW::vector< Vector3 > tris;
};
BW::vector< Vector3 > TriangleAccumulator::tris;


/**
 *	This is a hack function to unproject the y coordinate of the
 *	input vertices.
 */
void unproject( const Matrix & view, uint num,
	void * array, uint stride )
{
	BW_GUARD;

	float yoff = view.applyToOrigin()[1];
	Vector4 tv;

	for ( uint i = 0; i < num; ++i )
	{
		Vector3 & v = *(Vector3*)(((char*)array) + (stride * i));
		view.applyPoint( tv, Vector4( v, 1 ) );
//		dprintf( "v (%f,%f,%f,1), tv (%f,%f,%f,%f), yoff %f, v + yoff %f\n",
//			v.x, v.y, v.z, tv.x, tv.y, tv.z, tv.w, yoff, v.y + yoff );
		v.y = tv.z * (v.y+yoff) - yoff;
	}
}


/**
 *	Helper function to do a drop test from a polygon height
 */
float vertexHeight( ChunkSpace * pSpace, const Vector2 & pt, float top )
{
	BW_GUARD;

	top += 0.5f;
	float drop = pSpace->collide(
		Vector3( pt.x, top, pt.y ),
		Vector3( pt.x, top-100.5f, pt.y ),
		ClosestObstacle::s_default );
	return (drop >= 0.f) ? top - drop : top;
}

/**
 *	This method annotates the given edge
 *  @param p polygon number
 *  @param v vertex number
 */
int WaypointAnnotator::annotate( int p, int v, bool slowly )
{
	BW_GUARD;

	IWaypointView::PolygonRef poly( *pView_, p );
	IWaypointView::VertexRef edgeBeg = poly[v];
	IWaypointView::VertexRef edgeEnd = poly[(v+1) % poly.size()];
	Vector2 p1 = edgeBeg.pos();
	Vector2 p2 = edgeEnd.pos();
	Vector2 delta = p2 - p1;
	Vector2 dir = delta;
	dir.normalise();
	LineEq edgeLine( p1, p2 );
	Vector2 normal = edgeLine.normal();

	//dprintf( "Edge is (%f,%f) to (%f,%f), normal is (%f,%f)\n",
	//	p1[0], p1[1], p2[0], p2[1], normal[0], normal[1] );

	// find all the waypoints that we could be interested in
	// that is, those that are in front of us within the sweep
	// between lines at 45 degree angles from our corners.
	// i.e. \                /
	//       \    sweep     /
	//        2<-----------1
	//        |  waypoint  |
	//        +------------+
	LineEq p1Sweep(     p1 - dir + normal, p1 );
	LineEq p2Sweep( p2, p2 + dir + normal );
	BW::vector<int> shore;
	for ( int sp = 0; sp < pView_->getPolygonCount(); ++sp )
	{
		if ( sp == p )
		{ 
			continue;	// we should never be selected
		}

		IWaypointView::PolygonRef spoly( *pView_, sp );
		for ( uint sv = 0; sv < spoly.size(); ++sv )
		{
			// waypoints are defined anticlockwise, so points inside the
			// waypoint must actually test 'false' to all the
			// LineEq.isInFrontOf tests from its edges.
			// similarly, the normals of the LineEq point out not in.

			// see if this vertex is on the correct side of the edge line
			Vector2 asvp = spoly[sv].pos();
			if ( !edgeLine.isInFrontOf( asvp ) ) 
			{
				continue;
			}

			// see if this vertex is inside the swept area
			bool ainp1s = p1Sweep.isInFrontOf( asvp );
			bool ainp2s = p2Sweep.isInFrontOf( asvp );
			if ( ainp1s && ainp2s )
			{
				shore.push_back( sp );
				break;
			}

			// see if the next vertex crosses either line then,
			// and if so whether the intersection is inside the edge
			Vector2 bsvp = spoly[(sv+1)%spoly.size()].pos();
			LineEq sedge( asvp, bsvp );
			if ( p1Sweep.isInFrontOf( bsvp ) != ainp1s )
			{
				if ( p1Sweep.intersect( sedge ) < 1.414f )
				{			// 1.414 is the dist to the end pt, i.e. p1
					shore.push_back( sp );
					break;
				}
			}
			if ( p2Sweep.isInFrontOf( bsvp ) != ainp2s )
			{
				if ( p2Sweep.intersect( sedge ) > 0.f )
				{			// 0 is the start of the line, i.e. p2
					shore.push_back( sp );
					break;
				}
			}
		}
	}

	// if there were no interesting waypoints then we must be against
	// a wall on the edge of the chunk, so give up now
	if ( shore.empty() )
	{
		return 0;
	}

	// find the real height along this edge, make sure it is reasonably flat
	float pheight = poly.maxHeight();
	float eheight = pheight;

	const float dropCheckRes = 0.1f;
	const float dropTolerance = 0.3f;

	float dist = delta.length();
	for ( float d = 0; d <= dist; d += dropCheckRes )
	{
		// could do a prism test but I am too lazy to figure it out
		Vector2 dropPt = p1 + delta * d / dist;
		float newHeight = vertexHeight( pSpace_, dropPt, pheight );

		// make sure we dropped at least our added height distance
		if ( newHeight > pheight+0.1f )
		{
			return 0;
		}

		// make sure the height is close enough
		if ( d == 0.f )
		{
			eheight = newHeight;
			continue;
		}
		if ( fabsf( newHeight - eheight ) > dropTolerance ) 
		{
			return 0;
		}

		// average it in (this does work like an ordinary average, trust me :)
		eheight = ( eheight * d + newHeight * dropCheckRes ) / ( d + dropCheckRes );
	}


	// ********


	const float maxSearchDist = 30.f;	// the biggest waypoint gap to work for

	Moo::Material flatWhite;
	flatWhite.doubleSided( true );
	flatWhite.fogged( false );
	flatWhite.textureFactor( 0xFFFFFFFF );
	Moo::TextureStage ts;
	ts.colourOperation(
		Moo::TextureStage::SELECTARG1,
		Moo::TextureStage::TEXTURE_FACTOR,
		Moo::TextureStage::TEXTURE_FACTOR );
	ts.alphaOperation(
		Moo::TextureStage::SELECTARG1,
		Moo::TextureStage::TEXTURE_FACTOR,
		Moo::TextureStage::TEXTURE_FACTOR );
	flatWhite.addTextureStage( ts );
	flatWhite.addTextureStage( Moo::TextureStage() );	// disabler

	BW::vector<Vector3>	colTriTemp[6];

	// for each of test areas
	uint32 adjFlags = 0;
	int a;
	for ( a = 0; a < 6; ++a )
	{
		// start a-rendering
		Moo::rc().beginScene();
		Moo::rc().device()->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
			0x00000000, 1, 0 );
		Moo::rc().nextFrame();

		flatWhite.set();
		Moo::rc().setVertexShader( NULL );
		Moo::rc().device()->SetPixelShader( NULL );
		Moo::rc().setFVF( Moo::VertexXYZ::fvf() );

		// calculate the view frustrum
		Vector3 frustrum[4];	// frontleft, frontright, backleft, backright
		if ( (a>>1) == 0 )		// front search
		{
			frustrum[0] = Vector3( p2.x, eheight, p2.y );
			frustrum[1] = Vector3( p1.x, eheight, p1.y );
			Vector2 p2out = p2 + normal * maxSearchDist;
			Vector2 p1out = p1 + normal * maxSearchDist;
			frustrum[2] = Vector3( p2out.x, eheight, p2out.y );
			frustrum[3] = Vector3( p1out.x, eheight, p1out.y );
		}
		else if ( (a>>1) == 1 )	// left search
		{
			Vector2 pmid = (p1+p2) * 0.5f;
			frustrum[0] = Vector3( p2.x, eheight, p2.y );
			frustrum[1] = Vector3( pmid.x, eheight, pmid.y );
			Vector2 p2sweep = p2 + (normal+dir) * maxSearchDist;
			Vector2 p2out = p2 + normal * maxSearchDist;
			frustrum[2] = Vector3( p2sweep.x, eheight, p2sweep.y );
			frustrum[3] = Vector3( p2out.x, eheight, p2out.y );
		}
		else					// right search
		{
			Vector2 pmid = (p1+p2) * 0.5f;
			frustrum[0] = Vector3( pmid.x, eheight, pmid.y );
			frustrum[1] = Vector3( p1.x, eheight, p1.y );
			Vector2 p1out = p1 + normal * maxSearchDist;
			Vector2 p1sweep = p1 + (normal-dir) * maxSearchDist;
			frustrum[2] = Vector3( p1out.x, eheight, p1out.y );
			frustrum[3] = Vector3( p1sweep.x, eheight, p1sweep.y );
		}

		//dprintf( "Frustrum is at (%f,%f)-(%f,%f) (%f,%f)-(%f,%f)\n",
		//	frustrum[0][0], frustrum[0][2],
		//	frustrum[1][0], frustrum[1][2],
		//	frustrum[2][0], frustrum[2][2],
		//	frustrum[3][0], frustrum[3][2] );

		// and the heights of this slice
		float blevel = eheight;
		float tlevel = eheight;
		if ( (a&1) == 0 )	// bottom slice
		{
			blevel += 0.f;
			tlevel += 1.25f;
		}
		else			// top slice
		{
			blevel += 1.5f;
			tlevel += 2.5f;
		}

		// set the camera, view and projection matrices up
		//  to effect this frustrum
		Matrix mworld = Matrix::identity;
		Matrix mview = Matrix::identity;
		Matrix mproj = Matrix::identity;
		float nearPlane = 0.f, farPlane = 0.f;
		Moo::rc().device()->SetTransform( D3DTS_WORLD, &mworld );

		if ( (a>>1) == 0 )
		{
			Vector3 frRight( frustrum[1] - frustrum[0] );
			Vector3 frForward( frustrum[2] - frustrum[0] );
			mview.lookAt( Vector3((frustrum[0].x+frustrum[1].x)*0.5f,
				(blevel+tlevel)*0.5f, (frustrum[0].z+frustrum[1].z)*0.5f ) -
				frForward*1.f/frForward.length(), frForward,
				Vector3( 0.f, 1.f, 0.f ) );
			Moo::rc().device()->SetTransform( D3DTS_VIEW, &mview );

			mproj.orthogonalProjection(
				frRight.length(),
				tlevel - blevel,
				1.f,
				1.f + frForward.length() );
			Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &mproj );
		}
		else
		{
			Vector3 frRightF( frustrum[1] - frustrum[0] );
			Vector3 frRightB( frustrum[3] - frustrum[2] );
			Vector3 frForwardDir( frRightF.z, 0.f, -frRightF.x );
			frForwardDir.normalise();
			float frForwardLen = frForwardDir.dotProduct(
				Vector3( frustrum[2] - frustrum[0] ) );

			LineEq frToBackLLine(
				Vector2(frustrum[0].x, frustrum[0].z),
				Vector2(frustrum[2].x, frustrum[2].z) );
			LineEq frToBackRLine(
				Vector2(frustrum[1].x, frustrum[1].z),
				Vector2(frustrum[3].x, frustrum[3].z) );
			Vector2 viewPt2 = frToBackLLine.param(
				frToBackLLine.intersect( frToBackRLine ) );
			Vector3 viewPt3( viewPt2.x, (blevel+tlevel)*0.5f, viewPt2.y );
			mview.lookAt( viewPt3, frForwardDir, Vector3( 0.f, 1.f, 0.f ) );

			nearPlane = -frForwardDir.dotProduct( viewPt3 - frustrum[0] );
			farPlane = nearPlane + frForwardLen;

			// now do the shear
			Matrix shearer = Matrix::identity;
			Vector3 frRightDir = frRightF; frRightDir.normalise();
			float backMidOffset = frRightDir.dotProduct(
				(frustrum[2]+frustrum[3])*0.5f - frustrum[0] );
			shearer[0][2] = -backMidOffset / frForwardLen;
			shearer[0][3] = nearPlane * -shearer[0][2];
			mview.postMultiply( shearer );

			//dprintf( "view is (%f,%f,%f,%f)\n", mview[0][0], mview[0][1], mview[0][2], mview[0][3] );
			//dprintf( "        (%f,%f,%f,%f)\n", mview[1][0], mview[1][1], mview[1][2], mview[1][3] );
			//dprintf( "        (%f,%f,%f,%f)\n", mview[2][0], mview[2][1], mview[2][2], mview[2][3] );
			//dprintf( "        (%f,%f,%f,%f)\n", mview[3][0], mview[3][1], mview[3][2], mview[3][3] );

			// We cannot make a
			// matrix that does a perspective transform on the x axis but
			// an orthogonal one on the y, because they use the w in
			// different ways. It is very annoying.

			Moo::rc().device()->SetTransform( D3DTS_VIEW, &mview );

			// now use a perspective projection
			D3DXMatrixPerspectiveLH( &mproj,
				frRightF.length(), (tlevel-blevel),
				nearPlane, farPlane );
			Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &mproj );
		}



		static VectorNoDestructor<Moo::VertexXYZ> renver;
		Moo::VertexXYZ quadver[4];

		// first draw all the waypoints we found above with alpha 0x50
		flatWhite.textureFactor( 0x50505050 );
		flatWhite.set();
		renver.clear();
		for ( uint i = 0; i < shore.size(); ++i )
		{
			int sp = shore[i];
			IWaypointView::PolygonRef spoly( *pView_, sp );
			float sheight = spoly.maxHeight();

			Vector2 apt, bpt = spoly[spoly.size()-1].pos();
			float	aht, bht = vertexHeight( pSpace_, bpt, sheight );
			for ( uint sv = 0; sv < spoly.size(); ++sv )
			{
				apt = bpt;
				aht = bht;
				bpt = spoly[sv].pos();
				bht = vertexHeight( pSpace_, bpt, sheight );
				quadver[0].pos_.set( apt.x, aht-0.5f, apt.y );
				quadver[1].pos_.set( apt.x, aht+4.f, apt.y );
				quadver[2].pos_.set( bpt.x, bht-0.5f, bpt.y );
				quadver[3].pos_.set( bpt.x, bht+4.f, bpt.y );
				renver.push_back( quadver[0] );
				renver.push_back( quadver[1] );
				renver.push_back( quadver[2] );
				renver.push_back( quadver[1] );
				renver.push_back( quadver[2] );
				renver.push_back( quadver[3] );
			}
		}
		if ( !renver.empty() )
		{
			size_t num = renver.size();
			MF_ASSERT( num <= UINT_MAX );
			if ((a>>1)!=0) unproject( mview,
				(uint) num, &renver.front(), sizeof( renver.front() ) );
			size_t primitiveCount = renver.size() / 3;
			MF_ASSERT( num <= UINT_MAX );
			Moo::rc().device()->DrawPrimitiveUP( D3DPT_TRIANGLELIST,
				( uint ) primitiveCount, &renver.front(), sizeof( renver.front() ) );
		}

		// find all the appropriate triangles in the collision scene
		TriangleAccumulator triac;
		Vector3 backTriA, backTriB, backTriC;
		WorldTriangle backTri;
		Vector3 frontTriA = frustrum[0]; frontTriA.y = blevel;

		backTriA = frustrum[2];	backTriA.y = blevel;
		backTriB = frustrum[3];	backTriB.y = tlevel;
		backTriC = frustrum[2];	backTriC.y = tlevel;
		backTri = WorldTriangle( backTriA, backTriB, backTriC );
		pSpace_->collide( backTri, frontTriA, triac );

		backTriA = frustrum[2];	backTriA.y = blevel;
		backTriB = frustrum[3];	backTriB.y = tlevel;
		backTriC = frustrum[3];	backTriC.y = blevel;
		backTri = WorldTriangle( backTriA, backTriB, backTriC );
		pSpace_->collide( backTri, frontTriA, triac );

		// draw all those triangles with alpha 0xFF
		if ( !triac.tris.empty() )
		{
			flatWhite.textureFactor( 0xFFFFFFFF );
			flatWhite.set();
			if ( (a>>1)!=0 ) 
			{
				size_t stride = sizeof( triac.tris[0] );
				size_t num = triac.tris.size();
				MF_ASSERT( stride <= UINT_MAX );
				MF_ASSERT( num <= UINT_MAX );
				unproject( mview, ( uint ) num, &triac.tris[0],
					( uint ) stride );
			}
			size_t primitiveCount = triac.tris.size()/3;
			MF_ASSERT( primitiveCount <= UINT_MAX );
			Moo::rc().device()->DrawPrimitiveUP( D3DPT_TRIANGLELIST,
				( uint ) primitiveCount, &triac.tris[0], sizeof( triac.tris[0] ) );
		}

		if ( slowly )
		{
			colTriTemp[a] = triac.tris;
		}

		// stop the rendering
		Moo::rc().endScene();

		// get out the texture
		RECT copyRect = { 0, 0, 64, 64 };
		HRESULT hr = Moo::rc().device()->GetRenderTargetData( &*pRTSurface_, &*pSurface_ );
		if ( hr != D3D_OK )
		{
			ERROR_MSG( "WaypointAnnotator: Could not copy render target. %x\n", hr );
			break;
		}
		D3DLOCKED_RECT lockedRect;
		hr = pSurface_->LockRect( &lockedRect, &copyRect,
			D3DLOCK_NOSYSLOCK | D3DLOCK_READONLY );
		if ( hr != D3D_OK )
		{
			ERROR_MSG( "WaypointAnnotator: Could not lock image surface.\n" );
			break;
		}

		const char * bits = (const char *)lockedRect.pBits;
		int pitch = lockedRect.Pitch;

		// add up the numbers of each kind of pixel
		int nNothing  = 0;
		int nWaypoint = 0;
		int nObstacle = 0;
		const uint8 * pline = (const uint8*)bits;
		for ( int y = 0; y < 64; ++y )
		{
			for ( int x = 0; x < 64*4; x+=4 )
			{
				uint8 palpha = pline[x];
				nNothing  += (palpha == 0x00);
				nWaypoint += (palpha == 0x50);
				nObstacle += (palpha == 0xFF);
			}

			pline += pitch;
		}

		// unlock the surface
		pSurface_->UnlockRect();

		// make sure we liked what we saw
		if ( nNothing + nWaypoint + nObstacle != 64*64 )
		{
			ERROR_MSG( "WaypointAnnotator: "
				"Only recognised %d out of %d pixels in rendered texture\n",
				nNothing + nWaypoint + nObstacle, int(64*64) );
		}

		// and set the adjacency flags appropriately
		float propWaypoint = float(nWaypoint) / (64.f*64.f);
		if ( propWaypoint > 0.85f )
			adjFlags |= 0x3 << (a<<1);			// clear
		else if ( propWaypoint > 0.2f )
			adjFlags |= 0x1 << (a<<1);			// cluttered
		else
			adjFlags |= 0x0 << (a<<1);			// solid

		if ( slowly )
		{
			int keep = drawTarget( pTarget_, p, v, a, adjFlags >> (a<<1) );
			if (keep >= 0) 
			{
				colTriDebug_ = colTriTemp[keep];
			}
		}
	}

	// get out now if we didn't finish that loop for some reason
	if ( a != 6 )
	{
		return 0;
	}

	// and finally, write the flags into the waypoint!
	edgeBeg.adjNavPoly( -int(adjFlags) );
	return adjFlags != 0 ? 2 : 1;

	// phew!
}

BW_END_NAMESPACE

// waypoint_annotator.cpp
