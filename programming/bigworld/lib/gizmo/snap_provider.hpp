#ifndef SNAP_PROVIDER_HPP
#define SNAP_PROVIDER_HPP

#include "math/vector3.hpp"
#include "math/matrix.hpp"

BW_BEGIN_NAMESPACE

class SnapProvider
{
public:
	enum SnapMode
	{
		SNAPMODE_XYZ,
		SNAPMODE_TERRAIN,
		SNAPMODE_OBSTACLE
	};
	virtual SnapMode snapMode( ) const { return SNAPMODE_XYZ; }
	/**
	 * Snap the absolute world position v, eg, if we're aligning objects to a
	 * grid
	 */
	virtual bool snapPosition( Vector3& v ) { return true; }
	virtual Vector3 snapNormal( const Vector3& v ) { return Vector3( 0, 1, 0 ); }

	/**
	 * Snap the position delta, eg, if we only want to move objects by
	 * multiples of x
	 */
	virtual void snapPositionDelta( Vector3& v ) { }

	virtual void snapAngles( Matrix& v ) { }
	virtual float angleSnapAmount() { return 0.f; }


	static SnapProvider* instance() { return ins(); }
	static void instance( SnapProvider* sp )
	{ 
		MF_ASSERT( sp );
		ins(sp);
	}

	static void fini()
	{
		if (s_internal_)
			bw_safe_delete(s_instance_);
		s_instance_ = NULL;
	}

private:
	static SnapProvider* ins( SnapProvider* instance = NULL )
	{
		if (instance == NULL)
		{
			if (s_instance_ == NULL)
			{
				s_internal_ = true;
				s_instance_ = new SnapProvider();
			}
		}
		else
		{
			if (s_instance_ && s_internal_)
				bw_safe_delete(s_instance_);
			s_instance_ = instance;
			s_internal_ = false;
		}
		return s_instance_;
	}
	static SnapProvider*		s_instance_;
	static bool					s_internal_;
};

BW_END_NAMESPACE
#endif	// SNAP_PROVIDER_HPP
