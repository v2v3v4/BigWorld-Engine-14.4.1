#ifndef ENTITY_AUTO_LOADER_HPP
#define ENTITY_AUTO_LOADER_HPP

#include "db_storage/entity_auto_loader_interface.hpp"

#include "cstdmf/bw_vector.hpp"
#include <utility>


BW_BEGIN_NAMESPACE

/**
 *	This class is used for loading entities from the database over a period of
 *	time.
 */
class EntityAutoLoader : public IEntityAutoLoader
{
public:
	EntityAutoLoader();

	virtual void reserve( int numEntities );

	virtual void start();

	virtual void abort();

	virtual void addEntity( EntityTypeID entityTypeID, DatabaseID dbID );

	virtual void onAutoLoadEntityComplete( bool isOK );

private:
	void checkFinished();
	void sendNext();

	bool allSent() const	{ return numSent_ >= int(entities_.size()); }

	typedef BW::vector< std::pair< EntityTypeID, DatabaseID > > Entities;
	Entities 	entities_;
	int 		numOutstanding_;
	int			maxOutstanding_;
	int 		numSent_;
	bool 		hasErrors_;
};

BW_END_NAMESPACE

#endif // ENTITY_AUTO_LOADER_HPP
