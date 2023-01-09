#include "pch.hpp"
#include "gizmo/tile_gizmo.hpp"
#include "gizmo/tool.hpp"
#include "gizmo/tool_manager.hpp"
#include "gizmo/radius_gizmo.hpp"
#include "moo/line_helper.hpp"
#include "moo/custom_mesh.hpp"
#include "moo/render_context.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	// Colour fo the grid in the x and z directions:
	const D3DCOLOR	GRID_CLR_X				= D3DCOLOR_RGBA( 37, 196, 37, 255);
	const D3DCOLOR	GRID_CLR_Z				= D3DCOLOR_RGBA(255,  37, 37, 255);

	// Default alpha to draw the texture with:
	const uint8		TEXTURE_ALPHA					= 0x80;

	// Maximum number of tiles to draw (if the scale gets very small this can
	// be an issue):
	const uint32	MAX_TILES						= 1000;

	// Size of the grid gizmo, in metres (minimum size, can be bigger if neede)
	const float		MIN_GRID_SIZE					= 1.0f;

	// The distance the projection gizmo has to be from the camera to start
	// scaling the minimum size of the grid so that the projection gizmo is
	// always a fixed minimum size in the viewport.  Distance in metres.
	const float		START_SCALING_GRID_SIZE_DIST	= 5.0f;
}


class UniformScaleProxy : public FloatProxy
{
public:
	UniformScaleProxy( MatrixProxyPtr matrixProxy ) :
		matrixProxy_( matrixProxy ),
		curTiling_( 1.0f ),
		newTransformSet_( true )
	{
		BW_GUARD;

		setToDefault();
	}

	virtual void setToDefault()
	{
		BW_GUARD;

		newTransformSet_ = true;
		Matrix mInv;
		matrixProxy_->getMatrix( mInv );
		mInv.invert();
		mInv.translation( Vector3::zero() );

		float mtxScale =
					(mInv.column( 0 ).length() + mInv.column( 2 ).length()) / 2.0f;
		if (almostZero( mtxScale ))
		{
			mtxScale = 1.0f;
		}

		curTiling_ = 1.0f / mtxScale;
	}

	virtual float get() const
	{
		return curTiling_;
	}

	virtual void set( float tiling, bool transient,
							bool addBarrier = true )
	{
		BW_GUARD;

		if (newTransformSet_)
		{
			matrixProxy_->recordState();
			newTransformSet_= false;
		}

		Matrix m;
		matrixProxy_->getMatrix( m );
		m[0].normalise();
		m[1].normalise();
		m[2].normalise();

		Matrix sm;
		sm.setScale( tiling, tiling, tiling );

		m.preMultiply( sm );

		if (matrixProxy_->setMatrix( m ))
		{
			curTiling_ = tiling;
		}

		if (!transient)
		{
			matrixProxy_->commitState( false, addBarrier );
			newTransformSet_= true;
		}
	}

private:
	MatrixProxyPtr matrixProxy_;
	float curTiling_;
	bool newTransformSet_;
};


/**
 *	This is the TileGizmo constructor.
 *
 *  @param matrixProxy		The matrix that is being edited.
 *  @param enableModifer	The modifier key(s) that enables the TileGizmo.
 *  @param disableModifier	The modifier key(s) that disables the TileGizmo.
 */
TileGizmo::TileGizmo
(
	MatrixProxyPtr			matrixProxy,
	uint32					enableModifier	/*= MODIFIER_SHIFT*/,
	uint32					disableModifier	/*= MODIFIER_CTRL | MODIFIER_ALT*/
):
	matrixProxy_(matrixProxy),
	enableModifier_(enableModifier),
	disableModifier_(disableModifier),
	drawOptions_(DRAW_ALL),
	opacity_(TEXTURE_ALPHA),
	scaledMinGridSize_(0)
{
	BW_GUARD;

	rotationGizmo_ = 
		new RotationGizmo(matrixProxy, enableModifier, disableModifier);
	scaleGizmo_ =
		new ScaleGizmo(matrixProxy, enableModifier, 1.0f, ScaleGizmo::SCALE_UV);

	uniformScaleProxy_ = new UniformScaleProxy( matrixProxy );

	uniformScaleGizmo_ =
		new RadiusGizmo( uniformScaleProxy_,
			matrixProxy,
			"Uniform U,V scale",
			0xffffffff,
			8.f,
			enableModifier,
			0.1f,
			true,
			NULL,
			NULL,
			RadiusGizmo::SHOW_SPHERE_NEVER,
			&SimpleFormatter::s_def );
}


/**
 *	This is the TileGizmo destructor.
 */
