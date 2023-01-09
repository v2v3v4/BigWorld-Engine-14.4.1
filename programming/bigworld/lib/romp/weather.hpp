#ifndef WEATHER_HPP
#define WEATHER_HPP

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"


BW_BEGIN_NAMESPACE

/*~ class BigWorld.Weather
 *	@components{ client, tools }
 *
 *	This class is used to control the weather in the clients world.
 *
 *	The weather object is used to set the wind speed  
 *	and wind gustiness.
 *	The wind affects the rate at which clouds move across the sky,
 *	and the pertubations of detail objects such as grass.
 */
class Weather : public PyObjectPlus
{
	Py_Header( Weather, PyObjectPlus )
public:
	Weather();
	~Weather();

	void activate();
	void deactivate();

	void tick( float dTime );

	PY_METHOD_DECLARE(py_windAverage);
	PY_METHOD_DECLARE(py_windGustiness);

	void windAverage( float xv, float yv )	{ windVelX_ = xv; windVelY_ = yv; }
	const Vector2& wind() const				{ return wind_; }
	void windGustiness( float amount )		{ windGustiness_ = amount; }

	Vector2 windAverage() const { return Vector2(windVelX_, windVelY_); }
	float windGustiness() const { return windGustiness_; }
	float floraWindFactor() const { return floraWindFactor_; }
	void floraWindFactor(float f) { floraWindFactor_ = f; }
	void forceImmediateUpdate() { wind_ = Vector2(windVelX_, windVelY_); }

private:
	Vector2			wind_;
	float			windVelX_;	//target velocity
	float			windVelY_;
	float			windGustiness_;
	float			floraWindFactor_;

};

BW_END_NAMESPACE

#endif // WEATHER_HPP
