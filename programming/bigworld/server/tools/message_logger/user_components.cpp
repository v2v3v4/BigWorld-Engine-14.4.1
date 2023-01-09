#include "user_components.hpp"

#include "logging_component.hpp"
#include "types.hpp"
#include "user_log.hpp"

#include "cstdmf/debug.hpp"


BW_BEGIN_NAMESPACE

UserComponents::UserComponents() :
	idTicker_( 0 )
{ }


UserComponents::~UserComponents()
{
	IDMap::const_iterator iter = idMap_.begin();

	while (iter != idMap_.end())
	{
		delete iter->second;
		++iter;
	}
}


bool UserComponents::init( const char *root, const char *mode )
{
	// We store the filename in this object because we might need it later
	// when we're trying to insert app IDs into the file retrospectively.
	filename_ = this->join( root, "components" );

	return this->BinaryFileHandler::init( filename_.c_str(), mode );
}


void UserComponents::flush()
{
	IDMap::iterator it = idMap_.begin();

	while (it != idMap_.end())
	{
		delete it->second;
		++it;
	}

	idMap_.clear();
	addrMap_.clear();
}


/*
 * Override from FileHandler
 */
bool UserComponents::read()
{
	if (pFile_->error())
	{
		ERROR_MSG( "UserComponents::read: '%s'\n", pFile_->strerror() );
		return false;
	}

	long len = pFile_->length();
	pFile_->seek( 0 );

	while (pFile_->tell() < len)
	{
		LoggingComponent *pComponent = new LoggingComponent( this );
		pComponent->read( *pFile_ );

		if (pFile_->error())
		{
			ERROR_MSG( "UserComponents::read: "
				"Error encountered while reading '%s': %s\n",
				filename_.c_str(), pFile_->strerror() );
			return false;
		}

		MessageLogger::UserComponentID componentAppTypeID =
										pComponent->getUserComponentID();

		addrMap_[ pComponent->getAddress() ] = pComponent;
		idMap_[ componentAppTypeID ] = pComponent;

		// Keep the ticker ahead of any components we read from disk so that we
		// don't re-use existing id's when new components register.
		if (componentAppTypeID >= idTicker_)
		{
			idTicker_ = componentAppTypeID + 1;
		}
	}

	return true;
}


/*
 * Override from FileHandler
 */
bool UserComponents::write( LoggingComponent *logComponent )
{
	MF_ASSERT( logComponent != NULL );

	logComponent->write( *pFile_ );
	return pFile_->good();
}


/**
 * Returns the Component object for a particular LCM and address, and adds the
 * component to the mapping if it doesn't already exist.
 */
LoggingComponent * UserComponents::getComponentFromMessage(
	const LoggerComponentMessage &msg, const Mercury::Address &addr,
	LogComponentNamesMLDB &logComponentNames )
{
	LoggingComponent *pComponent = NULL;

	AddrMap::iterator it = addrMap_.find( addr );
	if (it != addrMap_.end())
	{
		// Remove existing entries for that address if it's a different process
		LoggingComponent &existing = *it->second;

		if (!existing.isSameComponentAs( msg ))
		{
			addrMap_.erase( it );
			idMap_.erase( idMap_.find( existing.getUserComponentID() ) );
			delete &existing;
		}
		else
		{
			pComponent = &existing;
		}
	}

	// If the component doesn't exist, create it and make entries for it in the
	// runtime mappings, but don't dump it to disk yet ... this is done in
	// UserLog::addEntry once we know the offset of the first log entry.
	if (pComponent == NULL)
	{
		MessageLogger::AppTypeID componentID =
				logComponentNames.getAppTypeIDFromName( msg.componentName_ );
		pComponent = new LoggingComponent( this, addr, msg, componentID );

		MF_ASSERT( pComponent != NULL );

		addrMap_[ addr ] = pComponent;
		idMap_[ pComponent->getUserComponentID() ] = pComponent;
	}

	return pComponent;
}


/**
 * Returns the component for a particular address, but obviously can't make the
 * entry if it doesn't already exist.
 */
LoggingComponent * UserComponents::getComponentByAddr(
	const Mercury::Address &addr )
{
	AddrMap::const_iterator it = addrMap_.find( addr );

	if (it == addrMap_.end())
	{
		return NULL;
	}

	return it->second;
}


const LoggingComponent * UserComponents::getComponentByID(
	MessageLogger::UserComponentID componentID ) const
{
	IDMap::const_iterator it = idMap_.find( componentID );

	if (it == idMap_.end())
	{
		return NULL;
	}

	return it->second;
}


bool UserComponents::erase( const Mercury::Address &addr )
{
	AddrMap::iterator adit = addrMap_.find( addr );
	if (adit == addrMap_.end())
	{
		return false;
	}

	LoggingComponent *pComponent = adit->second;
	addrMap_.erase( adit );

	IDMap::iterator idit = idMap_.find( pComponent->getUserComponentID() );
	if (idit != idMap_.end())
	{
		idMap_.erase( idit );
	}
	else
	{
		ERROR_MSG( "UserComponents::erase: %s wasn't in the ID map!\n",
			pComponent->getString().c_str() );
	}

	delete pComponent;

	return true;
}


/**
 *
 */
const char * UserComponents::getFilename() const
{
	return filename_.c_str();
}


/**
 *
 */
MessageLogger::UserComponentID UserComponents::getNextUserComponentID()
{
	return idTicker_++;
}


/**
 *
 */
bool UserComponents::visitAllWith( UserComponentVisitor & visitor ) const
{
	IDMap::const_iterator iter = idMap_.begin();
	bool status = true;

	while ((iter != idMap_.end()) && (status == true))
	{
		status = visitor.onComponent(
					const_cast< LoggingComponent & >( *(iter->second) ));
		++iter;
	}

	return status;
}

BW_END_NAMESPACE

// user_components.cpp
