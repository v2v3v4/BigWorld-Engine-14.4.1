#include "pch.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/checksum_stream.hpp"

#include "network/public_key_cipher.hpp"
#include "network/sha_checksum_scheme.hpp"
#include "network/signed_checksum_scheme.hpp"

BW_BEGIN_NAMESPACE

namespace // (anonymous)
{
const BW::string privateKey = 
"-----BEGIN RSA PRIVATE KEY-----\n\
MIICXwIBAAKBgQChXiKCstcIVvsccBq3C/38ITpHrLSD4imW29q9wsUHBLfn2Yve\n\
KrKzFeYc4mLsJbDjqTrnDjMBmk4n/PvkZpklKwiFRy0b0EY1BF1Zl6FG8vyheMwI\n\
UoBUEbL1WDL46GmEL5y3wx8Xs+Iis/zFO3tn8IpMh/D4UUqjWk50rwzUMQIDAQAB\n\
AoGBAJ2kcOJuBFRJZRfrDK42MPHXJDBRMCiHEUonjhJD7Gdm3KLLjDCGVf1OL3eY\n\
UjuQtrYx5TFEVeAC9sdqBkqjUIfJlxl6RRNO9q0QIw933DGwuzqLeFFS4lJAYK72\n\
Mu8L6oan4QRtlc4RhHpBbFFN8Z0P17pVc7hSgZ2Ga2CAcGrhAkEA0t68/aa0DjV0\n\
UiVze+yz9Q8cW2tamqJKaJTGoLy3b/M56Hk7J+AnGJeziEWc4e2kjIutr+5IdhUG\n\
gSBGdKjAgwJBAMPnPCydGDAQct/Ezr7MLNWin3pjJ4w4m3N+iHKNXLJrBSRmpPLq\n\
gkheH7BgG3tKGLqRJhmwjPbnPfCXjao/0jsCQQCOBg7sGBc1arNJkIfTc31RFDhZ\n\
Klj/xUawYWPWZsR11i+ub9hz5vjuC16T7a7YTCKDtp/o2mhbf5W96msJr47ZAkEA\n\
syz6P3/LUKKqvnl98spBs3/SxxiLYlef7mlrmQIsJ0902486DKdqU5ArAaFVYVUq\n\
+vCo3VQ6CdUENNoYiv9mYwJBAMBLmrGUkD7q9q3Wxwdwf4GzQX02LiUH2E+KxKVQ\n\
FCsnIHcm7sA/J9pphVaFWsY1csO3uZQl2PtTFIIdv3r6P4s=\n\
-----END RSA PRIVATE KEY-----\n";

const BW::string publicKey =
"-----BEGIN PUBLIC KEY-----\n\
MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQChXiKCstcIVvsccBq3C/38ITpH\n\
rLSD4imW29q9wsUHBLfn2YveKrKzFeYc4mLsJbDjqTrnDjMBmk4n/PvkZpklKwiF\n\
Ry0b0EY1BF1Zl6FG8vyheMwIUoBUEbL1WDL46GmEL5y3wx8Xs+Iis/zFO3tn8IpM\n\
h/D4UUqjWk50rwzUMQIDAQAB\n\
-----END PUBLIC KEY-----\n";


const char BLOB_DATA[] = "Here is a blob";
const int32 testInt = 0xdeadbeef;
const BW::string testString( "some data" );

} // end namespace (anonymous)


TEST( SignedChecksumScheme )
{
	MemoryOStream base;

	Mercury::PublicKeyCipher * pPrivateKeyCipher = 
		Mercury::PublicKeyCipher::create( /* isPrivate */ true );
	pPrivateKeyCipher->setKey( privateKey );

	CHECK( pPrivateKeyCipher->isGood() );

	ChecksumSchemePtr pScheme = ChainedChecksumScheme::create(
			MD5SumScheme::create(),
			SignedChecksumScheme::create(
				*pPrivateKeyCipher ) );

	{
		ChecksumOStream writer( base, pScheme );
		BinaryOStream & stream = writer;
		stream << testInt << testString;

		stream.addBlob( BLOB_DATA, sizeof( BLOB_DATA ) );

		int sizeBeforeFinalising = writer.size();

		// When writer is destructed, finalise() is called automatically, test
		// here anyway so we can check sizes.
		writer.finalise();

		size_t checksumSize = writer.size() - sizeBeforeFinalising;

		CHECK_EQUAL( pScheme->streamSize(), checksumSize );
	}

	bw_safe_delete( pPrivateKeyCipher );

	base.rewind();

	Mercury::PublicKeyCipher * pPublicKeyCipher = 
		Mercury::PublicKeyCipher::create( /* isPrivate */ false );
	pPublicKeyCipher->setKey( publicKey );
	CHECK( pPublicKeyCipher->isGood() );

	pScheme = ChainedChecksumScheme::create(
			MD5SumScheme::create(),
			SignedChecksumScheme::create( *pPublicKeyCipher ) );

	{
		ChecksumIStream reader( base, pScheme );

		CHECK( !reader.error() );

		int32 testIntActual = 0;
		BW::string testStringActual;
		reader >> testIntActual >> testStringActual;

		CHECK_EQUAL( testIntActual, testInt );
		CHECK_EQUAL( testStringActual, testString );
		CHECK_EQUAL( reader.remainingLength(), int( sizeof( BLOB_DATA ) ) );

		const void * blobData = reader.retrieve( reader.remainingLength() );
		CHECK_EQUAL( memcmp( blobData, BLOB_DATA, sizeof( BLOB_DATA ) ), 0 );

		CHECK( reader.verify() );

		CHECK( reader.remainingLength() == 0 );
		CHECK( !reader.error() );

		CHECK( base.remainingLength() == 0 );
		CHECK( !base.error() );
	}

	bw_safe_delete( pPublicKeyCipher );
	bw_safe_delete( pPrivateKeyCipher );
}


TEST( SHAChecksumScheme )
{
	MemoryOStream base;

	ChecksumSchemePtr pScheme = SHAChecksumScheme::create();

	{
		ChecksumOStream writer( base, pScheme );
		BinaryOStream & stream = writer;
		stream << testInt << testString;

		stream.addBlob( BLOB_DATA, sizeof( BLOB_DATA ) );

		int sizeBeforeFinalising = writer.size();

		// When writer is destructed, finalise() is called automatically, test
		// here anyway so we can check sizes.
		writer.finalise();

		size_t checksumSize = writer.size() - sizeBeforeFinalising;

		CHECK_EQUAL( pScheme->streamSize(), checksumSize );
	}

	base.rewind();
	pScheme = SHAChecksumScheme::create();

	{
		ChecksumIStream reader( base, pScheme );

		CHECK( !reader.error() );

		int32 testIntActual = 0;
		BW::string testStringActual;
		reader >> testIntActual >> testStringActual;

		CHECK_EQUAL( testIntActual, testInt );
		CHECK_EQUAL( testStringActual, testString );
		CHECK_EQUAL( reader.remainingLength(), int( sizeof( BLOB_DATA ) ) );

		const void * blobData = reader.retrieve( reader.remainingLength() );
		CHECK_EQUAL( memcmp( blobData, BLOB_DATA, sizeof( BLOB_DATA ) ), 0 );

		CHECK( reader.verify() );
		CHECK( reader.remainingLength() == 0 );
		CHECK( !reader.error() );

		CHECK( base.remainingLength() == 0 );
		CHECK( !base.error() );
	}
}


BW_END_NAMESPACE

// test_network_checksum_schemes.cpp
