#ifndef ENTITY_DEF_CONSTANTS_HPP
#define ENTITY_DEF_CONSTANTS_HPP

#include "cstdmf/md5.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class stores some constants associated with entity definitions.
 */
class EntityDefConstants
{
public:
	EntityDefConstants() :
		digest_(),
		maxExposedClientMethodCount_( 0 ),
		maxExposedBaseMethodCount_( 0 ),
		maxExposedCellMethodCount_( 0 )
	{}

	EntityDefConstants( const MD5::Digest & digest,
			unsigned int maxExposedClientMethodCount,
			unsigned int maxExposedBaseMethodCount,
			unsigned int maxExposedCellMethodCount ) :
		digest_( digest ),
		maxExposedClientMethodCount_( maxExposedClientMethodCount ),
		maxExposedBaseMethodCount_(   maxExposedBaseMethodCount ),
		maxExposedCellMethodCount_(   maxExposedCellMethodCount )
	{
	}

	const MD5::Digest & digest() const	{ return digest_; }

	unsigned int maxExposedClientMethodCount() const
										{ return maxExposedClientMethodCount_; }
	unsigned int maxExposedBaseMethodCount() const
										{ return maxExposedBaseMethodCount_; }
	unsigned int maxExposedCellMethodCount() const
										{ return maxExposedCellMethodCount_; }

private:
	MD5::Digest digest_;
	unsigned int maxExposedClientMethodCount_;
	unsigned int maxExposedBaseMethodCount_;
	unsigned int maxExposedCellMethodCount_;
};

BW_END_NAMESPACE

#endif // ENTITY_DEF_CONSTANTS_HPP
