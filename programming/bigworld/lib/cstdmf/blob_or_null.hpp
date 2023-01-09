#ifndef BLOB_OR_NULL_HPP
#define BLOB_OR_NULL_HPP

#include "cstdmf/stdmf.hpp"

BW_BEGIN_NAMESPACE

class BinaryIStream;
class BinaryOStream;

/**
 *	This class is used to represent a Binary Large OBject.
 */
class BlobOrNull
{
public:
	BlobOrNull() : pData_( NULL ), length_( 0 ) {}	// NULL blob
	BlobOrNull( const char * pData, uint32 len ) :
		pData_( pData ), length_( len ) {}

	bool isNull() const 	{ return (pData_ == NULL); }

	const char * pData() const	{ return pData_; }
	uint32 length() const		{ return length_; }

private:
	const char *	pData_;
	uint32			length_;
};

BinaryOStream & operator<<( BinaryOStream & stream, const BlobOrNull & blob );
BinaryIStream & operator>>( BinaryIStream & stream, BlobOrNull & blob );

BW_END_NAMESPACE

#endif // BLOB_OR_NULL_HPP
