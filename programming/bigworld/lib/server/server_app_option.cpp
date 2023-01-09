#include "server_app_option.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Config", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ServerAppOptionIniter
// -----------------------------------------------------------------------------

ServerAppOptionIniter::Container * ServerAppOptionIniter::s_pContainer_;

/**
 *	Constructor.
 *
 *	@param configPath The setting to read from bw.xml
 *	@param watcherPath The path to the watcher value
 *	@param watcherMode Indicates whether the watcher should be read-only.
 */
ServerAppOptionIniter::ServerAppOptionIniter( const char * configPath,
			const char * watcherPath,
			Watcher::Mode watcherMode ) :
		IntrusiveObject< ServerAppOptionIniter >( s_pContainer_ ),
		configPath_( configPath ),
		watcherPath_( watcherPath ),
		watcherMode_( watcherMode )
{
}


/**
 *	This method initialises all of the configuration options. This includes
 *	reading the value from bw.xml and creating watcher entries, if appropriate.
 */
void ServerAppOptionIniter::initAll()
{
	if (s_pContainer_ == NULL)
	{
		return;
	}

	Container::iterator iter = s_pContainer_->begin();

	while (iter != s_pContainer_->end())
	{
		(*iter)->init();

		++iter;
	}
}


/**
 *	This method prints the value of all configuration options.
 */
void ServerAppOptionIniter::printAll()
{
	if (s_pContainer_ == NULL)
	{
		return;
	}

	INFO_MSG( "Configuration options (from server/bw.xml):\n" );

	Container::iterator iter = s_pContainer_->begin();

	while (iter != s_pContainer_->end())
	{
		(*iter)->print();

		++iter;
	}
}


/**
 *	This method cleans up the objects associated with initialising configuration
 *	options.
 */
void ServerAppOptionIniter::deleteAll()
{
	while (s_pContainer_ != NULL)
	{
		delete s_pContainer_->back();
	}
}


template <>
void ServerAppOptionIniterT<float>::print()
{
	const char * name = configPath_;

	if ((name  == NULL) || (name[0] == '\0'))
	{
		return;
	}

	if (almostEqual( value_, defaultValue_ ))
	{
		CONFIG_INFO_MSG( "  %-34s = %s\n",
			name, watcherValueToString( value_ ).c_str() );
	}
	else
	{
		CONFIG_INFO_MSG( "  %-34s = %s (default: %s)\n",
			name,
			watcherValueToString( value_ ).c_str(),
			watcherValueToString( defaultValue_ ).c_str() );
	}
}


template <>
void ServerAppOptionIniterT<double>::print()
{
	const char * name = configPath_;

	if ((name  == NULL) || (name[0] == '\0'))
	{
		return;
	}

	if (almostEqual( value_, defaultValue_ ))
	{
		CONFIG_INFO_MSG( "  %-34s = %s\n",
			name, watcherValueToString( value_ ).c_str() );
	}
	else
	{
		CONFIG_INFO_MSG( "  %-34s = %s (default: %s)\n",
			name,
			watcherValueToString( value_ ).c_str(),
			watcherValueToString( defaultValue_ ).c_str() );
	}
}



template <>
void ServerAppOptionIniterGetSetT<float>::print()
{
	const char * name = configPath_;
	float value = getterMethod_();

	if ((name  == NULL) || (name[0] == '\0'))
	{
		return;
	}

	if (almostEqual( value, defaultValue_ ))
	{
		CONFIG_INFO_MSG( "  %-34s = %s\n",
			name, watcherValueToString( value ).c_str() );
	}
	else
	{
		CONFIG_INFO_MSG( "  %-34s = %s (default: %s)\n",
			name,
			watcherValueToString( value ).c_str(),
			watcherValueToString( defaultValue_ ).c_str() );
	}
}


template <>
void ServerAppOptionIniterGetSetT<double>::print()
{
	const char * name = configPath_;
	double value = getterMethod_();

	if ((name  == NULL) || (name[0] == '\0'))
	{
		return;
	}

	if (almostEqual( value, defaultValue_ ))
	{
		CONFIG_INFO_MSG( "  %-34s = %s\n",
			name, watcherValueToString( value ).c_str() );
	}
	else
	{
		CONFIG_INFO_MSG( "  %-34s = %s (default: %s)\n",
			name,
			watcherValueToString( value ).c_str(),
			watcherValueToString( defaultValue_ ).c_str() );
	}
}
BW_END_NAMESPACE

// server_app_option.cpp
