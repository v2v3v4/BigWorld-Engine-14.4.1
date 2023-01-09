#ifndef LIGHTING_PLAN_H_
#define LIGHTING_PLAN_H_

#pragma warning( disable:4786 )
#include <Classes.hpp>
#include <Controls.hpp>
#include "..\flatstyle\TFlatComboBoxUnit.hpp"
#include "cstdmf/bw_vector.hpp"
#include "moo/light_container.hpp"
#include "moo/moo_math.hpp"
#include "resmgr/datasection.hpp"
class Chunk;

/**
 *	This class represents the set of lights
 *	defined in a current sub-space.
 *
 *	This class can be saved/loaded from .MVL files
 */
class LightingPlan
{
public:

	LightingPlan() :
	  current_(0)
	{
	}

	enum LightType
	{
		LIGHT_NONE = -1,
		LIGHT_AMBIENT,
		LIGHT_DIRECTIONAL,
		LIGHT_SPOT,
		LIGHT_OMNI
	};

	/**
	 *	This class represents an editable light that can be
	 *	easily switched between lighting types.
	 */
	class VirtualLight
	{
	public:
		VirtualLight():
			index_(0),
			type_( LIGHT_AMBIENT ),
			colour_( 0xffffffff ),
			transform_( Matrix::identity ),
			inner_( 0.f ),
			outer_( 10.f ),
			angle_( 30.f ),
			enabled_( true ),
            visible_( true )
		{
		};

		bool	load( DataSectionPtr spSection );
		bool	save( DataSectionPtr spSection );
        void	draw();
        void	drawOmni();
        void	drawSpot();
        void	drawDirectional();

		int index_;
		LightType type_;
		Moo::Colour colour_;
		Matrix transform_;
		float inner_;
		float outer_;
		float angle_;
		bool enabled_;
        bool visible_;
	};

	typedef BW::vector<VirtualLight>	VirtualLights;

    virtual bool load( const BW::string & filename );
    virtual bool save( const BW::string & filename );
    virtual void draw();

	int		current();
	void	current( int index );

	void	add();
	void	add( const VirtualLight& vLight );
	void	del();

    uint32  nLights() const     { return lights_.size(); }

	//these methods work on the current light
	const LightType type() const		{ return light().type_; }
	void	type( LightType t )	{ light().type_ = t; }

	Moo::Colour colour() const	{ return light().colour_; }
	void	colour( const Moo::Colour& c )	{ light().colour_ = c; }

	const Matrix& transform() const	{ return light().transform_; }
	void  	transform( const Matrix& t )	{ light().transform_ = t; }

	float	inner() const		{ return light().inner_; }
	void	inner( float i )	{ light().inner_ = i; }

	float	outer() const		{ return light().outer_; }
	void	outer( float o )	{ light().outer_ = o; }

	float	angle() const		{ return light().angle_; }
	void	angle( float a )	{ light().angle_ = a; }

	bool	enabled() const		{ return light().enabled_; }
	void	enabled( bool state )	{ light().enabled_ = state; }

    bool	visible() const		{ return light().visible_; }
	void	visible( bool state )	{ light().visible_ = state; }

    const VirtualLights& lights() const	{ return lights_; }
	VirtualLights& lights()	{ return lights_; }

protected:
	const VirtualLight&	light() const { return (current() != -1 ? lights_[current()] : badLight_); }
    VirtualLight&	light() 		  { return (current() != -1 ? lights_[current()] : badLight_); }
    void			reIndex();

    //Plan management
	VirtualLights	lights_;
	VirtualLight	badLight_;

private:
	int		current_;
};


/**
 *	This class defines a lighting plan for chunks.
 */
class ChunkLightingPlan : public LightingPlan
{
public:
	void	to( Chunk& chunk );
	void	from( const Chunk& chunk );
};


/**
 *	This class defines a lighting plan for light containers.
 */
class LightContainerLightingPlan : public LightingPlan
{
public:
	void	to( Moo::LightContainerPtr plc );
	void	from( const Moo::LightContainerPtr plc );
};


/**
 * 	This class defines a lighting plan for TFlatComboBox
 */
class TFlatComboBoxLightingPlan : public LightingPlan
{
public:
	void	to( TFlatComboBox& cmb );
    void	from( LightingPlan& plan );
};

#endif
