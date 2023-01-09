#ifndef ENTITY_EXTENSIONS_HPP
#define ENTITY_EXTENSIONS_HPP

#include "cstdmf/debug.hpp"

BW_BEGIN_NAMESPACE

class BWEntity;
class EntityExtension;
class EntityExtensionFactoryManager;

class EntityExtensions
{
public:
	typedef EntityExtension ** iterator;
	typedef EntityExtension * const * const_iterator;

	EntityExtensions();
	~EntityExtensions();

	void init( BWEntity & entity, EntityExtensionFactoryManager & factories );
	void clear();

	iterator begin()	{ return extensions_ ? extensions_ : 0; }
	const_iterator begin() const
						{ return extensions_ ? extensions_ : 0; }

	iterator end()
			{ return extensions_ ? extensions_ + numExtensions_ : 0; }
	const_iterator end() const
			{ return extensions_ ? extensions_ + numExtensions_ : 0; }

	EntityExtension * getForSlot( int slot ) const
	{
		MF_ASSERT( 0 <= slot && slot < numExtensions_ );
		return extensions_[ slot ];
	}

private:
	EntityExtension ** extensions_;
	int numExtensions_;
};

BW_END_NAMESPACE

#endif // ENTITY_EXTENSIONS_HPP
