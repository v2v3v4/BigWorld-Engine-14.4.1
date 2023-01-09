#ifndef ENTITY_AUTO_LOADER_INTERFACE_HPP
#define ENTITY_AUTO_LOADER_INTERFACE_HPP

#include "network/basictypes.hpp"

#include "cstdmf/bw_vector.hpp"
#include <utility>


BW_BEGIN_NAMESPACE

/**
 *	This class is used for loading entities from the database over a period of
 *	time.
 */
class IEntityAutoLoader
{
public:
	virtual ~IEntityAutoLoader() {}

	virtual void reserve( int numEntities ) = 0;

	virtual void start() = 0;

	virtual void abort() = 0;

	virtual void addEntity( EntityTypeID entityTypeID, DatabaseID dbID ) = 0;

	virtual void onAutoLoadEntityComplete( bool isOK ) = 0;

private:
	void checkFinished();
	bool sendNext();

	bool allSent() const	{ return numSent_ >= int(entities_.size()); }

	typedef BW::vector< std::pair< EntityTypeID, DatabaseID > > Entities;
	Entities 	entities_;
	int 		numOutstanding_;
	int 		numSent_;
	bool 		hasErrors_;
};

BW_END_NAMESPACE

#endif // ENTITY_AUTO_LOADER_INTERFACE_HPP
