#include "pch.hpp"
#include "binary_stream.hpp"

#include <sstream>
#include <iomanip>
#include <stdlib.h>

BW_BEGIN_NAMESPACE

// Static return buffer returned from retrieve() when not enough data for
// request.
char BinaryIStream::errBuf[ BS_BUFF_MAX_SIZE ];


/**
 *	This method creates a debug string for outputing to logs. It is a hex
 *	representation of the remaining bytes in the stream.
 */
BW::string BinaryIStream::remainingBytesAsDebugString( int maxBytes/* = 16 */,
		bool shouldConsume /* = true */ )
{
	BW::stringstream output;

	int numBytes = this->remainingLength();
	
	if (maxBytes > 0)
	{
		numBytes = std::min( this->remainingLength(), maxBytes );

		output << "Bytes (" <<
			numBytes << " of " << this->remainingLength() << "):";
	}

	output << std::hex << std::setfill( '0' );

	const uint8 * data = 
		reinterpret_cast< const uint8 * >( this->retrieve( 0 ) );

	for (int i = 0; i < numBytes; ++i)
	{
		if (i % 16 == 0)
		{
			output << std::endl;
		}
		else if (i % 8 == 0)
		{
			output << "  ";
		}
		else
		{
			output << " ";
		}

		output << std::setw( 2 ) << int(*(data++) );
	}

	if (shouldConsume)
	{
		this->finish();
	}

	return output.str();
}

BW_END_NAMESPACE

// binary_stream.cpp
