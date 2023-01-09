#include "services_map.hpp"

#include "cstdmf/binary_stream.hpp"


BW_BEGIN_NAMESPACE


/**
 *	Constructor.
 */
ServicesMap::ServicesMap():
		fragments_()
{}


/**
 *	Destructor.
 */
ServicesMap::~ServicesMap()
{}


/**
 *	This method adds a service fragment to the mapping.
 *
 *	@param serviceName 	The name of the service for which to add the fragment.
 *	@param mailBox 		A mailbox to the service fragment.
 */
void ServicesMap::addFragment( const BW::string & serviceName, 
		const EntityMailBoxRef & mailBox )
{
	fragments_.insert( Fragments::value_type( serviceName, mailBox ) );
}


/**
 *	This method removes a service fragment from the mapping.
 *
 *	@param serviceName 		The name of the service for which to remove the
 *							fragment.
 *	@param address 			The address of the ServiceApp to remove fragment(s)
 *							for.
 *	@param pDidLoseService 	If not-NULL, this is filled with true if there are
 *							no more fragments for the given service.
 *
 *	@return 				The number of fragments removed.
 */
int ServicesMap::removeFragment( const BW::string & serviceName,
		const Mercury::Address & address, bool * pDidLoseService )
{
	std::pair< Fragments::iterator, Fragments::iterator > range = 
		fragments_.equal_range( serviceName );

	Names lostServices;
	int numDeleted = this->removeFragmentsForAddress( address,
		range.first, range.second, pDidLoseService ? &lostServices : NULL );

	if (pDidLoseService)
	{
		*pDidLoseService = !lostServices.empty();
	}

	return numDeleted;
}


/**
 *	This method removes all fragments for a given ServiceApp.
 *
 *	@param address 			The address of the ServiceApp to which the
 *							fragments to remove belong to.
 *	@param pLostServices 	If not NULL, this is filled with the names of the
 *							services removed that no longer have any fragments
 *							anywhere.
 *
 *	@return 				The number of fragments removed.
 */
int ServicesMap::removeFragmentsForAddress( const Mercury::Address & address,
		ServicesMap::Names * pLostServices )
{
	return this->removeFragmentsForAddress( address, 
		fragments_.begin(), fragments_.end(), pLostServices );
}


/**
 *	This method removes all fragments for a given ServiceApp within the given
 *	iterator range.
 *
 *	@param address 			The address of the ServiceApp to which the
 *							fragments to remove belong to.
 *	@param begin 			The start of the range.
 *	@param end 				The end of the range.
 *	@param pLostServices 	If not NULL, this is filled with the names of the
 *							services removed that no longer have any fragments
 *							anywhere.
 *
 *	@return 				The number of fragments removed.
 */
int ServicesMap::removeFragmentsForAddress( const Mercury::Address & address,
		const ServicesMap::Fragments::iterator & begin, 
		const ServicesMap::Fragments::iterator & end,
		ServicesMap::Names * pLostServices )
{
	if (pLostServices)
	{
		pLostServices->clear();
	}

	if (begin == end)
	{
		return 0;
	}

	int numDeleted = 0;
	
	Fragments::iterator iFragment = begin;
	BW::string currentServiceName = iFragment->first;
	bool currentServiceStillExists = false; // unless we discover otherwise

	while (iFragment != end)
	{
		Fragments::iterator iCurrentFragment = iFragment;
		++iFragment;

		if (pLostServices && (currentServiceName != iCurrentFragment->first))
		{
			// Next service range. Check the last service range's existence.
			if (!currentServiceStillExists)
			{
				pLostServices->insert( currentServiceName );
			}

			currentServiceName = iCurrentFragment->first;
			currentServiceStillExists = false; // until we discover otherwise
		}

		if (iCurrentFragment->second.addr == address)
		{
			fragments_.erase( iCurrentFragment );
			++numDeleted;
		}
		else
		{
			// There's a service fragment for the current service name that we
			// didn't delete and so this service still exists.
			currentServiceStillExists = true;
		}

	}

	// Check the last service's range's existence.
	if (pLostServices && !currentServiceStillExists)
	{
		pLostServices->insert( currentServiceName );
	}

	return numDeleted;
}


/**
 *	This method retrieves the set of service names registered as keys.
 *
 *	@param names 	This is filled with the names of the services registered.
 */
void ServicesMap::getServiceNames( ServicesMap::Names & names ) const
{
	names.clear();

	Fragments::const_iterator iFragment = fragments_.begin();

	while (iFragment != fragments_.end())
	{
		names.insert( iFragment->first );

		++iFragment;
	}
}


/**
 *	This method selects a random fragment from amongst a service's registered
 *	fragments, and returns its mailbox.
 *
 *	@param serviceName 	The name of the service.
 *	@param outRef 		This is filled with the mailbox of the chosen service
 *						fragment.
 *
 *	@return true if a mailbox was found, false otherwise.
 */
bool ServicesMap::chooseFragment( const BW::string & serviceName,
		EntityMailBoxRef & outRef ) const
{
	std::pair< Fragments::const_iterator, Fragments::const_iterator > range =
		fragments_.equal_range( serviceName );

	if (range.first == range.second)
	{
		return false;
	}

	// TODO: Make this more efficient. Change to BW::map of std:vector?
	int count = std::distance( range.first, range.second );

	Fragments::const_iterator iter = range.first;

	if (count > 1)
	{
		int randIndex = rand() % count;

		for (int i = 0; i < randIndex; ++i)
		{
			++iter;
		}
	}

	outRef = iter->second;

	return true;
}


/**
 *	This method retrieves mailboxes to all the fragments for the service with
 *	the given name.
 *
 *	@param serviceName 		The name of the service to return fragments for.
 *	@param outFragments 	This is filled with the fragments for the given
 *							service.
 */
void ServicesMap::getFragmentsForService( const BW::string & serviceName,
		ServicesMap::FragmentMailBoxes & outFragments ) const
{
	outFragments.clear();

	std::pair< Fragments::const_iterator, Fragments::const_iterator > range = 
		fragments_.equal_range( serviceName );

	while (range.first != range.second)
	{
		outFragments.push_back( range.first->second );
		++range.first;
	}
}


BW_END_NAMESPACE


// services_map.cpp

