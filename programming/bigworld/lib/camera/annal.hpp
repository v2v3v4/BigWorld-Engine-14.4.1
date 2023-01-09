#ifndef ANNAL_HPP
#define ANNAL_HPP

/**
* DEPRECATED: Note this source file and contained classes have been deprecated.
*/
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/binaryfile.hpp"


BW_BEGIN_NAMESPACE

class AnnalBase;


/**
 *	DEPRECATED: This singleton class stores the annals of game history
 */
class AnnalVault
{
public:
	AnnalVault();
	~AnnalVault();

	// State functions
	void stop();
	void record();
	void playback();

	// State accessors
	static bool isStopped()			{ return instance().state_ == STOPPED; }
	static bool isRecording()		{ return instance().state_ == RECORD; }
	static bool isPlaying()			{ return instance().state_ == PLAYBACK; }

	// Frame changed (meaningless in 'stop' mode)
	bool frame( float dTime );

	// Get time passed (or supposed to have passed)
	static float tTime()			{ return instance().tTime_; }
	static float dTime()			{ return instance().dTime_; }
	static int atFrame()			{ return instance().atFrame_; }
	static float tween()			{ return instance().tween_; }

	// Adding and deleting annals
	void add( AnnalBase * pAnnal );
	void del( AnnalBase * pAnnal );

	// Saving and loading
	void save( const BW::string & filename );
	void load( const BW::string & filename );

	// Instance accessor
	static AnnalVault & instance();

private:
	enum
	{
		STOPPED = 0,
		RECORD = 1,
		PLAYBACK = 2
	}		state_;

	float	tTime_;
	float	dTime_;

	int					atFrame_;
	float				tween_;
	BW::vector<float>	tTimes_;

	typedef BW::vector< AnnalBase * >	Annals;
	Annals				annals_;
};


/*
 *	For the workings of annals, I see two related choices:
 *
 *	You can either expect a lot of events to occur within a frame,
 *	so during playback you'll want to process each event until you get to
 *	one in the next frame, or you expect exactly one event to
 *	occur each frame, so during playback you'll want to interpolate
 *	between the two frames either side.
 *
 *	and (for the former case above)
 *
 *	You can either remember the frame number at which each event
 *	occurs, or you can record the sequence number of the event
 *	history at each frame
 */

/*
 *	So I think I'll give the user the choice
 */



/**
 *	This is the base class for an annal
 */
class AnnalBase
{
public:
	AnnalBase()
	{
		AnnalVault::instance().add( this );
	}

	~AnnalBase()
	{
		AnnalVault::instance().del( this );
	}

	virtual void clear() {}		// called before recording
	virtual void setup() {}		// called before playing

	virtual void save( BinaryFile & bf ) const = 0;
	virtual void load( BinaryFile & bf ) = 0;

protected:
	// Some statics for quicker access by derived classes
	static int			s_atFrame_;
	static float		s_tween_;

	friend class AnnalVault;
};






/**
 *	This helper struct is used by SynchronisedAnnal to provide the default
 *	function used to interpolate between two event structures. When t is 0,
 *	the current time is exactly at input a, and when t is 1 it's at input b.
 *
 *	@see AnnalSynchronised
 *	@see TweenNumeric
 */
template <class T> struct TweenFirst
{
	static void tween( const T & a, const T & /*b*/, float /*t*/, T & out )
	{
		out = a;
	}
};


/**
 *	This helper struct can be used with SynchronisedAnnal to interpolate
 *	between	two classes with number-like syntax (i.e. can multiply by
 *	constants and add)
 *
 *	@see AnnalSynchronised
 *	@see TweenFirst
 */
template <class T> struct TweenNumeric
{
	static void tween( const T & a, const T & b, float t, T & out )
	{
		out = (a * (1-t)) + (b * t);
	}
};


/**
 *	This helper struct can be used with SynchronisedAnnal to interpolate
 *	between	two classes with number-like syntax, in some cases where
 *	TweenNumeric is not appropriate (i.e. can multiply by
 *	constants, add and subtract)
 *
 *	@see TweenNumeric
 */
template <class T> struct TweenNumericAlt
{
	static void tween( const T & a, const T & b, float t, T & out )
	{
		out = a + (b-a) * t;
	}
};