TileGizmo::~TileGizmo()
{
}


/**
 *	This is called to draw the TileGizmo.
 *
 *  @param force			Force drawing.
 */
/*virtual*/ bool TileGizmo::draw( Moo::DrawContext& drawContext, bool force )
{
	BW_GUARD;

	if ((drawOptions_ & DRAW_FORCED) != 0)
		force = true;

	if 
	( 
		!force 
		&&
		enableModifier_ != ALWAYS_ENABLED
		&& 
		( 
			(InputDevices::modifiers() & enableModifier_) == 0 
			||
			(InputDevices::modifiers() & disableModifier_) != 0
		)
	)
	{
		return false;
	}

	Matrix m = objectTransform();		

	// Get the distance between the projection gizmo and the camera.
	Vector3 camToGizmo =
		GizmoManager::instance().getLastCameraPosition() - m[3];
	float camToGizmoDist = camToGizmo.length();

	// Calculate the new scaled min grid size
	scaledMinGridSize_ =
		(camToGizmoDist < START_SCALING_GRID_SIZE_DIST) ?
			MIN_GRID_SIZE
		:
			MIN_GRID_SIZE *
				(camToGizmoDist / START_SCALING_GRID_SIZE_DIST);

	
	if ((drawOptions_ & DRAW_TEXTURE) != 0 && texture_ != NULL)
		drawTexture(m);
	if ((drawOptions_ & DRAW_GRID) != 0)
		drawGrid(m);

	bool ok = true;
	if ((drawOptions_ & DRAW_ROTATION) != 0)
		ok &= rotationGizmo_->draw( drawContext, force );
	if ((drawOptions_ & DRAW_SCALE) != 0)
	{
		ok &=
			scaleGizmo_->draw( drawContext, force ) &&
			uniformScaleGizmo_->draw( drawContext, force );
	}

	return ok;
}


/**
 *	This is called to test whether the Gizmo has been hit by a line-segment.
 *
 *  @param origin			The origin of the line-segment.
 *  @param direction		The direction of the line-segment.
 *  @param t				If there is a hit then this is set to the t-value
 *							of intersection.
 *  @param force			Force drawing.
 *  @returns				True if hit, false otherwise.
 */
/*virtual*/ bool 
TileGizmo::intersects( Vector3 const &origin, Vector3 const &direction,
														float &t, bool force )
{
	BW_GUARD;

	bool intersect = false;
	if ((drawOptions_ & DRAW_ROTATION) != 0)
	{
		intersect = rotationGizmo_->intersects( origin, direction, t, force );
	}
	if (!intersect && ((drawOptions_ & DRAW_SCALE) != 0))
	{
		intersect = scaleGizmo_->intersects( origin, direction, t, force );
		if (!intersect)
		{
			intersect =
				uniformScaleGizmo_->intersects( origin, direction, t, force );
		}
	}
	return intersect;
}



/**
 *	This is called when the user clicks on the Gizmo.
 *
 *  @param origin			The origin of the click world-ray.
 *  @param direction		The direction of teh click world-ray.
 */
/*virtual*/ void TileGizmo::click
(
	Vector3		const &origin, 
	Vector3		const &direction
)
{
	BW_GUARD;

	if ((drawOptions_ & DRAW_ROTATION) != 0)
	{
		rotationGizmo_->click( origin, direction );
	}
	if ((drawOptions_ & DRAW_SCALE) != 0)
	{
		scaleGizmo_->click( origin, direction );
		uniformScaleProxy_->setToDefault();
		uniformScaleGizmo_->click( origin, direction );
	}
}


/** 
 *	This is called then the user moves the mouse over the Gizmo.
 */
/*virtual*/ void TileGizmo::rollOver
(
	Vector3		const &origin, 
	Vector3		const &direction
)
{
	BW_GUARD;

	if ((drawOptions_ & DRAW_ROTATION) != 0)
	{
		rotationGizmo_->rollOver( origin, direction );
	}
	if ((drawOptions_ & DRAW_SCALE) != 0)
	{
		scaleGizmo_->rollOver( origin, direction );
		uniformScaleGizmo_->rollOver( origin, direction );
	}
}


/*virtual*/ Matrix TileGizmo::objectTransform() const
{
	BW_GUARD;

	Matrix m;
	matrixProxy_->getMatrix(m);
	return m;
}


/**
 *	This gets the optional texture to use when drawing.
 *
 *  @return			The texture used for drawing.  If there is no such texture
 *					then this returns NULL.
 */
Moo::BaseTexturePtr TileGizmo::texture() const
{
	return texture_;
}


