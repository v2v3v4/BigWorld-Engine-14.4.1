#ifndef ENTITY_MEMBER_STATS_HPP
#define ENTITY_MEMBER_STATS_HPP

#include "cstdmf/smartpointer.hpp"
#include "cstdmf/timestamp.hpp"
#include "math/stat_with_rates_of_change.hpp"
#include "math/stat_watcher_creator.hpp"


BW_BEGIN_NAMESPACE

class Watcher;
typedef SmartPointer<Watcher> WatcherPtr;

/**
 *	This class is a base class for MethodDescription and DataDescription. It is
 *	used to store statistics about these instances.
 */
class EntityMemberStats
{
public:
#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();

	void countSentToOwnClient( int bytes )
	{
		sentToOwnClient_.count( bytes );
	}

	void countSentToOtherClients( int bytes )
	{
		sentToOtherClients_.count( bytes );
	}

	void countAddedToHistoryQueue( int bytes )
	{
		addedToHistoryQueue_.count( bytes );
	}

	void countSentToGhosts( int bytes )
	{
		sentToGhosts_.count( bytes );
	}

	void countSentToBase( int bytes )
	{
		sentToBase_.count( bytes );
	}

	void countReceived( int bytes )
	{
		received_.count( bytes );
	}

	void countOversized( int bytes )
	{
		oversized_.count( bytes );
	}

	static void limitForBaseApp()
	{
		s_limitForBaseApp_ = true;
	}

	class Stat
	{
		typedef IntrusiveStatWithRatesOfChange< unsigned int >  SubStat;
		SubStat messages_;
		SubStat bytes_;
		static SubStat::Container * s_pContainer;
	public:
		Stat():
			messages_( s_pContainer ),
			bytes_( s_pContainer )
		{
			StatWatcherCreator::initRatesOfChangeForStat( messages_ );
			StatWatcherCreator::initRatesOfChangeForStat( bytes_ );
		}
		Stat & operator=( const Stat & other )
		{
			return *this;
		}
		void count( uint32 bytes )
		{
			messages_++;
			bytes_ += bytes;
		}

		static void tickAll( double deltaTime = 1.0 );
		static void tickSome( size_t index, size_t total, double deltaTime );

		void addWatchers( WatcherPtr watchMe, BW::string name );
	};

private:
	Stat sentToOwnClient_;
	Stat sentToOtherClients_;
	Stat addedToHistoryQueue_;
	Stat sentToGhosts_;
	Stat sentToBase_;
	Stat received_;
	Stat oversized_;

	static bool s_limitForBaseApp_;
#endif // ENABLE_WATCHERS
};

BW_END_NAMESPACE

#endif // ENTITY_MEMBER_STATS_HPP
