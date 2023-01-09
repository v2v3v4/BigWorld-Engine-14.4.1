#ifndef DUMP_CONTROLLERS_EXTRA_HPP
#define DUMP_CONTROLLERS_EXTRA_HPP

#include "cellapp/entity_extra.hpp"


BW_BEGIN_NAMESPACE

/**
 *	The DumpControllersExtra provides a simple method to allow the
 *	controllers associated with an entity to be discovered. See the
 *	documentation for  visitControllers()  in the .cpp for more information.
 */
class DumpControllersExtra : public EntityExtra
{
	Py_EntityExtraHeader( DumpControllersExtra );

public:
	DumpControllersExtra( Entity & e );
	~DumpControllersExtra();

	PY_AUTO_METHOD_DECLARE( RETOK, visitControllers, ARG( PyObjectPtr, END ) );
	bool visitControllers( PyObjectPtr callback );

	static const Instance< DumpControllersExtra > instance;
};

BW_END_NAMESPACE

#endif // DUMP_CONTROLLERS_EXTRA_HPP
