#include "pch.hpp"

#include "cstdmf/binary_stream.hpp"

#include "replay_metadata.hpp"


BW_BEGIN_NAMESPACE


/**
 *	Constructor.
 */
ReplayMetaData::ReplayMetaData() :
		collection_(),
		streamSize_( 0U ),
		pChecksumScheme_(),
		signatureLength_( 0U )
{}


/**
 *	This method initialises this object for reading/writing with
 *	signing/verification.
 *
 *	@param pChecksumScheme 	The checksum scheme to use for signing.
 */
void ReplayMetaData::init( ChecksumSchemePtr pChecksumScheme )
{
	collection_.clear();
	streamSize_ = 0U;
	pChecksumScheme_ = pChecksumScheme;
	signatureLength_ = pChecksumScheme->streamSize();
}


/**
 *	This method initialises this object for reading without verification.
 *
 *	@param signatureLength 	The expected length of the signature.
 */
void ReplayMetaData::init( size_t signatureLength )
{
	collection_.clear();
	streamSize_ = 0U;
	pChecksumScheme_ = NULL;
	signatureLength_ = signatureLength;
}


/**
 *	This method adds the meta-data block to the given stream.
 *
 *	This object must have been initialised with a proper checksum scheme.
 *
 *	@param rawStream 	The stream to add the meta-data block to.
 *	@param pSignature 	If non-NULL, this is filled with the signature that was
 *						written.
 */
void ReplayMetaData::addToStream( BinaryOStream & rawStream,
		BinaryOStream * pSignature ) const
{
	MF_ASSERT( pChecksumScheme_ );

	ChecksumOStream stream( rawStream, pChecksumScheme_,
		/* shouldReset */ false );

	stream << streamSize_;

	const_iterator iter = collection_.begin();
	while (iter != collection_.end())
	{
		stream << iter->first << iter->second;
		++iter;
	}

	stream.finalise( pSignature );
}


/**
 *	This method reads the meta-data block from the given stream.
 *
 *	@param pSignature 	If non-NULL, this will be filled with the signature
 *						read from the stream.
 */
bool ReplayMetaData::readFromStream( BinaryIStream & rawStream,
		BinaryOStream * pSignature /* = NULL */,
		BW::string * pErrorString /* = NULL */ )
{
	ChecksumIStream stream( rawStream, pChecksumScheme_,
		/* shouldReset */ false );

	stream >> streamSize_;

	if (streamSize_ > static_cast<size_t>( stream.remainingLength() ))
	{
		if (pErrorString)
		{
			*pErrorString = "Insufficient data for meta-data";
		}
		return false;
	}

	MemoryIStream myStream( stream.retrieve( streamSize_ ), streamSize_ );

	Collection collection;
	while (myStream.remainingLength() && !myStream.error())
	{
		BW::string key;
		BW::string value;
		myStream >> key >> value;

		if (!myStream.error())
		{
			collection.insert( Collection::value_type( key, value ) );
		}
	}

	if (myStream.error())
	{
		if (pErrorString)
		{
			*pErrorString = "Meta-data corrupted";
		}
		return false;
	}

	collection_.swap( collection );

	if (!pChecksumScheme_.exists() && pSignature)
	{
		pSignature->transfer( rawStream, static_cast<int>( signatureLength_ ) );
	}
	else if (!stream.verify( pSignature ))
	{
		if (pErrorString)
		{
			*pErrorString = pChecksumScheme_->errorString();
		}
		return false;
	}

	return true;
}


/**
 *	This method adds the key and associated value to the meta-data.
 *
 *	@param key 		The key string.
 *	@param value 	The associated value string.
 */
void ReplayMetaData::add( const BW::string & key, const BW::string & value )
{
	Collection::iterator iter = collection_.find( key );
	if (iter != collection_.end())
	{
		streamSize_ -= static_cast<StreamSize>(
			(this->calculateStringPackedSize( key ) +
				this->calculateStringPackedSize( iter->second )) );
		collection_.erase( iter );
	}

	collection_.insert( Collection::value_type( key, value ) );
	streamSize_ += static_cast< StreamSize >( 
		this->calculateStringPackedSize( key ) +
			this->calculateStringPackedSize( value ) );
}


/**
 *	This method returns whether the given key is present.
 *
 *	@param key 	The key to check.
 *	@return 	true if the key is in the collection, false otherwise.
 */
bool ReplayMetaData::hasKey( const BW::string & key ) const
{
	return (collection_.count( key ) != 0);
}


/**
 *	This method returns the associated value for the given key, or the default
 *	value if none was found.
 *
 *	@param key 				The key to search.
 *	@param defaultValue 	The default value to use instead, if the key is not
 *							in the collection.
 *
 *	@return 				The value associated with the given key, or the
 *							default value if the key is not in the collection.
 */
const BW::string ReplayMetaData::get( const BW::string & key,
		const char * defaultValue ) const
{
	Collection::const_iterator iter = collection_.find( key );

	if (iter == collection_.end())
	{
		return defaultValue;
	}

	return iter->second;
}


/**
 *	This method checks whether the given data in memory has the necessary
 *	length to parse a complete replay meta-data block.
 *
 *	@param data 			The data block.
 *	@param length 			The size of the data block.
 *	@param signatureLength 	The length of signatures used in this replay file.
 *	@param metaDataLength 	This is filled with the length of the meta-data
 *							block, if it is known.
 *
 *	@return 				true if there is sufficient data to read the
 *							meta-data block, or false otherwise.
 */
bool ReplayMetaData::checkSufficientLength( const void * data, size_t length,
		size_t signatureLength, size_t & metaDataLength )
{
	if (length < sizeof(StreamSize))
	{
		return false;
	}

	StreamSize streamSize = *reinterpret_cast< const StreamSize * >( data );

	metaDataLength = sizeof(StreamSize) + streamSize + signatureLength;
	return (length >= metaDataLength);
}


/**
 *	This method returns the packed size length of the given string.
 */
size_t ReplayMetaData::calculateStringPackedSize( const BW::string & s )
{
	// This is based off BinaryOStream::appendString() /
	// BinaryOStream::writePackedInt().
	return ((s.size() < 256) ? 1 : 3) + s.size();
}


/**
 *	This method is used to swap this meta-data contents with those of the given
 *	object.
 *
 *	@param other 	The other meta-data object to swap contents with.
 */
void ReplayMetaData::swap( ReplayMetaData & other )
{
	collection_.swap( other.collection_ );
	std::swap( streamSize_,  other.streamSize_ );
	std::swap( pChecksumScheme_, other.pChecksumScheme_ );
	std::swap( signatureLength_, other.signatureLength_ );
}


BW_END_NAMESPACE


// replay_metadata.cpp
