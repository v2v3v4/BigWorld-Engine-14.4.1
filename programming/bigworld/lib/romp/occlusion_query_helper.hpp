#ifndef OCCLUSION_QUERY_HELPER_HPP
#define OCCLUSION_QUERY_HELPER_HPP

#include "moo/device_callback.hpp"
#include "handle_pool.hpp"


BW_BEGIN_NAMESPACE

//must be greater than 0 and less than 256 (uint8)
#define MAX_FRAME_LAG 4


/** 
 *	This helper class handles asynchronous occlusion queries.  To use it,
 *	obtain query handles by passing in an ID, and draw some geometry in
 *	between beginQuery and endQuery calls.  Internally, the helper will
 *	handle the asynchronous occlusion query calls by storing multiple
 *	frames of results - call avgResult to obtain the answer. 
 *	Note that if you are testing 'coverage' instead of 'visibility'
 *	then you may not want to have 0 as your default value, instead you
 *	may like to use the full coverage value.
 */
class OcclusionQueryHelper : public Moo::DeviceCallback
{
public:
	OcclusionQueryHelper(
		uint16 maxHandles,
		uint32 defaultValue = 0,
		uint8 numFrames = MAX_FRAME_LAG );
	~OcclusionQueryHelper();	

	//public interface - call these in order.
	void		begin();
	bool		beginQuery( HandlePool::Handle idx );
	void		endQuery( HandlePool::Handle idx );
	void		end();

	//returns the average number of pixels drawn over the last n frames for
	//this query handle.
	int			avgResult( HandlePool::Handle idx );
	HandlePool& handlePool()	{ return handlePool_; }

	//from the Moo::DeviceCallback interface
	void		deleteUnmanagedObjects();

private:
	void		getVizResults( HandlePool::Handle idx );
	bool		*resultPending_;
	///convert handle ID + frame number into a result array index
	uint16		resultIndex( HandlePool::Handle h, int frameNum )
	{
		return h + handlePool_.numHandles() * frameNum;
	}
	uint32		*results_;
	DX::Query** queries_;
	uint8		frameNum_;
	uint8		numFrames_;
	uint16		numResults_;
	uint32		defaultValue_;
	HandlePool	handlePool_;
};

BW_END_NAMESPACE

#endif	//OCCLUSION_QUERY_HELPER_HPP
