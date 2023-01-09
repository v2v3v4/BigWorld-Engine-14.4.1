#include "pch.hpp"

#include "bounding_box_gui_component.hpp"


#include "moo/render_context.hpp"

#include "cstdmf/debug.hpp"

#ifndef CODE_INLINE
#include "bounding_box_gui_component.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "2DComponents", 0 )


BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( BoundingBoxGUIComponent )

PY_BEGIN_METHODS( BoundingBoxGUIComponent )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( BoundingBoxGUIComponent )

	/*~ attribute BoundingBoxGUIComponent.source
	 *	@components{ client, tools }
	 *
	 *	The source attribute specifies the matrix provider which gives the
	 *	bounding box in world space.  This is normally set to the bounds
	 *	attribute of the model the box is to draw a box around.
	 *
	 *	@type	MatrixProvider
	 */
	PY_ATTRIBUTE( source )

	/*~ attribute BoundingBoxGUIComponent.clipSpaceSource
	 *	@components{ client, tools }
	 *	When this is true, the source matrix (from the matrix provider) is taken to
	 *  already be in clip coordinates and the min and max bounding box clip co-ords
	 *  are thus directly extracted from it. In addition, when this attribute is true
	 *  but no source matrix provider is set, the component's own vertices are used to
	 *  determine the size of the bounding box.
	 *
	 *	Using this in conjunction with clipToBox and offsetSubspace allows the
	 *	BoundingBoxGUIComponent to be used to build almost any window-based widget,
	 *	eg, scrolling lists of items, drop-down combo boxes, etc...
	 *
	 *	@type	boolean (0 or 1)
	 */
	PY_ATTRIBUTE( clipSpaceSource )

	/*~ attribute BoundingBoxGUIComponent.clipToBox
	 *	@components{ client, tools }
	 *	When this is true, the component's children (excluding its corners if it
	 *	has them) are draw clipped to the screen projection of the bounding box.
	 *
	 *	Using this in conjunction with clipSpaceSource and offsetSubspace allows the
	 *	BoundingBoxGUIComponent to be used to build almost any window-based widget,
	 *	eg, scrolling lists of items, drop-down combo boxes, etc...
	 *
	 *	@type	boolean (0 or 1)
	 */
	PY_ATTRIBUTE( clipToBox )

	/*~ attribute BoundingBoxGUIComponent.absoluteSubspace
	 *	@components{ client, tools }
	 *	Determines the rules for the subspace that the bounding box's children are in.
	 *	The lowest two bits of this value control whether or not the x and y axes are
	 *	scaled to the size of the bounding box, or left at the size of the screen.
	 *	The default is the former (relative) option, for both axes.
	 *	@type	integer
	 */
	PY_ATTRIBUTE( absoluteSubspace )

	/*~ attribute BoundingBoxGUIComponent.offsetSubspace
	 *	@components{ client, tools }
	 *	This Vector3 is added to all the childrens positions (when they are
	 *	rescaled to the co-ordinate system of the bounding box).
	 *
	 *	Using this in conjunction with clipSpaceSource and clipToBox allows the
	 *	BoundingBoxGUIComponent to be used to build almost any window-based widget,
	 *	eg, scrolling lists of items, drop-down combo boxes, etc...
	 *
	 *	@type	tuple of size 3
	 */
	PY_ATTRIBUTE( offsetSubspace )

	/*~ attribute BoundingBoxGUIComponent.alwaysDisplayChildren
	 *	@components{ client, tools }
	 *	By default, when the bounding box is not on the screen, the children will not be drawn.
	 *	Set this to true to ensure the children will be drawn when the BoundingBoxGUIComponent
	 *	is off-screen.  Defaults to 0 (false).
	 *	@type	boolean (0 or 1)
	 */
	PY_ATTRIBUTE( alwaysDisplayChildren )

PY_END_ATTRIBUTES()

/*~	function GUI.BoundingBox
 *	@components{ client, tools }
 *	Creates and returns a new BoundingBoxGUIComponent with the given texture.
 *	@param textureName The full resource path of the texture to be used.
 *	@return The newly created BoundingBoxGUIComponent
 */
PY_FACTORY_NAMED( BoundingBoxGUIComponent, "BoundingBox", GUI )

COMPONENT_FACTORY( BoundingBoxGUIComponent )


/**
 *	Constructor.
 */
