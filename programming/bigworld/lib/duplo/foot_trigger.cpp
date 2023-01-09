#include "pch.hpp"
#include "foot_trigger.hpp"
#include "foot_print_renderer.hpp"
#include "pymodel.hpp"
#include "fmodsound/py_sound.hpp"
#include "math/planeeq.hpp"
#include "romp/enviro_minder.hpp"
#include "particle/actions/source_psa.hpp"
#include "pyscript/py_callback.hpp"

#include "space/client_space.hpp"
#include "space/deprecated_space_helpers.hpp"

#include "physics2/collision_callback.hpp"
#include "physics2/collision_obstacle.hpp"
#include "physics2/material_kinds.hpp"
#include "physics2/worldtri.hpp"

#include "moo/vertex_formats.hpp"
#include "romp/flora.hpp"
#include "moo/custom_mesh.hpp"

#include "moo/render_context.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/main_loop_task.hpp"
#include "resmgr/auto_config.hpp"

#include "fmodsound/py_sound.hpp"
#include "fmodsound/py_sound_parameter.hpp"
#include <deque>

DECLARE_DEBUG_COMPONENT2( "Duplo", 2 )


BW_BEGIN_NAMESPACE

namespace { // anonymous

// Named Constants
const int32 NORMAL_COLOUR       = 0xff00ff00;
const int32 ARMED_COLOUR        = 0xffffff00;
const int32 TRIGGERED_COLOUR    = 0xffff0000;
const int32 NOT_TERRAIN_COLOUR  = 0Xff000000;
const int32 TOO_HIGH_COLOUR     = 0Xffff00ff;
const int32 UNDER_GROUND_COLOUR = 0Xffffffff;
const int32 THRESHOLD_COLOUR    = 0Xff0000ff;
const float REFRESH_MAX_TIME    = 1.0f;

} // namespace anonymous


// -----------------------------------------------------------------------------
// Section: FootPrints
// -----------------------------------------------------------------------------

/**
 *	This global method specifies the resources required by this file
 */
static BasicAutoConfig< float > s_upperBound( "environment/footprintThreshold", 0.035f );
#define MAX_NUM_VALUES 200

// -----------------------------------------------------------------------------
// Section: FootTriggerPlotter
// -----------------------------------------------------------------------------

/**
 *	Utility class for ploting a variable varying in time.
 *	TODO: make it more general accessible from
 *	other classes and also from Python.
 */
class FootTriggerPlotter : public MainLoopTask
{
public:
	FootTriggerPlotter();

	void addPoint( float dTime );
	void setData( int curveIndex, float value, int32 colour );

	virtual void tick( float dGameTime, float dRenderTime );
	virtual void draw();

private:
	struct PointData
	{
		PointData() :
			value_( 0.0f ),
			colour_( 0xffffffff ) {}

		float value_;
		int32 colour_;
	};

	typedef BW::vector< PointData >            PointDataVector;
	typedef std::pair< float, PointDataVector > FloatDataPair;
	typedef std::deque< FloatDataPair >         CurveDataDeque;

	CurveDataDeque values_;
	float          max_;
	float          accTime_;
	int            curveCount_;
};


/**
 *	Constructor.
 */
FootTriggerPlotter::FootTriggerPlotter() :
	values_(),
	max_( 1.0 ),
	accTime_( 0.0 ),
	curveCount_( 0 )
{
	BW_GUARD;
	MainLoopTasks::root().add( this, "Flora/FootPrintsPloter", ">App", NULL );
}


/**
 *	Adds a new (uninitalised) point to the curves. Use the setData
 *	method to actually set the curves data for this point.
 *
 *	@param	dTime	frame delta time
 */
void FootTriggerPlotter::addPoint( float dTime )
{
	BW_GUARD;
	this->values_.push_front( std::make_pair(
			dTime, PointDataVector( this->curveCount_ ) ) );

	if (this->values_.size() > MAX_NUM_VALUES)
	{
		this->values_.pop_back();
	}

	accTime_ += dTime;
	if (accTime_ > REFRESH_MAX_TIME)
	{
		max_ = 1.0;
		CurveDataDeque::const_iterator curveIt  = this->values_.begin();
		CurveDataDeque::const_iterator curveEnd = this->values_.end();
		while (curveIt != curveEnd)
		{
			PointDataVector::const_iterator dataIt = curveIt->second.begin();
			PointDataVector::const_iterator dataEnd = curveIt->second.end();
			while (dataIt != dataEnd)
			{
				max_ = std::max( max_, dataIt->value_ );
				++dataIt;
			}
			++curveIt;
		}
		accTime_ = 0;
	}
}


