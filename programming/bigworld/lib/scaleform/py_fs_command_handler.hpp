#ifndef PY_FS_COMMAND_HANDLER_HPP
#define PY_FS_COMMAND_HANDLER_HPP
#if SCALEFORM_SUPPORT

#include "pyscript/script.hpp"
#include "pyscript/pyobject_plus.hpp"

#include "config.hpp"

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{
	/**
	 * This class stores a callback pointer, and implements the FSCommandHandler
	 * interface.
	 */
	class PyFSCommandHandler : public GFx::FSCommandHandler
	{
	public:
		PyFSCommandHandler( PyObject * pCallback );
		~PyFSCommandHandler();
		void Callback(GFx::Movie* pmovie, const char* pcommand, const char* parg);
	private:
		PyObjectPtr pCallback_;
	};

}	// namespace ScaleformBW

BW_END_NAMESPACE

#endif //#if SCALEFORM_SUPPORT
#endif //PY_FS_COMMAND_HANDLER_HPP