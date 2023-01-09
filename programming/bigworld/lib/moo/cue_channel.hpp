#ifndef CUE_CHANNEL_HPP
#define CUE_CHANNEL_HPP


#include "animation_channel.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/vectornodest.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{


class CueBase;
typedef SmartPointer<CueBase> CueBasePtr;

/**
 *	This class contains the base information for a Cue.
 *	They are put in a global table so as to not waste memory.
 */
class CueBase : public ReferenceCount, public BW::string
{
public:
	~CueBase();

	char type() const					{ return (*this)[0]; }
	const char * identifier() const		{ return this->c_str()+1; }

	static CueBasePtr get( const BW::string & raw );
	static CueBasePtr find( char type, const char * identifier );

private:
	CueBase( const BW::string & raw );

	/**
	 *	Helper class for global set of cue bases
	 */
	class CueBaseDumbPtr
	{
	public:
		explicit CueBaseDumbPtr( CueBase * pObject ) : pObject_( pObject ) { }

		bool operator<( const CueBaseDumbPtr & other ) const
			{ return *pObject_ < *other.pObject_; }
		bool operator>( const CueBaseDumbPtr & other ) const
			{ return *pObject_ > *other.pObject_; }
		bool operator==( const CueBaseDumbPtr & other ) const
			{ return *pObject_ == *other.pObject_; }
		bool operator!=( const CueBaseDumbPtr & other ) const
			{ return *pObject_ != *other.pObject_; }

		CueBase * pObject_;
	};

	typedef BW::set< CueBaseDumbPtr > CueBases;
	static CueBases s_pool_;
};


/**
 *	This class is a Cue as it appears in a CueChannel
 */
class Cue
{
public:
	CueBasePtr					cueBase_;
	std::basic_string<float>	args_;
};


class CueShot;
typedef VectorNoDestructor<CueShot> CueShots;

/**
 *	This class describes a cue that has fired
 */
class CueShot
{
public:
	CueShot( const Cue * pCue, float missedBy ) :
		pCue_( pCue ), missedBy_( missedBy ) { }

	const Cue *	pCue_;
	float		missedBy_;	// in frames

	static CueShots	s_cueShots_;
};


/**
 *	This class is an AnimationChannel of cues tied to particular frames
 *	in the animation.
 */
class CueChannel : public AnimationChannel
{
public:
	virtual bool		wantTick() const					{ return true; }
	virtual void		tick( float dtime, float otime, float ntime,
							float btime, float etime ) const;

	virtual bool		load( BinaryFile & bf );
	virtual bool		save( BinaryFile & bf ) const;

	virtual void		preCombine ( const AnimationChannel & rOther ) { }
	virtual void		postCombine( const AnimationChannel & rOther ) { }

	virtual int			type() const				{ return 6; }

	virtual AnimationChannel * duplicate() const
		{ return new CueChannel( *this ); }

private:
	CueChannel& operator=( const CueChannel& );

	typedef BW::vector< std::pair< float, Cue > > CueKeyframes;
	CueKeyframes		cues_;

	static AnimationChannel * New()
		{ return new CueChannel(); }
	static TypeRegisterer s_rego_;
};


} // namespace Moo

BW_END_NAMESPACE

#endif // CUE_CHANNEL_HPP
