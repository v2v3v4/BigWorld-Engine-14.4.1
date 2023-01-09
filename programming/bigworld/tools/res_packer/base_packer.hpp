#ifndef __BASE_PACKER_HPP__
#define __BASE_PACKER_HPP__

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

class BasePacker
{
public:
	/**
	 *	Returns true if it can process the files, and if so, prepares itself.
	 */
	virtual bool prepare( const BW::string & src, const BW::string & dst ) = 0;

	/**
	 *	Output the class's string representation to stdout. Useful for XML.
	 */
	virtual bool print() = 0;

	/**
	 *	Pack the resource. Usualy requires copying the file to the destination,
	 *	and doing whatever processing is required.
	 */
	virtual bool pack() = 0;
};

BW_END_NAMESPACE

#endif // __BASE_PACKER_HPP__
