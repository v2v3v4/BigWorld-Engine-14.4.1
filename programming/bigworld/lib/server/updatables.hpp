#ifndef UPDATABLES_HPP
#define UPDATABLES_HPP

#include "cstdmf/bw_namespace.hpp"

#include <cstddef>
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class Updatable;

class Updatables
{
public:
	Updatables( int numLevels = 2 );

	bool add( Updatable * pObject, int level );
	bool remove( Updatable * pObject );

	void call();

	size_t size() const	{ return objects_.size(); }

private:
	Updatables( const Updatables & );
	Updatables & operator=( const Updatables & );

	void adjustForAddedObject();

	BW::vector< Updatable * > objects_;
	BW::vector< int > levelSize_;
	bool inUpdate_;
	int deletedObjects_;
};

BW_END_NAMESPACE

#endif // UPDATABLES_HPP