/**
 *	Sets the data on one the plot's curve, for the last added point.
 *
 *	@param	curveIndex	index of the curve whose data is being set
 *	@param	value		curve's value for the last added point
 *	@param	colour		curve's colour for the last added point
 */
void FootTriggerPlotter::setData( int curveIndex, float value, int32 colour )
{
	BW_GUARD;
	if ( curveIndex >= this->curveCount_ )
	{
		this->values_.clear();
		this->curveCount_ = curveIndex + 1;
		this->addPoint( 0 );
	}

	this->values_.front().second[ curveIndex ].value_  = value;
	this->values_.front().second[ curveIndex ].colour_ = colour;
}


/**
 *	MainLoopTask tick method.
 */
void FootTriggerPlotter::tick( float dGameTime, float /* dRenderTime */ )
{
	BW_GUARD;
	if (FootTrigger::plotEnabled_)
	{
		this->addPoint( dGameTime );
	}
}


/**
 *	MainLoopTask draw method. Plots the curves.
 */
void FootTriggerPlotter::draw()
{
	BW_GUARD;
	if (!FootTrigger::plotEnabled_)
	{
		return;
	}

	// these will be used to
	// build and render the curves
	typedef BW::vector< Moo::VertexTL > VertexBuffer;
	typedef BW::vector< VertexBuffer > BufferVector;
	static VertexBuffer vertexBuffer;
	static BufferVector bufferVector;

	Moo::VertexTL vertex;
	vertex.pos_.z = 0;
	vertex.pos_.w = 1;

	// one buffer for each curve
	float doneTime = 0;
	bufferVector.clear();
	bufferVector.resize( this->curveCount_ );

	// for each point in the curves
	CurveDataDeque::const_iterator dataIt = this->values_.begin();
	CurveDataDeque::const_iterator dataEnd = this->values_.end();
	while (dataIt != dataEnd)
	{
		// for each curve
		for (int f=0; f<this->curveCount_; ++f)
		{
			// add vertex
			vertex.pos_.x = float( doneTime / 2.0 );
			vertex.pos_.x *= Moo::rc().screenWidth();
			vertex.pos_.x = Moo::rc().screenWidth() - vertex.pos_.x;

			vertex.pos_.y = float( dataIt->second[f].value_ / max_ );
			vertex.pos_.y *= -Moo::rc().screenHeight() / 2;
			vertex.pos_.y += Moo::rc().screenHeight() / 2;
			vertex.colour_ = dataIt->second[f].colour_;

			bufferVector[f].push_back( vertex );
		}
		doneTime += dataIt->first;
		++dataIt;
	}

	if (bufferVector.size() > 0)
	{
		// setup render state
		Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );

		Matrix origView = Moo::rc().view();
		Matrix origProj = Moo::rc().projection();
		Moo::rc().view( Matrix::identity );
		Moo::rc().projection( Matrix::identity );
		Moo::rc().updateViewTransforms();

		Moo::rc().device()->SetTransform( D3DTS_WORLD, &Matrix::identity );
		Moo::rc().device()->SetTransform( D3DTS_VIEW, &Matrix::identity );
		Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Matrix::identity );
		Moo::rc().setVertexShader( NULL );

		Moo::rc().push();
		Moo::rc().world(Matrix::identity);

		Moo::Material::setVertexColour();

		Moo::rc().setRenderState( D3DRS_ZENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );

		Moo::rc().setVertexShader( NULL );
		Moo::rc().setFVF( D3DFVF_XYZRHW | D3DFVF_DIFFUSE );

		// draw all buffers
		BufferVector::const_iterator buffIt = bufferVector.begin();
		BufferVector::const_iterator buffEnd = bufferVector.end();
		while (buffIt != buffEnd)
		{
			Moo::rc().drawPrimitiveUP( 
				D3DPT_LINESTRIP, 
				static_cast<UINT>(buffIt->size()-1),
				&(buffIt->front()), 
				static_cast<UINT>(sizeof( Moo::VertexTL )));
			++buffIt;
		}

		// restore render state
		Moo::rc().pop();

		Moo::rc().setRenderState( D3DRS_LIGHTING, TRUE );
		Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );

		Moo::rc().view( origView );
		Moo::rc().projection( origProj );
		Moo::rc().updateViewTransforms();
	}
}

