#ifndef SCRIPT_OUTPUT_HOOK_HPP
#define SCRIPT_OUTPUT_HOOK_HPP

#include "cstdmf/bw_string.hpp"


namespace BW
{

class ScriptOutputWriter;

/**
 *	This abstract class is the interface for classes that want to hook
 *	the scripting system's stdout and stderr streams.
 *
 *	ScriptOutputWriter * should be treated as an opaque token by
 *	ScriptOutputHooks, the should never need to dereference the pointer or
 *	interfere with its reference counting.
 *
 *	TODO: Implement a generic WeakPointer system and use it here instead of a
 *	ScriptOutputWriter pointer, and replace onOutputWriterDestroyed.
 */
class ScriptOutputHook
{
public:
	/**
	 *	This method receives stdout or stderr output from the scripting system
	 *
	 *	@param output	The BW::string that was output.
	 *	@param isStderr	True if the output was to stderr, false otherwise.
	 */
	virtual void onScriptOutput( const BW::string & output, bool isStderr ) = 0;

	/**
	 *	This method is notified when the ScriptOutputWriter we're hooked to is
	 *	going to be destroyed.
	 *
	 *	After this call returns, pOwner is no longer a valid pointer and should
	 *	not be dereferenced.
	 *
	 *	@param pOwner	The ScriptOutputWriter that is to be destroyed.
	 */
	virtual void onOutputWriterDestroyed( ScriptOutputWriter * pOwner ) = 0;

	/**
	 *	This static method is used to activate a hook and make it start
	 *	receiving stdout and stderr from the Scripting system.
	 *
	 *	@param pHook	The ScriptOutputHook to activate
	 *	@return			The ScriptOutputWriter pHook was hooked to, or NULL
	 *					if it could not be hooked.
	 */
	static ScriptOutputWriter * hookScriptOutput( ScriptOutputHook * pHook );

	/**
	 *	This static method is used to deactivate a hook and stop it from
	 *	receiving stdout and stderr from the Scripting system.
	 */
	static void unhookScriptOutput( ScriptOutputWriter * pWriter,
		ScriptOutputHook * pHook );
};

} // namespace BW

#endif // SCRIPT_OUTPUT_HOOK_HPP
