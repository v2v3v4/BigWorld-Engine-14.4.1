#include "pch.hpp"
#include "py_loft.hpp"
#include "moo/draw_context.hpp"
#include "moo/render_context.hpp"
#include "moo/moo_math.hpp"
#include "moo/fog_helper.hpp" 
#include "moo/geometrics.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

//static int s_nTris = -1;


// -----------------------------------------------------------------------------
// Section: SimplePath
// -----------------------------------------------------------------------------
/**
 *	This method ages all the control points, and throws out any that are too
 *	old.
 *	@param	dTime	delta time for the frame.
 *	@param	maxAge	age at which history entries expire.
 */
void SimplePath::age( float dTime, float maxAge )
{
	History::iterator it = history_.begin();
	History::iterator en = history_.end();
	while (it != en)
	{
		ControlPoint& b = *it;		
		b.age_ += dTime;

		if (b.age_ > maxAge)
		{
			history_.erase( it, en );
			break;
		}

		it++;
	}
}


/**
 *	This method adds a single control point, to the front of the history list.
 *
 *	@param	ControlPoint&	New ControlPoint to add to the history list.
 */
void SimplePath::add( const ControlPoint& cp )
{
	history_.push_front( cp );
}


/**
 *	This method returns the size of the history list.
 *
 *	@return size_t	Number of entries in the history list.
 */
size_t SimplePath::size() const
{
	return history_.size();
}


/**
 *	This method returns an interator pointing to the beginning of the
 *	history list (the most recent control point).
 *
 *	@param	ControlPoint&	[Out]The first, most recent, control point.
 *	@return	bool			True if a control point was output, or False if the
 *							history list is empty.
 */
bool SimplePath::begin(ControlPoint& ret)
{
	it_ = history_.begin();
	return this->next(ret);
}


/**
 *	This method returns the next control point in the iteration.
 *
 *	@param	ControlPoint&	[Out]The next control point in the iteration.
 *	@return	bool			True if a control point was output, or False if the
 *							iteration is finished.
 */
bool SimplePath::next(ControlPoint& ret)
{
	if (it_ != history_.end())
	{
		ret = *it_++;
		return true;
	}
	return false;
}


// -----------------------------------------------------------------------------
// Section: InterpolatedPath
// -----------------------------------------------------------------------------
//Constructor.
InterpolatedPath::InterpolatedPath():
	threshold_( 0.1f )	
{
}


/**
 *	This method ages all the control points, and throws out any that are too
 *	old.
 *	@param	dTime	delta time for the frame.
 *	@param	maxAge	age at which history entries expire.
 */
void InterpolatedPath::age( float dTime, float maxAge )
{
	History::iterator it = history_.begin();
	History::iterator en = history_.end();
	while (it != en)
	{
		ControlPoint& b = *it;		
		b.age_ += dTime;

		if (b.age_ > maxAge)
		{
			history_.erase( it, en );
			break;
		}

		it++;
	}
}


/**
 *	This method adds a new control point to the history list.
 *	It performs tessellation as necessary between the second
 *	and third most recent points using Linear interpolation
 *
 *	@param	ControlPoint&	New ControlPoint to add to the history list.
 */
void InterpolatedPath::add( const ControlPoint& cp )
{
	//set temporary_'s age to -1.  This internally signifies we	
	//don't want to use it when rendering.
	temporary_.age_ = -1.f;

	if ( history_.empty() )
	{
		history_.push_front( cp );
	}
	else
	{
		ControlPoint& last = history_.front();
		float stDistSq = (cp.st_ - last.st_).lengthSquared();
		float enDistSq = (cp.en_ - last.en_).lengthSquared();

		if ( max(stDistSq,enDistSq) <= (threshold_*threshold_) )
		{
			//system has incremented less than the threshold,
			//just record it as temporary_.  However we don't
			//want to do this if there's only one entry in our
			//history, as that means we have not yet moved far
			//enough to generate the first quad.
			if (history_.size() > 1)
			{
				temporary_ = cp;
			}
		}
		else
		{
			//subdivide because the gap between the last position
			//and this was too much.
			float maxDist = max( sqrtf(stDistSq), sqrtf(enDistSq) );
			float nSubDivisions = ceilf(maxDist / threshold_);

			Vector3 dSt = (cp.st_-last.st_) / nSubDivisions;
			Vector3 dEn = (cp.en_-last.en_) / nSubDivisions;
			float dAge = -last.age_ / nSubDivisions;
			uint32 n = (uint32)(nSubDivisions);
			ControlPoint curr( last );

			for ( uint32 i=1; i<=n; i++ )
			{
				curr.st_ += dSt;
				curr.en_ += dEn;
				curr.age_ += dAge;
				this->addPoint(curr);
			}
		}
	}
}


