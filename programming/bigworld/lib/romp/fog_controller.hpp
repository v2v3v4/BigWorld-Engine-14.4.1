
#ifndef FOG_CONTROLLER_HPP
#define FOG_CONTROLLER_HPP

#include <iostream>
#include "moo/moo_math.hpp"
#include "moo/material.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class manages the fog for the application.
 *
 *	The fog always extends to the far plane.
 *	The fog can be brought in.
 *	The current fog settings can be queried.
 *
 *	You can add fog emitters at positions in the world,
 *	and setup standard parameters.
 */
class FogController
{
public:
	static FogController & instance();

	/**
	 * TODO: to be documented.
	 */
	class Emitter
	{
	public:
		Emitter()
			:position_(0,0,0),
			 colour_(0xffff8080),	//gross pink, just so you know it's unitialised
			 maxMultiplier_(1.f),
			 nearMultiplier_(0.15f),
			 id_(-1),
			 localised_( true ),
			 falloffRange_(0),		//BC: added initialisation
			 innerRadiusSquared_(0),
			 innerRadius_(0),
			 outerRadiusSquared_(0)
		{
		};

		Vector3			position_;
		uint32			colour_;
		float			maxMultiplier_;
		float			nearMultiplier_;
		mutable int		id_;
		bool			localised_;

		void innerRadius( float radius )
		{
			innerRadius_ = radius;
			innerRadiusSquared_ = radius * radius;
			falloffRange_ = ( sqrtf( outerRadiusSquared_ ) - radius );
		}

		float innerRadius( void ) const
		{
			return innerRadius_;
		}

		void outerRadius ( float radius )
		{
			outerRadiusSquared_ = radius * radius;
			falloffRange_ = ( radius - innerRadius_ );
		}

		float outerRadiusSquared( void ) const
		{
			return outerRadiusSquared_;
		}

		float innerRadiusSquared( void ) const
		{
			return innerRadiusSquared_;
		}

		float falloffRange( void ) const
		{
			return falloffRange_;
		}

		bool localised( void ) const
		{
			return localised_;
		}

		void localised( bool state )
		{
			localised_ = state;
		}

		bool operator==(const Emitter& other)
		{
			return ( position_ == other.position_ );
		}

	private:
		float falloffRange_;
		float innerRadiusSquared_;
		float innerRadius_;
		float outerRadiusSquared_;
	};

	typedef BW::vector< Emitter >	Emitters;

	void	enable( bool state );
	bool	enable( void ) const;

	int		addEmitter( const Emitter & emitter );
	void	delEmitter( int emitterID );

	uint32	colour( void ) const;
	void	colour( uint32 c );

	const Vector4& v4colour( void ) const	{ return v4Colour_; }

	float	multiplier( void ) const;
	void	multiplier( float );

	float	totalFogContributions( void ) const	{ return multiplierTotal_; }

	float	nearMultiplier( void ) const;
	void	nearMultiplier( float );

	void	update( Emitter & emitter );

	///this method commits the current fog settings to the device
	void	updateFogParams();

	///this method returns the amount of fogging on non-foggy distant objects
	uint32	farObjectTFactor( void ) const;
	uint32	additiveFarObjectTFactor( void ) const;

	///this method must be called after the camera has moved.
	void	tick();

private:
	FogController();

	FogController(const FogController&);
	FogController& operator=(const FogController&);

	///standard fog values
	uint32	colour_;
	Vector4 v4Colour_;		//vector4 duplicate of colour_
	float	multiplier_;	//far clipping plane / multiplier
	float	nearMultiplier_;//-far clipping plane * multiplier
	float	multiplierTotal_;	//sum of all fog multipliers

	bool	enabled_;

	Emitters	emitters_;

	int		global_emitter_id_;

	uint32	farObjectTFactor_;			//for normal materials
	uint32	additiveFarObjectTFactor_;	//for additive materials

	friend std::ostream& operator<<(std::ostream&, const FogController&);
};

#ifdef CODE_INLINE
#include "fog_controller.ipp"
#endif

BW_END_NAMESPACE

#endif
/*fog_controller.hpp*/
