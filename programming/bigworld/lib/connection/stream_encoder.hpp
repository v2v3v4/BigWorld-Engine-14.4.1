#ifndef STREAM_ENCODER_HPP
#define STREAM_ENCODER_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class BinaryIStream;
class BinaryOStream;

/**
 *	Interface for encrypting a stream.
 */
class StreamEncoder
{
public:
	virtual ~StreamEncoder() {}

	/**
	 *	Encode the given cleartext input stream into the ciphertext output
	 *	stream.
	 *
	 *	@param clearText	The input stream.
	 *	@param cipherText 	The output stream.
	 *
	 *	@return	true on success, false otherwise.
	 */
	virtual bool encrypt( BinaryIStream & clearText, 
			BinaryOStream & cipherText ) const = 0;

	/**
	 *	Decode the given ciphertext input stream into the output stream as
	 *	cleartext.
	 *
	 *	@param cipherText	The input stream.
	 *	@param clearText 	The output stream.
	 *
	 *	@return true on success, false otherwise.
	 */
	virtual bool decrypt( BinaryIStream & cipherText, 
			BinaryOStream & clearText ) const = 0;
};

BW_END_NAMESPACE

#endif // STREAM_ENCODER_HPP

