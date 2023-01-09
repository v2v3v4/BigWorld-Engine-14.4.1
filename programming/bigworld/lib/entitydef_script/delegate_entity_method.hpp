#ifndef DELEGATE_ENTITY_METHOD
#define DELEGATE_ENTITY_METHOD

#include "pyscript/pyobject_plus.hpp"
#include "delegate_interface/entity_delegate.hpp"

BW_BEGIN_NAMESPACE

class MethodDescription;

class DelegateEntityMethod : public PyObjectPlus
{
	Py_Header( DelegateEntityMethod, PyObjectPlus );
public:
	DelegateEntityMethod( const IEntityDelegatePtr & delegate,
		const MethodDescription * pMethodDescription,
		EntityID sourceID,
		PyTypeObject * pType = &s_type_ );

	PY_KEYWORD_METHOD_DECLARE( pyCall );
private:
	IEntityDelegatePtr delegate_;
	const MethodDescription * pMethodDescription_;
	EntityID sourceID_;
};

BW_END_NAMESPACE

#endif
