#ifndef SCRIPT_OBJECT_QUERY_OPERATION_HPP
#define SCRIPT_OBJECT_QUERY_OPERATION_HPP

#include "scene/forward_declarations.hpp"

#include "scene/scene_type_system.hpp"
#include "scene/scene_object.hpp"

#include "script/script_object.hpp"

namespace BW
{

class IScriptObjectQueryOperationTypeHandler
{
public:
	virtual ~IScriptObjectQueryOperationTypeHandler(){}

	virtual ScriptObject doGetScriptObject( const SceneObject & object ) = 0;
};


class ScriptObjectQueryOperation
	: public SceneObjectOperation<IScriptObjectQueryOperationTypeHandler>
{
public:
	ScriptObject scriptObject( const SceneObject & object );
};

} // namespace BW

#endif // SCRIPT_OBJECT_QUERY_OPERATION_HPP
