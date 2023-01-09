#include "pch.hpp"
#include "genprop_gizmoviews.hpp"

#include "general_properties.hpp"

#include "position_gizmo.hpp"
#include "rotation_gizmo.hpp"
#include "scale_gizmo.hpp"
#include "radius_gizmo.hpp"
#include "angle_gizmo.hpp"
#include "input/input.hpp"
#include "cstdmf/debug.hpp"
#include "current_general_properties.hpp"
#include "property_scaler_helper.hpp"

DECLARE_DEBUG_COMPONENT2( "Gizmo", 0 )

BW_BEGIN_NAMESPACE

int genprop_gizmoviews_token;

/**
 *	This is a demonstration view class that puts a gizmo in the world
 *	whenever a position property is elected
 */
class GenPosPropAxes : public GeneralProperty::View
{
public:
	GenPosPropAxes( GenPositionProperty & prop ) : prop_( prop )
	{		
	}

	~GenPosPropAxes()
	{	
	}

	virtual void elect()
	{
		BW_GUARD;

		if (s_count_++ == 0)
		{
			s_pGizmo_ = new PositionGizmo( MODIFIER_SHIFT | MODIFIER_CTRL |MODIFIER_ALT );
			GizmoManager::instance().addGizmo( s_pGizmo_ );
		}
	}

	virtual void expel()
	{
		BW_GUARD;

		if (--s_count_ == 0)
		{
			GizmoManager::instance().removeGizmo( s_pGizmo_ );
			s_pGizmo_ = NULL;
		}
	}

	virtual void select()
	{
		BW_GUARD;

		if (s_pGizmo_ != NULL)
		{
			GizmoSetPtr set = new GizmoSet();
			set->add( s_pGizmo_ );
			GizmoManager::instance().forceGizmoSet( set );
		}
	}

	static GeneralProperty::View * create( GenPositionProperty & prop )
	{
		BW_GUARD;

		return new GenPosPropAxes( prop );
	}

private:
	GenPositionProperty	& prop_;
	static GizmoPtr		s_pGizmo_;
	static int			s_count_;

	static struct ViewEnroller
	{
		ViewEnroller()
		{
			BW_GUARD;

			GenPositionProperty_registerViewFactory(
				GizmoViewKind::kindID(), &create );
		}
	}	s_viewEnroller;
};

GenPosPropAxes::ViewEnroller GenPosPropAxes::s_viewEnroller;
int GenPosPropAxes::s_count_ = 0;
GizmoPtr GenPosPropAxes::s_pGizmo_ = NULL;


/**
 *	This is a demonstration view class that puts a gizmo in the world
 *	whenever a rotation property is elected
 */
class GenRotPropDiscs : public GeneralProperty::View
{
public:
	GenRotPropDiscs( GenRotationProperty & prop ) : prop_( prop )
	{		
	}

	~GenRotPropDiscs()
	{		
	}

	virtual void elect()
	{
		BW_GUARD;

		if (s_count_++ == 0)
		{
			if ( prop_.allowGizmo() )
			{
				s_pGizmo_ = new RotationGizmo( prop_.pMatrix(), MODIFIER_SHIFT );
				GizmoManager::instance().addGizmo( s_pGizmo_ );
			}
		}
	}

	virtual void expel()
	{
		BW_GUARD;

		if (--s_count_ == 0)
		{
			GizmoManager::instance().removeGizmo( s_pGizmo_ );
			s_pGizmo_ = NULL;
		}
	}

	virtual void select()
	{
		BW_GUARD;

		if (s_pGizmo_ != NULL)
		{
			GizmoSetPtr set = new GizmoSet();
			if (s_pGizmo_ != NULL)
				set->add( s_pGizmo_ );
			if (set->vector().size() != 0)
				GizmoManager::instance().forceGizmoSet( set );
		}
	}

	static GeneralProperty::View * create( GenRotationProperty & prop )
	{
		BW_GUARD;

		return new GenRotPropDiscs( prop );
	}

private:
	GenRotationProperty	& prop_;
	static GizmoPtr		s_pGizmo_;
	static int			s_count_;

	static struct ViewEnroller
	{
		ViewEnroller()
		{
			BW_GUARD;

			GenRotationProperty_registerViewFactory(
				GizmoViewKind::kindID(), &create );
		}
	}	s_viewEnroller;
};

GenRotPropDiscs::ViewEnroller GenRotPropDiscs::s_viewEnroller;
int GenRotPropDiscs::s_count_ = 0;
GizmoPtr GenRotPropDiscs::s_pGizmo_ = NULL;



/**
 * Applies a uniform scale to all current scale properties.
 *
 * Doesn't try to extract the current uniform scale, simply adds another one
 * on top.
 */
