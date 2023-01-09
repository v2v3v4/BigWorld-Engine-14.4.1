#ifndef NUB_EXCEPTION_HPP
#define NUB_EXCEPTION_HPP

#include "basictypes.hpp"
#include "misc.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 *	This is the base class for all exception types that are thrown by various
 *	Mercury classes.
 *
 *	@ingroup mercury
 */
class NubException
{
public:
	NubException( Reason reason, const Address & addr = Address::NONE );
	~NubException() {};
	Reason reason() const;
	bool getAddress( Address & addr ) const;

private:
	Reason		reason_;
	Address		address_;
};

#include "nub_exception.ipp"

} // namespace Mercury

BW_END_NAMESPACE

#endif // NUB_EXCEPTION_HPP