/**
 *	This template class defines an annal that is synchronised to
 *	the input frame clock. i.e. there is one event for every
 *	input frame. A class providing a function to interpolate between
 *	two event structures can be provided in the second template
 *	argument. [Considered but rejected was calling on a member
 *	of T]
 */
template < class T, class Tween=TweenFirst<T>, class Vec=BW::vector<T> >
	class AnnalSynchronised : public AnnalBase
{
public:
	AnnalSynchronised() :
		first_( AnnalVault::isRecording() ? s_atFrame_ : 0 )
	{
	}

	void push( const T & event )
	{
		history_.push_back( event );
	}

	void pop( T & event )
	{
		int fat = s_atFrame_ - first_;
		if (fat <= 0)
		{
			event = history_[ 0 ];
			return;
		}
		if (fat >= int(history_.size()))
		{
			event = history_[ history_.size() - 1 ];
			return;
		}

		Tween::tween( history_[ fat-1 ], history_[ fat ],
			s_tween_, event );
	}

	virtual void save( BinaryFile & bf ) const
	{
		bf << first_;
		bf.writeSequence( history_ );
	}

	virtual void load( BinaryFile & bf )
	{
		bf >> first_;
		bf.readSequence( history_ );
	}


private:

	void clear()	{ first_ = 0; history_.clear(); }

	int				first_;
	Vec				history_;

	/*
	 * first_ is used to take care of the case where one of these annals
	 * is created while recording. it is assumed that when this occurs,
	 * then it will get a push for that frame. if it's created during
	 * playback however, then good luck! (should probably register these
	 * into a different list ... hmmm)
	 */
};



/**
 *	This template class defines an annal whose events occur
 *	at irregular times. It is intended for use in cases where
 *	at most one event occurs per input frame (on average) ...
 *	i.e. there is no frame overhead for this class of annal.
 */
template <class T> class AnnalIrregular : public AnnalBase
{
public:
	typedef std::pair<int,T>	value_type;

	AnnalIrregular() : nextPop_( 0 )	{}


	void push( const T & event )
	{
		history_.push_back( value_type( s_atFrame_, event ) );
	}

	bool pop( T & event )
	{
		if (nextPop_ >= int(history_.size()) ||
			history_[ nextPop_ ].first >= s_atFrame_) return false;

		event = history_[ nextPop_++ ].second;
		return true;
	}

	int left() const
	{
		int i;
		for (i = nextPop_; nextPop_ < history_.size() &&
			history_[ i ].first < s_atFrame_; i++) ; // scan
		return i - nextPop_;
	}

	virtual void save( BinaryFile & bf ) const
	{
		bf << history_.size();
		for (uint i = 0; i < history_.size(); i++)
		{
			bf << history_[i].first << history_[i].second;
		}
		// not using writeSequence so you can write your own BinaryFile
		//  operator<< for your type without a fuss.
	}

	virtual void load( BinaryFile & bf )
	{
		history_.clear();

		BW::vector<value_type>::size_type sz;
		bf >> sz;
		for (uint i = 0; i < sz; i++)
		{
			value_type::first_type	typeF;
			value_type::second_type	typeS;

			bf >> typeF >> typeS;
			history_.push_back( value_type( typeF, typeS ) );
		}
		// not using writeSequence so you can write your own BinaryFile
		//  operator>> for your type without a fuss.
	}


private:

	void clear()	{ history_.clear(); }
	void setup()	{ nextPop_ = 0; }

	int		nextPop_;
	BW::vector<value_type>	history_;
};


/**
 *	This template class defines an annal whose events occur
 *	frequently, with an average of more than one per frame.
 *	There is a constant overhead per frame - i.e. if no events
 *	are pushed in a given frame, then the overhead will be wasted;
 *	however if 40 events are pushed in a frame, then the overhead
 *	will have been put to good use.
 */
template <class T> class AnnalAbundant : public AnnalIrregular<T>
{
	// TODO: Write thie class :) Have history_ as a BW::vector<T>
	// and index_ as a BW::vector<int> indicating the offset into
	// history_ that a given input frame starts on
};

BW_END_NAMESPACE

#endif // ANNAL_HPP
