#include "pch.hpp"

#include "moo/geometrics.hpp"
#include "math/colour.hpp"

#include "graph_gui_component.hpp"
#include "simple_gui.hpp"

#include "cstdmf/debug.hpp"


DECLARE_DEBUG_COMPONENT2( "2DComponents", 0 )


BW_BEGIN_NAMESPACE

namespace {
	void addLine( CustomMesh< Moo::VertexXYZDUV >& mesh,
					const Vector3& p1, const Vector3& p2,
					uint32 colour )
	{
		Moo::VertexXYZDUV v1;
		Moo::VertexXYZDUV v2;

		v1.pos_ = p1;
		v1.colour_ = colour;

		v2.pos_ = p2;
		v2.colour_ = colour;

		mesh.push_back( v1 );
		mesh.push_back( v2 );
	}
} // anonymous namespace


PY_TYPEOBJECT( GraphGUIComponent )

PY_BEGIN_METHODS( GraphGUIComponent )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( GraphGUIComponent )

	/*~	attribute GraphGUIComponent.input
	 *	@components{ client, tools }
	 *	@type Vector4Provider
	 *
	 *	This stores the information to be displayed on the screen.
	 *	The Vector4Provider is represented as 4 line graphs, one for
	 *	each of the x,y,z,w components.
	 *	 
	 *	Example:
	 *	@{	 
	 *	import GUI
	 *	import Math
	 *	g = GUI.Graph( "" )
	 *	GUI.addRoot(g)
	 *	g.input = Math.Vector4Product()
	 *	g.input.a = Math.Vector4LFO()
	 *	g.input.b = ( 1.0, 0.8, 0.4, 0.2 )
	 *	g.frequency = 0.015
	 *	g.nPoints = 100
	 *	g.size = (1.5, 1.5)
	 *	@}
	 */
	PY_ATTRIBUTE( input )

	/*~	attribute GraphGUIComponent.nPoints
	 *	@components{ client, tools }
	 *	unsigned int
	 *
	 *	This stores the number of points that will be displayed in the graph.
	 *	Defaults to 25.
	 */
	PY_ATTRIBUTE( nPoints )

	/*~	attribute GraphGUIComponent.frequency
	 *	@components{ client, tools }
	 *	@type float
	 *
	 *	This specifies the rate at which the graph will be updated.
	 *	If frequency is less than 0, the update rate will be in frames (ticks).
	 *	If it is greater than 0, it will be in seconds.
	 *	Defaults to 0.033
	 */
	PY_ATTRIBUTE( frequency )

	/*~	attribute GraphGUIComponent.lineColours
	 *	@components{ client, tools }
	 *	@type list of Vector4
	 *
	 *	This list configures the colours used to display each
	 *	line in the graph (maximum of four lines).
	 */
	PY_ATTRIBUTE( lineColours )

	/*~	attribute GraphGUIComponent.minY
	 *	@components{ client, tools }
	 *	@type float
	 *
	 *	Specifies the minimum height that the graph will be displayed on the screen.
	 *	Value should be between maxY and 0.0 (bottom of screen).
	 *	Defaults to 0.0
	 */
	PY_ATTRIBUTE( minY )

	/*~	attribute GraphGUIComponent.maxY
	 *	@components{ client, tools }
	 *	@type float
	 *
	 *	Specifies the maximum height that the graph will be displayed on the screen.
	 *	Value should be between minY and 1.0 (top of screen).
	 *	Defaults to 1.0
	 */
	PY_ATTRIBUTE( maxY )

	/*~	attribute GraphGUIComponent.showGrid
	 *	@components{ client, tools }
	 *	@type float
	 *
	 *	Toggles the display of a background grid. Defaults to False.
	 *
	 *	@see gridSpacing
	 *	@see gridColour
	 */
	PY_ATTRIBUTE( showGrid )

	/*~	attribute GraphGUIComponent.gridSpacing
	 *	@components{ client, tools }
	 *	@type float
	 *
	 *	Specifies the grid spacing for the background grid, in pixels. 
	 *	Defaults to 10.
	 *
	 *	@see showGrid
	 *	@see gridColour
	 */
	PY_ATTRIBUTE( gridSpacing )

	/*~	attribute GraphGUIComponent.gridColour
	 *	@components{ client, tools }
	 *	@type float
	 *
	 *	Specifies the colour to use when drawing the background grid.
	 *	Defaults to (255, 255, 255, 255).
	 *
	 *	@see showGrid
	 *	@see gridSpacing
	 */
	PY_ATTRIBUTE( gridColour )

PY_END_ATTRIBUTES()

COMPONENT_FACTORY( GraphGUIComponent )

/*~ function GUI.Graph
 *	@components{ client, tools }
 *
 *	Creates and returns a new GraphGUIComponent, which is used to graph a Vector4Provider on the screen.
 *
 *	@return	The new GraphGUIComponent.
 */
