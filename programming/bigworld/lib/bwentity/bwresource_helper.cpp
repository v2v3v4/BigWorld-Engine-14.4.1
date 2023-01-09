#include "pch.hpp"

#include "bwentity_types.hpp"
#include "bwresource_helper.hpp"

BWENTITY_BEGIN_NAMESPACE

static const size_t BWENTITY_MAX_ALLOWED_PATHS = 100;

BWResourceHelper::BWResourceHelper() :
	initialized_(false)	
{
}

BWResourceHelper::~BWResourceHelper()
{
	destroy();
}

/**
 * This method creates and initialize BW::BWResource singleton 
 */ 
bool BWResourceHelper::create( const char ** resPathArray )
{
	// create our own res paths by concatenate the list provided
	BW::vector<const BW::string> paths;
	for (size_t i = 0; i < BWENTITY_MAX_ALLOWED_PATHS; i++)
	{
		// Until empty string which is end of array indicator
		if (!resPathArray[i])
			break;

		paths.push_back(string(resPathArray[i]));
	}
	
	if (paths.empty())
	{
		ERROR_MSG( "BWResourceHelper::create: string array is empty\n" );
		return false;
	}
	
	//Init BWResource singleton
	new BW::BWResource();
	initialized_ = true;

	if (!BW::BWResource::init( paths ))
	{
		ERROR_MSG( "BWResourceHelper::create: "
				"could not initialise BWResource\n" );
		
		this->destroy();
		return false;
	}
	
	return true;	
}
	
/**
 * This method deletes initialize BW::BWResource singleton  
 */ 
void BWResourceHelper::destroy()
{
	if (initialized_)
	{
		delete BW::BWResource::pInstance();
		BW::BWResource::fini();
		initialized_ = false;
	}
}
	
/**
 * This method opens a BW::BWResource section by path  
 */ 
DataSectionPtr BWResourceHelper::openSection( char * pathToResource )
{
	DataSectionPtr pResult = BW::BWResource::openSection( pathToResource );
	
	if (!pResult)
	{	ERROR_MSG( "BWResourceHelper::openSection: "
				"Failed to open BWResource section: %s\n",
			pathToResource ? pathToResource : "<NULL>" );
	}
	return pResult;
}

BWENTITY_END_NAMESPACE
