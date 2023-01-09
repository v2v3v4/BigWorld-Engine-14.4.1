/**
 *	@internal
 *	@file
 */

#ifndef DOGWATCH_HPP
#define DOGWATCH_HPP


#include "cstdmf/config.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/timestamp.hpp"
#include "cstdmf/profiler.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/static_array.hpp"

BW_BEGIN_NAMESPACE

#if !ENABLE_DOG_WATCHERS
	#define NO_DOG_WATCHES
#endif

/**
 *	@internal
 *
 *	This class is used to time events during a frame. It keeps a short history
 *	of accumulated time per frame, and organises itself hierarchially based on
 *	what other DogWatchs are running.
 *
 *	DogWatches should be static or global, as they are intended to be used as
 *	timers for recurring events.
 */
class DogWatch
{
public:
	CSTDMF_DLL DogWatch( const char * title );

	CSTDMF_DLL void start();
	CSTDMF_DLL void stop();

	CSTDMF_DLL uint64 slice() const;
	CSTDMF_DLL const BW::string & title() const;

	// Keep a slice around to initialise dog watch before starting, this way
	// we don't have to check for null in stop().
	CSTDMF_DLL static		uint64 s_nullSlice_;

private:
	uint64		started_;
	uint64		*pSlice_;
	int			id_;
	BW::string title_;
};


/**
 *	@internal
 *
 *	Starts and stops a dogwatcher. This class is usefull
 *	when a method has more than one exit points. It will
 *	stop the dogWatcher when it goes out of scope.
 */
class ScopedDogWatch
{

public:
	ScopedDogWatch( DogWatch & dogWatch ) :
		dogWatch_( dogWatch )
	{
		this->dogWatch_.start();
	}

	~ScopedDogWatch()
	{
		this->dogWatch_.stop();
	}

private:
	DogWatch & dogWatch_;
};


/**
 *	@internal
 *	This singleton class manages the dog watches and performs
 *	functions on all of them.
 */
class DogWatchManager
{
public:
	enum
	{
		NUM_SLICES		= 120,
		MAX_WATCHES		= 128
	};

public:
	DogWatchManager();

	// move on to the next slice
	CSTDMF_DLL void tick();

	class iterator;

	CSTDMF_DLL iterator begin() const;
	CSTDMF_DLL iterator end() const;

	CSTDMF_DLL static DogWatchManager & instance();
	CSTDMF_DLL static void fini();

private:
	static DogWatchManager		* pInstance;

	void clearCache();

	int add( DogWatch & watch );

	uint64 & grabSlice( int id );
	void giveSlice();

	uint64 & grabSliceMissedCache( int id );

	friend class DogWatch;

	/// The slice index we're currently writing to
	uint32	slice_;

	/// The table elements currently in use
	struct TableElt;
	BW::vector<TableElt *>	stack_;

	/// The manifestation cache
	struct Cache
	{
		TableElt	* stackNow;
		TableElt	* stackNew;
		uint64		* pSlice;
	} cache_[MAX_WATCHES];

	/// The manifestation of a watch
	class Stat : public StaticArray<uint64,NUM_SLICES>, public ReferenceCount
	{
	public:
		Stat() :
			watchid(),
			flags()
		{
			this->resize( NUM_SLICES );
		}
		int			watchid;
		mutable int flags;
	};

	typedef SmartPointer < Stat > StatPtr;
	typedef BW::vector< StatPtr >	Stats;
	Stats	stats_;

	StatPtr newManifestation( TableElt & te, int watchid );

	/// Tables organising the manifestations
	struct TableElt
	{
		int		statid;
		int		tableid;
	};
	class Table : public StaticArray<TableElt, MAX_WATCHES>, public ReferenceCount
	{
	public:
		Table()
		{
			this->resize( MAX_WATCHES );
		}
	};


	typedef SmartPointer < Table > TablePtr;
	typedef BW::vector< TablePtr >	Tables;
	Tables	tables_;

	TablePtr newTable( TableElt & te );

	/// Root table elt
	TableElt	root_;

	/// Frame timer
	uint64		frameStart_;

	/// Vector of added DogWatches
	typedef BW::vector<DogWatch*>	DogWatches;
	DogWatches	watches_;


public:
	/**
	 *	@internal
	 *	This class is used to iterate over the elements in a DogWatchManager.
	 */
	class iterator
	{
	public:
		CSTDMF_DLL iterator( const Table* pTable, int id,
			const TableElt * pTE = NULL );
		CSTDMF_DLL iterator( const Table* pTable );

		CSTDMF_DLL iterator & operator=( const iterator & iter );

		CSTDMF_DLL bool operator==( const iterator & iter );
		/// This method implements the not equals operator.
		CSTDMF_DLL bool operator!=( const iterator & iter ) { return !(*this == iter); }
		CSTDMF_DLL iterator & operator++();

		CSTDMF_DLL iterator begin() const;
		CSTDMF_DLL iterator end() const;

		CSTDMF_DLL const BW::string & name() const;

		CSTDMF_DLL int flags() const;
		CSTDMF_DLL void flags( int flags );

		CSTDMF_DLL uint64 value( int frameAgo = 1 ) const;
		CSTDMF_DLL uint64 average( int firstAgo, int lastAgo ) const;

	private:
		const Table		* pTable_;
		int				id_;
		const TableElt	& te_;
	};

	friend class DogWatchManager::iterator;
};



// This class is always inlined, sorry.
#include "dogwatch.ipp"

//-- Added to minimize overhead of writing boring code.
#if ENABLE_DOG_WATCHERS

#define BW_SCOPED_DOG_WATCHER(name)									\
	static DogWatch	hidden_dog_watcher(name);						\
	ScopedDogWatch scoped_hidden_dog_watcher(hidden_dog_watcher);

#else

#define BW_SCOPED_DOG_WATCHER(name)

#endif //-- ENABLE_DOG_WATCHERS

BW_END_NAMESPACE

#endif // DOGWATCH_HPP
