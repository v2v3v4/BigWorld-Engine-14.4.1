#ifndef PY_RESOURCE_REFS_HPP
#define PY_RESOURCE_REFS_HPP

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"


BW_BEGIN_NAMESPACE

class ResourceRef;
typedef BW::vector<ResourceRef> ResourceRefs;

/*~ class BigWorld.PyResourceRefs
 *	@components{ client, tools }
 *
 *	This class is a resource reference holder.  It cannot be constructed
 *	explicitly, instead you must call BigWorld.loadResourceListBG - the
 *	ResourceRefs object is passed into the callback function.
 *	This class holds references to those resources it is initialised with,
 *	so as long as you hold a reference to the PyResourceRefs object, you will
 *	also be holding onto references to all of its resources.
 */
/**
 *	This class is a resource reference holder.  It cannot be constructed
 *	explicitly, instead you must call BigWorld.loadResourceListBG; the
 *	ResourceRefs object is passed into the callback function.
 *	This class holds references to those resources it is initialised with,
 *	so as long as you hold a reference to the ResoureRefs object, you will
 *	also be holding onto references to all of its resources.
 */
class PyResourceRefs : public PyObjectPlus
{
	Py_Header( PyResourceRefs, PyObjectPlus )

public:	
	PyResourceRefs(
		PyObject * pResourceIDs,
		PyObject * pCallbackFn,
		int priority,
		PyTypeObject * pType = &s_type_ );

	PyResourceRefs( const ResourceRefs& rr, PyTypeObject * pType = &s_type_ );	

	PY_METHOD_DECLARE( py_pop );
	PY_METHOD_DECLARE( py_has_key )
	PY_METHOD_DECLARE( py_keys )
	PY_METHOD_DECLARE( py_values )
	PY_METHOD_DECLARE( py_items )

	ScriptList failedIDs();
	PY_RO_ATTRIBUTE_DECLARE( this->failedIDs(), failedIDs )

	PyObject * 			subscript( PyObject * entityID );
	int					length();
	static PyObject * 	s_subscript( PyObject * self, PyObject * entityID );
	static Py_ssize_t	s_length( PyObject * self );

	void				addLoadedResourceRef( ResourceRef& r );
	void				addFailedResourceID( const BW::string& resourceID );	

	PY_FACTORY_DECLARE()	

protected:	
	ResourceRefs		resourceRefs_;	
	BW::vector<BW::string> pFailedIDs_;

private:
	PyObject *			pyResource( const BW::string& resourceID );
	ResourceRef*		refByName( const BW::string& name );
	void				erase( const BW::string& name );
};

PY_SCRIPT_CONVERTERS_DECLARE( PyResourceRefs )

BW_END_NAMESPACE

#endif // PY_RESOURCE_REFS_HPP
