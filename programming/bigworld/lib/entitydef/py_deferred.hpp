#ifndef PY_DEFERRED_HPP
#define PY_DEFERRED_HPP

#include "Python.h"

#include "pyscript/pyobject_pointer.hpp"


BW_BEGIN_NAMESPACE

namespace Mercury
{
class NubException;
}


/**
 *	This class is used to create Twisted Deferred instances.
 */
class PyDeferred
{
public:
	static bool staticInit();

	PyDeferred();

	bool callback( PyObjectPtr pArg );
	bool errback( PyObjectPtr pArg );
	bool errback( const char * excType, const char * msg );
	bool mercuryErrback( const Mercury::NubException & exception );

	PyObject * get() const	{ return pObject_.get(); }

private:
	bool callMethod( const char * methodName, PyObjectPtr pArg );

	PyObjectPtr pObject_;
};

BW_END_NAMESPACE

#endif // PY_DEFERRED_HPP
