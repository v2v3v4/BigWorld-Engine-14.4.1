#include "pch.hpp"

#include "sha_checksum_scheme.hpp"

namespace BWOpenSSL
{
#include "openssl/sha.h"
} // end namespace BWOpenSSL

#include <string.h>


BW_BEGIN_NAMESPACE


/**
 *	This class implements the SHAChecksumScheme.
 */
class SHAChecksumScheme::Impl
{
public:
	Impl();
	~Impl();

	void reset();
	void readBlob( const void * rawData, size_t size );
	void addToStream( BinaryOStream & out );
	bool verifyFromStream( BinaryIStream & in );
private:
	BWOpenSSL::SHA256_CTX sha256_;
};


/**
 *	Factory method to create a SHA checksum scheme object.
 */
ChecksumSchemePtr SHAChecksumScheme::create()
{
	return new SHAChecksumScheme();
}


/**
 *	Constructor.
 */
SHAChecksumScheme::SHAChecksumScheme() :
	ChecksumScheme(),
	pImpl_( new Impl() )
{}


/**
 *	Constructor.
 */
SHAChecksumScheme::Impl::Impl()
{
	this->reset();
}


/**
 *	Destructor.
 */
SHAChecksumScheme::~SHAChecksumScheme()
{
	delete pImpl_;
}


/**
 *	Destructor.
 */
SHAChecksumScheme::Impl::~Impl()
{
}


/*
 *	Override from ChecksumScheme.
 */
size_t SHAChecksumScheme::streamSize() const
{
	return SHA256_DIGEST_LENGTH;
}


/*
 *	Override from ChecksumScheme.
 */
void SHAChecksumScheme::doReset()
{
	pImpl_->reset();
}


/**
 *	This method implements the SHAChecksumScheme::reset() method.
 */
void SHAChecksumScheme::Impl::reset()
{
	BWOpenSSL::SHA256_Init( &sha256_ );
}


/*
 *	Override from ChecksumScheme.
 */
void SHAChecksumScheme::readBlob( const void * rawData, size_t size )
{
	pImpl_->readBlob( rawData, size );
}


/**
 *	This method implements the SHAChecksumScheme::readBlob() method.
 */
void SHAChecksumScheme::Impl::readBlob( const void * rawData, size_t size )
{
	BWOpenSSL::SHA256_Update( &sha256_, rawData, size );
}


/*
 *	Override from ChecksumScheme.
 */
void SHAChecksumScheme::addToStream( BinaryOStream & out )
{
	pImpl_->addToStream( out );
}


/**
 *	This method implements the SHAChecksumScheme::addToStream() method.
 */
void SHAChecksumScheme::Impl::addToStream( BinaryOStream & out )
{
	unsigned char digest[SHA256_DIGEST_LENGTH];
	BWOpenSSL::SHA256_Final( digest, &sha256_ );

	out.addBlob( digest, SHA256_DIGEST_LENGTH );
}


BW_END_NAMESPACE


// sha_checksum_scheme.cpp
