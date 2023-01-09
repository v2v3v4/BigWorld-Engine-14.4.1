/*~ module BigWorld
 *
 *	The BigWorld Module is a Python module that provides the world related
 *	interfaces to the client. It is used to find out application specific
 *	information, such as unique, special-case Entities, connecting Entities and
 *	other game objects by their respective IDs. It is used to provide
 *	information on game world state from the server.
 *
 */
#ifndef SCRIPT_BIGWORLD_HPP
#define SCRIPT_BIGWORLD_HPP

#include <Python.h>

#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE

class BaseCamera;
class ProjectionAccess;
#if ENABLE_CONSOLES
class XConsole;
#endif // ENABLE_CONSOLES
class InputEvent;

/**
 *	This namespace contains functions exclusive to the scripting of the client.
 */
namespace BigWorldClientScript
{
	bool init( DataSectionPtr engineConfig );
	void fini();
	void tick();

	bool sinkKeyboardEvent( const InputEvent& event );

	SpaceEntryID nextSpaceEntryID();

	// TODO:PM This is probably not the best place for this.
	bool addAlert( const char * alertType, const char * msg );

	void setUsername( const BW::string & username );
	void setPassword( const BW::string & password );
}

BW_END_NAMESPACE

#endif // SCRIPT_BIGWORLD_HPP