BoundingBoxGUIComponent::BoundingBoxGUIComponent(
		const BW::string& textureName,
		PyTypeObject * pType ) :
	SimpleGUIComponent( textureName, pType ),
	pSource_( NULL ),
	dTime_( 0.f ),
	onScreen_( false ),
	clipSpaceSource_( false ),
	clipToBox_( false ),
	absoluteSubspace_( 0 ),
	offsetSubspace_( Vector3::zero() ),
	minInClip_( Vector3(-1.f,-1.f,-1.f ) ),
	maxInClip_( Vector3( 1.f, 1.f, 1.f ) ),
	savedView_( Matrix::identity ),
	savedProjection_( Matrix::identity ),
	alwaysDisplayChildren_( false )
{
	BW_GUARD;
	for (int i = 0; i < 4; i++)
	{
		corners_[i] = new SimpleGUIComponent( textureName );
		corners_[i]->horizontalPositionMode( POSITION_MODE_LEGACY );
		corners_[i]->verticalPositionMode( POSITION_MODE_LEGACY );
		corners_[i]->widthMode( SIZE_MODE_LEGACY );
		corners_[i]->heightMode( SIZE_MODE_LEGACY );
	}

	corners_[0]->angle( SimpleGUIComponent::ROT_90 );
	corners_[0]->anchor( SimpleGUIComponent::ANCHOR_H_LEFT,
		SimpleGUIComponent::ANCHOR_V_BOTTOM );
	corners_[1]->angle( SimpleGUIComponent::ROT_180 );
	corners_[1]->anchor( SimpleGUIComponent::ANCHOR_H_LEFT,
		SimpleGUIComponent::ANCHOR_V_TOP );
	corners_[2]->angle( SimpleGUIComponent::ROT_270 );
	corners_[2]->anchor( SimpleGUIComponent::ANCHOR_H_RIGHT,
		SimpleGUIComponent::ANCHOR_V_TOP );
	corners_[3]->angle( SimpleGUIComponent::ROT_0 );
	corners_[3]->anchor( SimpleGUIComponent::ANCHOR_H_RIGHT,
		SimpleGUIComponent::ANCHOR_V_BOTTOM );
}


/**
 *	Destructor.
 */
BoundingBoxGUIComponent::~BoundingBoxGUIComponent()
{
	BW_GUARD;
	for (int i = 0; i < 4; i++)
	{
		Py_DECREF( corners_[i] );
	}
}


/**
 *	Static python factory method
 */
PyObject * BoundingBoxGUIComponent::pyNew( PyObject * args )
{
	BW_GUARD;
	char * textureName;
	if (!PyArg_ParseTuple( args, "s", &textureName ))
	{
		PyErr_SetString( PyExc_TypeError, "pyGUI_BoundingBox: "
			"Argument parsing error: Expected a texture name" );
		return NULL;
	}

	return new BoundingBoxGUIComponent( textureName );
}



/**
 *	This simply caches dTime ( because we have very different
 *	update requirements than other GUI components - we are
 *	very much dependent on the current render context, which
 *	at this time will not have been set up. )
 *	i.e. we have to update at draw time.  This is called preDraw
 *
 *	@param dTime	Delta time for this frame of the app
 */

void BoundingBoxGUIComponent::update( float dTime, float relParentWidth, float relParentHeight )
{
	BW_GUARD;
	dTime_ = dTime;
	runTimeColour( SimpleGUIComponent::colour() );
	runTimeTransform( Matrix::identity );
	//this is valid because CameraApp::tick sets an appropriate viewProjectionMatrix,
	//which occurs well before GUI::tick.
	savedView_ = Moo::rc().view();
	savedProjection_ = Moo::rc().projection();
}


/**
 *	This method prepares to draw the bounding box component.
 *	It is the private internal update method
 *
 *	@param m		The model whos bounding box we use for positioning
 */
