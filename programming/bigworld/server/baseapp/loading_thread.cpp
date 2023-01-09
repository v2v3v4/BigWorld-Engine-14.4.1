#include "script/first_include.hpp"

#include "loading_thread.hpp"

#include "baseapp.hpp"
#include "baseapp_config.hpp"

#include "chunk/chunk_format.hpp"
#include "pyscript/py_data_section.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"
#include "chunk/chunk_grid_size.hpp"
#include "cstdmf/binary_stream.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: TickedWorkerJob
// -----------------------------------------------------------------------------

// Static data
TickedWorkerJob::Jobs TickedWorkerJob::jobs_;

/**
 *	Constructor. Constructing an object of this type adds the object to the
 *	static collection of ticked jobs.
 */
TickedWorkerJob::TickedWorkerJob()
{
	jobs_.push_back( this );
}


/**
 *	This static method calls tick on each job. If tick returns true, that job
 *	is deleted and no longer ticked.
 */
void TickedWorkerJob::tickJobs()
{
	Jobs::iterator iter = jobs_.begin();

	while (iter != jobs_.end())
	{
		Jobs::iterator curr = iter;
		++iter;

		if ((*curr)->tick())
		{
			delete *curr;
			jobs_.erase( curr );
		}
	}
}


// -----------------------------------------------------------------------------
// Section: BigWorld.fetchDataSection
// -----------------------------------------------------------------------------

/**
 *	This class is used to implement BigWorld.fetchDataSection. It is used to
 *	load a data section from the loading thread.
 */
class FetchDataSectionJob : public TickedWorkerJob
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param path	The path to the data section to load.
	 *	@param pHandler	The Python "function" that will be called with the
	 *	loaded data section.
	 */
	FetchDataSectionJob( const BW::string & path,
			PyObjectPtr pHandler ) :
		path_( path ),
		pHandler_( pHandler ),
		loadingFinished_( false )
	{
		this->submit( &BaseApp::instance().workerThread() );
	}

private:
	// WorkerJob override. Called in loading thread
	virtual float operator()()
	{
		pDataSection_ = BWResource::openSection( path_ );
		loadingFinished_ = true;

		return -1.f;
	}

	// TickedWorkerJob override. Called in main thread.
	bool tick()
	{
		if (loadingFinished_ && pHandler_)
		{
			PyObjectPtr pArg;
			if (pDataSection_)
				pArg = PyObjectPtr( new PyDataSection( pDataSection_ ),
								PyObjectPtr::STEAL_REFERENCE );
			else
				pArg = Py_None;

			PyObject * pRes = PyObject_CallFunctionObjArgs(
					pHandler_.get(), pArg.get(), NULL );

			if (pRes == NULL)
			{
				PyErr_Print();
				ERROR_MSG( "FetchDataSectionJob::tick: Callback failed\n" );
			}
			else
			{
				Py_DECREF( pRes );
			}
			// Needed if isDisowned returns false below.
			pHandler_ = NULL;
		}

		// Returning true causes this object to be deleted.
		return loadingFinished_ && this->isDisowned();
	}

	// Written in constructor, read in loading thread.
	const BW::string path_;

	// Only used in main thread.
	PyObjectPtr pHandler_;

	// Written in loading thread, read in main thread.
	DataSectionPtr 	pDataSection_;
	bool			loadingFinished_;
};


/**
 *	This function implements the BigWorld.fetchDataSection script method.
 */
PyObject * py_fetchDataSection( PyObject * args )
{
	char * path;
	PyObject * pHandler = NULL;

	if (!PyArg_ParseTuple( args, "sO:fetchDataSection", &path, &pHandler ))
	{
		return NULL;
	}

	if (!PyCallable_Check( pHandler ))
	{
		PyErr_SetString( PyExc_TypeError, "Handler must be callable" );
		return NULL;
	}

	// This object will be deleted when it has finished being ticked.
	new FetchDataSectionJob( path, pHandler );

	Py_RETURN_NONE;
}
/*~ function BigWorld.fetchDataSection
 *	@components{ base }
 *	The function fetchDataSection loads a data section using the loading thread
 *	that is then passed to the input function to be handled in the main thread.
 *	@param path	The path to the data section to load.
 *	@param callback callback is a callable object (e.g. a function) that takes one
 *		argument, the data section specified by path. If the data section could
 *		not be loaded, the callback is called with None as the argument.
 */
