#include "script/first_include.hpp"

#include "baseapp_config.hpp"

#include "service_starter.hpp"

#include "entity_creator.hpp"



BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
ServiceStarter::ServiceStarter() :
	tags_(),
	hasTags_( false )
{
}


/**
 *	This method initialises the service starter with the set of service names
 *	that should be started.
 */
bool ServiceStarter::init()
{
	TagsMessage query;
	query.tags_.push_back( "Services" );

	Mercury::Reason reason = query.sendAndRecv( 0, LOCALHOST, this );

	if (reason != Mercury::REASON_SUCCESS)
	{
		ERROR_MSG( "ServiceStarter::init: MGM query failed (%s)\n",
				Mercury::reasonToString( reason ) );

		return false;
	}

	return true;
}


/**
 *	This method handles the reply from bwmachined.
 */
bool ServiceStarter::onTagsMessage( TagsMessage &tm, uint32 addr )
{
	hasTags_ = tm.exists_;

	if (hasTags_)
	{
		std::swap( tags_, tm.tags_ );
	}

	CONFIG_INFO_MSG( "Found %d service fragments in [Services] "
					"section in /etc/bwmachined.conf",
				(int)tags_.size() );
	return true;
}


/**
 *	This method merges the set of service names with an initial layout,
 *	if specified.
 */
bool ServiceStarter::finishInit( int layoutId )
{
	// Iterate through all serviceApp/app layouts.
	DataSectionPtr pLayouts =
		BWConfig::getSection( "serviceApp/initialLayouts" );

	if (!pLayouts)
	{
		return true;
	}

	Tags fragments;

	DataSection::iterator iLayoutConfig = pLayouts->begin();

	bool hasFoundLayout = false;
	int currLayout = 0;

	while (iLayoutConfig != pLayouts->end())
	{
		DataSectionPtr pLayout = *iLayoutConfig;

		if (pLayout)
		{
			if (pLayout->sectionName() == "app")
			{
				if (currLayout == layoutId)
				{
					CONFIG_INFO_MSG(
								"ServiceStarter: Applying initial layout #%d "
								"for this ServiceApp.",
							layoutId );

					this->readFragmentsFromLayout( pLayout, fragments );
					hasFoundLayout = true;
					break;
				}

				currLayout++;
			}
		}

		++iLayoutConfig;
	}

	if (!hasFoundLayout)
	{
		DataSectionPtr pDefaultLayout = pLayouts->openSection( "default" );

		if (pDefaultLayout)
		{
			this->readFragmentsFromLayout( pDefaultLayout, fragments );

			CONFIG_INFO_MSG( "ServiceStarter: Applying default initial layout for "
						"this ServiceApp." );

			hasFoundLayout = true;
		}
	}

	if (!hasFoundLayout)
	{
		return true;
	}

	if (fragments.empty())
	{
		CONFIG_WARNING_MSG( "ServiceStarter: An initial layout found for "
					"this ServiceApp but the list is empty!\n" );
	}

	for (Tags::iterator iFragment = fragments.begin();
			iFragment != fragments.end();)
	{
		if (hasTags_ &&
			std::find( tags_.begin(), tags_.end(), *iFragment ) == tags_.end())
		{
			CONFIG_WARNING_MSG( "ServiceStarter: Service fragment %s is not "
					"present in [Services] section in /etc/bwmachined.conf. "
					"It will be ignored.\n",
				(*iFragment).c_str() );

			iFragment = fragments.erase( iFragment );
		}
		else
		{
			CONFIG_INFO_MSG( "ServiceStarter: Registered fragment %s "
							"from initial services layout.\n",
						(*iFragment).c_str() );

			iFragment++;
		}
	}

	tags_.swap( fragments );

	// if there's an initial layout but no fragments in the list,
	// we don't run any service fragments
	hasTags_ = true;

	return true;
}


/**
 * This method is a helper to read fragments from a layout section
 */
void ServiceStarter::readFragmentsFromLayout( DataSectionPtr pLayout,
											Tags &fragments )
{
	for (DataSection::iterator iFragment = pLayout->begin();
			iFragment != pLayout->end();
			iFragment++)
	{
		DataSectionPtr pFragment = *iFragment;
		if (pFragment)
		{
			fragments.push_back( pFragment->sectionName() );
		}
	}
}


/**
 *	This method returns whether the service with the given name should be
 *	started on this ServiceApp. This is determined by the [Services] section
 *	in /etc/bwmachined.conf. and by whether service should be active
 *	on current serverMode
 *
 */
bool ServiceStarter::shouldStartService(
		const EntityTypePtr & pType ) const
{
	bool isValidServerMode;
	INFO_MSG( "ServiceStarter::shouldStartService: service '%s',  "
			"activeOnServerModes: '%s' [any if empty]\n",
			pType->name(),
			pType->description().activeOnServerModes().c_str() );

	isValidServerMode = (pType->description().activeOnServerModes().length() == 0) ||
			(pType->description().activeOnServerModes().find(
					BaseAppConfig::serverMode.getRef()
					) != BW::string::npos);

	// Note: Brute-force search but should be small.
	return isValidServerMode && (!hasTags_ ||
		(std::find( tags_.begin(), tags_.end(), pType->name() ) != tags_.end()));
}


/**
 *	This method starts the appropriate services on this ServiceApp.
 */
bool ServiceStarter::run( EntityCreator & entityCreator ) const
{
	EntityTypeID typeID = 0;
	EntityTypePtr pType = EntityType::getType( typeID++ );

	while (pType)
	{
		if (pType->description().isService())
		{
			if (this->shouldStartService( pType ))
			{
				PyObject * pNewEntity =
					entityCreator.createBase( pType.get(), NULL );

				if (!pNewEntity)
				{
					ERROR_MSG( "BaseApp::startServiceFragments: "
							"Failed to start %s\n", pType->name() );
					PyErr_Clear();
					return false;
				}
				else
				{
					Py_DECREF( pNewEntity );
				}
			}
			else
			{
				INFO_MSG( "ServiceStarter::run: Not starting '%s'\n",
						pType->name() );
			}
		}

		pType = EntityType::getType( typeID++ );
	}

	return true;
}

BW_END_NAMESPACE

// service_starter.cpp
