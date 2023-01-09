#include "script/first_include.hpp"

#include "bases.hpp"

#include "base.hpp"

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This method returns the requested entity from the set of known entities.
 *
 *	@returns Entity matching id on success, NULL if unknown.
 */
Base * Bases::findEntity( EntityID id ) const
{
	const_iterator iter = container_.find( id );

	return (iter != container_.end()) ? iter->second : NULL;
}


/**
 *	This method adds the provided Entity to the set of known Bases.
 */
void Bases::add( Base * pBase )
{
	container_[ pBase->id() ] = pBase;
}


/**
 *	This method removes all known entities from the collection.
 */
void Bases::discardAll( bool shouldDestroy )
{
	BW::vector< BasePtr > copyOfBases;
	// We make a copy of bases_ because Base::discard removes that base
	// from BaseApp::bases_.
	copyOfBases.resize( container_.size() );
	{
		int i = 0;
		Bases::iterator iter = container_.begin();
		while (iter != container_.end())
		{
			MF_ASSERT( !iter->second->isDestroyed() );
			copyOfBases[i] = iter->second;
			++iter;
			++i;
		}
	}

	{
		BW::vector< BasePtr >::iterator iter = copyOfBases.begin();
		while (iter != copyOfBases.end())
		{
			if (shouldDestroy)
			{
				iter->get()->destroy(
						/*deleteFromDB*/ false, /*writeToDB*/ false );
			}
			else
			{
				iter->get()->discard();
			}
			++iter;
		}
	}
}


/**
 *	This method returns a Base entity that has a type that matches the string
 *	that is passed in.
 */
Base * Bases::findInstanceWithType( const char * typeName ) const
{
	Container::const_iterator iter = container_.begin();

	while (iter != container_.end())
	{
		Base * pBase = iter->second;

		if (strcmp( pBase->pType()->name(), typeName ) == 0)
		{
			Py_INCREF( pBase );
			return pBase;
		}

		++iter;
	}

	return NULL;
}


/**
 *	This method is used to re-register services with a newly spawned BaseAppMgr.
 */
void Bases::addServicesToStream( BinaryOStream & stream ) const
{
	stream << uint32( container_.size() );

	Container::const_iterator iter = container_.begin();

	while (iter != container_.end())
	{
		Base * pBase = iter->second;

		stream << pBase->pType()->description().name();
		stream << pBase->baseEntityMailBoxRef();

		++iter;
	}
}

BW_END_NAMESPACE

// bases.cpp