PY_MODULE_FUNCTION( fetchDataSection, BigWorld )


// -----------------------------------------------------------------------------
// Section: BigWorld.fetchEntitiesFromChunks
// -----------------------------------------------------------------------------

/**
 *	This class is used to implement the BigWorld.fetchEntitiesFromChunks and
 *	fetchFromChunks methods.
 */
class FetchFromChunksJob : public TickedWorkerJob
{
public:
	/**
	 *	This interface is used to help decide which data sections should be
	 *	visited.
	 */
	class Filter
	{
	public:
		virtual bool shouldAccept( DataSection * pDS ) = 0;
		virtual ~Filter() {}
	};

	/**
	 *	This class is used to match just data sections whose tag matches the
	 *	construction string.
	 */
	class SimpleFilter : public Filter
	{
	public:
		SimpleFilter( const BW::string & match ) : match_( match ) {}

	private:
		virtual bool shouldAccept( DataSection * pDS )
		{
			return match_ == pDS->sectionName();
		}

		BW::string match_;
	};

	/**
	 *	This class is used to match all data sections that match any of a set
	 *	of strings.
	 */
	class SetFilter : public Filter
	{
	public:
		void addMatch( const BW::string & match )
		{
			set_.insert( match );
		}

	private:
		virtual bool shouldAccept( DataSection * pDS )
		{
			return set_.find( pDS->sectionName() ) != set_.end();
		}

		BW::set< BW::string > set_;
	};

	/**
	 *	This filter is used to match all data sections that should be read to
	 *	create server-side entities.
	 */
	class EntityFilter : public Filter
	{
	private:
		virtual bool shouldAccept( DataSection * pDS )
		{
			return (pDS->sectionName() == "entity") &&
				!pDS->readBool( "clientOnly", false );
		}
	};

public:
	FetchFromChunksJob( const BW::string & path, PyObjectPtr pHandler,
		   Filter * pFilter ) :
		// Loading thread data
		path_( path + "/" ),
		currX_( 0 ),
		currY_( 0 ),
		gridSize_( 0 ),
		minX_( 0 ),
		maxX_( 0 ),
		maxY_( 0 ),
		singleDir_( false ),
		loadingStarted_( false ),
		// Both
		loadingFinished_( false ),
		// Main thread
		pHandler_( pHandler ),
		pFilter_( pFilter )
	{
		this->submit( &BaseApp::instance().workerThread() );
	}

private:
	// ---- Loading thread ----
	virtual float operator()()
	{
		// TODO: At the moment, the loading thread goes as fast as it can. It
		// may be good to limit this depending on the queue_ size. That is, the
		// amount of outstanding information for the main thread to process.

		if (!loadingStarted_)
		{
			// TODO: It could be good to use some functionality in the chunk
			// library. The committed 1.1 version
			// did this but was a bit ugly and did not know if loading failed.
			loadingStarted_ = true;
			DataSectionPtr pSpaceDS = BWResource::openSection( path_ );
			if (pSpaceDS)
				pSpaceDS = pSpaceDS->openSection( "space.settings" );

			if (pSpaceDS)
			{
				gridSize_  = pSpaceDS->readFloat( "chunkSize", g_defaultGridResolution );
				minX_  = pSpaceDS->readInt( "bounds/minX" );
				maxX_  = pSpaceDS->readInt( "bounds/maxX" );
				currY_ = pSpaceDS->readInt( "bounds/minY" );
				maxY_  = pSpaceDS->readInt( "bounds/maxY" );
				currX_ = minX_;
				singleDir_ = pSpaceDS->readBool( "singleDir", false );
			}
			else
			{
				WARNING_MSG( "FetchFromChunksJob::operator(): "
						"%s is not a valid space\n", path_.c_str() );
				loadingFinished_ = true;
				return -1.f;
			}
		}

		// An outdoor chunk is first loaded and then all of its overlappers.

		if (overlappers_.empty())
		{
			if (!this->getNextOutdoorChunk())
			{
				// The loading thread job is done.
				loadingFinished_ = true;
				return -1.f;
			}
		}
		else
		{
			bool done = false;

			while (!overlappers_.empty() && !done)
			{
				BW::string nextChunk = overlappers_.back();
				overlappers_.pop_back();

				// An indoor chunk may be in more than one overlapper sets.
				if (visited_.find( nextChunk ) == visited_.end())
				{
					done = true;
					visited_.insert( nextChunk );
					this->loadChunk( nextChunk, NULL );
				}
			}
		}

		return 0.f;
	}

