#ifndef RETURN_VALUES_HANDLER_HPP
#define RETURN_VALUES_HANDLER_HPP

#include "py_deferred.hpp"

#include "network/interfaces.hpp"
#include "pyscript/script.hpp"


BW_BEGIN_NAMESPACE

class MethodDescription;

/**
 *	This class is used to receive a reply from a remote script method call that
 *	has return values.
 */
class ReturnValuesHandler : public Mercury::ReplyMessageHandler
{
public:
	ReturnValuesHandler( const MethodDescription & methodDescription );

	void handleMessage( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data, void * arg );

	void handleException( const Mercury::NubException & exception, void * arg );

	PyObjectPtr getDeferred() const	{ return deferred_.get(); }

	static bool staticInit();

private:
	const MethodDescription & methodDescription_;
	PyDeferred deferred_;
};

BW_END_NAMESPACE

#endif // RETURN_VALUES_HANDLER_HPP
