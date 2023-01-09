#include "pch.hpp"

#include "py_component.hpp"
#include "entitydef/method_description.hpp"
#include "entitydef/data_description.hpp"
#include "entitydef/script_data_sink.hpp"

BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( PyComponent )

PY_BEGIN_ATTRIBUTES( PyComponent )
PY_END_ATTRIBUTES()

PY_BEGIN_METHODS( PyComponent )
PY_END_METHODS()

PyComponent::PyComponent( 
			const IEntityDelegatePtr & delegate,
			EntityID entityID,
			const BW::string & componentName,
			PyTypeObject * pType ) : PyObjectPlusWithVD( pType ),
	delegate_( delegate ),
	entityID_( entityID ),
	componentName_( componentName )
{
	MF_ASSERT( delegate );
}

ScriptObject PyComponent::pyGetAttribute( const ScriptString & attrObj )
{
	const char * attr = attrObj.c_str();
	
	DataDescription * pProperty = 
		this->findProperty( attr, componentName_.c_str() );
	
	if (pProperty)
	{
		return getAttribute( *delegate_, *pProperty, 
						 /*isPersistentOnly*/ false );
	}
	
	return PyObjectPlusWithVD::pyGetAttribute( attrObj );
}

bool PyComponent::pySetAttribute( const ScriptString & attrObj,
		const ScriptObject & value )
{
	const char * attr = attrObj.c_str();
	
	// see if it is a component property
	DataDescription * pProperty = 
		this->findProperty( attr, componentName_.c_str() );
	
	if (pProperty)
	{
		return setAttribute( *delegate_, *pProperty, value, 
						 /*isPersistentOnly*/ false );
	}
	return PyObjectPlusWithVD::pySetAttribute( attrObj, value );
}

//static 
ScriptObject PyComponent::getAttribute( IEntityDelegate & delegate, 
										const DataDescription & descr,
									    bool isPersistentOnly )
{
	MemoryOStream stream;
	ScriptDataSink sink;
	if (!delegate.getDataField(stream, descr, isPersistentOnly) ||
		!descr.createFromStream(stream, sink, isPersistentOnly))
	{
		PyErr_Format( PyExc_ValueError,
					 "Failed to get field '%s' value of %s component",
					 descr.name().c_str(), 
					 descr.componentName().c_str() );
		return ScriptObject();
	}
	return sink.finalise();
}

//static 
bool PyComponent::setAttribute( IEntityDelegate & delegate, 
								const DataDescription & descr,
								const ScriptObject & value,
							    bool isPersistentOnly )
{
	ScriptDataSource source( value );
	MemoryOStream stream;
	if (!descr.addToStream( source, stream, isPersistentOnly ))
	{
		PyErr_Format( PyExc_TypeError,
				"Failed to convert value to stream" );
		return false;
	}
	if (!delegate.updateProperty( stream, descr,isPersistentOnly ))
	{
		PyErr_Format( PyExc_TypeError,
					"Failed to update '%s' property of component %s",
					descr.name().c_str(),
					descr.componentName().c_str() );
		return false;
	}
	return true;
}

BW_END_NAMESPACE
