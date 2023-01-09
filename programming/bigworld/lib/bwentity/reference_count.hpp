#ifndef BWENTITY_REFERENCE_COUNT_HPP
#define BWENTITY_REFERENCE_COUNT_HPP

#include "object_deleters.hpp"

BWENTITY_BEGIN_NAMESPACE

template< typename Ty >
class ReferenceCount : public BW::SafeReferenceCount
{
protected:
	virtual void destroy() const
	{
		Ty * thisObj = 
			static_cast<Ty *>( const_cast<ReferenceCount *>(this) );
		destroyObject( thisObj );
	}

	virtual ~ReferenceCount(){}
};

BWENTITY_END_NAMESPACE

#endif /* BWENTITY_REFERENCE_COUNT_HPP */
