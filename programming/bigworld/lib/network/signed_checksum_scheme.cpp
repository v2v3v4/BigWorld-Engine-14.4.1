#include "pch.hpp"

#include "signed_checksum_scheme.hpp"

#include "cstdmf/memory_stream.hpp"

#include "network/public_key_cipher.hpp"


BW_BEGIN_NAMESPACE


/**
 *	Constructor.
 */
SignedChecksumScheme::SignedChecksumScheme( 
		Mercury::PublicKeyCipher & cipher ) :
	cipher_( cipher )
{}


/*
 *	Override from ChecksumScheme.
 */
size_t SignedChecksumScheme::streamSize() const
{
	return cipher_.size();
}


/*
 *	Override from ChecksumScheme.
 */
void SignedChecksumScheme::doReset()
{
	buffer_.reset();
}


/*
 *	Override from ChecksumScheme.
 */
void SignedChecksumScheme::readBlob( const void * data, size_t size )
{
	// See RSA_public_encrypt(3) for details about the padding allowance.
	static const int MAX_PADDING = 41;

	IF_NOT_MF_ASSERT_DEV( (size_t(buffer_.size()) + size) < 
			size_t(cipher_.size() - MAX_PADDING) )
	{
		return;
	}

	buffer_.addBlob( data, static_cast<int>(size) );
}


/*
 *	Override from ChecksumScheme.
 */
void SignedChecksumScheme::addToStream( BinaryOStream & out )
{
	buffer_.rewind();

	cipher_.privateEncrypt( buffer_, out );
}


/*
 *	Override from ChecksumScheme.
 */
bool SignedChecksumScheme::doVerifyFromStream( BinaryIStream & in )
{
	if (!cipher_.isGood())
	{
		errorString_ = "Signing key did not initialise successfully";
		return false;
	}

	buffer_.rewind();

	MemoryIStream streamSignature( in.retrieve( cipher_.size() ), 
		cipher_.size() );

	int expectedLength = buffer_.remainingLength();

	MemoryOStream decryptedSum( expectedLength );

	if (!cipher_.publicDecrypt( streamSignature, decryptedSum ))
	{
		this->setErrorMismatched();
		return false;
	}

	if ((decryptedSum.remainingLength() != expectedLength) ||
			(0 != memcmp( buffer_.retrieve( expectedLength ), 
				decryptedSum.retrieve( expectedLength ),
				expectedLength )))
	{
		this->setErrorMismatched();
		return false;
	}

	return true;
}


BW_END_NAMESPACE


// signed_checksum_scheme.cpp