namespace {
FootTriggerPlotter f_plotter;
}

// -----------------------------------------------------------------------------
// Section: FootTrigger
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( FootTrigger )

PY_BEGIN_METHODS( FootTrigger )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( FootTrigger )
	/*~ attribute FootTrigger.maxLod
	 *
	 *	If the FootTrigger is greater than maxLod away from the camera, it will
	 *	not be processed or drawn.
	 *
	 *	@type Float
	 */
	PY_ATTRIBUTE( maxLod )
	/*~ attribute FootTrigger.odd
	 *
	 *	This attribute is used to distinguish between different FootTriggers.
	 *	It is passed to the callback function as an argument.
	 *
	 *	Footstep triggers were designed for bipedal use, and it was envisaged
	 *	that 0, means that this is a right foot, anything else means a
	 *	left foot.  However, it would be possible to interpret it differently.
	 *
	 *	@type Integer
	 */
	PY_ATTRIBUTE( odd )
	/*~ attribute FootTrigger.dustSource
	 *
	 *	The dustSource attribute is a SourcePSA (The particle system action
	 *	responsible for creating particles), that will have its SourcePSA.force
	 *	method called when the foot strikes the ground.
	 *
	 *	@type SourcePSA
	 */
	PY_ATTRIBUTE( dustSource )
	/*~ attribute FootTrigger.footstepCallback
	 *
	 *	A callback function object, which is called when the foot strikes the
	 *	ground.  The function is supplied with two arguments, the first is the
	 *	velocity that the model is moving at, the second is a flag which is 0
	 *	if it is a right foot, 1 if it is a left foot.
	 *
	 *	@type Callable Object
	 */
	PY_ATTRIBUTE( footstepCallback )
PY_END_ATTRIBUTES()

/*~ function BigWorld.FootTrigger
 *
 *	This function creates a new FootTrigger object.  This can be attached
 *	to PyModelNodes to test for them hitting the ground, and calling a
 *	function when they do.
 *
 *	@param		Boolean	left [False] foot or right [True] foot
 *	@param		string	[optional] sound trigger prefix
 *	@return		a new FootTrigger object.
 */
PY_FACTORY_NAMED( FootTrigger, "FootTrigger", BigWorld )


/**
 *	Constructor
 */
 FootTrigger::FootTrigger( bool odd, BW::string soundTagPrefix, PyTypeObject * pType ) :
	PyAttachment( pType ),
	lod_( 0.0f ),
	maxLod_( 150.f ),
	rest_( -1.f ),
	armed_( false ),
	odd_( odd ),
	lastPosition_( Vector3::zero() ),
	dTime_( 0.f ),
	plotIndex_( 0 ),
	soundTagPrefix_( soundTagPrefix ),
#if FMOD_SUPPORT
	pSound_( NULL ),
#endif
	lastMaterialKind_( 0 )
{
	BW_GUARD;
	FootTrigger::instanceCounter_++;
	this->plotIndex_ = FootTrigger::instanceCounter_;
}


/**
 *	This class finds the closest triangle hit and records it
 */
class ClosestTriangleFT : public CollisionCallback
{
private:
	virtual int operator()( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float dist )
	{
		hit_ = WorldTriangle(
			obstacle.transform().applyPoint( triangle.v0() ),
			obstacle.transform().applyPoint( triangle.v1() ),
			obstacle.transform().applyPoint( triangle.v2() ),
			triangle.flags() );

		return COLLIDE_BEFORE;
	}

public:
	WorldTriangle	hit_;
};


/**
 *	The tick method
 */
void FootTrigger::tick( float dTime )
{
	dTime_ += dTime;
}


/**
 *	Check if this if visible or lodded out.
 *	@return true if this object is visible, false if lodded out.
 */
