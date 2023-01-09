#ifndef STRING_MAP_HPP
#define STRING_MAP_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/bw_unordered_map.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_memory.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"
#include <cstring>
#include <climits>

BW_BEGIN_NAMESPACE

#if defined( __GNUC__ ) || defined( _WIN32 )

/**
 *	This class provides a fast-lookup for objects based on a BW::string.
 *
 *  This class has 90% of the functionality of StringMap with less issues in
 *  terms of what strings are allowed, capacities etc.
 */
template<typename _Ty>
class StringHashMap : public BW::unordered_map<BW::string, _Ty>
{
};

template<typename _Ty>
class WStringHashMap : public BW::unordered_map<BW::wstring, _Ty>
{
};

#endif // defined( __GNUC__ ) || defined( _WIN32 )

class ConstPolicy
{
public:
	const char * insert( const char * value )
	{
		return value;
	}

	void erase( const char * value )
	{
	}
};

class StrdupPolicy
{
public:
	const char * insert( const char * value )
	{
		return bw_strdup( value );
	}

	void erase( const char * value )
	{
		bw_free( const_cast<char*>( value ) );
	}
};

class FreePolicy
{
public:
	const char * insert( const char * value )
	{
		return value;
	}

	void erase( const char * value )
	{
		bw_free( const_cast<char*>( value ) );
	}
};


class DeletePolicy
{
public:
	const char * insert( const char * value )
	{
		return value;
	}
	
	void erase( const char * value )
	{
		delete[] value;
	}
};

struct StringLess
{
	bool operator()(const char * left, const char * right) const
	{
		return strcmp( left,right ) < 0;
	}
};

struct OwningStringRefPolicy
{
	BW::StringRef insert( const BW::StringRef& value )
	{
		char* copy = static_cast< char* >( bw_malloc( value.length() ) );
		memcpy( copy, value.data(), value.length() );
		return BW::StringRef( copy, value.length() );
	}

	void erase( const BW::StringRef& value )
	{
		bw_free( const_cast <char* >( value.data() ) );
	}
};

template< class MAPPED_TYPE, class POLICY = StrdupPolicy,
	class STRING_TYPE = const char *,
	class MAP_TYPE = BW::map< STRING_TYPE, MAPPED_TYPE, StringLess > >
class StringMap
{
public:
	typedef STRING_TYPE key_type;
	typedef MAPPED_TYPE mapped_type;
	typedef typename MAP_TYPE::value_type value_type;
	typedef typename MAP_TYPE::iterator iterator;
	typedef typename MAP_TYPE::const_iterator const_iterator;
	typedef typename MAP_TYPE::size_type size_type;

	StringMap()
	{
	}
	
	~StringMap()
	{
		this->clear();
	}
	
	// TODO: Find and destroy copying! Then move these methods to private.
	StringMap( const StringMap & other )
	{
		this->copy( other );
	}

	StringMap & operator=( const StringMap & other )
	{
		this->copy( other );
		return *this;
	}

	void clear()
	{
		iterator it = map_.begin();
		while (it != map_.end())
		{
			policy_.erase( it->first );
			++it;
		}
		map_.clear();
	}
	
	size_type size() const
	{
		return map_.size();
	}
	
	bool empty() const
	{
		return map_.empty();
	}

	std::pair< iterator, bool > insert( const value_type & value )
	{
		STRING_TYPE newKey = policy_.insert( value.first );
		std::pair< iterator, bool > retval =
			map_.insert( value_type( newKey, value.second ) );

		if (!retval.second)
		{
			// An extra alloc and free on failure, but hopefully this isn't
			// frequent
			policy_.erase( newKey );
		}

		return retval;
	}

	template< typename INPUT_ITERATOR >
	void insert( INPUT_ITERATOR begin, INPUT_ITERATOR end )
	{
		for (INPUT_ITERATOR iter = begin;
				iter != end;
				++iter)
		{
			this->insert( *iter );
		}
	}
	
