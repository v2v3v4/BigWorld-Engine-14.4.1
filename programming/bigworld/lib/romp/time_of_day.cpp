#include "pch.hpp"

#include "time_of_day.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/locale.hpp"
#include "cstdmf/watcher.hpp"

#include "math/colour.hpp"
#include "math/matrix.hpp"
#include "resmgr/dataresource.hpp"
#include "resmgr/datasection.hpp"
#include "resmgr/bwresource.hpp"
#include "moo/render_context.hpp"
#include "math/blend_transform.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 );


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "time_of_day.ipp"
#endif

const float HOURS_IN_DAY = 24;


/**
 *	Constructor
 */
TimeOfDay::TimeOfDay( float currentTime ) :
	time_( currentTime ),
	startTime_( 0.f ),
	secondsPerGameHour_( 160.f ),
	sunAngle_( 80.f ),
	sunColor_(255.0f, 255.0f, 255.0f),
	moonAngle_( 80.f ),
	moonColor_(255.0f, 255.0f, 255.0f),
	sunAnimation_( true, HOURS_IN_DAY ),	
	ambientAnimation_( true, HOURS_IN_DAY ),
	fogAnimation_( true, HOURS_IN_DAY ),
	file_( "" ),
	updateNotifiersOn_( true ),
	timeConfigChanged_( false )
{
	BW_GUARD;
}

/**
 *	Destructor
 */
TimeOfDay::~TimeOfDay()
{
	BW_GUARD;
}


/**
 *	This method ticks over the game time of day by dTime real time,
 *	which is translated to game time by dividing it be secondsPerGameHour_
 */
void TimeOfDay::tick( float dTime )
{
	BW_GUARD;
	if (secondsPerGameHour_ > 0.f)
	{
		time_ = fmodf( time_ + dTime / secondsPerGameHour_, HOURS_IN_DAY );
	}

	if (!sunAnimation_.empty())
	{
		now_.sunColour = Vector4( sunAnimation_.animate( time_ )/255.f, 1.f );		
		now_.ambientColour = Vector4( ambientAnimation_.animate( time_ )/255.f, 1.f);
		now_.fogColour = Vector4( fogAnimation_.animate( time_ )/255.f, 1.f);		
	}

	now_.sunTransform.setRotateX( DEG_TO_RAD( - sunAngle_ ) );
	now_.sunTransform.preRotateY( 2.f * MATH_PI - DEG_TO_RAD( ( time_ / HOURS_IN_DAY ) * 360 ) );

	now_.moonTransform.setRotateX( DEG_TO_RAD( - moonAngle_ ) );
	now_.moonTransform.preRotateY( MATH_PI + DEG_TO_RAD( ( time_ / HOURS_IN_DAY ) * 360 ) );

	Vector3 localDir( 0, 0, -1 );
	Vector3 sunPos( now_.sunTransform.applyPoint( localDir ) );
	Vector3 moonPos( now_.moonTransform.applyPoint( localDir ) );

	now_.useMoonColour(moonPos.y > sunPos.y);
}


/**
 *	This method loads the outside lighting animation definitions and
 *	other settings used by this class, from an XML section
 */
void TimeOfDay::load( DataSectionPtr root, bool ignoreTimeChanged /*= false*/ )
{
	BW_GUARD;
	sunAnimation_.clear();	
	fogAnimation_.clear();
	ambientAnimation_.clear();

	DataSectionPtr dayNightSection = root->openSection("day_night_cycle");

	if (dayNightSection)
	{
		if (!timeConfigChanged_ || ignoreTimeChanged)
		{
			startTime_ = dayNightSection->readFloat( "starttime", 12.f );
			secondsPerGameHour_ =
				dayNightSection->readFloat( "hourlength", 160.f );
		}
		sunAngle_ = dayNightSection->readFloat( "angle", 80.f );
		sunColor_ = dayNightSection->readVector3( "sunColor", Vector3( 255, 255, 255 ));
		moonAngle_ = dayNightSection->readFloat( "moonAngle", 80.f );
		moonColor_ = dayNightSection->readVector3( "moonColor", Vector3( 255, 255, 255 ));

		sunAndMoonNeedUpdate_ = true;

		if (!timeConfigChanged_)
		{
			time_ = startTime_;

			timeConfigChanged_ = true;
		}

		BW::vector< DataSectionPtr > lightKeys;

		dayNightSection->openSections( "lightkey", lightKeys );

		for( uint i = 0; i < lightKeys.size(); i++ )
		{
			float time;
			Vector3 colour;
			time = lightKeys[i]->readFloat( "time", HOURS_IN_DAY + 1 );
			colour = Vector3(lightKeys[i]->readVector3( "colour", Vector3( 0, 0, 0 ) ));

			sunAnimation_.addKey( time, colour );
		}		

		BW::vector< DataSectionPtr > fogKeys;
		dayNightSection->openSections( "fogkey", fogKeys );

		for( uint i = 0; i < fogKeys.size(); i++ )
		{
			float time;
			Vector3 colour;
			time = fogKeys[i]->readFloat( "time", HOURS_IN_DAY + 1 );
			colour = Vector3(fogKeys[i]->readVector3( "colour", Vector3( 0, 0, 0 ) ));

			fogAnimation_.addKey( time, colour );
		}

		BW::vector< DataSectionPtr > ambientKeys;
		dayNightSection->openSections( "ambientkey", ambientKeys );

		for( uint i = 0; i < ambientKeys.size(); i++ )
		{
			float time;
			Vector3 colour;
			time = ambientKeys[i]->readFloat( "time", HOURS_IN_DAY + 1 );
			colour = Vector3(ambientKeys[i]->readVector3( "colour", Vector3( 0, 0, 0 ) ));

			ambientAnimation_.addKey( time, colour );
		}
	}
}


