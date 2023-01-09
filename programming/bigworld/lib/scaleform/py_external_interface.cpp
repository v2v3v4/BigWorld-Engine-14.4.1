#include "pch.hpp"
#if SCALEFORM_SUPPORT
#include "py_external_interface.hpp"
#include "util.hpp"

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{

PyExternalInterface::PyExternalInterface( PyObject * callback ):
	pCallback_( callback )
{
};


PyExternalInterface::~PyExternalInterface()
{
}


void PyExternalInterface::Callback(GFx::Movie* pmovie, const char* command, const GFx::Value* args, UInt argCount)
{
	BW_GUARD;
	PyObject * ret;
	PyObject* tuple = PyTuple_New( argCount );

	for (size_t i=0; i<argCount; i++)
	{
		PyObject * pArg;
		valueToObject( args[i], &pArg );
		PyTuple_SetItem( tuple, i, pArg );
	}

	PyObject* pyArgs = Py_BuildValue("sN", command, tuple );

	Py_XINCREF( pCallback_.get() );
	ret = Script::ask( pCallback_.getObject(), pyArgs, "PyExternalInterface::Callback", false );

	GFx::Value gfxRet;
	objectToValue( ret, gfxRet );
	pmovie->SetExternalInterfaceRetVal( gfxRet );
}

PyExternalInterfaceHandler::PyExternalInterfaceHandler(PyObject * callback):
	pCallback_( callback )
{
	TRACE_MSG( "PyExternalInterfaceHandler %p constructed\n", this );
}


PyExternalInterfaceHandler::~PyExternalInterfaceHandler()
{
	TRACE_MSG( "PyExternalInterfaceHandler %p destructed\n", this );
}

void PyExternalInterfaceHandler::Accept(CallbackProcessor* cbreg) 
{
	TRACE_MSG( "PyExternalInterfaceHandler.%p Accept() called\n", this );
}

void PyExternalInterfaceHandler::handleCallback(const FxDelegateArgs& pparams, const char* methodName) 
{
	BW_GUARD;
	PyExternalInterfaceHandler* handler = (PyExternalInterfaceHandler*)pparams.GetHandler();
	PyObject * ret;
	UInt argCount = pparams.GetArgCount(); 
	PyObject* tuple = PyTuple_New( argCount );

	for (size_t i=0; i<argCount; i++)
	{
		PyObject * pArg;
		valueToObject( pparams[i], &pArg );
		PyTuple_SetItem( tuple, i, pArg );
	}

	PyObject* pyArgs = Py_BuildValue("sN", methodName, tuple );

	Py_XINCREF( handler->pCallback_.getObject() );
	ret = Script::ask( handler->pCallback_.getObject(), pyArgs, "PyExternalInterfaceHandler::Callback", false );

	GFx::Value gfxRet;
	objectToValue( ret, gfxRet );
	pparams.GetMovie()->SetExternalInterfaceRetVal( gfxRet );
}


} // namespace ScaleformBW

BW_END_NAMESPACE

#endif //#if SCALEFORM_SUPPORT