/**
 *	This method returns the size of the history list.
 *
 *	@return size_t	Number of entries in the history list.
 */
size_t InterpolatedPath::size() const
{
	if ( temporary_.age_ >= 0.f )
	{
		return history_.size() + 1;
	}
	else
	{
		return history_.size();
	}
}


/**
 *	This method adds a single point to the front of the history list.
 *
 *	@param	ControlPoint&	New ControlPoint to add to the history list.
 */
void InterpolatedPath::addPoint( const ControlPoint& cp )
{
	history_.push_front( cp );
}


/**
 *	This method sets an iterator pointing to the start of the history list,
 *	and returns true if the iterator is valid.
 *
 *	@param	ControlPoint&	[Out]The first, most recent, control point.
 *	@return	bool			True if a control point was output, or False if the
 *							history list is empty.
 */
bool InterpolatedPath::begin( ControlPoint& cp )
{
	if (temporary_.age_ >= 0.f && it_ == history_.end() )
	{
		cp = temporary_;
		it_ = history_.begin();
		return true;
	}

	it_ = history_.begin();
	return this->next(cp);
}


/**
 *	This method advances the iterator and returns true if the new
 *	iterator is valid.
 *
 *	@param	ControlPoint&	[Out]The next control point in the iteration.
 *	@return	bool			True if a control point was output, or False if the
 *							iteration is finished.
 */
bool InterpolatedPath::next(ControlPoint& cp)
{
	if ( it_ != history_.end()  )
	{
		cp = *it_++;
		return true;
	}
	else
	{
		return false;
	}		
}


// -----------------------------------------------------------------------------
// Class: SmoothedPath
// -----------------------------------------------------------------------------
/**
 *	This method adds a single point to the front of the history list.
 *	It performs tessellation as necessary between the second
 *	and third most recent points using Catmull-Rom interpolation
 *
 *	@param	ControlPoint&	New ControlPoint to add to the history list.
 */
void SmoothedPath::addPoint( const ControlPoint& basis5 )
{
	history_.push_front( basis5 );

	if ( history_.size() >= 5 )
	{		
		//recalculate/smooth the intermediate point.
		History::iterator it=history_.begin();
		History::iterator en=history_.end();
		ControlPoint& basis4 = *it++;
		ControlPoint& basis3 = *it++;
		ControlPoint& basis2 = *it++;
		ControlPoint& basis1 = *it;

		//recalculate pt 3
		D3DXVec3CatmullRom( &basis3.st_, &basis1.st_, &basis2.st_, &basis4.st_, &basis5.st_, 0.5f );
		D3DXVec3CatmullRom( &basis3.en_, &basis1.en_, &basis2.en_, &basis4.en_, &basis5.en_, 0.5f );
	}
}


// -----------------------------------------------------------------------------
// Section: ProgressiveRefinementPath
// -----------------------------------------------------------------------------
//Constructor.
ProgressiveRefinementPath::ProgressiveRefinementPath():
	threshold_( 0.1f )	
{
}


/**
 *	This method ages all the control points, and throws out any that are too
 *	old.
 *	@param	dTime	delta time for the frame.
 *	@param	maxAge	age at which history entries expire.
 */
void ProgressiveRefinementPath::age( float dTime, float maxAge )
{	
	History::iterator it = history_.begin();
	History::iterator en = history_.end();
	while (it != en)
	{
		ControlPoint& b = *it;
		b.age_ += dTime;

		if (b.age_ > maxAge)
		{
			history_.erase( it, en );
			break;
		}

		it++;
	}
}


