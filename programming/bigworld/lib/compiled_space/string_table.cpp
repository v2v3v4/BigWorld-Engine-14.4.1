#include "pch.hpp"

#include "string_table.hpp"

#include "cstdmf/string_builder.hpp"
#include "cstdmf/profiler.hpp"

#include <limits>

namespace BW {
namespace CompiledSpace {

// ----------------------------------------------------------------------------
StringTable::StringTable() :
	pReader_( NULL ),
	pStream_( NULL ),
	pStringData_( NULL )
{

}


// ----------------------------------------------------------------------------
StringTable::~StringTable()
{
	this->close();
}


// ----------------------------------------------------------------------------
bool StringTable::read( BinaryFormat& reader )
{
	PROFILER_SCOPED( StringTable_readFromSpace );

	typedef StringTableTypes::Header Header;
	typedef StringTableTypes::Entry Entry;

	MF_ASSERT( pReader_ == NULL );
	pReader_ = &reader;

	pStream_ = pReader_->findAndOpenSection( StringTableTypes::FORMAT_MAGIC,
		StringTableTypes::FORMAT_VERSION, "StringTable" );
	if (!pStream_)
	{
		ASSET_MSG( "Failed to map string table '%s' into memory.\n",
			pReader_->resourceID() );
		this->close();
		return false;
	}

	if (!readFromStream( pStream_ ))
	{
		ASSET_MSG( "'%s' is malformed.\n", pReader_->resourceID() );
		this->close();
		return false;
	}

	return true;
}


// ----------------------------------------------------------------------------
bool StringTable::readFromStream( BinaryFormat::Stream* pStream )
{
	typedef StringTableTypes::Header Header;
	typedef StringTableTypes::Entry Entry;

	if (!pStream->read( entries_ ))
	{
		ASSET_MSG( "Failed to read string table\n");
		return false;
	}

	// Read the rest in as string data
	uint32 stringDataSize = 0;
	pStream->read( stringDataSize );
	pStringData_ = (char*)pStream->readRaw( stringDataSize );
	if ( pStringData_ == NULL )
	{
		ASSET_MSG( "String table has invalid string data.\n");
		return false;
	}

	size_t accumulatedSize = 0;
	for (uint32 i = 0; i < this->size(); ++i)
	{
		Entry& entry = entries_[i];

		if (stringDataSize < entry.offset_ + entry.length_)
		{
			ASSET_MSG( "String table has incomplete data.\n" );
			return false;
		}

		accumulatedSize += entry.length_ + 1;

		// check we have a null in the expected place
		if (pStringData_[entry.offset_ + entry.length_] != NULL)
		{
			return false;
		}
	}

	if (stringDataSize != accumulatedSize)
	{
		return false;
	}

	return true;
}


// ----------------------------------------------------------------------------
void StringTable::close()
{
	entries_.reset();
	pStringData_ = NULL;

	if (pReader_ && pStream_)
	{
		pReader_->closeSection( pStream_ );
	}

	pReader_ = NULL;
	pStream_ = NULL;
}


// ----------------------------------------------------------------------------
bool StringTable::isValid() const
{
	return entries_.valid();
}


// ----------------------------------------------------------------------------
size_t StringTable::size() const
{
	MF_ASSERT( this->isValid() );
	return entries_.size();
}


// ----------------------------------------------------------------------------
const char* StringTable::entry( size_t idx ) const
{
	MF_ASSERT( this->isValid() );
	
	if (idx < this->size() )
	{
		return pStringData_ + entries_[idx].offset_;
	}
	else
	{
		return NULL;
	}	
}


// ----------------------------------------------------------------------------
size_t StringTable::entryLength( size_t idx ) const
{
	MF_ASSERT( this->isValid() );
	MF_ASSERT( idx < this->size() );

	return (size_t)entries_[idx].length_;
}

} // namespace CompiledSpace
} // namespace BW
