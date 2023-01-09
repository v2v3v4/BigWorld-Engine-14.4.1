/**
 * 	This file defines the IDGenerator interface, and the datatypes used
 * 	by it.
 */ 	

#ifndef _ID_GENERATOR_HEADER
#define _ID_GENERATOR_HEADER

#include "cstdmf/stdmf.hpp"
#include "cstdmf/binary_stream.hpp"


BW_BEGIN_NAMESPACE

/**
 * 	This structure stores a continuous block of IDs, such that 
 * 	(start <= id <= end).
 */ 	
struct IDBlock
{
	uint32 	start;
	uint32	end;
};

/**
 * 	This type stores multiple blocks of IDs. Often a request for IDs
 * 	cannot be satisfied by a single continuous block of IDs, so multiple
 * 	blocks are returned.
 */ 	
TYPEDEF_STREAMING_VECTOR( IDBlock, IDBlockVector );


/**
 *	This interface defines an object that can generate blocks of IDs.
 */	
class IDGenerator
{
public:
	virtual ~IDGenerator() {};
	virtual void getBlocks(IDBlockVector& blockVector, 
			uint32 count, uint32 tolerance) = 0;

	virtual void putBlocks(const IDBlockVector& blockVector) = 0;		
};

BW_END_NAMESPACE

#endif