/**
 *	This method adds a new control point to the history list.
 *
 *	@param	ControlPoint&	New ControlPoint to add to the history list.
 */
void ProgressiveRefinementPath::add( const ControlPoint& cp )
{
	//set temporary_'s age to -1.  This internally signifies we	
	//don't want to use it when rendering.
	temporary_.age_ = -1.f;

	if ( history_.empty() )
	{
		history_.push_front( cp );
	}
	else
	{
		ControlPoint& last = history_.front();
		float stDistSq = (cp.st_ - last.st_).lengthSquared();
		float enDistSq = (cp.en_ - last.en_).lengthSquared();

		if ( max(stDistSq,enDistSq) <= (threshold_*threshold_) )
		{
			//system has incremented less than the threshold,
			//just record it as temporary_.  However we don't
			//want to do this if there's only one entry in our
			//history, as that means we have not yet moved far
			//enough to generate the first quad.
			if (history_.size() > 1)
			{
				temporary_ = cp;
			}
		}
		else
		{
			history_.push_front( cp );
		}
	}	
}


/**
 *	This method returns the size of the history list.
 *
 *	@return size_t	Number of entries in the history list.
 */
size_t ProgressiveRefinementPath::size() const
{
	if ( temporary_.age_ >= 0.f )
	{
		return history_.size() + 1;
	}
	else
	{
		return history_.size();
	}
}


/**
 *	This method performs a single smoothing run over the entire mesh.  If
 *	any section between two adjacent path-steps is too large, that section
 *	is tessellated once using Catmull-Rom interpolation.  It is designed to
 *	be called once per frame to progressively smooth the mesh.
 */
void ProgressiveRefinementPath::smooth()
{	
	if ( history_.size() >= 4 )
	{
		History::iterator cp[4];
		History::iterator it = history_.begin();
		History::iterator en = history_.end();

		cp[0] = it++;
		cp[1] = it++;
		cp[2] = it++;
		cp[3] = it++;

		this->refine( cp[0], cp[1], cp[2], cp[3] );

		while (it != en)
		{
			cp[0]++;
			cp[1]++;
			cp[2]++;
			cp[3]++;
			it++;
			this->refine( cp[0], cp[1], cp[2], cp[3] );
		}
	}
}


/**
 *	This method inserts a new point in between cp[1] and cp[2], if
 *	the maximum distance between them is > threshold.  It makes sure
 *	that if a point is added, the 4 iterators remain valid.
 *
 *	@param	cp0		Control Point 0, used to define the spline.
 *	@param	cp1		Control Point 1, used to define the spline.
 *	@param	cp2		Control Point 2, used to define the spline.
 *	@param	cp3		Control Point 3, used to define the spline.
 *
 *	@return	bool	Whether or not a new control point was added.
 */
bool ProgressiveRefinementPath::refine( History::iterator& cp0, History::iterator& cp1, History::iterator& cp2, History::iterator& cp3 )
{
	bool refined = false;
	float distSq = ((*cp1).st_- (*cp2).st_).lengthSquared();
	if (distSq > threshold_*threshold_)
	{
		//insert a new control point between cp[1] and cp[2], and update
		//the iterators cp[0] and cp[1].
		ControlPoint cp;		
		cp.age_ = ( cp1->age_ + cp2->age_ ) / 2.f;
		D3DXVec3CatmullRom( &cp.st_, &cp0->st_, &cp1->st_, &cp2->st_, &cp3->st_, 0.5f );
		D3DXVec3CatmullRom( &cp.en_, &cp0->en_, &cp1->en_, &cp2->en_, &cp3->en_, 0.5f );
		cp0 = cp1;
		cp1 = history_.insert( cp2, cp );
		return true;
	}
	return false;
}


/**
 *	This method sets an iterator pointing to the start of the history list,
 *	and returns true if the iterator is valid.  It also instigates one
 *	smoothing run over the entire mesh.
 *
 *	@param	ControlPoint&	[Out]The first, most recent, control point.
 *	@return	bool			True if a control point was output, or False if the
 *							history list is empty.
 */
