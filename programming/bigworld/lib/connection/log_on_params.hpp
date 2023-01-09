#ifndef LOG_ON_PARAMS_HPP
#define LOG_ON_PARAMS_HPP

#include "cstdmf/md5.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/stdmf.hpp"

#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

class BinaryIStream;
class BinaryOStream;

class StreamEncoder;

/**
 *  This class wraps the parameters sent by the client during login.  These
 *  need to be passed from the loginapp -> dbapp -> baseapp.
 */
class LogOnParams : public SafeReferenceCount
{
public:
	/// An enumeration of flags for fields that are optionally streamed.
	typedef uint8 Flags;
	static const Flags HAS_DIGEST = 0x1;
	static const Flags HAS_ALL = 0x1;
	static const Flags PASS_THRU = 0xFF;

	LogOnParams() :
		flags_( HAS_ALL ),
		nonce_( std::rand() )
	{
		digest_.clear();
	}

	LogOnParams( const BW::string & username, const BW::string & password,
		const BW::string & encryptionKey ) :
		flags_( HAS_ALL ),
		username_( username ),
		password_( password ),
		encryptionKey_( encryptionKey ),
		nonce_( std::rand() )
	{
		digest_.clear();
	}

	bool addToStream( BinaryOStream & data, Flags flags = PASS_THRU,
		const StreamEncoder * pEncoder = NULL ) const;

	bool readFromStream( BinaryIStream & data,
		const StreamEncoder * pEncoder = NULL );

	Flags flags() const { return flags_; }

	const BW::string & username() const { return username_; }
	void username( const BW::string & username ) { username_ = username; }

	const BW::string & password() const { return password_; }
	void password( const BW::string & password ) { password_ = password; }

	const BW::string & encryptionKey() const { return encryptionKey_; }
	void encryptionKey( const BW::string & s ) { encryptionKey_ = s; }

	const MD5::Digest & digest() const { return digest_; }
	void digest( const MD5::Digest & digest ){ digest_ = digest; }

	/**
	 *	Check if security information is the same between two login requests.
	 */
	bool operator==( const LogOnParams & other )
	{
		return username_ == other.username_ &&
			password_ == other.password_ &&
			encryptionKey_ == other.encryptionKey_ &&
			nonce_ == other.nonce_;
	}

private:

	void addToStreamInternal( BinaryOStream & data, Flags flags ) const;
	bool readFromStreamInternal( BinaryIStream & data );

	Flags flags_;
	BW::string username_;
	BW::string password_;
	BW::string encryptionKey_;
	uint32 nonce_;
	MD5::Digest digest_;
};

typedef SmartPointer< LogOnParams > LogOnParamsPtr;


/**
 *  Streaming operator for LogOnParams that ignores encryption.
 */
inline BinaryOStream & operator<<( BinaryOStream & out,
	const LogOnParams & params )
{
	params.addToStream( out );
	return out;
}


/**
 *  Streaming operator for LogOnParams that ignores encryption.
 */
inline BinaryIStream & operator>>( BinaryIStream & in, LogOnParams & params )
{
	params.readFromStream( in );
	return in;
}

BW_END_NAMESPACE


#endif // LOG_ON_PARAMS_HPP