	/*
	 *	This method is called by the loading thread to move to the next outdoor
	 *	chunk.
	 */
	bool getNextOutdoorChunk()
	{
		if (currY_ > maxY_)
		{
			return false;
		}
		Matrix m = Matrix::identity;
		m.setTranslate( (float)currX_ * gridSize_, 0.f, (float)currY_ * gridSize_ );
		DataSectionPtr pDS = this->loadChunk(
			ChunkFormat::outsideChunkIdentifier( currX_, currY_, singleDir_ ), &m );

		if (!pDS)
		{
			ERROR_MSG( "FetchFromChunksJob::getNextOutdoorChunk: "
					"Failed to load chunk at %d, %d\n", currX_, currY_ );
			return false;
		}

		// Scan the outdoor chunks an X row at a time, from min x to max x. The
		// rows are from min y to max y.
		++currX_;
		if (currX_ > maxX_)
		{
			++currY_;
			currX_ = minX_;
		}

		pDS->readStrings( "overlapper", overlappers_ );

		return true;
	}

	/**
	 *	This method loads the chunk with the input identifier and delivers it
	 *	to the main thread.
	 */
	DataSectionPtr loadChunk( const BW::string & identifier, Matrix* m )
	{
		BW::string path = path_ + identifier + ".chunk";
		DataSectionPtr pDS = BWResource::openSection( path );
		Matrix transform;
		if (m)
		{
			transform = *m;
		}
		else if (pDS)
		{
			transform = pDS->readMatrix34( "transform" );
		}
		else
		{
			transform.setIdentity();
		}

		if (pDS)
		{
			mutex_.grab();
			queue_.push_back( std::make_pair( pDS, transform ) );
			mutex_.give();
		}
		else
		{
			ERROR_MSG( "FetchFromChunksJob::loadChunk: "
					"Failed to load %s\n", path.c_str() );
		}

		return pDS;
	}

	// ---- Main thread ----
	/**
	 *	This method is called every game tick in the main thread.
	 */
	bool tick()
	{
		bool doneForNow = false;
		bool doneForever = false;

		while (!doneForNow)
		{
			// If we do not have a current data section, get one.
			if (!pMainThreadDS_)
			{
				mutex_.grab();
				if (!queue_.empty())
				{
					pMainThreadDS_ = queue_.front().first;
					mainThreadMatrix_ = queue_.front().second;
					if (pMainThreadDS_)
					{
						mainThreadIter_ = pMainThreadDS_->begin();
					}
					queue_.pop_front();
				}
				else
				{
					doneForever = loadingFinished_;
				}
				mutex_.give();
			}

			if (pMainThreadDS_)
			{
				while ((mainThreadIter_ != pMainThreadDS_->end()) &&
						!doneForNow)
				{
					DataSectionPtr pLocalDS = *mainThreadIter_;
					MF_ASSERT( pLocalDS );

					if (pFilter_->shouldAccept( pLocalDS.get() ))
					{
						Matrix transform = pLocalDS->readMatrix34( "transform", Matrix::identity );
						transform.postMultiply( mainThreadMatrix_ );

						PyObject * pPyDS = new PyDataSection( pLocalDS );
						PyObject * pMatrix = Script::getData( transform );

						PyObject * pResult =
							PyObject_CallMethod( pHandler_.get(),
								"onSection", "OO", pPyDS, pMatrix );

						Py_DECREF( pMatrix );
						Py_DECREF( pPyDS );

						if (pResult)
						{
							doneForNow = PyObject_IsTrue( pResult );
							Py_DECREF( pResult );
						}
						else
						{
							PyErr_Print();
							ERROR_MSG( "FetchFromChunksJob::tick: "
									"Script call failed\n" );
						}
					}

					++mainThreadIter_;
				}

				if (mainThreadIter_ == pMainThreadDS_->end())
				{
					pMainThreadDS_ = NULL;
				}

				doneForNow |= BaseApp::instance().nextTickPending();
			}
			else
			{
				doneForNow = true;
			}
		}

		if (doneForever && pHandler_)
		{
			Script::call(
				PyObject_GetAttrString( pHandler_.get(), "onFinish" ),
				PyTuple_New( 0 ), "onFinish", true );
			// Needed if isDisowned return false below.
			pHandler_ = NULL;
		}

		// isDisowned is needed to avoid a race condition. The loading thread
		// needs to be done with it.
		return doneForever && this->isDisowned();
	}