bool ProgressiveRefinementPath::begin( ControlPoint& cp )
{
	if (temporary_.age_ >= 0.f && it_ == history_.end() )
	{
		cp = temporary_;
		it_ = history_.begin();
		return true;
	}

	this->smooth();

	it_ = history_.begin();
	return this->next(cp);
}


/**
 *	This method advances the iterator and returns true if the new
 *	iterator is valid.
 *
 *	@param	ControlPoint&	[Out]The next control point in the iteration.
 *	@return	bool			True if a control point was output, or False if the
 *							iteration is finished.
 */
bool ProgressiveRefinementPath::next(ControlPoint& cp)
{
	if ( it_ != history_.end()  )
	{
		cp = *it_++;
		return true;
	}
	else
	{
		return false;
	}		
}


/**
 *	This class implements the sorted draw item required for additive or blended
 *	lofts.
 */
class PyLoftDrawItem : public Moo::DrawContext::UserDrawItem
{
public:
	PyLoftDrawItem( PyLoft* loft, const Matrix& worldTransform )
	:	loft_( loft ),
		worldTransform_( worldTransform )		
	{
	}
	~PyLoftDrawItem()
	{

	}
	void draw()
	{
		loft_->realDraw( worldTransform_ );
	}
	void fini()
	{
		delete this;
	}
private:
	PyLoft* loft_;
	Matrix	worldTransform_;	
};


// -----------------------------------------------------------------------------
//	Section - PyLoft
// -----------------------------------------------------------------------------
//Constructor.
PyLoft::PyLoft( PyTypeObject *pType ):
	PyAttachment( pType ),
	textureName_( "" ),
	endPoint_( NULL ),
	maxAge_( 0.1f ),
	path_( &path4_ ),
	bb_( BoundingBox::s_insideOut_ )
{
	BW_GUARD;
	material_.alphaTestEnable( false );
	material_.alphaBlended( true );
	material_.destBlend( Moo::Material::ONE );	
	material_.srcBlend( Moo::Material::ONE );	
	material_.zBufferRead( true );
	material_.zBufferWrite( false );
	material_.fogged( true );	
	material_.doubleSided( true );

	Moo::TextureStage ts;
	ts.colourOperation( Moo::TextureStage::MODULATE,	
		Moo::TextureStage::CURRENT,
		Moo::TextureStage::TEXTURE );
	ts.alphaOperation( Moo::TextureStage::DISABLE );
	ts.textureWrapMode( Moo::TextureStage::CLAMP );
	ts.useMipMapping( true );
	material_.addTextureStage( ts );
	ts.colourOperation( Moo::TextureStage::DISABLE );
	ts.textureWrapMode( Moo::TextureStage::CLAMP );
	material_.addTextureStage( ts );

	/*if (s_nTris == -1)
	{
		MF_WATCH( "nLoft Tris", s_nTris );
		s_nTris = 0;
	}*/
}


//Destructor.
PyLoft::~PyLoft()
{
	BW_GUARD;
}


/**
 *	This method sets the texture name for the PyLoft.
 *	Note the fetches the texture synchronously, so make sure to preload
 *	the resource using prerequisites, preloads, or loadResourceListBG.
 *
 *	@param	BW::string		resourceID for the texture.
 */
void PyLoft::textureName( const BW::string& v )
{
	BW_GUARD;
	Moo::BaseTexturePtr pTexture = Moo::TextureManager::instance()->get( v, true, true, true, "texture/py_loft" );
	if ( !pTexture.hasObject() )
		return;
	textureName_ = v;
	material_.textureStage(0).pTexture( pTexture );
}


/**
 *	This method draws the loft.  The world transform specified where the loft
 *	is attached in the world.
 */
