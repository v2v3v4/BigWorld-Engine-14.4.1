#include "pch.hpp"

#include "config.hpp"
#include "profile.hpp"
#include "timestamp.hpp"
#include "binary_stream.hpp"

#include <limits>

BW_BEGIN_NAMESPACE

#if ENABLE_WATCHERS

CSTDMF_DLL TimeStamp ProfileVal::s_warningPeriod_ = std::numeric_limits< uint64 >::max();

// -----------------------------------------------------------------------------
// Section: ProfileVal
// -----------------------------------------------------------------------------

/**
 *  Constructor.
 */
ProfileVal::ProfileVal( const BW::string & name, ProfileGroup * pGroup ) :
	name_( name ),
	pGroup_( pGroup )
{
	// Initialise all counters.
	this->reset();

	// At the moment we don't support have values that aren't in a group at
	// all.  Not hard to add support for this if we ever actually need it.
	if (pGroup_ == NULL)
	{
		pGroup_ = &ProfileGroup::defaultGroup();
	}

	if (!name_.empty())
	{
		pGroup_->add( this );
	}
}


/**
 *  Destructor.
 */
ProfileVal::~ProfileVal()
{
	if (pGroup_)
	{
		std::remove( pGroup_->begin(), pGroup_->end(), this );
	}
}


/**
 *	This method returns the Watcher associated with this class.
 */
WatcherPtr ProfileVal::pSummaryWatcher()
{
	static WatcherPtr pWatcher;

	if (pWatcher == NULL)
	{
		ProfileVal * pNull = NULL;
		pWatcher =
			new DataWatcher< ProfileVal >( *pNull, Watcher::WT_READ_WRITE );
	}

	return pWatcher;
}


/**
 *
 */
WatcherPtr ProfileVal::pWatcherStamps()
{
	static WatcherPtr pWatcher = NULL;

	if (pWatcher == NULL)
	{
		pWatcher = new DirectoryWatcher();
		ProfileVal	* pNull = NULL;

		pWatcher->addChild( "count",        makeWatcher( pNull->count_ ) );
		pWatcher->addChild( "inProgress",   makeWatcher( pNull->inProgress_ ) );
		pWatcher->addChild( "sumQuantity",  makeWatcher( pNull->sumQuantity_ ) );
		pWatcher->addChild( "lastQuantity", makeWatcher( pNull->lastQuantity_ ) );
		pWatcher->addChild( "sumTime",      makeWatcher( (uint64&)pNull->sumTime_ ) );
		pWatcher->addChild( "lastTime",     makeWatcher( (uint64&)pNull->lastTime_ ) );
		pWatcher->addChild( "sumIntTime",   makeWatcher( (uint64&)pNull->sumIntTime_ ) );
		pWatcher->addChild( "lastIntTime",  makeWatcher( (uint64&)pNull->lastIntTime_ ) );
	}

	return pWatcher;
}



/**
 *
 */
WatcherPtr ProfileVal::pWatcherSeconds()
{
	static WatcherPtr pWatcher = NULL;

	if (pWatcher == NULL)
	{
		pWatcher = new DirectoryWatcher();
		ProfileVal	* pNull = NULL;

		pWatcher->addChild( "count",        makeWatcher( pNull->count_ ) );
		pWatcher->addChild( "inProgress",   makeWatcher( pNull->inProgress_ ) );
		pWatcher->addChild( "sumQuantity",  makeWatcher( pNull->sumQuantity_ ) );
		pWatcher->addChild( "lastQuantity", makeWatcher( pNull->lastQuantity_ ) );

		pWatcher->addChild( "sumTime",      makeWatcher( pNull->sumTime_ ) );
		pWatcher->addChild( "lastTime",     makeWatcher( pNull->lastTime_ ) );
		pWatcher->addChild( "sumIntTime",   makeWatcher( pNull->sumIntTime_ ) );
		pWatcher->addChild( "lastIntTime",  makeWatcher( pNull->lastIntTime_ ) );
	}

	return pWatcher;
}