	// Set in main thread, read in loading thread.
	const BW::string path_;

	// ---- Used in loading thread ----
	BW::set< BW::string > visited_;
	BW::vector< BW::string > overlappers_;
	int currX_;
	int currY_;
	float gridSize_;
	int minX_;
	int maxX_;
	int maxY_;
	bool singleDir_;
	int loadingStarted_;

	// ---- Used by both threads ----
	typedef BW::list< std::pair< DataSectionPtr, Matrix > > DataQueue;
	DataQueue queue_;
	bool loadingFinished_;

	// ---- Used in main thread ----
	PyObjectPtr pHandler_;
	// The current data section and where we are up to in it.
	DataSectionPtr pMainThreadDS_;
	DataSection::iterator mainThreadIter_;
	std::auto_ptr< Filter > pFilter_;
	// Used to cache the matrix of the current data section
	Matrix mainThreadMatrix_;

	SimpleMutex mutex_;
};


/**
 *	This function implements the BigWorld.fetchEntitiesFromChunks script method.
 */
PyObject * py_fetchEntitiesFromChunks( PyObject * args )
{
	char * path;
	PyObject * pHandler = NULL;

	if (!PyArg_ParseTuple( args, "sO:fetchEntitiesFromChunks",
				&path, &pHandler ))
	{
		return NULL;
	}

	if (!PyObject_HasAttrString( pHandler, "onSection" ))
	{
		PyErr_SetString( PyExc_TypeError,
				"Handler must have an onSection method" );
		return NULL;
	}

	// This object will be deleted when it has finished being ticked.
	new FetchFromChunksJob( path, pHandler,
			new FetchFromChunksJob::EntityFilter() );

	Py_RETURN_NONE;
}
/*~ function BigWorld.fetchEntitiesFromChunks
 *	@components{ base }
 *	The function fetchEntitiesFromChunks function loads the chunks of a space
 *	and visits every entity section.
 *	@param path The path to the space that is to be loaded.
 *	@param handler A Python object with an onSection method and optionally an
 *	onFinish method.
 *	The onSection method will be called for each entity section encountered in
 *	the chunks. This method is called with two arguments. The first is the
 *	entity data section and the second is a matrix corresponding to the position
 *	of the entity. If the handler wants to pause processing until the next game
 *	tick, True should be returned from this function, otherwise None or False
 *	should be returned.
 *	The onFinish method is called with no arguments when all chunks have been
 *	traversed.
 */
PY_MODULE_FUNCTION( fetchEntitiesFromChunks, BigWorld )



/**
 *	This function implements the BigWorld.fetchFromChunks script method.
 */
