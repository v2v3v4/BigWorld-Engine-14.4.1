#ifndef CHUNK_PROCESSOR_HPP
#define CHUNK_PROCESSOR_HPP

#include "cstdmf/bgtask_manager.hpp"

BW_BEGIN_NAMESPACE

class ChunkProcessorManager;
class UnsavedList;
class ChunkProcessorListener;

/**
 *	Base class for processing secondary chunk data from the primary chunk data.
 *	
 *	Though it is not necessary to have it while calculating secondary data,
 *	it is required if we want to calculate by utilising multiple threads.
 *	
 *	Usually when we are calculating secondary data, we will first pack all data
 *	that required calculating in the main thread, and then add the 
 *	ChunkProcessor into BgTaskManager. Then we will do the part that is
 *	CPU intensive while not related to other data in background thread.
 *	This can ensure that we utilise the extra cores in modern CPUs while not
 *	accessing same data with other threads. After the processing inside
 *	background thread is finished, we will add the object back to main thread
 *	to get the rest work done. Usually you will write back the result to
 *	appropriate object. Then we can clean up the resources.
 *	
 *	Usually classes derived from ChunkProcessor should implement four functions:
 *	1.	processInBackground
 *	2.	processInMainThread
 *	3.	cleanup
 *	4.	stop
 */
class ChunkProcessor : public BackgroundTask
{
public:
	ChunkProcessor();

	bool process( ChunkProcessorManager& mgr );

	virtual void cancel( TaskManager & mgr );

	// Subclasses _must_ call this for correct listener notification
	virtual void cleanup( ChunkProcessorManager& manager );

	void addListener(ChunkProcessorListener * pListener);
	void delListener(ChunkProcessorListener * pListener);

private:

	void notifyListenerFinished();
	/**
	 *	Perform background processing.
	 *	@return true on success, false on failure, eg. stop() was called on
	 *		background task.
	 */
	virtual bool processInBackground( ChunkProcessorManager& manager ) = 0;

	/**
	 *	Perform main thread processing.
	 *	@return true on success.
	 */
	virtual bool processInMainThread( ChunkProcessorManager& manager,
		bool backgroundTaskResult ) = 0;

	virtual void doMainThreadTask( TaskManager& mgr );
	virtual void doBackgroundTask( TaskManager& mgr );

	volatile bool backgroundResult_;

	typedef BW::vector<ChunkProcessorListener *> ChunkProcessorListenerVector;
	ChunkProcessorListenerVector listeners_;

	bool cleanedUp_;
};


typedef SmartPointer<ChunkProcessor> ChunkProcessorPtr;



class ChunkProcessorListener
{
public:
	/**
	*	this function is called whenever a processor finished
	*	if we registered as a listener of this processor
	*	@param processor the processor which just finished.
	*	@param hasUnregisteredOffListener out,if I have unregistered
	*	  myself as a listener of this processor.
	*/
	virtual void onChunkProcessorFinished( 
		ChunkProcessor* pProcessor, bool& hasUnregisteredFromListener )
	{}
};

typedef BW::list<ChunkProcessorPtr> ChunkProcessorList;

// a simple wrapper of ChunkProcessorList
class ChunkProcessors
{
public:

	void delProcessor(ChunkProcessorPtr processor );
	void addProcessor( ChunkProcessorPtr processor );

	bool isEmpty(){ return (chunkProcessors_.size() == 0);}
	ChunkProcessorList& chunkProcessors(){ return chunkProcessors_; }

private:
	ChunkProcessorList chunkProcessors_;

};

BW_END_NAMESPACE

#endif//CHUNK_PROCESSOR_HPP
