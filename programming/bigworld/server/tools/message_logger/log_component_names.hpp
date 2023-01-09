#ifndef LOG_COMPONENT_NAMES_HPP
#define LOG_COMPONENT_NAMES_HPP

#include "types.hpp"


BW_BEGIN_NAMESPACE

/**
 * This abstract class provides a template which can be used to retrieve
 * the names of components that have generated logs.
 */
class LogComponentsVisitor
{
public:
	virtual bool onComponent( const BW::string &componentName ) = 0;
};


/**
 * Handles the mapping between ids and component names, i.e. 0 -> cellapp,
 * 1 -> baseapp etc.  This is shared amongst all users, and is based on the
 * order in which messages from unique components are delivered.
 */
class LogComponentNames
{
public:
	virtual bool writeComponentNameToDB( const BW::string & componentName ) = 0;

	MessageLogger::AppTypeID getAppTypeIDFromName(
		const BW::string & componentName );

	const char * getNameFromID( int componentID ) const;

	bool visitAllWith( LogComponentsVisitor &visitor ) const;

protected:
	void clear();

	MessageLogger::AppTypeID getComponentNameSize()
		{ return componentNames_.size(); }

	void pushbackComponentName( BW::string & name )
		{ componentNames_.push_back( name ); }

	// TODO: Why is this artificially limited?
	static const MessageLogger::AppTypeID MAX_COMPONENTS = 31;

private:
	MessageLogger::StringList componentNames_;
};

BW_END_NAMESPACE

#endif // LOG_COMPONENT_NAMES_HPP