void PyLoft::draw( Moo::DrawContext& drawContext, const Matrix & worldTransform )
{
	BW_GUARD;	

	float distance = 
		(worldTransform.applyToOrigin() - Moo::rc().invView().applyToOrigin()).length();
	drawContext.drawUserItem(new PyLoftDrawItem( this, worldTransform ),
		Moo::DrawContext::TRANSPARENT_CHANNEL_MASK, distance );
}


/**
 *	This method ticks the loft.  It ages the control points and culls any that
 *	are too old.
 */
void PyLoft::tick( float dTime )
{
	BW_GUARD;

	path_->age(dTime,maxAge_);
	//s_nTris = 0;
}


/**
 *	This method records the current position and end point of the shape, and
 *	outputs a new control point in the path.
 */
void PyLoft::recordPosition( const Matrix & worldTransform )
{
	BW_GUARD;	

	Vector3 startPos = worldTransform.applyToOrigin();
	Vector3 endPos;	
	if ( endPoint_.hasObject() )
	{
		Matrix m;
		endPoint_->matrix(m);
		endPos = m.applyToOrigin();
	}
	else
	{
		endPos = startPos + worldTransform.applyToUnitAxisVector(1);
	}

	path_->add(ControlPoint(0.f, startPos, endPos));

	bb_.addBounds( startPos );
	bb_.addBounds( endPos );
}


//#include "input/input.hpp"
/**
 *	This method actually draws the loft, in response to a direct call, or
 *	indirectly via the SortedDrawItem mechanism.
 */
void PyLoft::realDraw( const Matrix & worldTransform )
{
	BW_GUARD_PROFILER( PyLoft_realDraw );	

/*	if (InputDevices::instance().isKeyDown( KeyCode::KEY_F1 ) )
	{
		path_ = &path1_;
	}
	if (InputDevices::instance().isKeyDown( KeyCode::KEY_F2 ) )
	{
		path_ = &path2_;
	}
	if (InputDevices::instance().isKeyDown( KeyCode::KEY_F3 ) )
	{
		path_ = &path3_;
	}
	if (InputDevices::instance().isKeyDown( KeyCode::KEY_F4 ) )
	{
		path_ = &path4_;
	}
	static bool wire_ = false;
	if (InputDevices::instance().isKeyDown( KeyCode::KEY_F5 ) )
	{
		wire_ = false;
	}
	if (InputDevices::instance().isKeyDown( KeyCode::KEY_F6 ) )
	{
		wire_ = true;
	}*/

	this->recordPosition( worldTransform );

	//bounding box always contain bounds of the last drawn mesh,
	//any new points created in the meantime.  When the mesh is
	//drawn again (which may not be for some time, if the loft if
	//offscreen), the bounds are recalculated again, up-to-date.
	Matrix cullTransform = Moo::rc().viewProjection();
	bb_.calculateOutcode( cullTransform );
	if (bb_.combinedOutcode())
	{
		return;
	}

	//set render states
	Moo::rc().setPixelShader( 0 );
	Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
	Moo::rc().setIndices( NULL );
	material_.set();

	if ( Geometrics::beginLoft() )
	{
		float dAge = maxAge_ / (float)path_->size();
		float dUV = 1.f / (float)path_->size();
		float age = 0.f;
		float uv = 0.f;

		ControlPoint p;
		if (path_->begin(p))		
		{
			bb_ = BoundingBox::s_insideOut_;
			do
			{
				Geometrics::texturedWorldLoftSegment( p.st_, p.en_, uv, 0xffffffff );
				bb_.addBounds( p.st_ );
				bb_.addBounds( p.en_ );
				//s_nTris++;
				uv += dUV;
			} while (path_->next(p) == true);
		}

		Geometrics::endLoft( Matrix::identity );
	}

	/*if ( wire_ )
		Moo::rc().setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );	*/

	Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_CCW );	
}


/**
 *	This method serialises the loft to/from a datasection.
 */
void PyLoft::serialiseInternal(DataSectionPtr pSect, bool load)
{
	BW_GUARD;
	SERIALISE(pSect, textureName_, String, load);	
	SERIALISE(pSect, maxAge_, Float, load);

	if ( load )
	{
		this->textureName( textureName_ );
		this->path4_.threshold( pSect->readFloat( "threshold_" ) );
	}
	else
	{
		pSect->writeFloat( "threshold_", this->path4_.threshold() );
	}
}


