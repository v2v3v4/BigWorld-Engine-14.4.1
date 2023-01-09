#ifndef NAMED_OBJECT_HPP
#define NAMED_OBJECT_HPP

#include "bw_string.hpp"
#include "bw_namespace.hpp"
#include "stringmap.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class keeps named objects that register themselves into
 *	a global pool. The pool is searched by name.
 */
template <class Type> class NamedObject
{
public:
	typedef NamedObject<Type> This;
	
	NamedObject( const BW::string & name, Type object ) :
		name_( name ),
		object_( object )
	{
		This::add( this );
	}

	~NamedObject()
	{
		This::del( this );
	}

	static Type get( const BW::string & name )
	{
		if (pMap_ == NULL)
		{
			return NULL;
		}

		typename ObjectMap::iterator it = pMap_->find( name );
		if (it == pMap_->end())
		{
			return NULL;
		}

		return it->second->object_;
	}

	const BW::string & name() const { return name_; }

private:
	NamedObject( const NamedObject & no );
	NamedObject & operator=( const NamedObject & no );

	BW::string name_;
	Type object_;

	static void add( This * f )
	{
		if (pMap_ == NULL)
		{
			pMap_ = new ObjectMap();
		}

		(*pMap_)[ f->name_ ] = f;
	}

	static void del( This * f )
	{
		if (pMap_ == NULL)
		{
			return;
		}

		typename ObjectMap::iterator it = pMap_->find( f->name_ );

		if (it == pMap_->end() || it->second != f)
		{
			return;
		}

		pMap_->erase( it );

		if (pMap_->empty())
		{
			bw_safe_delete( pMap_ );
		}
	}

	typedef StringHashMap<This*> ObjectMap;
	static ObjectMap * pMap_;
};

BW_END_NAMESPACE

#endif // NAMED_OBJECT_HPP
