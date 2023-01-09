/**
 *	@file
 */

#ifndef PROFILE_H
#define PROFILE_H

#include "cstdmf/config.hpp"
#include "cstdmf/cstdmf_dll.hpp"
#include "cstdmf/timestamp.hpp"

#if ENABLE_WATCHERS

#include "cstdmf/bw_vector.hpp"
#include "cstdmf/profiler.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/watcher.hpp"

#endif // ENABLE_WATCHERS

#include <iostream>


BW_BEGIN_NAMESPACE

#if ENABLE_WATCHERS

class ProfileVal;
class ProfileGroup;
typedef SmartPointer< ProfileGroup > ProfileGroupPtr;

/**
 *  A class to wrap up a group of profiles. The grouping associates profiles
 *  with each for internal time calculations, i.e. only one profile from a group
 *  can be accumulating internal time at once.
 *
 *  Profiles in the same group must be started and stopped in a stack-like
 *  fashion.  Attempting to bridge starts and stops will trigger an assertion.
 */
class ProfileGroup : public DirectoryWatcher
{
public:
	explicit ProfileGroup( const char * watcherPath = NULL );
	~ProfileGroup();

	typedef BW::vector< ProfileVal* > Profiles;
	typedef Profiles::iterator iterator;

	iterator begin() { return profiles_.begin(); }
	iterator end() { return profiles_.end(); }

	Profiles & stack() { return stack_; }
	void add( ProfileVal * pVal );
	void reset();

	ProfileVal * pRunningTime() { return profiles_[0]; }
	const ProfileVal * pRunningTime() const { return profiles_[0]; }
	TimeStamp runningTime() const;

	static ProfileGroup & defaultGroup();

private:
	/// The profiles that are part of this group.
	Profiles profiles_;

	/// The stack of profiles currently executing in this group.
	Profiles stack_;

	/// The watcher subdirectories for this group
	DirectoryWatcherPtr pSummaries_;
	DirectoryWatcherPtr pDetails_;
	DirectoryWatcherPtr pDetailsInSeconds_;
};


/**
 *	This class is used to profile the performance of parts of the code.
 */
class ProfileVal
{
public:
	CSTDMF_DLL ProfileVal( const BW::string & name = "", ProfileGroup * pGroup = NULL );
	CSTDMF_DLL ~ProfileVal();

	/**
	 *	This method starts this profile.
	 */
	void start()
	{
		TimeStamp now = timestamp();

		if (inProgress_ == 0)
		{
			lastTime_ = now;
		}

		++inProgress_;

		ProfileGroup::Profiles & stack = pGroup_->stack();

		// Disable the existing internal profile
		if (!stack.empty())
		{
			ProfileVal & profile = *stack.back();
			profile.lastIntTime_ = now - profile.lastIntTime_;
			profile.sumIntTime_ += profile.lastIntTime_;
		}

		// This profile is now the active internal profile
		stack.push_back( this );
		lastIntTime_ = now;
	}


	/**
	 *  This method stops this profile.
	 */
	void stop( uint32 qty = 0 )
	{
		TimeStamp now = timestamp();

		if (--inProgress_ == 0)
		{
			lastTime_ = now - lastTime_;
			sumTime_ += lastTime_;

		}

		lastQuantity_ = qty;
		sumQuantity_ += qty;

		++count_;

		ProfileGroup::Profiles & stack = pGroup_->stack();
		MF_ASSERT( stack.back() == this );
		stack.pop_back();

		// Disable internal time counting for this profile
		lastIntTime_ = now - lastIntTime_;
		sumIntTime_ += lastIntTime_;

		// Re-enable the internal counter for the frame above this one.
		if (!stack.empty())
		{
			stack.back()->lastIntTime_ = now;
		}
	}

	/**
	 *	This method stops the profile and warns if it took too long.
	 */
	inline bool stop( const char * filename, int lineNum, uint32 qty = 0 )
	{
		this->stop( qty );

		const bool tooLong = this->isTooLong();

		if (tooLong)
		{
			WARNING_MSG( "%s:%d: Profile %s took %.2f seconds\n",
				filename, lineNum,
				name_.c_str(),
				lastTime_  / stampsPerSecondD() );
		}

		return tooLong;
	}

	inline bool isTooLong() const
	{
		return !this->running() &&
			(lastTime_ > s_warningPeriod_);
	}


	/**
	 *  This method resets this profile.
	 */
	void reset()
	{
		lastTime_ = 0;
		sumTime_ = 0;
		lastIntTime_ = 0;
		sumIntTime_ = 0;
		lastQuantity_ = 0;
		sumQuantity_ = 0;
		count_ = 0;
		inProgress_ = 0;
	}


	/**
	 *	This method returns whether or not this profile is currently running.
	 *	That is, start has been called more times than stop.
	 */
	bool running() const
	{
		return inProgress_ > 0;
	}


	TimeStamp lastTime() const
	{
		return this->running() ? TimeStamp( 0 ) : lastTime_;
	}

