#include "pch.hpp"
#include "annal.hpp"

/**
* DEPRECATED: Note this source file and contained classes have been deprecated.
*/

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Camera", 0 )


BW_BEGIN_NAMESPACE

/**
 *	Constructor
 */
AnnalVault::AnnalVault() :
	state_(),
	tTime_( 0 ),
	dTime_( 0 ),
	atFrame_( 0 ),
	tween_( 0 )
{
}


/**
 *	Destructor
 */
AnnalVault::~AnnalVault()
{
}



/**
 *	This method stops recording or playback
 */
void AnnalVault::stop()
{
	state_ = STOPPED;
}


/**
 *	This method begins recording. Should (logically) be called soon after
 *	'frame' would have been. Note: Please always record at least two frames!
 */
void AnnalVault::record()
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( state_ == STOPPED )
	{
		return;
	}

	state_ = RECORD;

	// set up the first frame
	dTime_ = 0;
	tTime_ = 0;

	atFrame_ = 0;
	tween_ = 0;
	tTimes_.clear();
	tTimes_.push_back( 0 );
		// before discrete input frame 0, total time was 0

	AnnalBase::s_atFrame_ = atFrame_;
	AnnalBase::s_tween_ = tween_;

	// now clear all our annals
	for (uint i=0; i < annals_.size(); i++)
	{
		annals_[i]->clear();
	}
}


/**
 *	This method begins playback. Should (logically) be called soon after
 *	'frame' would have been.
 */
void AnnalVault::playback()
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( state_ == STOPPED )
	{
		return;
	}

	state_ = PLAYBACK;

	dTime_ = 0;
	tTime_ = 0;

	// set up for the first frame
	atFrame_ = 1;	// atFrame_ must have a frame either side of it
	tween_ = 0.f;

	AnnalBase::s_atFrame_ = atFrame_;
	AnnalBase::s_tween_ = tween_;

	// now setup all our annals
	for (uint i=0; i < annals_.size(); i++)
	{
		annals_[i]->setup();
	}
}




/**
 *	A frame has just passed. In record mode, dTime is the time
 *	it took; in playback mode, dTime is the time it should look
 *	like it took.
 *
 *	@return	in record mode, always true. in playback mode, true
 *		if the store of recorded frames has not yet been exhausted.
 */
bool AnnalVault::frame( float dTime )
{
	BW_GUARD;
	if (state_ == RECORD)
	{
		MF_ASSERT_DEV( dTime > 0.f );

		tTime_ += dTime;
		dTime_ = dTime;

		atFrame_++;
		tween_ = 0.f;
		tTimes_.push_back( tTime_ );
			// before discrete input frame atFrame_, total time was tTime_

		// during recording, atFrame_ is the input discrete frame
		//  number that new events occur at.

		AnnalBase::s_atFrame_ = atFrame_;
		AnnalBase::s_tween_ = tween_;

		return true;
	}
	else if (state_ == PLAYBACK)
	{
		tTime_ += dTime;
		dTime_ = dTime;

		// ok, figure out what frame we should be at then
		while (tTimes_[ atFrame_ ] <= tTime_ && atFrame_ < int(tTimes_.size()))
		{
			atFrame_++;
		}

		// did we run out of frames then? (we maintain the condition that
		//  frames exist at both atFrame_-1 and atFrame_, for tweening)
		if (atFrame_ >= int(tTimes_.size())) return false;

		// during playback, atFrame_ is the input discrete frame
		//  number one after the time wishing to be represented.

		// tween tells you how close the playback frame is to the recorded
		// frame at atFrame_ (vs the one at atFrame_-1). i.e. it's 0 if
		// tTime_ == tTimes_[ atFrame_ - 1 ] and 1 if
		// tTime_ == tTimes_[ atFrame_ ] == tTime_
		tween_ = (tTime_ - tTimes_[ atFrame_ - 1 ]) /
			(tTimes_[ atFrame_ ] - tTimes_[ atFrame_ - 1 ]) ;

		AnnalBase::s_atFrame_ = atFrame_;
		AnnalBase::s_tween_ = tween_;

		return true;
	}

	MF_ASSERT_DEV( state_ == RECORD || state_ == PLAYBACK );

	return false;
}




/**
 *	Adds an annal to our collection
 */
void AnnalVault::add( AnnalBase * pAnnal )
{
	BW_GUARD;
	annals_.push_back( pAnnal );
}


/**
 *	Deletes an annal from out collection
 */
void AnnalVault::del( AnnalBase * pAnnal )
{
	BW_GUARD;
	Annals::iterator it = std::find( annals_.begin(), annals_.end(), pAnnal );
	if (it != annals_.end())
	{
		annals_.erase( it );
	}
	else
	{
		ERROR_MSG( "Can't find annal at %p to delete it!\n", pAnnal );
	}
}



/**
 *	Saves all this vault's annals out to the specified file
 */
void AnnalVault::save( const BW::string & filename )
{
	BW_GUARD;
	BinaryFile bf( bw_fopen( filename.c_str(), "wb" ) );
	if (bf.file() == NULL)
	{
		ERROR_MSG( "AnnalVault::save: Couldn't create %s\n", filename.c_str() );
		return;
	}

	bf.writeSequence( tTimes_ );

	for (uint i=0; i < annals_.size(); i++)
	{
		annals_[i]->save( bf );
	}
}


/**
 *	Loads all this vault's annals in from the specified file
 */
void AnnalVault::load( const BW::string & filename )
{
	BW_GUARD;
	BinaryFile bf( bw_fopen( filename.c_str(), "rb" ) );
	if (bf.file() == NULL)
	{
		ERROR_MSG( "AnnalVault::load: Couldn't open %s\n", filename.c_str() );
		return;
	}

	if (!bf.cache()) {
		ERROR_MSG( "AnnalVault::load: couldn't load data from %s\n", filename.c_str() );
		return;
	}

	// things had better be registered in the same order...
	//  we should probably write out the compile time or
	//  something here to make sure we're running identical clients

	bf.readSequence( tTimes_ );

	for (uint i=0; i < annals_.size(); i++)
	{
		annals_[i]->load( bf );
	}
}




/**
 *	Static instance method
 */
AnnalVault & AnnalVault::instance()
{
	static AnnalVault	vault;
	return vault;
}


// Initialise the static data in AnnalBase
int AnnalBase::s_atFrame_ = 0;
float AnnalBase::s_tween_ = 0.f;




/*
#include "math/vector3.hpp"

static struct TestMe
{
	TestMe()
	{
		asi.push( Vector3(0,1,0) );
		Vector3	rv;
		asi.pop( rv );

		int	i = 4;
		aii.push( i );
		while (aii.pop( i ));

		float	f = 39.5f;
		aai.push( 123.3f );
		int maxi = aai.left();
		while (maxi >= 0) { aai.pop( f ); }
	}

	AnnalSynchronised< Vector3, TweenNumeric<Vector3> >	asi;
	AnnalIrregular< int >	aii;
	AnnalAbundant< float >	aai;
} s_testMe;
*/

BW_END_NAMESPACE

// annal.cpp