void BoundingBoxGUIComponent::preDraw( float relParentWidth, float relParentHeight )
{
	BW_GUARD;
	// get the bounding box, which is described by a matrix.
	// the bb is actually always (0,0,0)-(1,1,1), the matrix
	// puts it into an oriented world space bounding box.
	bb_.setBounds( Vector3(0.f,0.f,0.f), Vector3(1.f,1.f,1.f) );

	Matrix bbMatrix = Matrix::identity;
	if (pSource_)
	{
		pSource_->matrix( bbMatrix );
	}
	// if we don't have a source but it should be in clip coords,
	//  use our position and dimensions (of our vertices)
	else if (clipSpaceSource_)
	{
		bbMatrix.translation( (Vector3&)vertices_[0].pos_ );
		Vector3 delta = vertices_[2].pos_ - vertices_[0].pos_;
		bbMatrix[0] *= delta.x;
		bbMatrix[1] *= delta.y;
	}
	// otherwise we don't draw
	else
	{
		bbMatrix[3][3] = 0.f;
	}

	// make sure the model is being drawn
	if (bbMatrix(3,3) != 1.f)
	{
		onScreen_ = false;
		return;
	}

	// assume we'll be onscreen
	onScreen_ = true;
	Vector3 min, max;

	if (clipSpaceSource_)
	{
		min = bbMatrix.applyToOrigin();
		max = bbMatrix.applyPoint( Vector3( 1.f, 1.f, 1.f ) );
	}
	else
	{
		//calculate the position of the corner of the bounding box, in clip coords
		//Transform the local bounding box into clip space
		//Then divide by w ( if we can )
		//Finally, work out the min/max extents in post-perspective
		//homogenous clip coordinates
		Matrix objectToCamera = bbMatrix;
		objectToCamera.postMultiply( savedView_ );

		if ( !calculatePostDivideMinMax( min, max, bb_, objectToCamera, savedProjection_ ) )
		{
			//the bounding box ended up behind us
			onScreen_ = false;
		}


		//w and h are clip-space values for the width and height of each corner
		float w = this->widthInClip( relParentWidth );
		float h = this->heightInClip( relParentHeight );

		//constrain corners, so that they don't draw off the screen
		float safe = 1.f;
		if ( min[0] < ( -safe + w ) )
			min[0] = -safe + w;
		if ( min[1] < ( -safe + h ) )
			min[1] = -safe + h;
		if ( max[0] > ( safe - w ) )
			max[0] = safe - w;
		if ( max[1] > ( safe - h ) )
			max[1] = safe - h;

		//constrain corners, so that they don't get closer than one width / height
		//from each other
		if ( (max[0] - min[0] ) < ( w*3.f ) )
		{
			float difference = ( w*3.f ) - (max[0] - min[0] );
			min[0] -= difference / 2.f;
			max[0] += difference / 2.f;
		}

		if ( (max[1] - min[1] ) < ( h*3.f ) )
		{
			float difference = ( h*3.f ) - (max[1] - min[1] );
			min[1] -= difference / 2.f;
			max[1] += difference / 2.f;
		}
	}

	// save this min and max for later
	minInClip_ = min;
	maxInClip_ = max;
	float w = (min[2] + max[2]) / 2.f;

	corners_[0]->position( Vector3( min[0], min[1], w ) );
	corners_[1]->position( Vector3( min[0], max[1], w ) );
	corners_[2]->position( Vector3( max[0], max[1], w ) );
	corners_[3]->position( Vector3( max[0], min[1], w ) );

	corners_[0]->update( dTime_, relParentWidth, relParentHeight );
	corners_[1]->update( dTime_, relParentWidth, relParentHeight );
	corners_[2]->update( dTime_, relParentWidth, relParentHeight );
	corners_[3]->update( dTime_, relParentWidth, relParentHeight );

	SimpleGUIComponent::ChildRecVector::iterator it = children_.begin();
	SimpleGUIComponent::ChildRecVector::iterator end = children_.end();

	while( it != end )
	{
		SimpleGUIComponentPtr c = it->second;

		SimpleGUIComponent::eHAnchor h = c->horizontalAnchor();
		SimpleGUIComponent::eVAnchor v = c->verticalAnchor();

		//For the draw call, we need to convert the x,y position
		//into a parameterized position along our corner positions.
		Vector3 pos = c->position();
		Vector3 newPos;


		float clipLeft = corners_[0]->position().x;
		float clipBot  = corners_[0]->position().y;
		float clipWidth  = corners_[3]->position().x - corners_[0]->position().x;
		float clipHeight = corners_[1]->position().y - corners_[0]->position().y;
		if (!(absoluteSubspace_ & 1))
			newPos.x = clipLeft + pos.x * clipWidth;
		else
			newPos.x = pos.x < 0 ?
				clipLeft + pos.x + 1.f :
				clipLeft + clipWidth + pos.x - 1.f;
		if (!(absoluteSubspace_ & 2))
			newPos.y = clipBot  + pos.y * clipHeight;
		else
			newPos.y = pos.y < 0 ?
				clipBot  + pos.y + 1.f :
				clipBot  + clipHeight + pos.y - 1.f;
		newPos.z = corners_[0]->position().z;

		newPos += offsetSubspace_;

		c->position( newPos );
		c->update( 0.f, relParentWidth, relParentHeight );
		c->position( pos );

		it++;
	}

	this->internalApplyShaders( dTime_ );
}


