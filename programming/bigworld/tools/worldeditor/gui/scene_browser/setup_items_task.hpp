#ifndef SETUP_ITEMS_TASK_HPP
#define SETUP_ITEMS_TASK_HPP


#include "world/item_info_db.hpp"
#include "list_group_states.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class processes list items to set them in groups, filter by search,
 *	gather statistics, etc.
 */
class SetupItemsTask
{
public:
	typedef BW::vector< ItemInfoDB::ItemPtr > Items;
	typedef BW::set< ItemInfoDB::Type > Types;

	SetupItemsTask( const BW::string & search,
		const ListGroupStates & groupStates, const Types & allowedTypes,
		ItemInfoDB::ComparerPtr pComparer );

	virtual ~SetupItemsTask() {}

	virtual void execute();

	virtual void results( Items & retItems, int & retNumItems,
									int & retNumTris, int & retNumPrimitives );

private:
	// input params
	const ListGroupStates & groupStates_;
	Types allowedTypes_;
	ItemInfoDB::ComparerPtr pComparer_;

	// return params
	Items retItems_;
	int retNumItems_;
	int retNumTris_;
	int retNumPrimitives_;

	// Other members
	typedef std::pair< BW::string, bool > SearchWord;
	typedef BW::vector< SearchWord > SearchWords;
	SearchWords searchWords_;
	bool executed_;

	bool itemInSearch( ItemInfoDB::Item * pItem ) const;

	void splitSearchInWords( const BW::string & search );
};


/**
 *	This class helps in processing items in a separate thread.
 */
class SetupItemsBackgroundTask : public BackgroundTask
{
public:
	SetupItemsBackgroundTask( const BW::string & search,
		const ListGroupStates & groupStates, const SetupItemsTask::Types & allowedTypes,
		ItemInfoDB::ComparerPtr pComparer );

	bool finished() { return finished_; }

	void results( SetupItemsTask::Items & retItems, int & retNumItems,
									int & retNumTris, int & retNumPrimitives );

private:

	virtual void doBackgroundTask( TaskManager & mgr );
	virtual void doMainThreadTask( TaskManager & mgr );

	SetupItemsTask task_;
	bool finished_;
};

BW_END_NAMESPACE

#endif // SETUP_ITEMS_TASK_HPP
