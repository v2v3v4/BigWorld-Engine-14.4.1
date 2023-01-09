#ifndef NETMASK_HPP
#define NETMASK_HPP

#include "cstdmf/stdmf.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is used to represent a network mask.
 */
class NetMask
{
public:
	NetMask();

	bool			parse( const char* str );
	bool			containsAddress( uint32 addr ) const;

	void			clear();

private:
	uint32			mask_;
	int				bits_;
};

BW_END_NAMESPACE

#endif // NETMASK_HPP