// Comment in header file.
std::ostream& operator<<( std::ostream & s, const ProfileVal & val )
{
	TimeStamp lastTime = val.inProgress_ ?
		TimeStamp(timestamp() - val.lastTime_) : val.lastTime_;

	s << "Count: " << val.count_;
	if (val.inProgress_)
	{
		s << " inProgress: " << NiceTime( lastTime );
	}
	s << std::endl;

	if (val.count_ == 0)
	{
		return s;
	}

	if (val.sumQuantity_ != 0)
	{
		s << "Quantity: " << val.sumQuantity_ <<
			" (last " << val.lastQuantity_ <<
			", avg " << ((double)val.sumQuantity_)/val.count_ << ')' << std::endl;
	}

	s << "Time: " << NiceTime( val.sumTime_ ) <<
	" (last " << NiceTime( lastTime ) << ')' << std::endl;

	if (val.sumTime_ != 0)
	{
		s << "Time/Count: " << NiceTime( val.sumTime_ / val.count_ ) << std::endl;

		if (val.sumQuantity_)
		{
			s << "Time/Qty: " <<
				NiceTime( val.sumTime_ / val.sumQuantity_ ) << std::endl;
		}
	}

	return s;
}


/**
 *	This function is the input stream operator for ProfileVal.
 *
 *	@relates ProfileVal
 */
std::istream& operator>>( std::istream & s, ProfileVal & v )
{
	v.reset();
	ProfileGroupResetter::instance().resetIfDesired( v );
	return s;
}


// -----------------------------------------------------------------------------
// Section: ProfileGroup
// -----------------------------------------------------------------------------

/**
 *  Constructor.
 */
ProfileGroup::ProfileGroup( const char * watcherPath ) :
	pSummaries_( new DirectoryWatcher() ),
	pDetails_( new DirectoryWatcher() ),
	pDetailsInSeconds_( new DirectoryWatcher() )
{
	// Make sure timers are set up.
	stampsPerSecond();

	ProfileVal * pRunningTime = new ProfileVal( "RunningTime", this );
	pRunningTime->start();

	this->addChild( "summaries", pSummaries_ );
	this->addChild( "details", pDetails_ );
	this->addChild( "detailsInSeconds", pDetailsInSeconds_ );

	// If we are the default group, then add ourselves to the root of the
	// watcher tree.
#ifdef MF_SERVER
	if (watcherPath)
	{
		Watcher::rootWatcher().addChild( watcherPath, this );
	}
#endif
}


/**
 *  Destructor.
 */
ProfileGroup::~ProfileGroup()
{
	// Clean up the running time profile
	delete this->pRunningTime();
}


/**
 *  This method adds a new profile to this group.
 */
void ProfileGroup::add( ProfileVal * pVal )
{
	profiles_.push_back( pVal );
	const char * name = pVal->c_str();

	pSummaries_->addChild( name, ProfileVal::pSummaryWatcher(), pVal );
	pDetails_->addChild( name, ProfileVal::pWatcherStamps(), pVal );
	pDetailsInSeconds_->addChild( name, ProfileVal::pWatcherSeconds(), pVal );
}


/**
 *  This method sets the timer profile for this group, i.e. the one that is
 *  always running and exists to track how long the group has been running for.
 */
TimeStamp ProfileGroup::runningTime() const
{
	return timestamp() - this->pRunningTime()->lastTime_;
}


/**
 *	This method resets all of the ProfileVal's in this ProfileGroup.
 */
void ProfileGroup::reset()
{
	for (iterator iter = this->begin(); iter != this->end(); ++iter)
	{
		(*iter)->reset();
	}

	this->pRunningTime()->start();
}


/**
 *	This method returns a reference to the default group.
 */
ProfileGroup & ProfileGroup::defaultGroup()
{
	static ProfileGroupPtr s_pDefaultGroup(new ProfileGroup( "profiles" ));

	return *s_pDefaultGroup;
}

// -----------------------------------------------------------------------------
// Section: ProfileGroupResetter
// -----------------------------------------------------------------------------


/// Constructor
ProfileGroupResetter::ProfileGroupResetter() :
	nominee_( 0 ),
	groups_( 0 ),
	doingReset_( false )
{
}