/**
 *	This method accumulates the loft's bounding box into the passed-in one.
 */
void PyLoft::localBoundingBox( BoundingBox & bb, bool skinny )
{
	BW_GUARD;	
	if (!bb_.insideOut() )
	{		
		BoundingBox lbb( bb_ );
		Matrix invWorld( worldTransform() );
		invWorld.invert();
		lbb.transformBy( invWorld );
		bb.addBounds(lbb);
	}
}


/**
 *	This method accumulates the loft's bounding box into the passed-in one. 
 */
void PyLoft::localVisibilityBox( BoundingBox & vbb, bool skinny )
{	
	localBoundingBox(vbb, skinny);
}


/**
 *	This method accumulates the loft's bounding box into the passed-in one.
 *	Note that PyModel wants this bounding box in local space, but our
 *	history and bounding box is stored in world space.
 */
void PyLoft::worldBoundingBox( BoundingBox & bb, const Matrix& world, bool skinny )
{
	BW_GUARD;	
	if (!bb_.insideOut() )
	{		
		bb.addBounds(bb_);
	}
}


/**
 *	This method accumulates the loft's bounding box into the passed-in one. 
 *	Note that PyModel wants this bounding box in local space, but our
 *	history and bounding box is stored in world space.
 */
void PyLoft::worldVisibilityBox( BoundingBox & vbb, const Matrix& world, bool skinny )
{	
	worldBoundingBox(vbb, world, skinny);
}


// ----------------------------------------------------------------------------
// Section: The Python Interface to the PyLoft.
// ----------------------------------------------------------------------------
#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE PyLoft::

PY_TYPEOBJECT( PyLoft )

/*~ function BigWorld.Loft
 *	This function creates and returns a new PyLoft object.
 */
PY_FACTORY_NAMED( PyLoft, "Loft", BigWorld )

PY_BEGIN_METHODS( PyLoft )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyLoft )	
	/*~ attribute PyLoft.textureName
	 *	This attribute specifies the name of the texture used to render the
	 *	loft trail.  The texture is stretched once along the entire loft,
	 *	with the u-axis of the texture mapped along the path, and the v-axis
	 *	mapped along the shape.
	 */
	PY_ATTRIBUTE( textureName )	
	/*~ attribute PyLoft.endPoint
	 *	This is a MatrixProvider that gives shape to the loft.
	 *	If an endpoint is provided, the loft is created using a shape,
	 *	where the shape is a line connecting the attachment point
	 *	and the endpoint matrix.
	 *
	 *	@type MatrixProvider. Default is None.
	 */
	PY_ATTRIBUTE( endPoint )	
	/*~ attribute PyLoft.maxAge
	 *	This specifies how long into the past the loft should draw.
	 *
	 *	@type float. Default is 0.1.
	 */
	PY_ATTRIBUTE( maxAge )
	/*~ attribute PyLoft.threshold
	 *	This specifies how far the source points must move before a path point
	 *	is output.  It filters out minute movements causing rendering anomalies.
	 *
	 *	@type float. Default is 0.1
	 */
	PY_ATTRIBUTE( threshold )
PY_END_ATTRIBUTES()


/**
 *	Static Python factory method. This is declared through the factory
 *	declaration in the class definition.
 *
 *	@param	args	The list of parameters passed from Python. This should
 *					just be a string (textureName.)
 */
PyObject *PyLoft::pyNew( PyObject *args )
{
	BW_GUARD;
	char *nameFromArgs = "None";
	if (!PyArg_ParseTuple( args, "|s", &nameFromArgs ) )
	{
		PyErr_SetString( PyExc_TypeError, "PyLoft() expects "
			"an optional texture name string" );
		return NULL;
	}

	PyLoft * apr = new PyLoft();
	if ( _stricmp(nameFromArgs,"None") )
		apr->textureName( BW::string( nameFromArgs ) );

	return apr;
}

BW_END_NAMESPACE

// py_loft.cpp
