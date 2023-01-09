#include "pch.hpp"
#include "pe_scripter.hpp"
#include "appmgr/options.hpp"
#include "resmgr/bwresource.hpp"
#include "particle/particle_system_manager.hpp"
#include "physics2/material_kinds.hpp"
#include "pyscript/script.hpp"
#include "pyscript/py_import_paths.hpp"
#include "pyscript/personality.hpp"
#include "romp/xconsole.hpp"
#include "romp/console_manager.hpp"
#include "common/compile_time.hpp"
#include "entitydef/constants.hpp"

DECLARE_DEBUG_COMPONENT2( "Shell", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Include doco for other modules
// -----------------------------------------------------------------------------

/*~ module BigWorld
 *  @components{ particleeditor }
 */

/*~ module Math
 *  @components{ particleeditor }
 */

/*~ module GUI
 *  @components{ particleeditor }
 */

/*~ module PostProcessing
 *  @components{ particleeditor }
 */


// -----------------------------------------------------------------------------
// Section: pe_scripter
// -----------------------------------------------------------------------------

/**
 *  BW script interface.
 */
bool Scripter::init(DataSectionPtr pDataSection)
{
	BW_GUARD;

	// Particle Systems are creatable from Python code
	MF_VERIFY( ParticleSystemManager::init() );

	PyImportPaths importPaths;
	importPaths.addResPath( "resources/scripts" );
	importPaths.addResPath( EntityDef::Constants::entitiesEditorPath() );
	importPaths.addResPath( EntityDef::Constants::entitiesClientPath() );
	importPaths.addResPath( EntityDef::Constants::userDataObjectsEditorPath() );

	// Call the general init function
	if (!Script::init( importPaths, "editor" ))
	{
		return false;
	}

	Options::initLoggers();

	if (!MaterialKinds::init())
	{
		return false;
	}

	PyObject *pInit =
		PyObject_GetAttrString(PyImport_AddModule("keys"), "init");
	if (pInit != NULL)
	{
		PyRun_SimpleString( PyString_AsString(pInit) );
	}
	PyErr_Clear();

	Personality::import( Options::getOptionString( "personality", "Personality" ) );

	return true;
}

void Scripter::fini()
{
	BW_GUARD;

	MaterialKinds::fini();

	// Fini scripts
	Script::fini();
	ParticleSystemManager::fini();
}
BW_END_NAMESPACE

