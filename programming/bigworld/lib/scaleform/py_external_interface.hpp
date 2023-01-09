#ifndef PY_EXTERNAL_INTERFACE_HPP
#define PY_EXTERNAL_INTERFACE_HPP
#if SCALEFORM_SUPPORT

#include "pyscript/script.hpp"
#include "pyscript/pyobject_plus.hpp"

#include "config.hpp"
#include "fx_game_delegate.hpp"

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{
	/**
	 * This class stores a callback pointer, and implements the ExternalInterface
	 * interface.
	 */
	class PyExternalInterface : public GFx::ExternalInterface
	{
	public:
		PyExternalInterface( PyObject * pCallback );
		~PyExternalInterface();
		void Callback(GFx::Movie* pmovie, const char* pcommand, const GFx::Value* args, UInt argCount);
	private:
		PyObjectPtr pCallback_;
	};

	/**
	 * This class stores a python callback pointer, and implements the 
	 *  FxDelegateHandler interface.
	 */
	class PyExternalInterfaceHandler : public FxDelegateHandler {
		public:
			PyExternalInterfaceHandler( PyObject * pCallback );
			~PyExternalInterfaceHandler();

			void Accept(CallbackProcessor* cbreg);
		
		static void handleCallback(const FxDelegateArgs& pparams, const char* methodName);

		private:
			PyObjectPtr pCallback_;
	};


}	// namespace ScaleformBW

BW_END_NAMESPACE

#endif //#if SCALEFORM_SUPPORT
#endif //PY_EXTERNAL_INTERFACE_HPP