/**
 *	This method is a stub fn, because we update our vertices in a
 *	different manner ( later on ) than when this is usually called
 *
 *	@param dTime	Delta time for this frame of the app
 */
void
BoundingBoxGUIComponent::applyShaders( float dTime )
{
	BW_GUARD;
	return;
}


/**
 *	This method is the root of the recursive shader descent for
 *	this component and its children
 *
 *	@param dTime	Delta time for this frame of the app
 */
void BoundingBoxGUIComponent::internalApplyShaders( float dTime )
{
	BW_GUARD;
	GUIShaderPtrVector::iterator it = shaders_.begin();
	GUIShaderPtrVector::iterator end = shaders_.end();

	while( it != end )
	{
		this->applyShader( *it->second, dTime );
		it++;
	}

	//Apply corner specific shaders
	for ( int i = 0; i < 4; i++ )
	{
		corners_[i]->applyShaders( dTime );
	}

	//Apply child shaders
	ChildRecVector::iterator cit = children_.begin();
	ChildRecVector::iterator cend = children_.end();

	while( cit != cend )
	{
		cit->second->applyShaders( dTime );
		cit++;
	}
}


/**
 *	This method applies a shader to our corner gui components and our children
 *
 *	@param shader	The shader to apply
 *	@param dTime	Delta time for this frame of the app
 */
void BoundingBoxGUIComponent::applyShader( GUIShader& shader, float dTime )
{
	BW_GUARD;
	for ( int i = 0; i < 4; i++ )
	{
		corners_[i]->applyShader( shader, 0.f );
	}

	ChildRecVector::iterator cit = children_.begin();
	ChildRecVector::iterator cend = children_.end();

	while( cit != cend )
	{
		cit->second->applyShader( shader, 0.f );
		cit++;
	}
}



/**
 *	This method draws the bounding box component.
 */
void BoundingBoxGUIComponent::draw( Moo::DrawContext& drawContext, bool reallyDraw, bool overlay )
{
	BW_GUARD;
	// figure out where to draw stuff (like update but needs rc)
	this->preDraw( SimpleGUI::instance().screenWidth(), SimpleGUI::instance().screenHeight() );

	Moo::rc().push();
	Moo::rc().preMultiply( runTimeTransform() );
	Moo::rc().device()->SetTransform( D3DTS_WORLD, &Moo::rc().world() );

	// and draw
	if (onScreen_)
	{
		Moo::rc().setRenderState( D3DRS_TEXTUREFACTOR, runTimeColour() );

		if (SimpleGUIComponent::visible() &&
			SimpleGUIComponent::textureName().length())
		{
			for (int i = 0; i < 4; i++)
			{
				corners_[i]->draw( drawContext, reallyDraw, overlay );
			}
		}

		DX::Viewport oldViewport;
		if (clipToBox_)
		{
			Vector2 halfDim(
				SimpleGUI::instance().halfScreenWidth(),
				SimpleGUI::instance().halfScreenHeight() );

			Moo::rc().getViewport( &oldViewport );
			DX::Viewport newViewport = oldViewport;

			newViewport.X = max( int(oldViewport.X),
				int((1+minInClip_.x) * halfDim.x) );
			newViewport.Y = max( int(oldViewport.Y),
				int((1-maxInClip_.y) * halfDim.y) );
			newViewport.Width = min( int(oldViewport.X + oldViewport.Width),
				int((1+maxInClip_.x) * halfDim.x)) - int(newViewport.X);
			newViewport.Height= min( int(oldViewport.Y + oldViewport.Height),
				int((1-minInClip_.y) * halfDim.y)) - int(newViewport.Y);

			if (int(newViewport.Height) <= 0) return;
			if (int(newViewport.Width)  <= 0) return;

			Moo::rc().setViewport( &newViewport );
		}

		SimpleGUIComponent::drawChildren( drawContext, reallyDraw, overlay );

		if (clipToBox_)
		{
			Moo::rc().setViewport( &oldViewport );
		}
	}
	else if (alwaysDisplayChildren_)
	{
		if (SimpleGUIComponent::visible() && (pSource_ || clipSpaceSource_))
		{

			SimpleGUIComponent::drawChildren( drawContext, reallyDraw, overlay );
		}
	}

	runTimeClipRegion_ = SimpleGUI::instance().clipRegion();
	Moo::rc().pop();
	Moo::rc().device()->SetTransform( D3DTS_WORLD, &Moo::rc().world() );
}