/**
 *	This method gets the current time of day as a string
 */
const BW::string &TimeOfDay::getTimeOfDayAsString() const
{
	BW_GUARD;
	return gameTimeToString(gameTime());
}

/**
 *	This method sets the current time of day from a string
 */
void TimeOfDay::setTimeOfDayAsString( const BW::string &newTime )
{
	BW_GUARD;
	float gt = gameTimeFromString(newTime, gameTime());
	this->gameTime( gt ); // use the accessor so any notifiers get called	
}

/**
 *  This method allows any time to be formatted as a string.
 */
/*static*/ const BW::string& TimeOfDay::gameTimeToString( float gt )
{
	BW_GUARD;
	char timeAsCharArray[256];
	static BW::string timeAsString;

	uint32 hourTime = uint32( gt );
	uint32 secondsTime = uint32( ( gt - hourTime ) * 60.0f );
	bw_snprintf( timeAsCharArray, sizeof(timeAsCharArray), "%02d:%02d", hourTime, secondsTime );
	timeAsString = timeAsCharArray;

	return timeAsString;
}

/**
 *  This method allows any time to be read from a string.
 */
/*static*/ float 
TimeOfDay::gameTimeFromString
( 
    const BW::string&  newTime,
    float               defaultTime
)
{
	BW_GUARD;
	float timeAsFloat = defaultTime;
	int hours;
	int minutes;

	if (sscanf( newTime.c_str(), "%d:%d", &hours, &minutes ) != 2)
	{		// try it as a float
		BW::istringstream istr( newTime );
		istr.imbue( Locale::standardC() );
		istr >> timeAsFloat;
		if (istr.fail())
		{
			WARNING_MSG( "TimeOfDay::gameTimeFromString: Incorrect Time Of Day format: %s\n",
						newTime.c_str() );
		}
	}
	else
	{
		timeAsFloat = (hours % 24) + (minutes % 60) / 60.0f;
	}
    return fmodf( fabsf( timeAsFloat ),24.f );
}

#ifdef EDITOR_ENABLED

void TimeOfDay::save( void )
{
	BW_GUARD;
	save( file_ );
}

void TimeOfDay::save( const BW::string filename )
{
	BW_GUARD;
	BW::string fullFile = BWResource::resolveFilename( filename );

	file_ = filename;

    BWResource::ensurePathExists( BWResource::getFilePath( fullFile ) );

	DataResource file( fullFile, RESOURCE_TYPE_XML );

	DataSectionPtr spRoot = file.getRootSection( );

	if ( spRoot )
	{
		save( spRoot );
	}

	file.save( );
}

void TimeOfDay::save( DataSectionPtr root )
{
	BW_GUARD;
	root->deleteSection( "day_night_cycle" );
	DataSectionPtr dayNightSection = root->openSection( "day_night_cycle", true);

	if( dayNightSection )
	{
		dayNightSection->writeFloat( "angle", sunAngle_ );
		dayNightSection->writeFloat( "moonAngle", moonAngle_ );
		dayNightSection->writeFloat( "hourlength", secondsPerGameHour_ );
		dayNightSection->writeFloat( "starttime", startTime_ );

		dayNightSection->deleteSection( "lightKey" );

        LinearAnimation< Vector3 >::iterator it = sunAnimation_.begin();

		while( it != sunAnimation_.end() )
		{
			DataSectionPtr key = dayNightSection->newSection( "lightkey" );
			key->writeFloat( "time", (*it).first );
			key->writeVector3( "colour", (*it).second );

            it++;
		}        

        it = fogAnimation_.begin();

		while( it != fogAnimation_.end() )
		{
			DataSectionPtr key = dayNightSection->newSection( "fogkey" );
			key->writeFloat( "time", (*it).first );
			key->writeVector3( "colour", (*it).second );

            it++;
		}

        it = ambientAnimation_.begin();

		while( it != ambientAnimation_.end() )
		{
			DataSectionPtr key = dayNightSection->newSection( "ambientkey" );
			key->writeFloat( "time", (*it).first );
			key->writeVector3( "colour", (*it).second );

            it++;
		}
	}
}

void TimeOfDay::load( const BW::string filename )
{
	BW_GUARD;
	DataSectionPtr spRoot = BWResource::instance().openSection( filename );

	if ( spRoot )
	{
		load( spRoot );
	}
}