/**
 *	This sets the optional texture to use when drawing.
 *
 *  @param texture	The texture to draw the tiling with.
 */
void TileGizmo::texture(Moo::BaseTexturePtr texture)
{
	BW_GUARD;

	texture_ = texture;
}


/**
 *	This gets the drawing options.
 *
 *  @returns			The draw options as from the DrawOptions bitfield.
 */
uint32 TileGizmo::drawOptions() const
{
	return drawOptions_;
}


/**
 *	This sets the drawing options.
 *
 *  @param options		The new drawing options, as from the DrawOptions
 *						bitfield.
 */
void TileGizmo::drawOptions(uint32 options)
{
	drawOptions_ = options;
}


/**
 *	This gets the opacity that the texture is drawn in.
 *
 *	@returns			The opacity used to draw the texture.
 */
uint8 TileGizmo::opacity() const
{
	return opacity_;
}


/**
 *	This sets the opacity that the texture is drawn in.
 *
 *  @param o			The new opacity.
 */
void TileGizmo::opacity(uint8 o)
{
	opacity_ = o;
}


/**
 *	This returns calculates the number of tiles depending on the scale.
 *
 *  @returns			The number of tiles across/down
 */
uint32 TileGizmo::numTiles( float scale ) const
{
	BW_GUARD;

	uint32 ret = 1;
	
	if (!almostZero( scale ))
	{
		ret = uint32( 0.5f * scaledMinGridSize_ / scale );
	}

	if (ret == 0)
	{
		ret = 1;
	}

	return ret;
}


/**
 *	This calculates the number of tiles needed for the actual drawing.
 *
 *  @returns			The number of tiles across/down
 */
float TileGizmo::drawTiles( float scale ) const
{
	// add 2 to select one more tile on each side.
	float num = float( numTiles( scale ) ) + 2.0f;
	return num < MAX_TILES ? num : MAX_TILES;
}


/**
 *	This extracts the scales from a matrix and normalizes the matrix to be
 *	a rotation and translation.
 */
void TileGizmo::extractScale(Matrix &m, float &sx, float &sy, float &sz) const
{
	BW_GUARD;

	// Save and set to zero the translational part of the matrix:
	float tx = m(0, 3); m(0, 3) = 0.0f;
	float ty = m(1, 3); m(1, 3) = 0.0f;
	float tz = m(2, 3); m(2, 3) = 0.0f;

	// Get the scales:
	sx = m.row(0).length();
	sy = m.row(1).length();
	sz = m.row(2).length();

	// Normalize the rotation part of the matrix:
    if (sx != 0)
        m.row(0, m.row(0)/sx );
    if (sy != 0)
        m.row(1, m.row(1)/sy );
    if (sz != 0)
        m.row(2, m.row(2)/sz );

	// Restore the translational part of the matrix:
	m(0, 3) = tx;
	m(1, 3) = ty;
	m(2, 3) = tz;
}


/**
 *	This draws the tile grid as lines.
 *
 *  @param m			The transform to apply.
 */
void TileGizmo::drawGrid(Matrix const &matrix)
{
	BW_GUARD;

	Matrix m = matrix;
	float scaleX, scaleY, scaleZ;
	extractScale(m, scaleX, scaleY, scaleZ);

	// Draw outer border.
	float fixScaleX = std::max( scaleX * 2.0f, scaledMinGridSize_ );
	float fixScaleZ = std::max( scaleZ * 2.0f, scaledMinGridSize_ );
	float fixNumT = 0.5f;

	// Draw internal grid
	float numTX = drawTiles( scaleX );
	for (float x = -numTX; x <= numTX; x += 1.0f)
	{
		float lineX = x * scaleX;
		if (x > 0)
		{
			lineX = std::min( lineX, fixNumT * fixScaleX );
		}
		else
		{
			lineX = std::max( lineX, -fixNumT * fixScaleX );
		}

		float lineZ = fixNumT * fixScaleZ;

		Vector3 v1 = m.applyPoint( Vector3( lineX, 0.0f, -lineZ ) );
		Vector3 v2 = m.applyPoint( Vector3( lineX, 0.0f, +lineZ ) );
		LineHelper::instance().drawLine( v1, v2, GRID_CLR_X );
	}
	float numTZ =  drawTiles( scaleZ );
	for (float z = -numTZ; z <= numTZ; z += 1.0f)
	{
		float lineX = fixNumT * fixScaleX;

		float lineZ = z * scaleZ;
		if (z > 0)
		{
			lineZ = std::min( lineZ, fixNumT * fixScaleZ );
		}
		else
		{
			lineZ = std::max( lineZ, -fixNumT * fixScaleZ );
		}

		Vector3 v1 = m.applyPoint( Vector3( -lineX, 0.0f, lineZ ) );
		Vector3 v2 = m.applyPoint( Vector3( +lineX, 0.0f, lineZ ) );
		LineHelper::instance().drawLine( v1, v2, GRID_CLR_Z );
	}
	LineHelper::instance().purge();
}


