#include "pch.hpp"
#include "glyph_reference_holder.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This method increments the reference to the given character.
 *	@param	c		wide character to increase the reference to.
 */
void GlyphReferenceHolder::incRef( wchar_t c )
{
	BW_GUARD;
	GlyphReferenceCountMap::iterator it = refCounts_.find(c);
	if ( it != refCounts_.end() )
	{
		it->second++;
	}
	else
	{
		refCounts_[c] = 1;
	}
}


/**
 *	This method erases the reference for the given character.
 *	@param	c		wide character to erase.
 */
void GlyphReferenceHolder::erase( wchar_t c )
{
	refCounts_.erase(c);
}


/**
 *	This method erases the reference for the given character.
 *	@param	it		ref holder iterator.
 */
GlyphReferenceHolder::GlyphReferenceCountMap::iterator 
GlyphReferenceHolder::erase( GlyphReferenceCountMap::iterator it )
{
	return refCounts_.erase(it);
}


/**
 *	This method decrements the reference to the given character.
 *	@param	c		wide character to decrease the reference to.
 *	@param	num		the number of references to release.
 */
int GlyphReferenceHolder::decRef( wchar_t c, int num )
{
	BW_GUARD;
	GlyphReferenceCountMap::iterator it = refCounts_.find(c);
	MF_ASSERT_DEV( it != refCounts_.end() )
	it->second -= num;
	return it->second;
}


/**
 *	This method returns the reference count for the given character.
 *	@param	c		wide character to return the reference count for.
 */
int GlyphReferenceHolder::refCount( wchar_t c ) const
{
	BW_GUARD;
	GlyphReferenceCountMap::const_iterator it = refCounts_.find(c);
	if ( it != refCounts_.end() )
	{
		return it->second;
	}
	else
	{
		return 0;
	}
}


/**
 *	This method lists all the reference counts as debug messages.
 */
void GlyphReferenceHolder::report() const
{
	BW_GUARD;
	GlyphReferenceCountMap::const_iterator it = refCounts_.begin();
	GlyphReferenceCountMap::const_iterator en = refCounts_.end();
	while (it != en)
	{
		DEBUG_MSG( "%c - %d\n", it->first, it->second );
		++it;
	}
}


/**
 *	This method releases the references held by another holder.
 *
 *	@param	other	the holder of the references to release.
 */
void GlyphReferenceHolder::releaseRefs( const GlyphReferenceHolder& other )
{
	BW_GUARD;
	GlyphReferenceCountMap::const_iterator it = other.refCounts_.begin();
	GlyphReferenceCountMap::const_iterator en = other.refCounts_.end();
	while (it != en)
	{
		this->decRef( it->first, it->second );
		++it;
	}
}


/**
 *	This method releases all held references.
 */
void GlyphReferenceHolder::reset()
{
	refCounts_.clear();
}


/**
 *	This method returns an iterator to the beginning of the reference counts
 *	container.
 */
GlyphReferenceHolder::GlyphReferenceCountMap::iterator GlyphReferenceHolder::begin()
{
	return refCounts_.begin();
}


/**
 *	This method returns an iterator to the end of the reference counts
 *	container.
 */
GlyphReferenceHolder::GlyphReferenceCountMap::iterator GlyphReferenceHolder::end()
{
	return refCounts_.end();
}

BW_END_NAMESPACE

// glyph_reference_holder.cpp
