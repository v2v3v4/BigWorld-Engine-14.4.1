#include "pch.hpp"

#include <stdio.h>

#include "cstdmf/memory_stream.hpp"
#include "network/exposed_message_range.hpp"


BW_BEGIN_NAMESPACE

/**
 *  This test verifies ExposedMethodMessageRange.
 */
TEST( ExposedMethodMessageRange )
{
	const int START_MSG_ID = 10;
	const int END_MSG_ID = 20;
	ExposedMethodMessageRange range( START_MSG_ID, END_MSG_ID );

	const int NUM_EXPOSED = 300;

	for (int i = 0; i < NUM_EXPOSED; ++i)
	{
		MemoryOStream stream;
		int16 msgID;
		int16 subMsgID;

		range.msgIDFromExposedID( i, NUM_EXPOSED, msgID, subMsgID );

		CHECK( START_MSG_ID <= msgID );
		CHECK( msgID <= END_MSG_ID );

		if (subMsgID != -1)
		{
			stream << uint8( subMsgID );
		}

		int exposedID = range.exposedIDFromMsgID( msgID, stream, NUM_EXPOSED );

		CHECK_EQUAL( i, exposedID );
	}
}

BW_END_NAMESPACE

// test_exposed_message_range.cpp
