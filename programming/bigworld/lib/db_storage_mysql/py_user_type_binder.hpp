#ifndef MYSQL_PY_USER_TYPE_BINDER_HPP
#define MYSQL_PY_USER_TYPE_BINDER_HPP

#include "namer.hpp"

#include "cstdmf/smartpointer.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

#include <stack>


BW_BEGIN_NAMESPACE

class CompositePropertyMapping;
class DataSection;
class PropertyMapping;

typedef SmartPointer< DataSection > DataSectionPtr;
typedef SmartPointer< PropertyMapping > PropertyMappingPtr;
typedef SmartPointer< CompositePropertyMapping > CompositePropertyMappingPtr;


/**
 *	This class allows scripts to specify how a DataSection should be bound to
 *	sql tables it then allows createUserTypeMapping to pull out a
 *	PropertyMappingPtr to apply this.
 */
class PyUserTypeBinder : public PyObjectPlus
{
	Py_Header( PyUserTypeBinder, PyObjectPlus )

public:
	PyUserTypeBinder( const Namer & namer, const BW::string & propName,
		DataSectionPtr pDefaultValue, PyTypeObject * pType = &s_type_ );

	void beginTable( const BW::string & propName );
	bool bind( const BW::string & propName, const BW::string & typeName,
			int databaseLength );
	bool endTable();

	// this method lets createUserTypeMapping figure out its return value
	PropertyMappingPtr getResult();
private:
	PY_AUTO_METHOD_DECLARE( RETVOID, beginTable, ARG( BW::string, END ) );
	PY_AUTO_METHOD_DECLARE( RETOK, endTable, END );
	PY_AUTO_METHOD_DECLARE( RETOK, bind,
			ARG( BW::string, ARG( BW::string, OPTARG( int, 255, END ) ) ) );

	struct Context
	{
		CompositePropertyMappingPtr	pCompositeProp;
		Namer						namer;
		DataSectionPtr				pDefaultValue;

		Context( CompositePropertyMappingPtr pProp,
				const Namer & inNamer,
				const BW::string & propName, bool isTable,
				DataSectionPtr pDefault ) :
			pCompositeProp(pProp),
			namer( inNamer, propName, isTable ),
			pDefaultValue( pDefault )
		{}

		Context( CompositePropertyMappingPtr pProp,
				const Namer & inNamer,
				DataSectionPtr pDefault ) :
			pCompositeProp( pProp ),
			namer( inNamer ),
			pDefaultValue( pDefault )
		{}
	};

	std::stack< Context > tables_;

	const Context & curContext() const;
};


typedef SmartPointer< PyUserTypeBinder > PyUserTypeBinderPtr;

BW_END_NAMESPACE

#endif // MYSQL_PY_USER_TYPE_BINDER_HPP
