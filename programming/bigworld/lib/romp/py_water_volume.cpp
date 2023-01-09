#include "pch.hpp"
#include "py_water_volume.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Python Interface to the PyWaterVolume.
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PyWaterVolume )

/*~ function BigWorld.PyWaterVolume
 *	@components{ client, tools }
 *
 *	Factory function to create and return a new PyWaterVolume object.
 *	@return A new PyWaterVolume object.
 */
PY_FACTORY_NAMED( PyWaterVolume, "WaterVolume", BigWorld )

PY_BEGIN_METHODS( PyWaterVolume )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyWaterVolume )
	/*~ attribute PyWaterVolume.surfaceHeight
	 *	@components{ client, tools }
	 *	The surfaceHeight attribute returns the world height of the level
	 *	surface of water.
	 *	@type Float.
	 */
	PY_ATTRIBUTE( surfaceHeight )
PY_END_ATTRIBUTES()


/**
 *	Constructor.
 */
PyWaterVolume::PyWaterVolume( Water* pWater, PyTypeObject *pType ):
	PyObjectPlus( pType ),
	pWater_( pWater )
{
	BW_GUARD;
}


/**
 *	This method returns the world y-value of the water surface.
 *
 *	@return float	The world y-value of the water surface.
 */
float PyWaterVolume::surfaceHeight() const
{
	BW_GUARD;
	return pWater_->position()[1];
}


/**
 *	This is a static Python factory method. This is declared through the
 *	factory declaration in the class definition.
 *
 *	@param args		The list of parameters passed from Python. This should
 *					be a vector3 in python.
 */
PyObject *PyWaterVolume::pyNew( PyObject *args )
{
	BW_GUARD;
	PyErr_SetString( PyExc_TypeError, "BigWorld.PyWaterVolume: "
			"This constructor is private.  Instances of these objects are "
			"passed into the water volume listener callback functions" );
	return NULL;
}


PY_SCRIPT_CONVERTERS( PyWaterVolume )

BW_END_NAMESPACE

// py_water_volume.cpp
