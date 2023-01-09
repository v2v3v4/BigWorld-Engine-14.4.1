#ifndef PUT_IDS_TASK_HPP
#define PUT_IDS_TASK_HPP

#include "background_task.hpp"

#include "network/basictypes.hpp"

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class encapsulates the MySqlDatabase::putIDs() operation
 *	so that it can be executed in a separate thread.
 */
class PutIDsTask : public MySqlBackgroundTask
{
public:
	PutIDsTask( int numIDs, const EntityID * ids );

	// MySqlBackgroundTask overrides
	virtual void performBackgroundTask( MySql & conn );
	virtual void performMainThreadTask( bool succeeded );

private:
	typedef BW::vector< EntityID > Container;
	Container	ids_;
};

BW_END_NAMESPACE

#endif // PUT_IDS_TASK_HPP
