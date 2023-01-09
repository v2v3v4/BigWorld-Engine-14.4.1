#ifndef __CHUNK_PACKER_HPP__
#define __CHUNK_PACKER_HPP__


#include "base_packer.hpp"
#include "packers.hpp"

#include <string>


BW_BEGIN_NAMESPACE

/**
 *	This class strips the .chunk files of unwanted data. In the client, it
 *	removes server entities, and in the server it removes client-only entities.
 */
class ChunkPacker : public BasePacker
{
public:
	virtual bool prepare( const BW::string & src, const BW::string & dst );
	virtual bool print();
	virtual bool pack();

private:
	DECLARE_PACKER()
	BW::string src_;
	BW::string dst_;
};

BW_END_NAMESPACE

#endif // __CHUNK_PACKER_HPP__
