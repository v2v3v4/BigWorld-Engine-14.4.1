#include "log_component_names.hpp"
#include "../types.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE


/*
 *	Override from FileHandler.
 */
bool LogComponentNamesMLDB::init( const char *logLocation, const char *mode )
{
	const char *logComponentsPath = this->join(
		logLocation, "component_names" );
	return TextFileHandler::init( logComponentsPath, mode );
}


/*
 *	Override from FileHandler.
 */
void LogComponentNamesMLDB::flush()
{
	this->clear();
}


/*
 *	Override from TextFileHandler.
 */
bool LogComponentNamesMLDB::handleLine( const char *line )
{
	BW::string name = line;

	// Ignore any components past our maximum limit
	if (this->getComponentNameSize() >= this->MAX_COMPONENTS)
	{
		CRITICAL_MSG( "LogComponentNamesMLDB::handleLine:"
			"Dropping component '%u'; max number of components reached",
			this->MAX_COMPONENTS );
	}

	this->pushbackComponentName( name );
	return true;
}


/**
 *
 */
bool LogComponentNamesMLDB::writeComponentNameToDB(
	const BW::string & componentName )
{
	// Make a new component name entry
	return this->writeLine( componentName.c_str() );
}


BW_END_NAMESPACE

// log_component_names.cpp