	/**
	 *  Returns the readable description of this profile.
	 */
	const char * c_str() const { return name_.c_str(); }

	double lastTimeInSeconds() const { return stampsToSeconds( lastTime_ ); }
	double sumTimeInSeconds() const  { return stampsToSeconds( sumTime_ ); }
	double lastIntTimeInSeconds() const { return stampsToSeconds( lastIntTime_ ); }
	double sumIntTimeInSeconds() const { return stampsToSeconds( sumIntTime_ ); }

	/// String description of this profile.
	BW::string	name_;

	/// The profile group this profile belongs to, if any.
	ProfileGroup * pGroup_;

	TimeStamp		lastTime_;		///< The time the profile was started.
	TimeStamp		sumTime_;		///< The total time between all start/stops.
	TimeStamp		lastIntTime_;	///< The last internal time for this profile.
	TimeStamp		sumIntTime_;	///< The sum of internal time for this profile.
	uint32		lastQuantity_;	///< The last value passed into stop.
	uint32		sumQuantity_;	///< The total of all values passed into stop.
	uint32		count_;			///< The number of times stop has been called.
	int			inProgress_;	///< Whether the profile is currently timing.

	CSTDMF_DLL static WatcherPtr pSummaryWatcher();
	CSTDMF_DLL static WatcherPtr pWatcherStamps();
	CSTDMF_DLL static WatcherPtr pWatcherSeconds();

	static void setWarningPeriod( TimeStamp warningPeriod )
							{ s_warningPeriod_ = warningPeriod; }

private:
	CSTDMF_DLL static TimeStamp s_warningPeriod_;
};

/**
 *	This function is the output stream operator for a ProfileVal.
 *
 *	@relates ProfileVal
 */
std::ostream& operator<<( std::ostream &s, const ProfileVal &v );

/**
 *	This function is the input stream operator for ProfileVal.
 *
 *	@relates ProfileVal
 */
std::istream& operator>>( std::istream &s, ProfileVal &v );


/**
 *	This singleton class resets all registered ProfileGroups when a nominated
 *	ProfileVal is reset.
 */
class ProfileGroupResetter
{
public:
	ProfileGroupResetter();
	~ProfileGroupResetter();

	void nominateProfileVal( ProfileVal * pVal = NULL );
	void addProfileGroup( ProfileGroup * pGroup );

	static ProfileGroupResetter & instance();

private:
	ProfileVal * nominee_;

	BW::vector< ProfileGroup * >	groups_;

	bool doingReset_;

	// called by every ProfileVal when it is reset
	void resetIfDesired( ProfileVal & val );

	friend std::istream& operator>>( std::istream &s, ProfileVal &v );
};


#define START_PROFILE( PROFILE ) PROFILE.start();



/**
 *	This utility class helps with profiling a scope.
 */
class ScopedProfile
{
public:
	ScopedProfile( ProfileVal & profile, const char * filename, int lineNum ) :
		profile_( profile ),
		filename_( filename ),
		lineNum_( lineNum )
	{
		profile_.start();
	}

	~ScopedProfile()
	{
		profile_.stop( filename_, lineNum_ );
	}

private:
	ProfileVal & profile_;
	const char * filename_;
	int lineNum_;
};


#if ENABLE_PROFILER
#define AUTO_SCOPED_PROFILE( NAME )											\
	ScopedProfiler _profScopedProfiler( NAME );								\
	static ProfileVal _localProfile( NAME );								\
	ScopedProfile _autoScopedProfile( _localProfile, __FILE__, __LINE__ );
#else
#define AUTO_SCOPED_PROFILE( NAME )											\
	static ProfileVal _localProfile( NAME );								\
	ScopedProfile _autoScopedProfile( _localProfile, __FILE__, __LINE__ );
#endif

#define SCOPED_PROFILE( PROFILE )											\
	PROFILER_SCOPED( PROFILE );												\
	ScopedProfile _scopedProfile( PROFILE, __FILE__, __LINE__ );

#define STOP_PROFILE( PROFILE )												\
	PROFILE.stop( __FILE__, __LINE__ );

#define STOP_PROFILE_WITH_CHECK( PROFILE )									\
	if (PROFILE.stop( __FILE__, __LINE__ ))

#define STOP_PROFILE_WITH_DATA( PROFILE, DATA )								\
	PROFILE.stop( __FILE__, __LINE__ , DATA );

#else

#define AUTO_SCOPED_PROFILE( NAME )

#endif //ENABLE_WATCHERS

/**
 *	This structure wraps up a TimeStamp timestamp delta and has an operator defined
 *	on it to print it out nicely.
 */
struct NiceTime
{
	/// Constructor
	explicit NiceTime(TimeStamp t) : t_(t) {}

	TimeStamp t_;	///< Associated timestamp.
};

/**
 *	This function is the output stream operator for a NiceTime.
 *
 *	@relates NiceTime
 */
CSTDMF_DLL std::ostream& operator<<( std::ostream &o, const NiceTime &nt );

BW_END_NAMESPACE

#endif	// PROFILE_H