	size_t erase( STRING_TYPE key )
	{
		iterator it = map_.find( key );
		if (it == map_.end())
		{
			return 0;
		}
		STRING_TYPE storedKey = it->first;
		map_.erase( it );
		policy_.erase( storedKey );
		
		return 1;
	}
	
	void erase( iterator it )
	{
		MF_ASSERT( it != map_.end() );
		STRING_TYPE storedKey = it->first;
		map_.erase( it );
		policy_.erase( storedKey );
	}
	
	iterator begin()
	{
		return map_.begin();
	}

	const_iterator begin() const
	{
		return map_.begin();
	}
	
	iterator end()
	{
		return map_.end();
	}

	const_iterator end() const
	{
		return map_.end();
	}
	
	iterator find( STRING_TYPE key )
	{
		return map_.find( key );
	}
	
	const_iterator find( STRING_TYPE key ) const
	{
		return map_.find( key );
	}
	
	MAPPED_TYPE & operator[]( STRING_TYPE key )
	{
		iterator it = map_.find( key );
		if (it != map_.end())
		{
			return it->second;
		}
		
		STRING_TYPE newKey = policy_.insert( key );
		return map_[ newKey ];
	}
private:
	void copy( const StringMap & other )
	{
		const_iterator it = other.map_.begin();

		while (it != other.map_.end())
		{
			STRING_TYPE newKey = policy_.insert( it->first );
			map_.insert( value_type( newKey, it->second ) );
			++it;
		}
	}
	POLICY policy_;
	MAP_TYPE map_;
};


template< class POLICY = StrdupPolicy,
	class STRING_TYPE = const char *,
	class SET_TYPE = BW::set< STRING_TYPE, StringLess > >
class StringSet
{
public:
	typedef typename SET_TYPE::value_type value_type;
	typedef typename SET_TYPE::iterator iterator;
	typedef typename SET_TYPE::const_iterator const_iterator;
	typedef typename SET_TYPE::size_type size_type;

	StringSet()
	{
	}
	
	~StringSet()
	{
		this->clear();
	}
	
	void clear()
	{
		iterator it = set_.begin();
		while (it != set_.end())
		{
			policy_.erase( *it );
			++it;
		}
		set_.clear();
	}
	
	size_type size() const
	{
		return set_.size();
	}
	
	bool empty() const
	{
		return set_.empty();
	}
	
	size_type count( const value_type& value ) const
	{
		return set_.count( value );
	}

	std::pair< iterator, bool > insert( const value_type & value )
	{
		STRING_TYPE newKey = policy_.insert( value );
		std::pair< iterator, bool > retval =
			set_.insert( newKey );

		if (!retval.second)
		{
			// An extra alloc and free on failure, but hopefully this isn't
			// frequent
			policy_.erase( newKey );
		}

		return retval;
	}

	template< typename INPUT_ITERATOR >
	void insert( INPUT_ITERATOR begin, INPUT_ITERATOR end )
	{
		for (INPUT_ITERATOR iter = begin;
				iter != end;
				++iter)
		{
			this->insert( *iter );
		}
	}
	
	size_t erase( STRING_TYPE key )
	{
		iterator it = set_.find( key );
		if (it == set_.end())
		{
			return 0;
		}
		STRING_TYPE storedKey = *it;
		set_.erase( it );
		policy_.erase( storedKey );
		
		return 1;
	}
	
	void erase( iterator it )
	{
		MF_ASSERT( it != set_.end() );
		STRING_TYPE storedKey = *it;
		set_.erase( it );
		policy_.erase( storedKey );
	}
	
	iterator begin()
	{
		return set_.begin();
	}

	const_iterator begin() const
	{
		return set_.begin();
	}
	
	iterator end()
	{
		return set_.end();
	}

	const_iterator end() const
	{
		return set_.end();
	}
	
	iterator find( STRING_TYPE key )
	{
		return set_.find( key );
	}
	
	const_iterator find( STRING_TYPE key ) const
	{
		return set_.find( key );
	}

private:
	StringSet ( const StringSet & other );

	StringSet & operator=( const StringSet & other );

	POLICY policy_;
	SET_TYPE set_;
};

