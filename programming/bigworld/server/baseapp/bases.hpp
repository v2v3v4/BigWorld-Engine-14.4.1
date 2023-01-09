#ifndef BASES_HPP
#define BASES_HPP

#include "network/basictypes.hpp"
#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

class Base;
class BinaryOStream;


/**
 *	This class represents a collection Base entities mapped by EntityID.
 */
class Bases
{
private:
	typedef BW::map< EntityID, Base * > Container;

public:
	typedef Container::iterator iterator;
	typedef Container::const_iterator const_iterator;

	typedef Container::size_type size_type;
	typedef Container::mapped_type mapped_type;
	typedef Container::key_type key_type;

	iterator begin()					{ return container_.begin(); }
	const_iterator begin() const		{ return container_.begin(); }

	iterator end()						{ return container_.end(); }
	const_iterator end() const			{ return container_.end(); }

	size_type size() const				{ return container_.size(); }

	bool empty() const					{ return container_.empty(); }

	iterator find( key_type key )		{ return container_.find( key ); }

	Base * findEntity( EntityID id ) const;

	void add( Base * pBase );
	void erase( EntityID id )			{ container_.erase( id ); }
	void clear()						{ return container_.clear(); }

	void discardAll( bool shouldDestroy = false );

	Base * findInstanceWithType( const char * typeName ) const;

	void addServicesToStream( BinaryOStream & stream ) const;

private:
	Container container_;
};

BW_END_NAMESPACE

#endif // BASES_HPP
