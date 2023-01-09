#ifndef PY_BOTS_HPP
#define PY_BOTS_HPP

#include "pyscript/pyobject_plus.hpp"


BW_BEGIN_NAMESPACE

/*~ class NoModule.PyBots
 *  @components{ bots } 
 *  
 *  An instance of PyBots emulates a dictionary of simulated client
 *  objects, indexed by its id attribute. It does not support item
 *  assignment, but can be used with the subscript operator. Note that
 *  the key must be an integer, and that a key error will be thrown if
 *  the key does not exist in the dictionary. Instances of this class
 *  are used by the engine to make lists of entities available to
 *  python. They cannot be created via script, nor can they be
 *  modified.
 *
 *  Code Example:
 *  @{
 *  e = BigWorld.bots # this is a PyBots object
 *  print "There are", len( e ), "simulated clients"
 *  @}
 */

/**
 *	This class is used to expose the collection of bots to scripting.
 */
class PyBots : public PyObjectPlus
{
	Py_Header( PyBots, PyObjectPlus )

	public:
		PyBots( PyTypeObject * pType = &PyBots::s_type_ );

		PyObject * 			subscript( PyObject * entityID );
		int					length();

		PY_METHOD_DECLARE( py_has_key )
		PY_METHOD_DECLARE( py_keys )
		PY_METHOD_DECLARE( py_values )
		PY_METHOD_DECLARE( py_items )

		static PyObject * 	s_subscript( PyObject * self, PyObject * entityID );
		static Py_ssize_t	s_length( PyObject * self );
};

BW_END_NAMESPACE

#endif // PY_BOTS_HPP
