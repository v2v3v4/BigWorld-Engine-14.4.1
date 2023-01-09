#ifndef PY_SERVER_FINDER_HPP
#define PY_SERVER_FINDER_HPP

#include "connection/server_finder.hpp"
#include "script/script_object.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: FindServerDoneCaller
// -----------------------------------------------------------------------------

/**
 *	This class is a simple helper to call the completion callback when
 *	everything is done.
 */
class FindServerDoneCaller : public ReferenceCount
{
public:
	FindServerDoneCaller( ScriptObject pCallback );
	~FindServerDoneCaller();

private:
	ScriptObject pCallback_;
};

typedef SmartPointer< FindServerDoneCaller > FindServerDoneCallerPtr;


// -----------------------------------------------------------------------------
// Section: ProbeReplyHandler
// -----------------------------------------------------------------------------

class PyServerProbeHandler : public ServerProbeHandler
{
public:
	PyServerProbeHandler( ScriptObject pCallback,
			const ServerInfo & serverInfo,
			FindServerDoneCallerPtr pDoneCaller );

	virtual void onKeyValue( const BW::string & key,
		const BW::string & value );

	virtual void onSuccess();

private:
	ScriptDict pDict_;
	ScriptObject pCallback_;
	FindServerDoneCallerPtr pDoneCaller_;
};


// -----------------------------------------------------------------------------
// Section: PyServerFinder
// -----------------------------------------------------------------------------

class PyServerFinder : public ServerFinder
{
public:
	PyServerFinder( PyObjectPtr pFoundCallback,
		PyObjectPtr pDetailsCallback, PyObjectPtr pDoneCallback );

protected:
	virtual void onServerFound( const ServerInfo & serverInfo );

private:
	ScriptObject pFoundCallback_;
	ScriptObject pDetailsCallback_;
	FindServerDoneCallerPtr pDoneCaller_;
};

BW_END_NAMESPACE

#endif // PY_SERVER_FINDER_HPP
