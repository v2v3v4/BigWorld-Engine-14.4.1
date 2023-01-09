#ifndef REPLAY_METADATA_HPP
#define REPLAY_METADATA_HPP

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/checksum_stream.hpp"


BW_BEGIN_NAMESPACE

class BinaryOStream;
class BinaryIStream;


/**
 *	This class is used to read or write user-supplied meta-data embedded into
 *	replay files.
 */
class ReplayMetaData
{
public:
	typedef BW::map< BW::string, BW::string > Collection;
	typedef Collection::const_iterator const_iterator;

	ReplayMetaData();

	void init( ChecksumSchemePtr pChecksumScheme );
	void init( size_t signatureLength );

	// Default copy constructor.
	// ReplayMetaData( const ReplayMetaData & other );
	// Default assignment constructor.
	// ReplayMetaData & operator=( const ReplayMetaData & other );

	void addToStream( BinaryOStream & stream,
		BinaryOStream * pSignature = NULL ) const;
	bool readFromStream( BinaryIStream & stream,
		BinaryOStream * pSignature = NULL,
		BW::string * pErrorString = NULL );

	/** This method returns the number of meta-data key-value pairs. */
	size_t size() const 	{ return collection_.size(); }

	/** This method clears the meta-data key-value pairs. */
	void clear() 			{ collection_.clear(); }

	void add( const BW::string & key, const BW::string & value );
	bool hasKey ( const BW::string & key ) const;
	const BW::string get( const BW::string & key,
		const char * defaultValue = "" ) const;

	/** This method returns an iterator for the beginning of the collection. */
	const_iterator begin() const	{ return collection_.begin(); }
	/** This method returns an iterator for the end of the collection. */
	const_iterator end() const 		{ return collection_.end(); }

	/** This method returns the size of the meta-data block when streamed. */
	size_t streamSize() const 		{ return streamSize_; }

	static bool checkSufficientLength( const void * data, size_t length,
		size_t signatureLength, size_t & metaDataLength );

	void swap( ReplayMetaData & other );

private:
	static size_t calculateStringPackedSize( const BW::string & s );

	typedef uint32 StreamSize;

	Collection 				collection_;
	StreamSize 				streamSize_;
	ChecksumSchemePtr 		pChecksumScheme_;
	size_t 					signatureLength_;
};

BW_END_NAMESPACE


#endif // REPLAY_METADATA_HPP
