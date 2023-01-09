#ifndef _AMORTISE_CHUNK_ITEM_DELETE_HPP_
#define _AMORTISE_CHUNK_ITEM_DELETE_HPP_


#include "cstdmf/singleton.hpp"
#include "chunk_item.hpp"
#include "cstdmf/bw_list.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Used to control the number of chunk items deleted in a single frame.  When
 *	constructing this class, three parameters are used to control how many chunk
 *	items are deleted per frame; baseTimeSlice, maxChunkItemCount, and curbAt.
 *
 *	Each frame the tick method should be called.  As tick method will continue
 *	deleting chunk items until an allotted time slice has been used.  The time
 *	slice is determined based on the three parameters listed above.
 *
 *	baseTimeSlice is the time slice used when there are fewer than curbAt number
 *	of chunk items in the deletion list.  If there are more than curbAt number
 *	of chunk items in the deletion list, baseTimeSlice is scaled to increase the
 *	amount of time allotted to delete chunk items.  The scaling factor ranges
 *	from one at the curbAt mark to infinity at maxChunkItemCount using the
 *	inverse square law.
 *
 *	Example:
 *	If this class was created with the following values:
 *				baseTimeSlice = 15000.0
 *				maxChunkItemCount = 10000
 *				curbAt = 4000
 *
 *	The following time slices would be used to delete chunk items:
 *
 *		List size = 4000 -> Time slice = 15000.0
 *	Because the size of the list if less than curbAt, the baseTimeSlice is used.
 *
 *		List size = 9000 -> Time slice = 60000.0
 *	Because the size of the list if greater than curbAt, baseTimeSlice is scaled
 *	based on the following factor:
 *		List size - curbAt = 5000.
 *		The ratio of this value to the difference in count between curbAt and
 *		maxChunkItemCount is 5000 / 10000 = 0.5.  We use the one minus this value
 *		1.0 - 0.5 = 0.5.
 *		The inverse square of this value is 1 / 0.5^2 = 1 / 0.25 = 4.0.
 *		Therefore baseTimeSlice * 4.0 = 15000.0 * 4.0 = 60000.0.
 */
class AmortiseChunkItemDelete : public Singleton< AmortiseChunkItemDelete >
{
public:
	AmortiseChunkItemDelete(
			double baseTimeSlice = 15000.0,
			uint32 maxChunkItemCount = 10000,
			uint32 curbAt = 4000 );
	~AmortiseChunkItemDelete();

	void add( ChunkItemPtr pItem );
	void purge();
	void tick();

private:
	/// Unscaled time slice allotted to clear chunk items each tick.
	double baseTimeSlice_;
	/// The maximum number of chunk items we can hold open.
	uint32 maxChunkItemCount_;
	/// The number of chunk items after which we start ramping the time slice to infinity.	
	uint32 curbAt_;
	/// The number of chunk items being held onto at the end of the previous tick.
	uint32 chunkItemListSizePreviousFrame_;
	/// The time slice used for the previous frame.
	double timeSlicePreviousFrame_;

	/// Mutex used to control access to the list of chunk items
	SimpleMutex chunkItemListMutex_;
	/// The list of chunk items to be deleted
	BW::list< ChunkItemPtr > chunkItemList_;
};

BW_END_NAMESPACE

#endif // chunk_item_amortise_delete.hpp