/// Destructor
ProfileGroupResetter::~ProfileGroupResetter()
{
}

/**
 * Records the ProfileVal whose reset should trigger a global reset.
 * Note: If this is in a ProfileGroup, you shouldn't add to the
 * group after you call this function as it could move the memory
 * containing the vector elements around and stuff everything up.
 */
void ProfileGroupResetter::nominateProfileVal( ProfileVal * pVal )
{
	nominee_ = pVal;
}

/**
 * Records a ProfileGroup which should be reset when the nominated
 * ProfileVal is. Don't call this when profiles are being reset.
 */
void ProfileGroupResetter::addProfileGroup( ProfileGroup * pGroup )
{
	if (doingReset_) return;

	groups_.push_back( pGroup );
}

/**
 * Static function to get the singleton instance.
 */
ProfileGroupResetter & ProfileGroupResetter::instance()
{
	static ProfileGroupResetter staticInstance;
	return staticInstance;
}

/**
 * Function called by a ProfileVal when it is reset to see if
 * ProfileGroupResetter should do its stuff.
 */
void ProfileGroupResetter::resetIfDesired( ProfileVal & val )
{
	if (doingReset_) return;
	if (nominee_ != &val) return;

	doingReset_ = true;

	BW::vector<ProfileGroup *>::iterator iter;

	for (iter = groups_.begin(); iter != groups_.end(); iter++)
	{
		(*iter)->reset();
	}

	doingReset_ = false;
}



#endif // ENABLE_WATCHERS



// Commented in header file.
std::ostream& operator<<(std::ostream &o, const NiceTime &nt)
{
	// Overflows after 12 hours if we don't do this with doubles.
	static double microsecondsPerStamp = 1000000.0 / stampsPerSecondD();

	double microDouble = ((double)(int64)nt.t_) * microsecondsPerStamp;
	TimeStamp micros = (uint64)microDouble;

	double truncatedStampDouble = (double)(int64)
		( nt.t_ - ((uint64)(((double)(int64)micros)/microsecondsPerStamp)));
	uint32 picos = (uint32)( truncatedStampDouble * 1000000.0 *
		microsecondsPerStamp );

	// phew! Logic of lines above is: turn micros value we have back into
	// stamps, subtract it from the total number of stamps, and turn the
	// result into seconds just like before (except in picoseconds this time).
	// Simpler schemes seem to suffer from floating-point underflow.

	// Now see if micros was rounded and if so square it (hehe :)
	// It bothers me that I have to check for this...
//	uint32 origPicos = picos;
	if (picos > 1000000)
	{
		uint32 negPicos = 1 + ~picos;
		if (negPicos > 1000000)
		{
			picos = 0;	// sanity check
		}
		else
		{
			picos = negPicos;
			micros--;
		}
	}
	// Well, it all doesn't work anyway. At high values (over 10s say) we
	// still get wild imprecision in the calculation of picos. We shouldn't
	// I'll just leave it here for the moment 'tho. TODO: Fix this.

	if (micros == 0 && picos == 0)
	{
		o << "0us";
		return o;
	}

	if (micros>=1000000)
	{
		uint32 s = uint32(micros/1000000);

		if (s>=60)
		{
			o << (s/60) << "min ";
		}
		o << (s%60) << "sec ";
	}

	//o.width( 3 );		o.fill( '0' );		o << std::ios::right;

	//o.form( "%03d", (micros/1000)%1000 ) << " ";
	//o.form( "%03d", micros%1000 ) << ".";
	//o.form( "%03d", (picos/1000)%1000 ) << "us";

	// OKok, I give up on streams!

	char	lastBit[32];
	bw_snprintf( lastBit, sizeof(lastBit), "%03d %03d",
		(int)(uint32)((micros/1000)%1000),
		(int)(uint32)(micros%1000) );
	o << lastBit;

	//if (micros < 60000000)
	{		// Only display this precision if <1min. Just silly otherwise.
		bw_snprintf(lastBit, sizeof(lastBit), ".%03d",
			(int)((picos/1000)%1000) );
		o << lastBit;
	}

	o << "us";

	return o;
}

BW_END_NAMESPACE

// profile.cpp