class UniformScaleFloatProxy : public FloatProxy
{
public:
	UniformScaleFloatProxy() : 
	  curScale_( 1.f ), 
	  newTransformOnSet_( true )
	{
	}

	virtual float get() const
	{
		return curScale_;
	}

	/**
	 *	This method sets all the current scale properties
	 *	@param f the scale factor
	 *	@param transient true if there are more moves to come
	 *		false if this is the final move
	 */
	virtual void set( float f, bool transient, bool addBarrier = true )
	{
		BW_GUARD;

		// We are starting over again
		// so reset the scales 
		if (newTransformOnSet_)
		{
			newTransformOnSet_ = false;

			// Initialise the scalerhelper, since we are
			// using uniform scale, the axis don't matter
			// and we use an identity matrix as the gizmo
			// transform
			scalerHelper_.init( Matrix::identity );
		}		

		// Don't allow 0 or negative scales
		if (f < 0.001f)
			f = 0.001f;

		// Store the current scale
		curScale_ = f;

		// Update the current scale properties
		scalerHelper_.updateScale( Vector3( f, f, f ) );

		// if transient == false this is the final move and
		// we finalise the scales
		if (!transient)
		{
			scalerHelper_.fini( true );
			curScale_ = 1.f;
			newTransformOnSet_ = true;
		}
	}

private:
	float					curScale_;
	bool					newTransformOnSet_;
	PropertyScalerHelper	scalerHelper_;
};



/** Exposes the current centre position of all the position properties */
class CurrentPositionMatrixProxy : public MatrixProxy
{
public:
	virtual void getMatrix( Matrix & m, bool world = true )
	{
		BW_GUARD;

		MF_ASSERT( world );

		m.setTranslate( CurrentPositionProperties::averageOrigin() );
	}

	virtual void getMatrixContext( Matrix & m )
	{
		MF_ASSERT( 0 );
	}

	virtual void getMatrixContextInverse( Matrix & m )
	{
		MF_ASSERT( 0 );
	}

	virtual bool setMatrix( const Matrix & m )
	{
		MF_ASSERT( 0 );
		return false;
	}

	virtual void recordState()
	{
		MF_ASSERT( 0 );
	}

	virtual bool commitState( bool revertToRecord = false, bool addUndoBarrier = true )
	{
		MF_ASSERT( 0 );
		return true;
	}

	virtual bool hasChanged()
	{
		MF_ASSERT( 0 );
		return false;
	}
};


/**
 *	This is a demonstration view class that puts a gizmo in the world
 *	whenever a scale property is elected
 */
class GenScalePropAxes : public GeneralProperty::View
{
public:
	GenScalePropAxes( GenScaleProperty & prop ) : prop_( prop )
	{		
	}

	~GenScalePropAxes()
	{		
	}

	virtual void elect()
	{
		BW_GUARD;

		if (s_count_++ == 0)
		{
			static bool addedWatcher = false;
			static float scaleSpeedFactor = 0.25f;
			static float uniformScaleSpeedFactor = 0.1f;

			if ( !addedWatcher )
			{
				MF_WATCH( "App/scaleSpeed", scaleSpeedFactor, Watcher::WT_READ_WRITE, "Speed multiplier for using the non-uniform scale gizmo." );
				MF_WATCH( "App/uniformScaleSpeed", uniformScaleSpeedFactor, Watcher::WT_READ_WRITE, "Speed multiplier for using the uniform scale gizmo." );
				addedWatcher = true;
			}			

			if (prop_.allowNonUniformScale())
			{
				s_pGizmo_ = new ScaleGizmo(NULL,MODIFIER_ALT,scaleSpeedFactor);
				GizmoManager::instance().addGizmo( s_pGizmo_ );
			}
			if (prop_.allowUniformScale())
			{
				s_pUniformScaleGizmo_ = new RadiusGizmo( new UniformScaleFloatProxy(),
					new CurrentPositionMatrixProxy(),
					"Uniform Scale",
					0xffffffff,
					8.f,
					MODIFIER_ALT,
					uniformScaleSpeedFactor,
					true,
					NULL,
					NULL,
					RadiusGizmo::SHOW_SPHERE_NEVER,
					&SimpleFormatter::s_def );	
				GizmoManager::instance().addGizmo( s_pUniformScaleGizmo_ );
			}			
		}
	}

	virtual void expel()
	{
		BW_GUARD;

		if (--s_count_ == 0)
		{
			GizmoManager::instance().removeGizmo( s_pUniformScaleGizmo_ );
			GizmoManager::instance().removeGizmo( s_pGizmo_ );
			s_pGizmo_ = NULL;
			s_pUniformScaleGizmo_ = NULL;
		}
	}