bool FootTrigger::isLodVisible() const
{
	BW_GUARD;

	// Infinite lod
	if (maxLod_ <= 0.0f)
	{
		return true;
	}

	if (lod_ <= maxLod_)
	{
		return true;
	}
	
	return false;
}


/**
 *	Set the LOD for this frame.
 *	@param worldTransform the world transform for this object.
 *	@param lod the lod for drawing.
 */
void FootTrigger::updateAnimations( const Matrix & worldTransform,
	float lod )
{
	BW_GUARD;
	lod_ = lod;
}


/**
 *	The draw method
 */
void FootTrigger::draw( Moo::DrawContext& drawContext, const Matrix & worldTransform )
{
	BW_GUARD;

	// See if we want to draw
	if (!this->isLodVisible())
	{
		return;
	}

	if (dTime_ <= 0.f)
	{
		return;
	}

	ClientSpacePtr pSpace = DeprecatedSpaceHelpers::cameraSpace();
	if (!pSpace)
	{
		return;
	}

	Matrix & worldMatrix = Moo::rc().world();

	// If this is the first frame after we've been attached,
	// store the current height as the rest height.
	// TODO: Do something less dodgy. Ideally dressInDefault, but
	// we ain't got the supermodel no more.
	if (rest_ == -1.f)
	{
		rest_ = worldTransform.applyToOrigin().y -
			worldMatrix.applyToOrigin().y;
	}

	const Matrix & worldFootMx = worldTransform;
	Vector3 worldFoot = worldFootMx.applyToOrigin();
	float localFootHeight =
		worldFoot.y - rest_ - worldMatrix.applyToOrigin().y;

	if (this->plotIndex_ == 1)
	{
		// only the odd foot will plot the threshold value
		f_plotter.setData( 0, s_upperBound.value(), THRESHOLD_COLOUR );
	}

	if (localFootHeight >= s_upperBound.value())
	{
		// above threshold
		f_plotter.setData( this->plotIndex_, localFootHeight, ARMED_COLOUR );
		armed_ = true;
		return;
	}

	if (!armed_)
	{
		// not armed
		f_plotter.setData( this->plotIndex_, localFootHeight, NORMAL_COLOUR );
		return;
	}

	// ok, here we go off then!
	armed_ = false;

	/* Take whatever action is desired for a foot press here */

	// see if it's on the ground
	ClosestTriangleFT ct;
	float dist = pSpace->collide(
		worldFoot + Vector3(0,0.5f,0), worldFoot - Vector3(0,0.5f,0), ct );
	if (dist >= 0.f)
	{
		Vector3 groundFoot( worldFoot.x, worldFoot.y+0.5f-dist, worldFoot.z );

		// no more than 40cm off the ground
		if (fabs(worldFoot.y - groundFoot.y) < 0.40f)
		{
			bool onTerrain = !!(ct.hit_.flags() & TRIANGLE_TERRAIN);

			Vector3 velocity =
				(worldMatrix.applyToOrigin() - lastPosition_) / dTime_;

			float vel = velocity.length();
			lastPosition_ = worldMatrix.applyToOrigin();
			dTime_ = 0.f;

			uint32 material = ct.hit_.materialKind();

			if (MaterialKinds::instance().isValid( material ))
			{
				// If the material kind has changed, then we need to get a new
				// sound event for it
				if ((material != lastMaterialKind_ && PyModel::pCurrent())
#if FMOD_SUPPORT
					//pSound_ might be stolen and removed from pyModel::sounds_ already,
					//in that case we re-get a new sound event as well
					|| (pSound_ && !pSound_->isValid())
#endif
					)
				{
#if FMOD_SUPPORT
					if (pSound_)
					{
						//remove the old pSound_ from the PyModel::sounds_ list
						if (pSound_->isValid())
						{
							pSound_->stop();
							PyModel::pCurrent()->detachSound( pSound_.get() );
						}

						pSound_ = NULL;
					}

					BW::string msound = MaterialKinds::instance().userString(
						material, "sound" );

					if (!msound.empty())
					{
						// Assemble the name of the event
						BW::string path = soundTagPrefix_;
						path.push_back( '/' );
						path.append( msound );

						PyObject *pSound = PyModel::pCurrent()->getSound( path );
						if (pSound != SoundManager::pyError())
						{
							pSound_ = PySoundPtr( (PySound*)pSound,
								PySoundPtr::STEAL_REFERENCE );
						}
					}
#endif // FMOD_SUPPORT
					lastMaterialKind_ = material;
				}

#if FMOD_SUPPORT
				// If we have a valid sound event to play, do it.
				if (pSound_ != NULL)
				{
					PyObject *p = NULL;

					p = pSound_->param( "Speed" );
					if (p != SoundManager::pyError())
					{
						PySoundParameter *pSpeed = (PySoundParameter*)p;

						if (vel < 4.0)
							pSpeed->value( 1.0 );
						else if (vel < 8.0)
							pSpeed->value( 2.0 );
						else
							pSpeed->value( 3.0 );

						Py_DECREF( pSpeed );
					}

					p = pSound_->param( "LeftRight" );
					if (p != SoundManager::pyError())
					{
						PySoundParameter *pLeftRight = (PySoundParameter*)p;
						pLeftRight->value( odd_ ? (float)0.0 : (float)1.0 );
						Py_DECREF( pLeftRight );
					}

					pSound_->play();
				}
#endif // FMOD_SUPPORT
			}

			// Issue warning message for invalid material kinds
			else
			{
				WARNING_MSG( "FootTrigger::draw: "
					"Stepped on invalid material kind %d at (%.2f, %.2f, %.2f)\n",
					material,
					worldFoot.x, worldFoot.y, worldFoot.z );
			}

#ifndef EDITOR_ENABLED
			// call the callback if we have one ... later
			if (footstepCallback_)
			{
				Py_INCREF( &*footstepCallback_ );
				Script::callNextFrame(
					&*footstepCallback_,
					Py_BuildValue( "(fi)", vel, int(odd_) ),
					"FootTrigger footstep callback: " );
			}
#endif

			// and leave a footprint, if we're on the terrain
			if (onTerrain)
			{
				// trigerring a foot print
				f_plotter.setData( this->plotIndex_, localFootHeight, TRIGGERED_COLOUR );

				PlaneEq	peq( ct.hit_.v0(), ct.hit_.v1(), ct.hit_.v2() );

				static const int c_numVertices = 4;
				Vector3 footPrint[c_numVertices];
				for (int j=0; j<c_numVertices; j++)
				{
					Vector3 footCorner = groundFoot +
						worldFootMx.applyVector( Vector3(
							(j&2)?0.3f:0, (j&1)?0.14f:-0.14f, 0 ) );

					footPrint[j] = Vector3( footCorner.x,
						peq.y( footCorner.x, footCorner.z ), footCorner.z );
				}

				EnviroMinder & envMinder = pSpace->enviro();
				envMinder.footPrintRenderer()->addFootPrint(&footPrint[0]);

				// Do dust clouds.
				if (dustSource_)// && (strcmp(terrainName, "gravel")==0) )
				{
					float velSQ = velocity.lengthSquared();
					int numParticles;
					if ( velSQ > 8.0f * 8.0f )			numParticles = 2;
					else if ( velSQ > 4.0f * 4.0f )		numParticles = 1;
					else								numParticles = 0;
					if (numParticles > 0)
						dustSource_->pAction()->force( numParticles );
				}
			}
			else
			{
				// not walking on terrain
				f_plotter.setData( this->plotIndex_, localFootHeight, NOT_TERRAIN_COLOUR );
			}
		}
		else
		{
			// foot is too high from the ground
			f_plotter.setData( this->plotIndex_, localFootHeight, TOO_HIGH_COLOUR );
		}
	}
	else
	{
		// foot has gone underground
		f_plotter.setData( this->plotIndex_, localFootHeight, UNDER_GROUND_COLOUR );
	}
}


/**
 *	Detach method override
 */
void FootTrigger::detach()
{
	BW_GUARD;
	this->PyAttachment::detach();
	rest_ = -1.f;
#if FMOD_SUPPORT
	pSound_ = NULL;
#endif
}


int  FootTrigger::instanceCounter_ = 0;
bool FootTrigger::plotEnabled_    = false;

BW_END_NAMESPACE

// foot_trigger.cpp