PyObject * py_fetchFromChunks( PyObject * args )
{
	PyObject * pMatches = NULL;
	char * path;
	PyObject * pHandler = NULL;

	if (!PyArg_ParseTuple( args, "OsO:fetchFromChunks",
				&pMatches, &path, &pHandler ))
	{
		return NULL;
	}

	if (!PyObject_HasAttrString( pHandler, "onSection" ))
	{
		PyErr_SetString( PyExc_TypeError,
				"Handler must have an onSection method" );
		return NULL;
	}

	FetchFromChunksJob::Filter * pFilter = NULL;

	if (PyString_Check( pMatches ))
	{
		pFilter = new FetchFromChunksJob::SimpleFilter(
						PyString_AS_STRING( pMatches ) );
	}
	else if (PySequence_Check( pMatches ))
	{
		std::auto_ptr< FetchFromChunksJob::SetFilter >
			pSetFilter( new FetchFromChunksJob::SetFilter() );
		int size = PySequence_Length( pMatches );

		for (int i = 0; i < size; ++i)
		{
			PyObjectPtr pItem( PySequence_GetItem( pMatches, i ),
					PyObjectPtr::STEAL_REFERENCE );

			if (PyString_Check( pItem.get() ))
			{
				pSetFilter->addMatch( PyString_AS_STRING( pItem.get() ) );
			}
			else
			{
				PyErr_Format( PyExc_TypeError,
					"Item %d of the matches sequence is not a string", i );
				return NULL;
			}
		}

		pFilter = pSetFilter.release();
	}

	if (!pFilter)
	{
		PyErr_SetString( PyExc_TypeError,
			"The 'matches' argument should be a string or sequence of strings" );
		return NULL;
	}

	// This object will be deleted when it has finished being ticked.
	new FetchFromChunksJob( path, pHandler, pFilter );

	Py_RETURN_NONE;
}
/*~ function BigWorld.fetchFromChunks
 *	@components{ base }
 *	The function fetchFromChunks function loads the chunks of a space and visits
 *	visits every matched section.
 *	@param matches If a string is passed in, sections named this are passed to the
 *	callback. If a sequence of strings is passed, any section with a name in the
 *	sequence is passed to the callback.
 *	@param path The path to the space that is to be loaded.
 *	@param handler A Python object with an onSection method and optionally an
 *	onFinish method.
 *	The onSection method will be called for each matched section encountered in
 *	the chunks. This method is called with two arguments. The first is the
 *	matched data section and the second is a matrix corresponding to the
 *	position of the section. If the handler wants to pause processing until the
 *	next game tick, True should be returned from this function, otherwise None
 *	or False should be returned.
 *	The onFinish method is called with no arguments when all chunks have been
 *	traversed.
 */
PY_MODULE_FUNCTION( fetchFromChunks, BigWorld )


// -----------------------------------------------------------------------------
// Section: FileStreamingJob
// -----------------------------------------------------------------------------

/**
 *  Create a new FileStreamingJob for the resource at the specified location,
 *  with the specified buffer size.
 */
FileStreamingJob::FileStreamingJob( const BW::string &path, int bufsize ) :
	path_( path ),
	file_( NULL ),
	buf_( new char[ bufsize ] ),
	offset_( 0 ),
	size_( 0 ),
	maxsize_( bufsize ),
	doneReading_( false ),
	error_( false )
{
	this->submit( &BaseApp::instance().workerThread() );
}


FileStreamingJob::~FileStreamingJob()
{
	if (!data_.empty())
	{
		WARNING_MSG( "FileStreamingJob::~FileStreamingJob: "
			"There were still %d unread bytes\n", size_ );

		for (Data::iterator it = data_.begin(); it != data_.end(); ++it)
		{
			delete *it;
		}
	}

	if (file_ != NULL)
	{
		WARNING_MSG( "FileStreamingJob::~FileStreamingJob: "
			"File wasn't closed before destruction\n" );

		fclose( file_ );
		file_ = NULL;
	}

	delete [] buf_;
}


/**
 *  Returns the number of bytes of data currently buffered in memory.  This
 *  should be called by the reader to decide whether a read() is a good idea.
 *  As long as only one thread is trying to read, this doesn't need to be
 *  synchronised with the mutex as the return value will at worst underestimate
 *  the true amount of buffered data.
 */
