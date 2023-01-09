#include "pch.hpp"
#include "automation.hpp"
#include "script.hpp"
#include "cstdmf/string_utils.hpp"

BW_BEGIN_NAMESPACE

namespace Automation
{
	void parseCommandLine( LPCTSTR lpCmdLine )
	{
		const char sepsArgs[] = " \t";
		char cmdLine[ 32767 ];
		bw_wtoutf8( lpCmdLine, wcslen( lpCmdLine ), cmdLine, 32767 );
		char * arg = strtok( cmdLine, sepsArgs );
		while ( arg != NULL )
		{
			if(( strcmp( arg, "-s" ) == 0 ) ||
			   ( strcmp( arg, "--script" ) == 0 ) )
			{
				char * scriptPath = strtok( NULL, sepsArgs );
				if ( scriptPath == NULL )
				{
					ERROR_MSG( "Automation::ParseCommandLine: script argument specified without scriptPath parameter\n" );
					return;
				}

				size_t scriptPathLen = strlen( scriptPath );
				if ( scriptPathLen > 2 &&
					 scriptPathLen < MAX_PATH - 1 &&
					 scriptPath[0] == '\"' &&
					 scriptPath[ scriptPathLen - 1] == '\"' )
				{
					char trimmedScriptPath[BW_MAX_PATH];
					bw_str_substr( trimmedScriptPath,
						ARRAY_SIZE( trimmedScriptPath ), scriptPath,
						1, scriptPathLen - 2 );
					return executeScript( trimmedScriptPath );
				}
				else
				{
					return executeScript( scriptPath );
				}
			}

			arg = strtok( NULL, sepsArgs );
		}
	}

	void executeScript( const char * scriptPath )
	{
		PyObject * module =	PyImport_ImportModule( scriptPath );
		if (!module)
		{
			ERROR_MSG( "Automation::ExecuteScript: Failed to import script module '%s':\n",
					   scriptPath );
			PyErr_Print();
			return;
		}

		bool res = Script::call( PyObject_GetAttrString( module, "run" ),
								 PyTuple_New( 0 ), 
								 "Automation::ExecuteScript" );
		if (!res)
		{
			ERROR_MSG( "Automation::ExecuteScript: Error running script module '%s':\n",
					   scriptPath );
			PyErr_Print();
			return;
		}

		Py_XDECREF( module );
	}
}

BW_END_NAMESPACE