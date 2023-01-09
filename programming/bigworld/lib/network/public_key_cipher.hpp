#ifndef PUBLIC_KEY_CIPHER_HPP
#define PUBLIC_KEY_CIPHER_HPP

#include "cstdmf/bw_string.hpp"

#include "cstdmf/binary_stream.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

class PublicKeyCipherImpl;

/**
 *  This class provides a simple interface to public key (i.e. asymmetric)
 *  encryption.
 */
class PublicKeyCipher
{
public:
	typedef BW::string Key;

	// Factory method
	static PublicKeyCipher * create( bool isPrivate );

protected:
	// This is protected to ensure that only the factory method is used.
	PublicKeyCipher( bool isPrivate ) :
		hasPrivate_( isPrivate ),
		isGood_( false )
	{}

private:
	// Prevent copy-construct or copy-assignment
	PublicKeyCipher(const PublicKeyCipher & other);
	PublicKeyCipher & operator=(const PublicKeyCipher & other);

public:
	virtual ~PublicKeyCipher()
	{}

	// PublicKeyCipher API
	bool setKey( const Key & key );

	/**
	 *	This method encrypts data with the public part of our key
	 */
	int publicEncrypt( BinaryIStream & clearStream,
		BinaryOStream & cipherStream ) const
	{
		return this->encrypt( clearStream, cipherStream, false );
	}

	/**
	 *	This method decrypts data with the public part of our key,
	 *	i.e., verifying a digital signature
	 */
	int publicDecrypt( BinaryIStream & cipherStream,
		BinaryOStream & clearStream ) const
	{
		return this->decrypt( cipherStream, clearStream, false );
	}

	/**
	 *	This method encrypts data with the private part of our key,
	 *	i.e., digitally signing the data
	 */
	int privateEncrypt( BinaryIStream & cipherStream,
		BinaryOStream & clearStream ) const
	{
		MF_ASSERT( hasPrivate_ );
		return this->encrypt( cipherStream, clearStream, true );
	}

	/**
	 *	This method decrypts data with the private part of our key
	 */
	int privateDecrypt( BinaryIStream & cipherStream,
		BinaryOStream & clearStream ) const
	{
		MF_ASSERT( hasPrivate_ );
		return this->decrypt( cipherStream, clearStream, true );
	}

	/**
	 *	This method returns true if this object is usable
	 */
	bool isGood() const { return isGood_; }

	/**
	 *	This method returns true if this object has the private key part
	 */
	bool hasPrivate() const { return hasPrivate_; }

	/**
	 *	This method returns a base64 representation of this key's public part.
	 */
	const char * c_str() const { return readableKey_.c_str(); }

	/**
	 *	This method returns a base64 representation of this key's public part.
	 */
	const Key & str() const { return readableKey_; }

	int size() const;

	/**
	 *	This method returns the size of the key in bits.
	 */
	int numBits() const { return this->size() * 8; }

	/**
	 *	This method returns the type of this key, for debugging purposes.
	 */
	const char * type() const { return hasPrivate_ ? "private" : "public"; }

private:
	// Internal methods
	void setReadableKey();

	int encrypt( BinaryIStream & clearStream, BinaryOStream & cipherStream,
		bool usePrivateKey ) const;
	int decrypt( BinaryIStream & cipherStream, BinaryOStream & clearStream,
		bool usePrivateKey ) const;

	const PublicKeyCipherImpl & impl() const;
	PublicKeyCipherImpl & impl();

	// Private data
	/// The base64 representation of this key's public part.
	Key readableKey_;

	/// Whether or not this key has a private part.
	bool hasPrivate_;

	/// Whether or not this key is usable
	bool isGood_;
};


} // namespace Mercury

BW_END_NAMESPACE

#endif // PUBLIC_KEY_CIPHER_HPP
