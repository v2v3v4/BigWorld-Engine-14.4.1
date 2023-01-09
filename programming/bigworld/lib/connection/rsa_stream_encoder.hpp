#ifndef RSA_STREAM_ENCODER_HPP
#define RSA_STREAM_ENCODER_HPP

#include "stream_encoder.hpp"

#include "network/public_key_cipher.hpp"

BW_BEGIN_NAMESPACE

class RSAStreamEncoder : public StreamEncoder
{

public:
	RSAStreamEncoder( bool keyIsPrivate ):
		StreamEncoder(),
		pKey_( Mercury::PublicKeyCipher::create( keyIsPrivate ) )
	{
	}


	virtual ~RSAStreamEncoder()
	{
		bw_safe_delete( pKey_ );
	}


	/**
	 * 	Initialise the key from the given string.
	 */
	bool initFromKeyString( const BW::string & keyString )
	{
		return pKey_->setKey( keyString );
	}


	/**
	 * 	Override from StreamEncoder.
	 */
	virtual bool encrypt( BinaryIStream & clearText, 
			BinaryOStream & cipherText ) const
	{
		return (pKey_->publicEncrypt( clearText, cipherText ) != -1);
	}


	/**
	 *	Override from StreamEncoder.
	 */
	virtual bool decrypt( BinaryIStream & cipherText,
			BinaryOStream & clearText ) const
	{
		return (pKey_->privateDecrypt( cipherText, clearText ) != -1);
	}


private:
	RSAStreamEncoder( const RSAStreamEncoder & );
	RSAStreamEncoder & operator=( const RSAStreamEncoder & );

	Mercury::PublicKeyCipher* 	pKey_;
};

BW_END_NAMESPACE

#endif // RSA_STREAM_ENCODER_HPP

