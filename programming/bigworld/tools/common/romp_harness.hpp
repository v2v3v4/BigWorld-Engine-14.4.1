//---------------------------------------------------------------------------

#ifndef romp_test_harnessH
#define romp_test_harnessH

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "resmgr/datasection.hpp"

BW_BEGIN_NAMESPACE

class EnviroMinder;
class TimeOfDay;

namespace Moo
{
	class DrawContext;
}

/*~ class NoModule.RompHarness
 *	@components{ tools }
 *
 *	This class keeps track of the space's environment, such as the current time,
 *	time scale, fog, etc.
 */
class RompHarness : public PyObjectPlus
{
	Py_Header( RompHarness, PyObjectPlus )
public:
	RompHarness( PyTypeObject * pType = &s_type_ );
	~RompHarness();

    bool	init();
	void	changeSpace();
    void	initWater( DataSectionPtr pProject );

    void	setTime( float t );
	void	setSecondsPerHour( float sph );
	void	fogEnable( bool state );
	bool	fogEnable() const;

    void	update( float dTime, bool globalWeather );
    void	drawPreSceneStuff( bool sparkleCheck = false, bool renderEnvironment = true );
	void	drawSceneStuff( bool showWeather = true,
		bool showFlora = true, bool showFloraShadowing = false );
	void	drawDelayedSceneStuff( bool renderEnvironment = true );
    void	drawPostSceneStuff( Moo::DrawContext& drawContext, bool showWeather = true );
	void	drawPostProcessStuff();

    TimeOfDay*	timeOfDay() const;
	EnviroMinder& enviroMinder() const;

	//-------------------------------------------------
	//Python Interface
	//-------------------------------------------------

	//methods
	PY_METHOD_DECLARE( py_setTime )
	PY_METHOD_DECLARE( py_setSecondsPerHour )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, fogEnable, fogEnable )

protected:
	virtual void disturbWater() {};

	Vector3		waterMovement_[2];
    
private:
	float		dTime_;
	class Distortion* distortion_;
    bool		inited_;
};
BW_END_NAMESPACE
//---------------------------------------------------------------------------
#endif
 