PY_FACTORY_NAMED( GraphGUIComponent, "Graph", GUI )



GraphGUIComponent::GraphGUIComponent( PyTypeObject * pType ) :
	SimpleGUIComponent( "", pType ),
	input_( NULL ),
	yValues_( NULL ),
	nPoints_( 25 ),
	minY_( 0.f ),
	maxY_( 1.f ),
	frequency_( 0.033f ),
	accumTime_( 0.f ),
	dTime_(),
	head_( 0 ),
	lineColoursHolder_( lineColours_, this, true ),
	showGrid_( false ),
	gridSpacing_( 10 ),
	gridColour_( 255, 255, 255, 255)
{	
	BW_GUARD;
	materialFX( FX_ADD );
	memset( mesh_, 0, sizeof( CustomMesh<Moo::VertexXYZDUV>* ) * 4 );

	lineColours_.push_back( Vector4(255,0,0,255) );
	lineColours_.push_back( Vector4(0,255,0,255) );
	lineColours_.push_back( Vector4(0,0,255,255) );
	lineColours_.push_back( Vector4(255,255,255,255) );

	//force creation of buffers
	this->nPoints( nPoints_ );
}


GraphGUIComponent::~GraphGUIComponent()
{
	BW_GUARD;
	bw_safe_delete_array(yValues_);
	for (size_t i=0; i < 4; i++)
	{
		bw_safe_delete(mesh_[i]);
	}
}


void GraphGUIComponent::nPoints( int n )
{
	BW_GUARD;
	nPoints_ = max(n,1);
	head_ = 0;

	bw_safe_delete_array(yValues_);
	yValues_ = new Vector4[nPoints_];	

	for (size_t i=0; i < nPoints_; i++)
	{
		yValues_[i].setZero();
	}

	for (size_t i=0; i < 4; i++)
	{
		if (mesh_[i])
		{
			bw_safe_delete(mesh_[i]);
		}
		mesh_[i] = new CustomMesh< Moo::VertexXYZDUV >( D3DPT_LINESTRIP );
		mesh_[i]->resize( nPoints_ );
	}	
}


/**
 *	Static python factory method
 */
PyObject * GraphGUIComponent::pyNew( PyObject * args )
{
	BW_GUARD;
	return new GraphGUIComponent;
}


/**
 *	This method draws the graph gui component.
 */
void GraphGUIComponent::update( float dTime, float relParentWidth, float relParentHeight )
{
	BW_GUARD;
	dTime_ = dTime;
	SimpleGUIComponent::update( dTime, relParentWidth, relParentHeight );
}


/**
 *	This method draws the graph gui component.
 */
