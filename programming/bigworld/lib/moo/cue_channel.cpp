#include "pch.hpp"
#include "cue_channel.hpp"
#include "cstdmf/binaryfile.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{


// -----------------------------------------------------------------------------
// Section: CueBase
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
CueBase::CueBase( const BW::string & raw ) :
	BW::string( raw )
{
}

/**
 *	Destructor
 */
CueBase::~CueBase()
{
	BW_GUARD;
	if (!this->empty())
	{
		s_pool_.erase( CueBaseDumbPtr( this ) );
	}
}

/**
 *	Static method to get a cue base for the given raw cue data
 */
CueBasePtr CueBase::get( const BW::string & raw )
{
	BW_GUARD;
	// make sure it's valid
	if (raw.length() < 1) return NULL;

	// search for a temporary cue base
	CueBase temp( raw );
	CueBaseDumbPtr cbdp( &temp );
	CueBases::iterator found = s_pool_.find( cbdp );

	// clear temp so the destructor doesn't remove it from the pool
	temp.assign( "" );

	// and return the existing object if found, or a new one if not
	if (found != s_pool_.end())
	{
		return found->pObject_;
	}
	else
	{
		CueBase * newCB = new CueBase( raw );
		s_pool_.insert( CueBaseDumbPtr( newCB ) );
		return newCB;
	}
}


/**
 *	Static method to find the given cue base.
 *	It returns NULL if it is now present.
 */
CueBasePtr CueBase::find( char type, const char * identifier )
{
	BW_GUARD;
	// make up the raw string
	BW::string raw( &type, 1 );
	raw.append( identifier );

	// search for a temporary cue base
	CueBase temp( raw );
	CueBaseDumbPtr cbdp( &temp );
	CueBases::iterator found = s_pool_.find( cbdp );

	// clear temp so the destructor doesn't remove it from the pool
	temp.assign( "" );

	// and return the existing object if found, or NULL if not
	return (found != s_pool_.end()) ? found->pObject_ : NULL;
}


/// static initialisers
CueBase::CueBases CueBase::s_pool_;

CueShots CueShot::s_cueShots_;


// -----------------------------------------------------------------------------
// Section: CueChannel
// -----------------------------------------------------------------------------


/**
 *	Tick method. This is where we fire off our cues.
 */
void CueChannel::tick( float dtime, float otime, float ntime,
	float btime, float etime ) const
{
	BW_GUARD;
	// get out if there's nothing to look at
	if (cues_.empty()) return;

	dtime = abs( dtime );

	// swap times if playing backwards
	float mtime = otime;
	if (otime > ntime)
	{		// note that cueshot output order is reversed in this case
		otime += ntime;
		ntime = otime - ntime;
		otime = otime - ntime;
		btime += etime;
		etime = btime - etime;
		btime = btime - etime;
		// negate dtime to make missedBy calc positive and correct
		dtime = -dtime;
	}

	// find animation segment range
	CueKeyframes::const_iterator bit = cues_.begin();
	while (bit != cues_.end() && bit->first < btime) bit++;
	CueKeyframes::const_iterator eit = bit;
	while (eit != cues_.end() && eit->first < etime) eit++;
	if (bit == eit) return;

	// skip any initial cycles (and avoid negativity)
	float ttime = etime - btime;
	float toff = floorf( otime / ttime ) * ttime;

	// find first cue
	CueKeyframes::const_iterator it = bit;
	while (toff + it->first < otime)
	{
		it++;
		if (it == eit)
		{
			it = bit;
			toff += ttime;
		}
	}

	// find last cue
	while (toff + it->first < ntime)
	{
		// firing shots in between
		float missedBy = (toff + it->first - mtime) * (ntime-otime) / dtime;
		CueShot::s_cueShots_.push_back(
			CueShot( &it->second, missedBy ) );

		it++;
		if (it == eit)
		{
			it = bit;
			toff += ttime;
		}
	}

	// note: a cue at exactly otime is fired but one at exactly ntime is not.
	// this is consistent with playing up to but not exactly on the last frame.
	// unfortunately when we swap otime and ntime when playing backwards,
	// the 'first' frame misses out but the 'last' frame doesn't ...
	// anyway, it at least still makes sense when creating the cues in MAX.
}


/**
 *	Cue key reader
 */
static BinaryFile & operator>>( BinaryFile & bf,
	std::pair<float,Cue> & cueKey )
{
	BW_GUARD;
	bf >> cueKey.first;

	BW::string cueBaseRaw;
	bf >> cueBaseRaw;
	cueKey.second.cueBase_ = CueBase::get( cueBaseRaw );

	BW::vector<float>	vargs;
	bf.readSequence( vargs );
	if (!vargs.empty())
	{
		cueKey.second.args_.assign( &*vargs.begin(), vargs.size() );
	}

	return bf;
}

/**
 *	Cue key writer
 */
static BinaryFile & operator<<( BinaryFile & bf,
	const std::pair<float,Cue> & cueKey )
{
	BW_GUARD;
	bf << cueKey.first;

	bf << BW::string( *cueKey.second.cueBase_ );

	if( cueKey.second.args_.length() > 0 )
	{
		BW::vector<float>	vargs( cueKey.second.args_.begin(),
			cueKey.second.args_.end() );
		bf.writeSequence( vargs );
	}
	else
	{
		bf << (int)0;
	}

	return bf;
}


/**
 *	Load method
 */
bool CueChannel::load( BinaryFile & bf )
{
	BW_GUARD;
	// not calling base method
	bf.readSequence( cues_ );
	return !!bf;
}

/**
 *	Save method
 */
bool CueChannel::save( BinaryFile & bf ) const
{
	BW_GUARD;
	// not calling base method
	bf.writeSequence( cues_ );
	return !!bf;
}


/// static initialiser
CueChannel::TypeRegisterer CueChannel::s_rego_( CUE_CHANNEL, New );

int CueChannel_token = 0;

} // namespace Moo

BW_END_NAMESPACE

// cue_channel.cpp