/**
 *	This method works out the min / max points in post-perspective
 *	homogenous clip coordinates.
 *
 *	The incoming bounding box should be in camera space.
 *
 *	@param min the resultant minimum bounds
 *	@param max the resultant maximum bounds
 *	@param bb the bounding box in pre-perspective-divide clip coordinates
 *
 *	@return true if the min/max could be calculated.  This method
 *			returns false if it cannot divide by w
 */
bool
BoundingBoxGUIComponent::calculatePostDivideMinMax( Vector3& min, Vector3& max,
										  BoundingBox& _bb,
										  const Matrix& objectToCamera,
										  const Matrix& cameraToClip )
{
	BW_GUARD;
	BoundingBox bb = _bb;

	bb.transformBy( objectToCamera );

	min = bb.minBounds();
	max = bb.maxBounds();

	//calulate the 8 corners of the camera space bounding box
	Vector4 camera[8];
	camera[0].set( min.x, min.y, min.z, 1 );
	camera[1].set( min.x, max.y, min.z, 1 );
	camera[2].set( max.x, max.y, min.z, 1 );
	camera[3].set( max.x, min.y, min.z, 1 );
	camera[4].set( min.x, min.y, max.z, 1 );
	camera[5].set( min.x, max.y, max.z, 1 );
	camera[6].set( max.x, max.y, max.z, 1 );
	camera[7].set( max.x, min.y, max.z, 1 );

	//transform the bounding box into clip coordinates,
	//and set the valid flag to true if 2 points made
	//it in front of the near clipping plane.
	//
	//While we do this, update the min/max vectors.
	//Only the x/y coordinates need be calculated
	Vector4 clip[8];
	bool valid = false;

	min.set( 1e23f, 1e23f, 1e23f );
	max.set( -1e23f, -1e23f, -1e23f );

	for ( int i = 0; i < 8; i++ )
	{
		cameraToClip.applyPoint( clip[i], camera[i] );
		float w = clip[i][3];

		if ( w > 0.0001f )
		{
			valid = true;
		}
		else if ( almostZero( w, 0.0001f ) )
		{
			w = 0.0001f;
		}
		else
		{
			w = -w;
		}

		float oow = 1.f / w;
		clip[i] *= oow;

		if ( clip[i][0] < min.x )
			min.x = clip[i][0];
		if ( clip[i][1] < min.y )
			min.y = clip[i][1];
		if ( clip[i][2] < min.z )
			min.z = clip[i][2];

		if ( clip[i][0] > max.x )
			max.x = clip[i][0];
		if ( clip[i][1] > max.y )
			max.y = clip[i][1];
		if ( clip[i][2] > max.z )
			max.z = clip[i][2];
	}

	return valid;
}


/**
 *	Load
 */
bool BoundingBoxGUIComponent::load( DataSectionPtr pSect, const BW::string& ownerName,
	LoadBindings & bindings )
{
	BW_GUARD;
	if (!this->SimpleGUIComponent::load( pSect, ownerName, bindings )) return false;

	clipSpaceSource_ = pSect->readBool( "clipSpaceSource", clipSpaceSource_ );
	clipToBox_ = pSect->readBool( "clipToBox", clipToBox_ );
	absoluteSubspace_ = pSect->readInt( "absoluteSubspace", absoluteSubspace_ );
	offsetSubspace_ = pSect->readVector3( "offsetSubspace", offsetSubspace_ );
	alwaysDisplayChildren_ = pSect->readBool( "alwaysDisplayChildren", alwaysDisplayChildren_ );

	bb_ = BoundingBox( Vector3::zero(), Vector3::zero() );
	pSource_ = NULL;
	dTime_ = 0.f;
	onScreen_ = false;

	minInClip_ = Vector3(-1.f,-1.f,-1.f);
	maxInClip_ = Vector3( 1.f, 1.f, 1.f);

	return true;
}

/**
 *	Save
 */
void BoundingBoxGUIComponent::save( DataSectionPtr pSect,
	SaveBindings & bindings )
{
	BW_GUARD;
	this->SimpleGUIComponent::save( pSect, bindings );

	pSect->writeBool( "clipSpaceSource", clipSpaceSource_ );
	pSect->writeBool( "clipToBox", clipToBox_ );
	pSect->writeInt( "absoluteSubspace", absoluteSubspace_ );
	pSect->writeVector3( "offsetSubspace", offsetSubspace_ );
	pSect->writeBool( "alwaysDisplayChildren", alwaysDisplayChildren_ );
}

BW_END_NAMESPACE

// bounding_box_gui_component.cpp
