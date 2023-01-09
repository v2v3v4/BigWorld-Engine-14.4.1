#include "pch.hpp"
#include "script_object_query_operation.hpp"

namespace BW
{

ScriptObject ScriptObjectQueryOperation::scriptObject(
	const SceneObject & object )
{
	IScriptObjectQueryOperationTypeHandler* pTypeHandler =
			this->getHandler( object.type() );
	if (pTypeHandler)
	{
		return pTypeHandler->doGetScriptObject( object );
	}

	return ScriptObject( NULL );
}


} // namespace BW