int FileStreamingJob::size()
{
	return size_;
}


/**
 *  Returns the number of bytes of free space available in the buffer at the
 *  moment.  This should be called by the writer before reading data from disk
 *  to append to the buffer.  This doesn't need to be synchronised for the same
 *  reason as size() above.
 */
int FileStreamingJob::freeSpace()
{
	return maxsize_ - size_;
}


/**
 *  Returns true if this job has completed successfully and can be deleted.
 */
bool FileStreamingJob::done()
{
	SimpleMutexHolder holder( lock_ );
	return doneReading_ && size_ == 0;
}


/**
 *  Returns false if an I/O error of some kind occured (e.g. file not found).
 */
bool FileStreamingJob::good()
{
	return !error_;
}


/**
 *  Write at most the specified number of bytes out of the buffer into the
 *  provided stream.
 */
void FileStreamingJob::read( BinaryOStream &os, int nBytes )
{
	SimpleMutexHolder holder( lock_ );

	int remaining = nBytes;

	while (remaining > 0 && !data_.empty())
	{
		BW::string *pChunk = data_.front();

		// Copy as much out of the first string as is either wanted or remaining
		int n = std::min( remaining, int( pChunk->size() - offset_ ) );

		os.addBlob( pChunk->c_str() + offset_, n );
		remaining -= n;
		offset_ += n;
		size_ -= n;

		// If we've finished reading the first chunk in the list, drop it
		if (offset_ == int( pChunk->size() ))
		{
			delete pChunk;
			data_.pop_front();
			offset_ = 0;
		}
	}
}


/**
 *  Read some data into the buffer.  You should call freeSpace() to find out
 *  how much data is actually wanted before doing this to make sure you're not
 *  overbuffering.
 */
void FileStreamingJob::write( const char *src, int nBytes )
{
	// We don't need to synchronise here because we've already got a lock in
	// operator() (see below) at this point.  Because our locks aren't
	// re-entrant, locking here causes a deadlock.
	BW::string *pChunk = new BW::string( src, nBytes );
	data_.push_back( pChunk );
	size_ += nBytes;
}


/**
 *  Called from the loading thread.  Does the actual reading of stuff off disk
 *  into the buffer.
 */
float FileStreamingJob::operator()()
{
	// If the file hasn't been opened yet, do it now
	if (file_ == NULL)
	{
		file_ = BWResource::instance().fileSystem()->posixFileOpen( path_, "r" );

		// Handle failed file open now
		if (file_ == NULL)
		{
			ERROR_MSG( "FileStreamingJob::operator(): "
				"File open for %s failed: %s\n",
				path_.c_str(), strerror( errno ) );

			error_ = true;

			return DONT_RESCHEDULE;
		}
	}

	int want = this->freeSpace();

	// Bail early if the buffer is already full
	if (want == 0)
	{
		return 1.0f / BaseAppConfig::updateHertz();
	}

	int got = fread( buf_, 1, want, file_ );

	// Check fread() errors
	if (ferror( file_ ))
	{
		error_ = true;

		ERROR_MSG( "FileStreamingJob::operator(): "
			"File read failed for %s: %s\n",
			path_.c_str(), strerror( errno ) );

		return DONT_RESCHEDULE;
	}

	// We need to synchronise from this point onwards to avoid the potential
	// situation where a consumer reads out of the buffer in between the write()
	// and setting doneReading_ to true, which would mean that all data would
	// have been read but done() would have returned false.
	SimpleMutexHolder holder( lock_ );

	this->write( buf_, got );

	// Don't reschedule if we're done reading
	if (got < want && feof( file_ ))
	{
		fclose( file_ );
		file_ = NULL;
		doneReading_ = true;

		return DONT_RESCHEDULE;
	}

	else
	{
		return 1.0f / BaseAppConfig::updateHertz();
	}
}

BW_END_NAMESPACE

// loading_thread.cpp
