#include "pch.hpp"

#include "data_type.hpp"
#include "entity_method_descriptions.hpp"

#include "network/exposed_message_range.hpp"

DECLARE_DEBUG_COMPONENT2( "DataDescription", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: EntityMethodDescriptions
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
EntityMethodDescriptions::EntityMethodDescriptions() :
	maxExposedMethodCount_( -1 )
{
}


/**
 *	This method initialises this collection of methods from a data section.
 *	NOTE: Despite its name, this method may be called more than once to add
 *	the methods from implemented interfaces.
 */
bool EntityMethodDescriptions::init( DataSectionPtr pMethods,
		MethodDescription::Component component, const char * interfaceName,
		const BW::string & componentName, unsigned int * pNumLatestEventMembers )
{
	if (!pMethods)
	{
		WARNING_MSG( "EntityMethodDescriptions::init: pMethods is NULL\n" );

		return false;
	}

	DataSectionIterator iter = pMethods->begin();

	while (iter != pMethods->end())
	{
		MethodDescription methodDescription;

		if (!methodDescription.parse( interfaceName, *iter,
				component, pNumLatestEventMembers ))
		{
			WARNING_MSG( "Error parsing method %s\n",
				methodDescription.name().c_str() );
			return false;
		}

		if (component == MethodDescription::CLIENT)
		{
			// all client methods are exposed
			methodDescription.setExposedToAllClients();
		}

		MF_ASSERT( internalMethods_.size() <= INT_MAX );
		methodDescription.internalIndex( ( int ) internalMethods_.size() );
		methodDescription.addImplementingComponent( componentName );
		
		const std::pair< Map::iterator, bool > insertResult = 
			map_.insert( std::make_pair( methodDescription.name().c_str(), 
										 methodDescription.internalIndex() ) );

		if (!insertResult.second) // method with the same name exists
		{
			MethodDescription & existMethod = 
				internalMethods_.at(insertResult.first->second);

			if (methodDescription.isSignatureEqual( existMethod ))
			{
				// it's the same method but from the other component
				if (!existMethod.addImplementingComponent( componentName ))
				{
					return false;
				}
			}
			else
			{
				ERROR_MSG( "EntityMethodDescriptions::init: "
					"Method '%s' already defined with a different signature \n",
					methodDescription.name().c_str() );
				return false;
			}
		}
		else
		{
			internalMethods_.push_back( methodDescription );

			if (methodDescription.isExposed())
			{
				exposedMethods_.push_back( methodDescription.internalIndex() );
			}
		}

		++iter;
	}

	return this->checkExposedForClientSafety( interfaceName );
}


namespace
{

/**
 *	Sort exposed methods list helper class
 */
class ExposedMethodsSortHelper
{
public:
	ExposedMethodsSortHelper( const MethodDescriptionList & methods ) :
		methods_( methods ) {}

	bool operator()( const unsigned int &methodIndex1,
					 const unsigned int &methodIndex2 )
	{
		int16 size1 = methods_[ methodIndex1 ].streamSize( true );
		int16 size2 = methods_[ methodIndex2 ].streamSize( true );
		// Both variable size
		if (size1 < 0 && size2 < 0)
		{
			return -size1 < -size2;
		}
		// Both fixed-size
		if (size1 >=0 && size2 >= 0)
		{
			return size1 < size2;
		}
		// Otherwise, one fixed and one variable.
		// Fixed (+ve) is always lesser.
		return size1 > size2;
	}

private:
	const MethodDescriptionList & methods_;
};

} // anonymous namespace


/**
 *	This method checks whether any of the exposed methods take unsafe arguments
 *	and warns appropriately.
 */
bool EntityMethodDescriptions::checkExposedForClientSafety(
		const char * interfaceName )
{
	bool methodsOk = true;

	for (uint eindex = 0; eindex < exposedMethods_.size(); eindex++)
	{
		uint iindex = exposedMethods_[eindex];
		MethodDescription & mdesc = internalMethods_[iindex];

		int clientSafety = mdesc.clientSafety();
		
		const char * const szMethodType = 
				mdesc.component() == MethodDescription::CLIENT ? 
				"a client" : "an Exposed";

		if (clientSafety & DataType::CLIENT_UNSAFE)
		{
			WARNING_MSG( "%s.%s is %s method that takes a PYTHON arg "
							"(potential security hole)\n",
						interfaceName, mdesc.name().c_str(), szMethodType );
		}

		if (clientSafety & DataType::CLIENT_UNUSABLE)
		{
			ERROR_MSG( "%s.%s is %s method that takes a MAILBOX arg "
							"(type is not able to be sent to or from client)\n",
						interfaceName, mdesc.name().c_str(), szMethodType );

			methodsOk = false;
		}
	}

	return methodsOk;
}


/**
 *	This helper method checks for subslots and tells MethodDescriptions about
 *	them. It also sorts the exposed ids so that fixed sized messages are
 *	first.
 *
 *	@param maxExposedMethodCount The maximum number of exposed methods over
 *		all entity types.
 *	@param pRange The message range that will be used for these remote calls.
 */
void EntityMethodDescriptions::setExposedMsgIDs( int maxExposedMethodCount,
		const ExposedMethodMessageRange * pRange )
{
	maxExposedMethodCount_ = maxExposedMethodCount;

	std::stable_sort( exposedMethods_.begin(), exposedMethods_.end(),
		ExposedMethodsSortHelper( internalMethods_ ) );

	for (uint exposedID = 0; exposedID < exposedMethods_.size(); ++exposedID)
	{
		MethodDescription & description =
			internalMethods_[ exposedMethods_[ exposedID ] ];

		description.setExposedMsgID( exposedID, maxExposedMethodCount, pRange );
	}
}


/**
 *	This method supersedes the methods in this collection.
 */
void EntityMethodDescriptions::supersede()
{
	map_.clear();
	MF_ASSERT( internalMethods_.size() <= UINT_MAX );
	uint32 i = 0;
	for (List::iterator it = internalMethods_.begin();
		it != internalMethods_.end(); it++, ++i )
	{
		BW::string & str = const_cast<BW::string&>( it->name() );
		str = "old_" + str;

		map_[ it->name().c_str() ] = i;
	}
}


/**
 *	This method returns the number of methods associated with this entity.
 *
 *	@return The number of methods associated with this entity.
 */
unsigned int EntityMethodDescriptions::size() const
{
	return static_cast<uint>(internalMethods_.size());
}


/**
 *	This method returns the description of the method associated with this
 *	entity that has the input index number.
 */
MethodDescription * EntityMethodDescriptions::internalMethod(
		unsigned int index ) const
{
	if (index >= internalMethods_.size())
	{
		ERROR_MSG( "EntityDescription::serverMethod: "
				"Do not have server method %d. There are only %d.\n"
				"	Check that entities.xml is up-to-date.\n",
				(int)index, (int)internalMethods_.size() );

		return NULL;
	}

	return const_cast<MethodDescription *>( &internalMethods_[ index ] );
}


/**
 *	This method returns the description of the exposed method associated with
 *	this entity that has the input index number, or NULL if the input index is
 *	higher than the highest exposed index of an exposed method of this entity.
 */
const MethodDescription * EntityMethodDescriptions::exposedMethod(
	unsigned int index ) const
{
	if (index >= exposedMethods_.size())
	{
		ERROR_MSG( "EntityDescription::exposedMethod: "
				"Attempted to call exposed method id %u when only %" PRIzu
				" methods are exposed\n",
			index, exposedMethods_.size() );

		return NULL;
	}

	unsigned int internalIndex = exposedMethods_[ index ];

	MF_ASSERT( internalIndex < internalMethods_.size() );

	return &internalMethods_[ internalIndex ];
}


/**
 *	This method returns the description of the exposed method associated with
 *	this entity that has the input index number.
 */
const MethodDescription * EntityMethodDescriptions::exposedMethodFromMsgID(
	Mercury::MessageID msgID, BinaryIStream & data,
	const ExposedMethodMessageRange & range ) const
{
	// setExposedMsgIDs must have been called.
	MF_ASSERT( maxExposedMethodCount_ != -1 );

	int exposedID =
		range.exposedIDFromMsgID( msgID, data, maxExposedMethodCount_ );

	const MethodDescription * pMethodDesc = this->exposedMethod( exposedID );

	// Client methods do not use individual exposed sizes. They use the max
	// over all entity types and so cannot use this method.
	MF_ASSERT( pMethodDesc == NULL ||
		pMethodDesc->component() != MethodDescription::CLIENT );

	return pMethodDesc;
}


/**
 *	This method returns the description of the server method for the given
 *	name and component.
 */
MethodDescription *
	EntityMethodDescriptions::find( const char * name ) const
{
	MF_ASSERT_DEV( name );
	
	const Map::const_iterator iter = map_.find( name );

	return (iter != map_.end()) ? 
		this->internalMethod( iter->second ) : NULL;
}

#if ENABLE_WATCHERS

WatcherPtr EntityMethodDescriptions::pWatcher()
{
	typedef SmartPointer< SequenceWatcher< List > > SequenceWatcherPtr;

	static SequenceWatcherPtr watchMe = NULL;

	if (watchMe == NULL)
	{
		EntityMethodDescriptions * pNull = NULL;
		watchMe = new SequenceWatcher< List >( pNull->internalMethods_ );
		watchMe->setLabelSubPath( "name" );
		watchMe->addChild( "*", MethodDescription::pWatcher() ); 
	}

	return watchMe;
}
#endif // ENABLE_WATCHERS

BW_END_NAMESPACE

// entity_method_descriptions.cpp