/**
 *	This draws the tile grid using a texture.
 *
 *  @param matrix		The transform to apply.
 */
void TileGizmo::drawTexture(Matrix const &matrix)
{
	BW_GUARD;

	Matrix m = matrix;
	float scaleX, scaleY, scaleZ;
	extractScale(m, scaleX, scaleY, scaleZ);

	float fixScaleX = std::max( scaleX * 2.0f, scaledMinGridSize_ );
	float fixScaleZ = std::max( scaleZ * 2.0f, scaledMinGridSize_ );
	float fixNumT = 0.5f;

	float numTX   = this->drawTiles( scaleX );
	float numTZ   = this->drawTiles( scaleZ );

	float clipFactorX = (fixNumT * fixScaleX) / (numTX * scaleX);
	float clipFactorZ = (fixNumT * fixScaleZ) / (numTZ * scaleZ);

	numTX *= clipFactorX;
	numTZ *= clipFactorZ;

	CustomMesh<Moo::VertexXYZDUV> mesh( D3DPT_TRIANGLEFAN );

	Moo::VertexXYZDUV v1; 
	v1.pos_    = m.applyPoint( Vector3( -numTX*scaleX, 0.0f, -numTZ*scaleZ ) );
	v1.uv_     = Vector2( -numTX, -numTZ );
	v1.colour_ = D3DCOLOR_ARGB( opacity_, 0xff, 0xff, 0xff );
	mesh.push_back(v1);

	Moo::VertexXYZDUV v2; 
	v2.pos_    = m.applyPoint( Vector3( +numTX*scaleX, 0.0f, -numTZ*scaleZ  ));
	v2.uv_     = Vector2( +numTX, -numTZ );
	v2.colour_ = D3DCOLOR_ARGB( opacity_, 0xff, 0xff, 0xff );
	mesh.push_back(v2);

	Moo::VertexXYZDUV v3; 
	v3.pos_    = m.applyPoint( Vector3( +numTX*scaleX, 0.0f, +numTZ*scaleZ ) ); 
	v3.uv_     = Vector2( +numTX, +numTZ );
	v3.colour_ = D3DCOLOR_ARGB( opacity_, 0xff, 0xff, 0xff );
	mesh.push_back(v3);

	Moo::VertexXYZDUV v4; 
	v4.pos_    = m.applyPoint( Vector3( -numTX*scaleX, 0.0f, +numTZ*scaleZ ) );
	v4.uv_     = Vector2( -numTX, +numTZ );
	v4.colour_ = D3DCOLOR_ARGB( opacity_, 0xff, 0xff, 0xff );
	mesh.push_back(v4);

	Moo::rc().setTexture(0, texture_->pTexture());

	Moo::rc().setRenderState(D3DRS_ALPHATESTENABLE , FALSE               );
	Moo::rc().setRenderState(D3DRS_ALPHABLENDENABLE, TRUE                );
	Moo::rc().setRenderState(D3DRS_DESTBLEND       , D3DBLEND_INVSRCALPHA);
	Moo::rc().setRenderState(D3DRS_SRCBLEND        , D3DBLEND_SRCALPHA   );
	Moo::rc().setRenderState(D3DRS_LIGHTING        , FALSE               );
	Moo::rc().setRenderState(D3DRS_ZENABLE         , TRUE                );
	Moo::rc().setRenderState(D3DRS_ZWRITEENABLE    , FALSE               );

	Moo::rc().setTextureStageState(0, D3DTSS_COLOROP  , D3DTOP_SELECTARG1);
	Moo::rc().setTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE    );
	Moo::rc().setTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE    );
	Moo::rc().setTextureStageState(0, D3DTSS_ALPHAOP  , D3DTOP_SELECTARG1);
	Moo::rc().setTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE    );
	Moo::rc().setTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE    );
	Moo::rc().setSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	Moo::rc().setSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	Moo::rc().setSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	Moo::rc().setSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	Moo::rc().setSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	Moo::rc().setTextureStageState(1, D3DTSS_ALPHAOP  , D3DTOP_DISABLE   );
	Moo::rc().setTextureStageState(1, D3DTSS_COLOROP  , D3DTOP_DISABLE   );

	mesh.draw();
}
BW_END_NAMESPACE

