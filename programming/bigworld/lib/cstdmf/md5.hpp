#ifndef MD5_HPP
#define MD5_HPP

#include "bw_namespace.hpp"

#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

class BinaryIStream;
class BinaryOStream;

class MD5Data;

/**
 *  This class implements the standard MD5 cryptographic hash function.
 */
class CSTDMF_DLL MD5
{
public:
	/**
	 *  This struct manages the message digest associated with an MD5.
	 */
	struct CSTDMF_DLL Digest
	{
		static const size_t NUM_BYTES = 16;
		unsigned char bytes[NUM_BYTES];

		Digest() { }
		Digest( MD5 & md5 ) { *this = md5; }
		explicit Digest( const char * quotedDigest )
			{ this->unquote( quotedDigest ); }
		Digest & operator=( MD5 & md5 )
			{ md5.getDigest( *this ); return *this; }
		void clear();

		bool operator==( const Digest & other ) const;
		bool operator!=( const Digest & other ) const
			{ return !(*this == other); }

		bool operator<( const Digest & other ) const;

		BW::string quote() const;
		bool unquote( const BW::string & quotedDigest );

		bool isEmpty() const
		{
			for (size_t i = 0; i < sizeof( bytes ); ++i)
			{
				if (bytes[i] != '\0')
				{
					return false;
				}
			}

			return true;
		}

		static int streamSize() { return NUM_BYTES; }

	};

	MD5();
	~MD5();

	void append( const void * data, int numBytes );
	void getDigest( Digest & digest );

private:
	
	typedef unsigned char md5_byte_t; /* 8-bit byte */
	typedef unsigned int md5_word_t; /* 32-bit word */

	/**
	 * Define the state of the MD5 Algorithm.
	 */
	struct md5_state_t 
	{
		md5_word_t count[2];	//!< message length in bits, lsw first
		md5_word_t abcd[4];		//!< digest buffer
		md5_byte_t buf[64];		//!< accumulate block
	};
	
	void md5_process(md5_state_t *pms, const md5_byte_t *data /*[64]*/);
	void md5_init(md5_state_t *pms);
	void md5_append(md5_state_t *pms, const md5_byte_t *data, int nbytes);
	void md5_finish(md5_state_t *pms, md5_byte_t digest[16]);

	md5_state_t state_;
};

CSTDMF_DLL BinaryIStream& operator>>( BinaryIStream &is, MD5::Digest &d );
CSTDMF_DLL BinaryOStream& operator<<( BinaryOStream &is, const MD5::Digest &d );

BW_END_NAMESPACE

#endif // MD5_HPP
