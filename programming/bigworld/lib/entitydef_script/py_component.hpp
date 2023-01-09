#ifndef PY_COMPONENT_H
#define PY_COMPONENT_H

#include "pyscript/pyobject_plus.hpp"
#include "delegate_interface/entity_delegate.hpp"

BW_BEGIN_NAMESPACE

class MethodDescription;
class DataDescription;

class PyComponent : public PyObjectPlusWithVD
{
	Py_Header( PyComponent, PyObjectPlus );
public:
	PyComponent( const IEntityDelegatePtr & delegate,
		EntityID entityID,
		const BW::string & componentName,
		PyTypeObject * pType = &s_type_ );
	virtual ~PyComponent() {}

	ScriptObject pyGetAttribute( const ScriptString & attrObj );
	bool pySetAttribute( const ScriptString & attrObj,
		const ScriptObject & value );
	
	static ScriptObject getAttribute( IEntityDelegate & delegate, 
									  const DataDescription & descr,
									  bool isPersistentOnly );
	
	static bool setAttribute( IEntityDelegate & delegate, 
							  const DataDescription & descr,
							  const ScriptObject & value,
							  bool isPersistentOnly );
	
private:
	virtual DataDescription * findProperty( const char * attribute,
											const char * component ) = 0;

	IEntityDelegatePtr delegate_;
	EntityID entityID_;
	BW::string componentName_;
};

BW_END_NAMESPACE

#endif // PY_COMPONENT_H
