#include "pch.hpp"
#if SCALEFORM_SUPPORT
#include "py_fs_command_handler.hpp"

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{

PyFSCommandHandler::PyFSCommandHandler( PyObject * callback ):
	pCallback_( callback )
{
};


PyFSCommandHandler::~PyFSCommandHandler()
{
}


void PyFSCommandHandler::Callback(GFx::Movie* pmovie, const char* command, const char* args)
{
	BW_GUARD;
	PyObject* pyArgs = Py_BuildValue("(s s)", command, args);
	Py_XINCREF( pCallback_.get() );
	Script::call( pCallback_.getObject(), pyArgs, "PyFSCommandHandler::Callback", false );
}

}	// namespace ScaleformBW

BW_END_NAMESPACE

#endif //#if SCALEFORM_SUPPORT