template< class mapped_type, class policy = OwningStringRefPolicy >
class StringRefMap : public StringMap< mapped_type, policy, BW::StringRef, BW::map< BW::StringRef, mapped_type > >
{
};

template< class mapped_type, class policy = OwningStringRefPolicy >
class StringRefUnorderedMap : public StringMap< mapped_type, policy, BW::StringRef, BW::unordered_map< BW::StringRef, mapped_type > >
{
};

template< class policy = OwningStringRefPolicy >
class StringRefSet : public StringSet< policy, BW::StringRef, BW::set< BW::StringRef > >
{
};

#define USE_ORDERED_STRINGMAP
//#define ORDERED_STRINGMAP_DEBUG

#ifdef USE_ORDERED_STRINGMAP

#ifdef ORDERED_STRINGMAP_DEBUG

BW_END_NAMESPACE

#include <iostream>
extern void watchTypeCount( const char * basePrefix,
						   const char * typeName, int & var );
#include <typeinfo>

BW_BEGIN_NAMESPACE

#endif // ORDERED_STRINGMAP_DEBUG


/**
 *	Please use StringMap defined above instead of this class.  It has a 
 *	similar interface and speed but has less issues with non-English languages 
 *	and uses less memory.
 *
 *	This map is intended to be fast to look up. (i.e. StringMap::find).
 *	It tries to use memory wisely but doesn't let it impact speed:
 *
 *		base memory overhead = 256 bytes
 *		for each string added, it can be split up into these segments:
 *			'existing shared prefix' + 'new shared prefix' +
 *				'unshared remainder'
 *			extra memory overhead for each string =
 *				256 * len(new shared prefix) bytes
 *	Addition and removal is not brilliant, but it's not terribly
 *		slow either. Unless values are deleted they will remain in the
 *		order they were added.
 *	There are two size limits:
 *		#(entries) <= numeric_limits<_I>::max(), and
 *		#(sum(len(new shared prefix))) < numeric_limits<_I>::max()
 *	These could be removed (and memory overhead doubled) by doubling the
 *		size of the index type; the second template argument.
 *	Alternatively, you could use half the memory by halving the size
 *		of the index type, with a max of 127 entries.
 *	Whatever size you make it, the type should be a signed integer.
 *
 *	@note No allocator because I can't figure out how to get it working with
 *	vector and pair properly.
 */
