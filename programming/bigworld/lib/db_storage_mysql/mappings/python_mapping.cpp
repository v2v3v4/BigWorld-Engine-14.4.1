#include "script/first_include.hpp"

#include "python_mapping.hpp"

#include "blob_mapping.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/debug.hpp"
#include "entitydef/data_types/python_data_type.hpp"
#include "pyscript/pickler.hpp"
#include "pyscript/script.hpp"
#include "resmgr/datasection.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

/**
 * Constructor.
 */
PythonMapping::PythonMapping( const Namer & namer, const BW::string & propName,
		int length, DataSectionPtr pDefaultValue ) :
	StringLikeMapping( namer, propName, INDEX_TYPE_NONE, length )
{
	if (pDefaultValue)
	{
		defaultValue_ = pDefaultValue->as< BW::string >();
	}

	if (defaultValue_.empty())
	{
		defaultValue_ = PythonMapping::getPickler().pickle( 
			ScriptObject::none() );
	}
	else if (PythonDataType::isExpression( defaultValue_ ))
	{
		PythonMapping::pickleExpression( defaultValue_, defaultValue_ );

		if (defaultValue_.empty())
		{
			CRITICAL_MSG( "PythonMapping::PythonMapping: "
					"the default value evaluated expression for %s "
					"could not be pickled",
				propName.c_str() );
		}
	}
	else
	{
		BlobMapping::decodeSection( defaultValue_, pDefaultValue );
	}

	if (defaultValue_.length() > charLength_)
	{
		ERROR_MSG( "PythonMapping::PythonMapping: "
					"Default value for property '%s' is too big to fit inside "
					"column. Defaulting to None\n",
				propName.c_str() );

		defaultValue_ = PythonMapping::getPickler().pickle( 
			ScriptObject::none() );

		if (defaultValue_.length() > charLength_)
		{
			CRITICAL_MSG( "PythonMapping::PythonMapping: "
						"Even None cannot fit inside column. Please "
						"increase DatabaseSize of property '%s'\n",
					propName.c_str() );
		}
	}
}


/**
 *	This method evaluates expr as a Python expression, pickles the
 *	resulting object and stores it in output.
 *
 *	@param output The string where the pickled data will be placed.
 *	@param expr   The expression to pickle.
 */
void PythonMapping::pickleExpression( BW::string & output,
		const BW::string & expr )
{
	ScriptObject pResult( Script::runString( expr.c_str(), false ),
			ScriptObject::FROM_NEW_REFERENCE );

	ScriptObject pToBePickled = pResult ? pResult : ScriptObject::none();

	output = PythonMapping::getPickler().pickle( pToBePickled );
}


/**
 *	This static method returns the Pickler object to be used for pickling.
 *
 *	@returns The static Pickler object.
 */
Pickler & PythonMapping::getPickler()
{
	static Pickler pickler;

	return pickler;
}

BW_END_NAMESPACE

// python_mapping.cpp
