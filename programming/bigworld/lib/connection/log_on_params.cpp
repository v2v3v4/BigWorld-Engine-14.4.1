#include "pch.hpp"

#include "log_on_params.hpp"

#include "stream_encoder.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/memory_stream.hpp"

#include "network/public_key_cipher.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This method writes the login parameters to a stream.  If pEncoding is
 *  non-NULL it will be used to encrypt the stream.
 */
bool LogOnParams::addToStream( BinaryOStream & data, Flags flags,
		const StreamEncoder * pEncoder ) const
{
	if (pEncoder)
	{
		// This is an intermediate stream we use for assembling the plaintext if
		// we're encrypting.
		MemoryOStream clearText;

		this->addToStreamInternal( clearText, flags );
		return pEncoder->encrypt( clearText, data );
	}
	else
	{
		this->addToStreamInternal( data, flags );
		return true;
	}
}


/**
 *  This method reads the login parameters from a stream.  If pEncoding is
 *  non-NULL, the stream will be decrypted.
 */
bool LogOnParams::readFromStream( BinaryIStream & data,
		const StreamEncoder * pEncoder )
{
	if (pEncoder)
	{
		// Intermediate stream used for writing the plaintext for encrypted
		// streams.
		MemoryOStream clearText( data.remainingLength() );

		if (!pEncoder->decrypt( data, clearText ))
		{
			return false;
		}

		return this->readFromStreamInternal( clearText );
	}
	else
	{
		return this->readFromStreamInternal( data );
	}
}



/**
 *	Non-encrypting streaming method.
 */
void LogOnParams::addToStreamInternal( BinaryOStream & data, 
		Flags flags ) const
{
	// Use the stored flags if none were passed.
	if (flags == PASS_THRU)
	{
		flags = flags_;
	}

	data << flags << username_ << password_ << encryptionKey_;

	if (flags & HAS_DIGEST)
	{
		data << digest_;
	}

	data << nonce_;
}


/**
 *	Non-decrypting de-streaming method.
 */
bool LogOnParams::readFromStreamInternal( BinaryIStream & data )
{
	/*
	*  If parsing crashes in development mode, there's probably res 
	*  tree mismatch between client and server (i.e. encryption 
	*  of credentials)
	*/
	data >> flags_ >> username_ >> password_ >> encryptionKey_;

	if (flags_ & HAS_DIGEST)
	{
		data >> digest_;
	}

	data >> nonce_;

	return !data.error();
}

BW_END_NAMESPACE

// log_on_params.cpp