int	TimeOfDay::sunAnimations( void ) 
{ 
	BW_GUARD;
	return static_cast<int>(sunAnimation_.size( )); 
}

void TimeOfDay::getSunAnimation( int index, float& time, Vector3& colour ) 
{ 
	BW_GUARD;
	if ( index >= 0 && index < (int)(sunAnimation_.size( )) )
    {
		LinearAnimation< Vector3 >::iterator it = sunAnimation_.begin();

        while( index ) it++, index--;

		time  = (*it).first;
		colour = (*it).second;
    }
}

void TimeOfDay::setSunAnimation( int index, float time, const Vector3 colour )
{
	BW_GUARD;
	if ( index >= 0 && index < (int)(sunAnimation_.size( )) )
	{
		LinearAnimation< Vector3 >::iterator it = sunAnimation_.begin();

        while( index ) it++, index--;

		sunAnimation_.erase( it );
		sunAnimation_[time] = colour;
	}
}


void TimeOfDay::addSunAnimation( float time, const Vector3 colour )
{
	BW_GUARD;
	sunAnimation_[time] = colour;
}

void TimeOfDay::clearSunAnimations()
{
	BW_GUARD;
    sunAnimation_.clear();
}

int TimeOfDay::fogAnimations( void )
{
	BW_GUARD;
	return static_cast<int>(fogAnimation_.size( ));
}

void TimeOfDay::getFogAnimation( int index, float& time, Vector3& colour )
{
	BW_GUARD;
	if ( index >= 0 && index < (int)(fogAnimation_.size( )) )
    {
		LinearAnimation< Vector3 >::iterator it = fogAnimation_.begin();

        while( index ) it++, index--;

		time  = (*it).first;
		colour = (*it).second;
    }
}

void TimeOfDay::setFogAnimation( int index, float time, const Vector3 colour ) 
{ 
	BW_GUARD;
	if ( index >= 0 && index < (int)(fogAnimation_.size( )) )
	{
		LinearAnimation< Vector3 >::iterator it = fogAnimation_.begin();

        while( index ) it++, index--;

		fogAnimation_.erase( it );
		fogAnimation_[time] = colour;
	}
}

void TimeOfDay::addFogAnimation( float time, const Vector3 colour ) 
{ 
	BW_GUARD;
	fogAnimation_[time] = colour;
}

void TimeOfDay::clearFogAnimations()
{
	BW_GUARD;
    fogAnimation_.clear();
}

int TimeOfDay::ambientAnimations( void ) 
{ 
	BW_GUARD;
	return static_cast<int>(ambientAnimation_.size( )); 
}

void TimeOfDay::getAmbientAnimation( int index, float& time, Vector3& colour ) 
{ 
	BW_GUARD;
	if ( index >= 0 && index < (int)(ambientAnimation_.size( )) )
    {
		LinearAnimation< Vector3 >::iterator it = ambientAnimation_.begin();

        while( index ) it++, index--;

		time  = (*it).first;
		colour = (*it).second;
    }
}

void TimeOfDay::setAmbientAnimation( int index, float time, const Vector3 colour ) 
{ 
	BW_GUARD;
	if ( index >= 0 && index < (int)(ambientAnimation_.size( )) )
	{
		LinearAnimation< Vector3 >::iterator it = ambientAnimation_.begin();

        while( index ) it++, index--;

		ambientAnimation_.erase( it );
		ambientAnimation_[time] = colour;
	}
}

void TimeOfDay::addAmbientAnimation( float time, const Vector3 colour ) 
{ 
	BW_GUARD;
	ambientAnimation_[time] = colour;
}

void TimeOfDay::clearAmbientAnimations()
{
	BW_GUARD;
    ambientAnimation_.clear();
}

void TimeOfDay::clear( void )
{
	BW_GUARD;
	sunAnimation_.clear( );	
	fogAnimation_.clear( );
	ambientAnimation_.clear( );
}

#endif


/**
 *	Add an update notifier
 */
void TimeOfDay::addUpdateNotifier( SmartPointer<UpdateNotifier> pUN )
{
	BW_GUARD;
	updateNotifiers_.push_back( pUN );
}

/**
 *	Del an update notifier
 */
void TimeOfDay::delUpdateNotifier( SmartPointer<UpdateNotifier> pUN )
{
	BW_GUARD;
	BW::vector< SmartPointer<UpdateNotifier> >::iterator found =
		std::find( updateNotifiers_.begin(), updateNotifiers_.end(), pUN );
	if (found != updateNotifiers_.end())
		updateNotifiers_.erase( found );
}


/**
 *	Call all the update notifiers, if this is enabled
 */
void TimeOfDay::timeConfigChanged()
{
	BW_GUARD;
	timeConfigChanged_ = true;
	if (!updateNotifiersOn_) return;

	for (uint i = 0; i < updateNotifiers_.size(); i++)
	{
		updateNotifiers_[i]->updated( *this );
	}
}

BW_END_NAMESPACE

// time_of_day.cpp
