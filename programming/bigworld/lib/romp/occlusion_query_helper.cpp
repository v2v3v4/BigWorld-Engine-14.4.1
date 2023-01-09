#include "pch.hpp"
#include "occlusion_query_helper.hpp"
#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
OcclusionQueryHelper::OcclusionQueryHelper(
	uint16 numHandles,
	uint32 defaultValue,
	uint8 numFrames ):
frameNum_( 0 ),
numFrames_( numFrames ),
handlePool_( numHandles ),
defaultValue_( defaultValue )
{
	BW_GUARD;
	//total number of results are number of handles * number of frames storage.
	//this is needed because the occlusion test is asynchronous, and the CPU
	//can get up to 3 frames ahead of the GPU (meaning numFrames should
	//always be at least 3)
	numResults_ = numFrames_ * numHandles;

	results_ = new uint32[numResults_];
	resultPending_ = new bool[numResults_];
	queries_ = new DX::Query*[numResults_];

	for (uint32 i=0; i<numResults_; i++)
		results_[i] = defaultValue_;

	memset( resultPending_, 0, sizeof(bool) * numResults_ );
	memset( queries_, 0, sizeof(DX::Query*) * numResults_ );
};


/**
 *	Destructor.
 */
OcclusionQueryHelper::~OcclusionQueryHelper()
{
	BW_GUARD;
	for ( uint16 i=0; i<numResults_; i++ )
	{
		if ( queries_[i] )
			queries_[i]->Release();
	}

	bw_safe_delete_array(results_);
	bw_safe_delete_array(resultPending_);
	bw_safe_delete_array(queries_);
}


/**
 *	This method begins a frame of occlusion queries.  It must be
 *	called before calling beginQuery() or handleFromId().
 *	If you call this, you must call end() too.
 *
 *	@see OcclusionQueryHelper::begin
 */
void OcclusionQueryHelper::begin()
{	
	BW_GUARD;
	handlePool_.beginFrame();
}


/**
 *	This method ends a frame of occlusion queries.  It must be called to
 *	make a pair of begin() and end() calls.
 *
 *	@see OcclusionQueryHelper::begin
 */
void OcclusionQueryHelper::end()
{	
	BW_GUARD;
	HandlePool::HandleMap::iterator it = handlePool_.begin();
	HandlePool::HandleMap::iterator end = handlePool_.end();
	while ( it != end )
	{
		HandlePool::Info& info = it->second;
		if ( info.used_ == false )
		{
			// If the handle exists but is currently unused, it means the lens flare wasn't
			// visible this frame, however it was sometime in the last n frames.  Check to
			// see if there are any pending queries.
			getVizResults( info.handle_ );

			bool resultPending = false;

			for ( int f=0; f<MAX_FRAME_LAG; f++ )
			{
				int result = resultIndex(info.handle_,f);

				if ( !resultPending_[result] )
				{
					results_[result] = defaultValue_;
				}
				else
				{
					resultPending = true;
				}
			}

			if ( resultPending )
			{
				info.used_ = true;
			}
		}
		it++;
	}

	handlePool_.endFrame();

	//update the frame parity		
	frameNum_ = (frameNum_ + 1) % numFrames_;	
}


/**
 *	This method is private, and asks the device for results for a particular
 *	query handle.
 */
void OcclusionQueryHelper::getVizResults( HandlePool::Handle h )
{	
	BW_GUARD_PROFILER( OQH_getVizResults );

	for ( int f=0; f<numFrames_; f++ )
	{
		uint16 result = resultIndex(h,f);

		if ( resultPending_[result] )
		{
			HRESULT hr = queries_[result]->GetData(
				&results_[result], sizeof(DWORD), 0 );

			resultPending_[result] = ( hr != D3D_OK );
		}
	}
}


/**
 *	This method is called to begin an occlusion query.  Call it just before
 *	drawing your geometry that you are using to test visibility with.
 *
 *	@param	h		an OcclusionQueryHelper::Handle identifying the test
 *
 *	@return bool	success of the query.  fails if the device is lost.
 */
bool OcclusionQueryHelper::beginQuery( HandlePool::Handle h )
{	
	BW_GUARD;
	if (h != HandlePool::INVALID_HANDLE)
	{
		uint16 result = resultIndex( h, frameNum_ );

		if (resultPending_[result])
		{
			this->getVizResults(h);

			//we are still using this query, can't issue another just yet.
			if (resultPending_[result])
				return false;
		}

		HRESULT hr;
		DX::Query* query = queries_[result];
		if ( !query )
		{
			hr = Moo::rc().device()->CreateQuery(D3DQUERYTYPE_OCCLUSION, &queries_[result]);
			if (FAILED(hr))
			{
				queries_[result] = NULL;
				return false;
			}
			query = queries_[result];
		}

		hr = query->Issue(D3DISSUE_BEGIN);
		return (SUCCEEDED(hr));
	}
	else
	{
		return false;
	}
}


/**
 *	This method is called to notify this class that the visibility testing
 *	geometry has been drawn.  Must be matched with beginQuery()
 *
 *	@param	h		an OcclusionQueryHelper::Handle identifying the test
 *	@see	OcclusionQueryHelper::beginQuery
 */
void OcclusionQueryHelper::endQuery( HandlePool::Handle h )
{	
	BW_GUARD;
	MF_ASSERT( h!= HandlePool::INVALID_HANDLE );
	uint16 result = resultIndex( h, frameNum_ );

	DX::Query* query = queries_[result];
	MF_ASSERT( query );
	HRESULT hr = query->Issue(D3DISSUE_END);
	resultPending_[result] = true;

	//won't have finished this particular one by now,
	//but we check our other frame_lag queries
	this->getVizResults(h);
}


/**
 *	This method returns the average number of pixels drawn by visibility
 *	testing geometry represented by the handle.
 *
 *	@param	h		an OcclusionQueryHelper::Handle identifying the test
 *	@return int		average number of pixels.
 */
int OcclusionQueryHelper::avgResult( HandlePool::Handle h )
{
	BW_GUARD;
	if ( h == HandlePool::INVALID_HANDLE )
		return 0;

	int acc = 0;
	int tot = 0;
	for ( int f=0; f<numFrames_; f++ )
	{
		uint16 result = resultIndex(h,f);

		if (!resultPending_[result])
		{
			acc += results_[result];
			tot++;
		}
	}

	if ( tot > 0 )
		return acc/tot;

	//all of our tests are outstanding.  This is very unusual but
	//it can happen (for this to happen the CPU has to get ahead
	//of the GPU by 4 frames, should never happen in practice but
	//may happen in empty spaces on some video cards.  In this case
	//we return the average of the last 4 known results
	for ( int f=0; f<numFrames_; f++ )
	{
		uint16 result = resultIndex(h,f);
		acc += results_[result];		
	}	

	return acc/numFrames_;
}


void OcclusionQueryHelper::deleteUnmanagedObjects()
{
	BW_GUARD;
	for ( uint16 i=0; i<numResults_; i++ )
	{
		if ( queries_[i] )
		{
			if (resultPending_[i])
			{
				HRESULT hr = queries_[i]->GetData( &results_[i], sizeof(int), D3DGETDATA_FLUSH );
			}
			queries_[i]->Release();
		}
	}

	memset( results_, defaultValue_, sizeof(results_[0]) * numResults_ );
	memset( resultPending_, 0, sizeof(bool) * numResults_ );
	memset( queries_, 0, sizeof(DX::Query*) * numResults_ );

	handlePool_.reset();

	frameNum_ = 0;
}

BW_END_NAMESPACE

// occlusion_query_header.cpp
