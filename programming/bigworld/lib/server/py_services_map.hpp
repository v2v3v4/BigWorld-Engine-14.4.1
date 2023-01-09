#ifndef PY_SERVICES_MAP_HPP
#define PY_SERVICES_MAP_HPP

#include "pyscript/pyobject_plus.hpp"

#include "services_map.hpp"


BW_BEGIN_NAMESPACE

/*~ class NoModule.PyServicesMap
 *  @components{ base, cell }
 *  An instance of this class emulates a dictionary of service
 *  fragment mailboxes.
 *
 *  Code Example:
 *  @{
 *  services = BigWorld.services
 *  print "A randomly chosen fragment of MyService is", services[ "MyService" ]
 *  print "All available fragments for MyService are", services.allFragmentsFor( "MyService" )
 *  print "There are", len( services ), "services."
 *  @}
 */
/**
 *	This class exposes a ServicesMap object to Python scripting.
 */
class PyServicesMap : public PyObjectPlus
{
	Py_Header( PyServicesMap, PyObjectPlus )
public:
	PyServicesMap( PyTypeObject * pTypeObject = &PyServicesMap::s_type_ );
	~PyServicesMap();

	/**
	 *	This method provides access to the wrapped ServicesMap object.
	 */
	ServicesMap & map() 				{ return map_; }

	/**
	 *	This method provides access to the wrapped ServicesMap object.
	 */
	const ServicesMap & map() const 	{ return map_; }

	// Python methods
	PY_METHOD_DECLARE( py_has_key )
	PY_METHOD_DECLARE( py_keys )
	PY_METHOD_DECLARE( py_items )
	PY_METHOD_DECLARE( py_get )
	PY_METHOD_DECLARE( py_allFragmentsFor )
private:
	ServicesMap map_;
};


BW_END_NAMESPACE

#endif // PY_SERVICES_MAP_HPP

