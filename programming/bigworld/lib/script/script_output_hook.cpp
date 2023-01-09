#include "pch.hpp"

#include "script_output_hook.hpp"

#include "script_output_writer.hpp"

namespace BW
{

/* static */ ScriptOutputWriter * ScriptOutputHook::hookScriptOutput(
	ScriptOutputHook * pHook )
{
	ScriptOutputWriter * pWriter = ScriptOutputWriter::getCurrentStdio();

	if (pWriter == NULL)
	{
		return NULL;
	}

	pWriter->addHook( pHook );
	return pWriter;
}


/* static */ void ScriptOutputHook::unhookScriptOutput(
	ScriptOutputWriter * pWriter, ScriptOutputHook * pHook )
{
	if (pWriter == NULL)
	{
		ERROR_MSG( "ScriptOutputHook::unhookScriptOutput: "
			"Attempted to unhook when not hooked to a ScriptOutputWriter\n" );
		return;
	}

	pWriter->delHook( pHook );
}


}

// script_output_hook.cpp