template < class _Ty, class _I = int16 > class OrderedStringMap
{
public:
	// Typedefs
	/// This type is a shorthand for the map type.
	typedef OrderedStringMap<_Ty,_I> _Myt;

	/// This type is the type of the elements in the map.
	typedef std::pair< const char *, _Ty > value_type;
	/// This type is the type of the key.
	typedef const char * key_type;
	/// This type is the type of the referent.
	typedef _Ty referent_type;
	/// This type is a reference to referent type.
	typedef _Ty & _Tref;
	/// This type is the index type.
	typedef _I index_type;

	/// Type of the container.
	typedef BW::vector<value_type> _Imp;
	/// Type of the size.
	typedef typename _Imp::size_type size_type;
	/// Type of a difference
	typedef typename _Imp::difference_type difference_type;
	/// Type of a reference.
	typedef typename _Imp::reference reference;
	/// Type of a const reference.
	typedef typename _Imp::const_reference const_reference;
	/// The iterator type.
	typedef typename _Imp::iterator iterator;
	/// The const iterator type.
	typedef typename _Imp::const_iterator const_iterator;
	/// The reverse iterator type.
	typedef typename _Imp::reverse_iterator reverse_iterator;
	/// The const reverse iterator type.
	typedef typename _Imp::const_reverse_iterator const_reverse_iterator;

	/// @name Construction/Destruction
	//@{
	OrderedStringMap()
	{
		tables_.push_back(new Table());
	}

	OrderedStringMap( const _Myt & toCopy )
	{
		this->copy( toCopy );
	}

	~OrderedStringMap()
	{
		this->destruct();
	}

	_Myt & operator=( const _Myt & toCopy )
	{
		if (this != &toCopy)
		{
			this->total_clear();
			this->copy( toCopy );
		}
		return *this;
	}
	//@}

	// ---- Iterators and others handled by the implementor class ----

	/// This method returns an iterator for STL style iteration.
	iterator begin()						{ return (entries_.begin()); }
	/// This method returns an iterator for STL style iteration.
	const_iterator begin() const			{ return (entries_.begin()); }
	/// This method returns an iterator for STL style iteration.
	iterator end()							{ return (entries_.end()); }
	/// This method returns an iterator for STL style iteration.
	const_iterator end() const				{ return (entries_.end()); }

	/// This method returns a reverse iterator for STL style iteration.
	reverse_iterator rbegin()				{ return (entries_.rbegin()); }
	/// This method returns a reverse iterator for STL style iteration.
	const_reverse_iterator rbegin() const	{ return (entries_.rbegin()); }
	/// This method returns a reverse iterator for STL style iteration.
	reverse_iterator rend()					{ return (entries_.rend()); }
	/// This method returns a reverse iterator for STL style iteration.
	const_reverse_iterator rend() const		{ return (entries_.rend()); }

	/// This method returns the number of elements in this container.
	size_type size() const					{ return (entries_.size()); }

	/**
	 *	This method returns whether or not this container is empty.
	 *
	 *	@return True if empty, otherwise false.
	 */
	bool empty() const						{ return (entries_.empty()); }

	// ---- BW::string methods ----

	/**
	 *	This method erases the element associated with the input key.
	 *
	 *	@param key	The key associated with the element to delete.
	 */
	bool erase( const BW::string & key )
		{ return this->erase( key.c_str() ); }

	/**
	 *	This method returns an iterator referring to the element associated with
	 *	the input key.
	 *
	 *	@param key	The key associated with the element to find.
	 *
	 *	@return	If found, an iterator referring to the found element, otherwise
	 *		an iterator referring to the end.
	 */
	iterator find( const BW::string key )
		{ return this->find( key.c_str() ); }

	/**
	 *	This method returns an iterator referring to the element associated with
	 *	the input key.
	 *
	 *	@param key	The key associated with the element to find.
	 *
	 *	@return	If found, an iterator referring to the found element, otherwise
	 *		an iterator referring to the end.
	 */
	const_iterator find( const BW::string key ) const
		{ return this->find( key.c_str() ); }

	/**
	 *	This method implements the index operator that takes a BW::string.
	 *
	 *	@param key	The key of the value to look up.
	 *
	 *	@return The value associated with the input key. If no such value is
	 *		found, a new one with a default value is added to the map and
	 *		returned.
	 */
	referent_type & operator[]( const BW::string key )
		{ return this->operator[]( key.c_str() ); }

	/**
	 *	This method returns the number of bytes of dynamic memory
	 *	allocated by this map. Note that it is quite slow.
	 */
	uint32 sizeInBytes() const
	{
		uint32 sz = 0;
		sz += entries_.capacity() * sizeof(value_type);
		sz += tables_.capacity() * sizeof(Table*);
		sz += tables_.size() * sizeof(Table);
		for (uint i = 0; i < entries_.size(); i++)
			sz += strlen( entries_[i].first ) + 1;
		return sz;
	}

	/**
	 *	This method returns the sume of dynamic memory used by the map,
	 *	including heap overheads. It is not fast.
	 */
	uint32 memoryUse() const;

	/**
	 *	This method returns the maximum size the map can achieve.
	 */
	size_type max_size() const
	{
		return 1UL << ((8 * sizeof(index_type)) - 1);
	}

	/**
	 *	This method inserts the element between the input iterators into the
	 *	map.
	 *
	 *	@param _F	An iterator referring to the first element to insert.
	 *	@param _L	An iterator referring to the element after the last element
	 *				to insert.
	 */
	void insert( iterator _FF, iterator _LL )
	{
		for (; _FF != _LL; ++_FF)
			this->insert(*_FF);
	}

	/**
	 *	This method removes the elements between the input iterators from this
	 *	map.
	 *
	 *	@param _F	An iterator referring to the first element to remove.
	 *	@param _L	An iterator referring to the element after the last element
	 *				to remove.
	 */
	void erase( iterator _FF, iterator _LL )
	{
		for (; _FF != _LL; --_LL)
			this->erase(_LL);
	}

	/**
	 *	This method erases the element associated with the input key.
	 *
	 *	@param key	The key associated with the element to erase.
	 *
	 *	@return	True if the element was erased, otherwise false.
	 */
	bool erase( const key_type key )
	{
		iterator found = this->find( key );
		if (found == this->end()) return false;
		this->erase( found );
		return true;
	}


	/**
	 *	This method clears this map. After the call, there are no elements in
	 *	the map.
	 */
	void clear()
	{
		this->total_clear();
		tables_.push_back(new Table());
	}

	/**
	 *	This method returns a iterator that refers to the element associated
	 *	with the input key.
	 *
	 *	@param key	The key of the element to find.
	 *
	 *	@return	If found, returns an iterator referring to the element,
	 *			otherwise returns an iterator to the end of the map.
	 */
	iterator find( const key_type key )
	{
		return this->find_and_insert( key, NULL );
	}

	/**
	 *	This method returns a iterator that refers to the element associated
	 *	with the input key.
	 *
	 *	@param key	The key of the element to find.
	 *
	 *	@return	If found, returns an iterator referring to the element,
	 *			otherwise returns an iterator to the end of the map.
	 */
	const_iterator find( const key_type key ) const
	{
		return const_cast<_Myt*>(this)->find( key );
	}		// it is, trust me!

	/**
	 *	This method inserts the input element into the map.
	 *
	 *	@param val	The element to insert.
	 *
	 *	@return	An iterator pointing to the newly inserted element.
	 */
	iterator insert( const value_type& val )
	{
		return this->find_and_insert( val.first, &val.second );
	}

	/**
	 *	This method erases the element referred to by the input iterator.
	 */
	void erase( iterator iter )
	{
		this->erase_iterator( iter );
	}

	/**
	 *	This method implements the index operator.
	 */
	referent_type & operator[]( const key_type key )
	{
		iterator i = this->find_and_insert( key, NULL );
		if (i == end())
		{
			referent_type	rt = referent_type();
			i = this->find_and_insert( key, &rt );
		}
		return i->second;
	}

private:

	// Some utility functions
	void copy( const _Myt & toCopy )
	{
		for (typename Tables::const_iterator i = toCopy.tables_.begin();
			i != toCopy.tables_.end();
			i++)
		{
			tables_.push_back( new Table(**i) );
		}

		for (typename _Imp::const_iterator i = toCopy.entries_.begin();
			i != toCopy.entries_.end();
			i++)
		{
			entries_.push_back( *i );
			value_type & rvt = entries_.back();
			int len = strlen(i->first) + 1;
			rvt.first = new char[len];
			memcpy( (char*)rvt.first, i->first, len );
		}
	}

	void destruct()
	{
		for (typename Tables::iterator i = tables_.begin();
				i != tables_.end(); i++)
		{
			bw_safe_delete(*i);
		}
		tables_.clear();

		for (typename _Imp::iterator i = entries_.begin();
				i != entries_.end(); i++)
		{
			char* ptr = (char*)(i->first);
			bw_safe_delete_array(ptr);
		}
		entries_.clear();
	}

	void total_clear()
	{
		this->destruct();
		tables_.clear();
		entries_.clear();
	}

	// The guts of it...


	// the big find and insert
	iterator find_and_insert(const key_type key_in, const referent_type * pRef )
	{
		key_type key = key_in;

		index_type	tableIndex = 0;
		index_type	lastTableIndex;
		unsigned char	kk;
		do
		{
			kk = *key++;	// post-increment intentional
			lastTableIndex = tableIndex;
			tableIndex = tables_[tableIndex]->e[kk];
		} while (tableIndex > 0 && kk != 0);
			// theoretically we don't need to check kk!=0, because
			// table->e[0] will always be 0 or negative

		// if we stopped because we ran out of string it's not there.
		// same if we stopped because there was a zero there
		if (tableIndex >= 0)
		{
			//if (kk == 0) { MF_ASSERT( tableIndex == 0 ); }

			if (pRef == NULL) return entries_.end();
			return this->simple_insert( *pRef, lastTableIndex, key_in, kk );
		}

		// ok, we know where to look now
		iterator i = entries_.begin() + ~tableIndex;

		// check anything that's left of the string
		//  (pretty much just strcmp now)
		size_t keyDistFull = (key - key_in) - 1;
		MF_ASSERT( keyDistFull <= INT_MAX );
		int key_dist = ( int ) keyDistFull;
		key_type ekey = i->first + key_dist;
		unsigned char ekk = *ekey++;
		while( kk != 0 && ekk == kk )
		{
			kk = *key++;
			ekk = *ekey++;
		}

		// if they both got to their ends simultaneously, then we found it
		if (kk == 0 && ekk == 0)
		{			// ok, exact replacement ... easy
			if (pRef != NULL) i->second = *pRef;
			return i;
		}
		else
		{
			if (pRef == NULL) return entries_.end();
			return this->complicated_insert( *pRef, lastTableIndex, key_in, i,
				key_dist );
		}
	}


	// insert into the table indicated
	iterator simple_insert( const referent_type & ref, index_type tableIndex,
		key_type key_in, unsigned char kk )
	{
		// first put it in entries
		size_t len = std::strlen( key_in ) + 1;
		// cast to char* so borland can compile.
		entries_.push_back( std::make_pair( (const char *)(new char[len]), ref ) );
		// damn - I wish I could avoid the double-copy of the
		//  referent type above, but I can't see how to do it
		//  without using its (possibly nonexistent) default constructor.
		MF_ASSERT( entries_.size() <= INT_MAX );
		iterator i = entries_.begin() + (entries_.size() - 1);
		memcpy( (char*)i->first, key_in, len );

		// now put it in our tables
		tables_[tableIndex]->e[kk] = ~( ( int ) ( i - entries_.begin() ) );

		return i;
	}


	// create new tables for as long as we match with our contender
	// note that this function assumes that the two strings are NOT identical
	iterator complicated_insert( const referent_type & ref, index_type tableIndex,
		key_type key_in, iterator contender, int key_dist )
	{
		key_type	new_key = key_in + key_dist;
		key_type	ext_key = contender->first + key_dist;

		// add as many tables as are necessary
		Table * pOldTable = tables_[tableIndex];

		unsigned char	nkk,	ekk;
		while ((nkk = *new_key++) == (ekk = *ext_key++))
		{
			Table * pNewTable = new Table();
			tables_.push_back( pNewTable );

			size_t fullTableIndex = (&tables_.back()) - (&tables_.front());
			MF_ASSERT( fullTableIndex == ( ( size_t ) ( ( index_type ) fullTableIndex ) ) );

			tableIndex = ( index_type ) fullTableIndex;
			pOldTable->e[nkk] = tableIndex;

			pOldTable = pNewTable;
		}

		MF_ASSERT( entries_.size() <= INT_MAX );
		// put the existing one back in the latest table
		pOldTable->e[ekk] = ~( ( int ) ( contender - entries_.begin() ) );

		// and add the new one in the normal fashion
		return this->simple_insert( ref, tableIndex, key_in, nkk );
	}


	// your everyday erase
	void erase_iterator( iterator iter )
	{
		key_type	key = iter->first;

		// First erase it from the tables

		// find the entry in the tables (assumes it's there)
		index_type	tableIndex = 0;
		index_type	lastTableIndex;
		unsigned char	kk;
		do
		{
			kk = *key++;	// post-increment intentional
			lastTableIndex = tableIndex;
			tableIndex = tables_[tableIndex]->e[kk];
		} while (tableIndex > 0);

		// ok, we found it, so set it to 0
		tables_[lastTableIndex]->e[kk] = 0;

		// free the memory for the string
		delete[] (char*)(iter->first);

		// note that tables are never deleted once created,
		//  unless clear is used.

		// Now erase it from the entries

		// is this the last entry in the vector of entries?
		MF_ASSERT( entries_.size() <= INT_MAX );
		if (iter != entries_.begin() + entries_.size() - 1)
		{
			// ok, copy the last entry on top of ourselves
			*iter = entries_.back();
				// we can't erase the back of the vector until
				// we've finished with this iterator, 'coz it
				// could be made invalid if the vector moves
				// (not that it should move when being made smaller,
				// but you never know)

			// and fix it up in the tables (assumes it's there)
			key = iter->first;
			tableIndex = 0;
			do
			{
				kk = *key++;
				lastTableIndex = tableIndex;
				tableIndex = tables_[tableIndex]->e[kk];
			} while (tableIndex > 0);

			// ok, we found it, so set it to its new value
			tables_[lastTableIndex]->e[kk] =
				~( ( int ) ( iter - entries_.begin() ) );
		}

		// now erase the last iterator
		entries_.erase( entries_.begin() + entries_.size() - 1);

		// And that's it. phew!
	}




	// Data structures

	struct Table
	{
#ifndef ORDERED_STRINGMAP_DEBUG
		Table() { memset(e,0,sizeof(e)); }
#else
		Table()	{ memset(e,0,sizeof(e)); modTableCount( 1 ); }
		Table( const Table & t ) { *this = t; modTableCount( 1 ); }
		~Table() { modTableCount( -1 ); }
#endif // ORDERED_STRINGMAP_DEBUG

		index_type	e[256];
	};

	typedef BW::vector<Table*>	Tables;
	Tables		tables_;
	_Imp		entries_;

#ifdef ORDERED_STRINGMAP_DEBUG
	static void modTableCount( int way )
	{
		static int tableCount = 0x80000000;
		if (tableCount == 0x80000000)
		{
			tableCount = 0;
			watchTypeCount(
				"OrderedStringMapTables/", typeid(_Ty).name(), tableCount );
		}
		tableCount += way;
	}

	friend std::ostream& operator<<(std::ostream&, const _Myt&);
#endif // ORDERED_STRINGMAP_DEBUG
};