	virtual void select()
	{
		BW_GUARD;

		GizmoSetPtr set = new GizmoSet();
		if (s_pGizmo_ != NULL)
			set->add( s_pGizmo_ );
		if (s_pUniformScaleGizmo_ != NULL)
			set->add( s_pUniformScaleGizmo_ );
		if (set->vector().size() != 0)
			GizmoManager::instance().forceGizmoSet( set );
	}

	static GeneralProperty::View * create( GenScaleProperty & prop )
	{
		BW_GUARD;

		return new GenScalePropAxes( prop );
	}

private:
	GenScaleProperty	& prop_;
	static GizmoPtr		s_pGizmo_;
	static GizmoPtr		s_pUniformScaleGizmo_;
	static int			s_count_;

	static struct ViewEnroller
	{
		ViewEnroller()
		{
			BW_GUARD;

			GenScaleProperty_registerViewFactory(
				GizmoViewKind::kindID(), &create );
		}
	}	s_viewEnroller;
};

GenScalePropAxes::ViewEnroller GenScalePropAxes::s_viewEnroller;
int GenScalePropAxes::s_count_ = 0;
GizmoPtr GenScalePropAxes::s_pGizmo_ = NULL;
GizmoPtr GenScalePropAxes::s_pUniformScaleGizmo_ = NULL;



/**
 *	This is a view class that puts a sphere gizmo in the world
 *	whenever a radius property is elected
 */
class GenRadiusPropDisc : public GeneralProperty::View
{
public:
	GenRadiusPropDisc( GenRadiusProperty & prop,
		uint32 editableColour,
		float editableRadius ) :
	prop_( prop )
	{
		BW_GUARD;

		pGizmo_ = new RadiusGizmo(
			prop.pFloat(),
			prop.pCenter(),
			prop.name().c_str(),
			editableColour,
			editableRadius,
			MODIFIER_ALT );
	}

	~GenRadiusPropDisc()
	{
		pGizmo_ = NULL;
	}

	virtual void elect()
	{
		BW_GUARD;

		GizmoManager::instance().addGizmo( pGizmo_ );
	}

	virtual void expel()
	{
		BW_GUARD;

		GizmoManager::instance().removeGizmo( pGizmo_ );
	}

	virtual void select()
	{
		BW_GUARD;

		if (pGizmo_ != NULL)
		{
			GizmoSetPtr set = new GizmoSet();
			set->add( pGizmo_ );
			GizmoManager::instance().forceGizmoSet( set );
		}
	}

	static GeneralProperty::View * create( GenRadiusProperty & prop )
	{
		BW_GUARD;

		return new GenRadiusPropDisc( prop, prop.widgetColour(), prop.widgetRadius() );
	}

private:
	GenRadiusProperty	& prop_;
	GizmoPtr			pGizmo_;

	static struct ViewEnroller
	{
		ViewEnroller()
		{
			BW_GUARD;

			GenRadiusProperty_registerViewFactory(
				GizmoViewKind::kindID(), &create );
		}
	}	s_viewEnroller;
};

GenRadiusPropDisc::ViewEnroller GenRadiusPropDisc::s_viewEnroller;


/**
 *	This is a view class that puts a cone gizmo in the world
 *	whenever an angle property is elected
 */
class GenAnglePropCone : public GeneralProperty::View
{
public:
	GenAnglePropCone( AngleProperty & prop,
		uint32 editableColour,
		float editableRadius ) :
	prop_( prop )
	{
		BW_GUARD;

		pGizmo_ = new AngleGizmo(
			prop.pCenter(),
			prop.pFloat(),
			MODIFIER_ALT | MODIFIER_SHIFT );
	}

	~GenAnglePropCone()
	{
		pGizmo_ = NULL;
	}

	virtual void elect()
	{
		BW_GUARD;

		GizmoManager::instance().addGizmo( pGizmo_ );
	}

	virtual void expel()
	{
		BW_GUARD;

		GizmoManager::instance().removeGizmo( pGizmo_ );
	}

	virtual void select()
	{
		BW_GUARD;

		if (pGizmo_ != NULL)
		{
			GizmoSetPtr set = new GizmoSet();
			set->add( pGizmo_ );
			GizmoManager::instance().forceGizmoSet( set );
		}
	}

	static GeneralProperty::View * create( AngleProperty & prop )
	{
		BW_GUARD;

		return new GenAnglePropCone( prop, 0xffffff00, 2.f );
	}

private:
	AngleProperty&		prop_;
	GizmoPtr			pGizmo_;

	static struct ViewEnroller
	{
		ViewEnroller()
		{
			AngleProperty_registerViewFactory(
				GizmoViewKind::kindID(), &create );
		}
	}	s_viewEnroller;
};

GenAnglePropCone::ViewEnroller GenAnglePropCone::s_viewEnroller;

BW_END_NAMESPACE
// genprop_gizmoviews.cpp
