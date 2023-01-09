#ifndef PY_FACTORY_METHOD_LINK
#define PY_FACTORY_METHOD_LINK

#include "script.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class allows factory methods to be added to modules. It is the
 *	same as PyModuleMethodLink except that it also sets the module name
 *	in the type dictionary.
 *
 *	@see PyModuleMethodLink
 */
class PyFactoryMethodLink :
	public Script::InitTimeJob,
	public Script::FiniTimeJob
{
public:
	PyFactoryMethodLink( const char * moduleName,
		const char * methodName, PyTypeObject * pType );
	~PyFactoryMethodLink();

	virtual void init();
	virtual void fini();

private:
	const char * moduleName_;
	const char * methodName_;
	PyTypeObject * pType_;

	const char * origTypeName_;
};

BW_END_NAMESPACE

#endif // PY_FACTORY_METHOD_LINK
