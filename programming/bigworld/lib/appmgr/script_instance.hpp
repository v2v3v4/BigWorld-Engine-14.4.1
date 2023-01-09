#ifndef SCRIPT_INSTANCE_HPP
#define SCRIPT_INSTANCE_HPP

#include <iostream>

#include "resmgr/datasection.hpp"
#include "moo/moo_math.hpp"

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is a common base class to all instance script objects.
 */
class ScriptInstance : public PyObjectPlus
{
public:
	ScriptInstance( PyTypeObject * pType );

	bool init( DataSectionPtr pSection,
		const char * moduleName, const char * defaultTypeName );

private:
	ScriptInstance( const ScriptInstance& );
	ScriptInstance& operator=( const ScriptInstance& );
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "script_instance.ipp"
#endif

#endif // SCRIPT_INSTANCE_HPP