void GraphGUIComponent::draw( Moo::DrawContext& drawContext, bool reallyDraw, bool overlay )
{
	BW_GUARD;
	float relativeParentWidth, relativeParentHeight;
	nearestRelativeDimensions( relativeParentWidth, relativeParentHeight );

	float clipX, clipY, clipWidth, clipHeight;
	this->layout( relativeParentWidth, relativeParentHeight,
					clipX, clipY, clipWidth, clipHeight );

	// Make size rectangle non-inclusive (i.e. the way D3D works).
	clipWidth -= SimpleGUI::instance().pixelToClip().x;
	clipHeight -= SimpleGUI::instance().pixelToClip().y;

	Moo::VertexXYZDUV v;

	bool takeASample = true;

	CustomMesh< Moo::VertexXYZDUV > gridMesh( D3DPT_LINELIST );

	if ( input_ )
	{
		//grab one more sample
		if ( frequency_ > 0.f )
		{
			//frequency > 0 - means real time, granular updates.
			accumTime_ += dTime_;
			if ( accumTime_ > frequency_ )
			{
				accumTime_ -= frequency_;
			}
			else
			{
				takeASample = false;
			}
		}
		else if ( frequency_ < 0.f )
		{
			//frequency < 0 - this means per-frame updates of exactly the right amount of time.
		}
		//else frequency == 0 ( we do not animate the vector4 provider )

		if ( takeASample )
		{
			Vector4 out;
			input_->output( out );
			for (size_t idx = 0; idx < 4; idx++)
			{				
				yValues_[head_][idx] = out[idx];
			}
			head_  = (head_+1)%nPoints_;
		}


		//draw the graph into the mesh buffer.
        float yRange = (maxY_ - minY_);
		float dx = ( clipWidth / ((float)nPoints_ - 1) );
		float yScale = ( clipHeight / yRange );
		float yMin = clipY - clipHeight;
		int idx = head_-1;
		if ( idx < 0 )
		{
			idx = nPoints_-1;
		}

		for ( uint32 c = 0; c < 4; c++ )
		{
			float x = clipX + clipWidth;
			CustomMesh< Moo::VertexXYZDUV >& mesh = *mesh_[c];
			for ( uint32 i = 0; i < nPoints_; i++  )
			{
				const Vector4 colour = 
					lineColours_.empty() ? Vector4(255,255,255,255) : 
								lineColours_[ c % lineColours_.size() ];

				v.uv_.set( 1.f-((float)i / (float)nPoints_), 1.f - (yValues_[idx][c] - minY_) / yRange );
				v.pos_.set( x, (yValues_[idx][c] - minY_) * yScale + yMin, position_.z );
				v.colour_ = Colour::getUint32( colour );
				mesh[i] = v;

				x -= dx;
				idx = (idx-1);
				if ( idx < 0 )
				{
					idx = nPoints_-1;
				}
			}
		}
	}

	if ( visible() )
	{
		if (showGrid_)
		{
			const uint32 gridColour = Colour::getUint32( gridColour_ );
			const Vector2 pixelSize = SimpleGUI::instance().pixelToClip() * (float)gridSpacing_;

			int numHorz = (int)floorf(clipHeight / pixelSize.y);
			int numVert = (int)floorf(clipWidth / pixelSize.x);

			float yoffset = clipY - pixelSize.y;
			for (int i = 0; i < numHorz; i++)
			{
				addLine( gridMesh, 
						Vector3(clipX, yoffset, position_.z), 
						Vector3(clipX+clipWidth, yoffset, position_.z), 
						gridColour );

				yoffset -= pixelSize.y;
			}

			float xoffset = clipX + pixelSize.x;
			for (int i = 0; i < numVert; i++)
			{
				addLine( gridMesh, 
					Vector3(xoffset, clipY, position_.z), 
					Vector3(xoffset, clipY-clipHeight, position_.z), 
					gridColour );

				xoffset += pixelSize.x;
			}
		}

		Moo::rc().push();
		Moo::rc().preMultiply( runTimeTransform() );
		Moo::rc().device()->SetTransform( D3DTS_WORLD, &Moo::rc().world() );

		if (reallyDraw)
		{
			if ((nPoints_ && mesh_[0]->size()) || gridMesh.size())
			{
				SimpleGUI::instance().setConstants(runTimeColour(), pixelSnap());
				material_->begin();
				for ( uint32 i=0; i<material_->numPasses(); i++ )
				{
					material_->beginPass(i);
					Moo::rc().setTexture( 0, texture_ ? texture_->pTexture() : NULL );
					Moo::rc().setSamplerState( 0, D3DSAMP_BORDERCOLOR, 0x00000000 );
					if ( !overlay )
					{
						Moo::rc().setRenderState( D3DRS_ZENABLE, TRUE );
						Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
						Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESS );
					}

					if (gridMesh.size())
					{
						gridMesh.draw();
					}

					if (nPoints_ && mesh_[0]->size())
					{
						for ( uint32 idx=0; idx<4; idx++ )
						{
							mesh_[idx]->draw();
						}
					}

					material_->endPass();
				}
				material_->end();

				Moo::rc().setVertexShader( NULL );
				Moo::rc().setFVF( GUIVertex::fvf() );
			}
		}

		SimpleGUIComponent::drawChildren( drawContext, reallyDraw, overlay );

		Moo::rc().pop();
		Moo::rc().device()->SetTransform( D3DTS_WORLD, &Moo::rc().world() );
	}

	momentarilyInvisible( false );
}


bool GraphGUIComponent::load( DataSectionPtr pSect, const BW::string& ownerName, LoadBindings & bindings )
{
	BW_GUARD;
	if (!this->SimpleGUIComponent::load( pSect, ownerName, bindings )) return false;

	this->nPoints( pSect->readInt( "nPoints", nPoints_ ) );
	this->minY_ = pSect->readFloat( "minY", minY_ );
	this->maxY_ = pSect->readFloat( "maxY", maxY_ );
	this->frequency_ = pSect->readFloat( "frequency", frequency_ );
	this->showGrid_ = pSect->readBool( "showGrid", showGrid_ );
	this->gridSpacing_ = pSect->readInt( "gridSpacing", gridSpacing_ );
	this->gridColour_ = pSect->readVector4( "gridColour", gridColour_ );

	lineColours_.clear();
	pSect->readVector4s( "lineColours/colour", lineColours_ );

	return true;
}


void GraphGUIComponent::save( DataSectionPtr pSect, SaveBindings & bindings )
{
	BW_GUARD;
	pSect->writeInt( "nPoints", nPoints_ );
	pSect->writeFloat( "minY", minY_ );
	pSect->writeFloat( "maxY", maxY_ );
	pSect->writeFloat( "frequency", frequency_ );
	pSect->writeBool( "showGrid", showGrid_ );
	pSect->writeInt( "gridSpacing", gridSpacing_ );
	pSect->writeVector4( "gridColour", gridColour_ );

	pSect->writeVector4s( "lineColours/colour", lineColours_ );

	this->SimpleGUIComponent::save( pSect, bindings );
}

BW_END_NAMESPACE

// graph_gui_component.cpp