#ifdef ORDERED_STRINGMAP_DEBUG
template< class _Ty, class _I > std::ostream & operator<<(
	std::ostream& o, const OrderedStringMap<_Ty,_I> & dmap)
{
	typedef OrderedStringMap<_Ty,_I>	MAP;

	o << "Entries: " << std::endl;
	int	n = 0;
	for (MAP::const_iterator i = dmap.entries_.begin();
		i != dmap.entries_.end(); i++)
	{
		o << n << ": '" << i->first << "' -> " << i->second << std::endl;
		n++;
	}
	o << std::endl;

	n = 0;
	for (MAP::Tables::const_iterator i = dmap.tables_.begin();
		i != dmap.tables_.end(); i++ )
	{
		o << "Table " << n << ":" << std::endl;
		for (int j = 0; j < 128; j++)
		{
			MAP::index_type	k = (*i)->e[j];
			if (k>0)
				o << j << ": " << "Table " << k << std::endl;
			else if (k<0)
				o << j << ": " << "Entry " << ~k << std::endl;
		}
		o << std::endl;
		n++;
	}
	o << "Done" << std::endl << std::endl;

	return o;
}
#endif // ORDERED_STRINGMAP_DEBUG


template <class T, class I> uint32 memoryUse( const OrderedStringMap<T,I> & obj )
{ return obj.memoryUse(); }

template < class _Ty, class _I >
uint32 OrderedStringMap< _Ty, _I >::memoryUse() const
{
	uint32 sz = 0;
	sz += ::BW_NAMESPACE memoryUse( entries_ );
	for (uint i = 0; i < entries_.size(); i++)
		sz += ::BW_NAMESPACE memoryUse( entries_[i].first );
	sz += ::BW_NAMESPACE memoryUse( tables_ );
	for (uint i = 0; i < tables_.size(); i++)
		sz += ::BW_NAMESPACE memoryUse( tables_[i] );
	return sz;
}

#endif // USE_ORDERED_STRINGMAP

BW_END_NAMESPACE

#endif // STRING_MAP_HPP

