#include "cstdmf/debug.hpp"
#include "cstdmf/bw_string.hpp"

#include "log_component_names.hpp"
#include "types.hpp"


BW_BEGIN_NAMESPACE


/*
 *
 */
void LogComponentNames::clear()
{
	componentNames_.clear();
}


/**
 * This method retrieves the ID associated with the specified component name.
 *
 * If no component name -> ID mapping is already known, a new ID will be
 * allocated and written to DB.
 *
 * @returns ID associated with the component.
 */
MessageLogger::AppTypeID LogComponentNames::getAppTypeIDFromName(
	const BW::string & componentName )
{
	MessageLogger::AppTypeID id = 0;
	MessageLogger::StringList::const_iterator iter = componentNames_.begin();

	// Search for existing records
	while (iter != componentNames_.end())
	{
		if (*iter == componentName)
		{
			return id;
		}

		++iter;
		++id;
	}

	if (id >= LogComponentNames::MAX_COMPONENTS)
	{
		CRITICAL_MSG( "LogComponentNames::resolve: "
			"You have registered more components than is supported (%d)\n",
			MAX_COMPONENTS );
	}

	componentNames_.push_back( componentName );
	this->writeComponentNameToDB( componentName );

	return id;
}


/**
 * This method retrieves the component name associated to the specified
 * component ID.
 *
 * @returns Component name as a string on success, NULL on error.
 */
const char * LogComponentNames::getNameFromID( int componentID ) const
{
	if (componentID >= (int)componentNames_.size())
	{
		ERROR_MSG( "LogComponentNames::getNameFromID:"
				"Cannot resolve unknown typeid (%d) from %" PRIzu
				" known records.\n", componentID, componentNames_.size() );
		return NULL;
	}

	return componentNames_[ componentID ].c_str();
}


/**
 * This method invokes onComponent on the visitor for all the known component
 * names stored in the component map.
 *
 * @returns true on success, false on error.
 */
bool LogComponentNames::visitAllWith( LogComponentsVisitor &visitor ) const
{
	MessageLogger::StringList::const_iterator iter = componentNames_.begin();
	bool status = true;

	while ((iter != componentNames_.end()) && (status == true))
	{
		status = visitor.onComponent( *iter );
		++iter;
	}

	return status;
}


BW_END_NAMESPACE

// log_component_names.cpp
