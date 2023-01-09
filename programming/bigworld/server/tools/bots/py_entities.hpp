#ifndef PY_ENTITIES_HPP
#define PY_ENTITIES_HPP

#include "connection_model/bw_entities.hpp"

#include "pyscript/pyobject_plus.hpp"


BW_BEGIN_NAMESPACE

/*~ class NoModule.PyEntities
 *  @components{ bots }
 *  A class which emulates a dictionary of PyEntity objects indexed by their id
 *  attributes. Instances of this class do not support item assignment, but can
 *  be used with the subscript operator. Note that the key must be an integer, 
 *  and that a key error will be thrown if the key given does not exist in the
 *  dictionary. Instances of this class are used by the engine to present lists
 *  of entities to script. They cannot be created via script, nor can they be 
 *  modified.
 *
 *  Example:
 *  @{
 *	bot = BigWorld.bots[332] #assume client with id 332 exists
 *  e = bot.entities # this is a PyEntities object
 *  e[ 100 ] # returns the entity with ID 100
 *  len( e ) # returns the number of entities in this dictionary
 *  @}
 */
/**
 *	This class is used to expose the collection of entities to scripting.
 */
class PyEntities : public PyObjectPlus
{
	Py_Header( PyEntities, PyObjectPlus )

public:
	PyEntities( const BWEntities & entities,
		PyTypeObject * pType = &PyEntities::s_type_ );
	~PyEntities();

	PyObject * 			subscript( PyObject * entityID );
	Py_ssize_t			length();

	PY_METHOD_DECLARE( py_has_key )
	PY_METHOD_DECLARE( py_keys )
	PY_METHOD_DECLARE( py_values )
	PY_METHOD_DECLARE( py_items )
	PY_METHOD_DECLARE( py_get )

	static PyObject * 	s_subscript( PyObject * self, PyObject * entityID );
	static Py_ssize_t	s_length( PyObject * self );

private:
	PyEntities( const PyEntities & );
	PyEntities& operator=( const PyEntities & );

	typedef PyObject * (*GetFunc)( const BWEntities::const_iterator & item );
	PyObject * makeList( GetFunc objectFunc );

	const BWEntities & entities_;
};


#ifdef CODE_INLINE
#include "py_entities.ipp"
#endif

BW_END_NAMESPACE

#endif // PY_ENTITIES_HPP
