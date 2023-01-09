#include "pch.hpp"
#include "py_event_category.hpp"

#include "cstdmf/guard.hpp"
#include "cstdmf/bw_namespace.hpp"

#if FMOD_SUPPORT
#include <fmod_errors.h>

DECLARE_DEBUG_COMPONENT2( "PyEventCategory", 0 );


BW_BEGIN_NAMESPACE

/*~ class FMOD.EventCategory
 *
 *	An EventCategory is a wrapper for an FMOD::EventCategory, and is a
 *  virtual folder used to organize event data into generic categories
 *  such as music. Allows volume and pitch changes to be applied to
 *  all events within the group.
 *
 *	For more information about event categories and how they are used, please 
 *	see the FMOD Designer API documentation and the FMOD Designer User Manual, 
 *	both available from www.fmod.org.
 */
PY_TYPEOBJECT( PyEventCategory );

PY_BEGIN_METHODS( PyEventCategory )

	/*~ function EventCategory.stopAllEvents
	 *
	 *  Stops all events in this category and subcategories.  
	 */
    PY_METHOD( stopAllEvents )

PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyEventCategory )

    /*~ attribute EventCategory.muted
	 *
	 *  Determines whether or not events in this category are currently muted.
	 *
	 *  @type	Read-write boolean
	 */
	PY_ATTRIBUTE( muted )

	/*~ attribute EventCategory.muted
	 *
	 *  Determines whether or not events in this category are currently paused.
	 *
	 *  @type	Read-write boolean
	 */
    PY_ATTRIBUTE( paused )

	/*~ attribute EventCategory.volume
	 *
	 *  The overall volume for this event category (0.0 is silent, 1.0 is full 
	 *	volume).
	 *
	 *  @type	Read-write float
	 */
    PY_ATTRIBUTE( volume )

	/*~ attribute EventCategory.pitch
	 *
	 *  The pitch modification for this event category (0.0 is normal pitch).
	 *
	 *  @type	Read-write float
	 */
    PY_ATTRIBUTE( pitch )

PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( PyEventCategory, "EventCategory", _FMOD )


PyEventCategory::PyEventCategory( SoundManager::EventCategory * pEventCategory, PyTypeObject * pType ) :
	PyObjectPlus( pType ),
    eventCategory_( pEventCategory ),
	muted_( false ),
	paused_( false ),
	volume_( 0.0 ),
	pitch_( 0.0 )
{
	BW_GUARD;	

	if (eventCategory_)
	{
		FMOD_RESULT result = eventCategory_->setUserData( this );
		SoundManager::FMOD_ErrCheck( result, "PyEventCategory::PyEventCategory" );
	}
}


PyEventCategory::~PyEventCategory()
{
}

void PyEventCategory::fini()
{
	eventCategory_ = NULL;
}

void PyEventCategory::stopAllEvents()
{
    BW_GUARD;
	if (eventCategory_)
	{
		FMOD_RESULT result = eventCategory_->stopAllEvents();
		SoundManager::FMOD_ErrCheck( result, "PyEventCategory::stopAllEvents" );
	}
}


bool PyEventCategory::muted() const
{
    BW_GUARD;
    if (eventCategory_)
	{
		FMOD_RESULT result = eventCategory_->getMute( &muted_ );
		SoundManager::FMOD_ErrCheck( result, "PyEventCategory::muted" );
	}

	return muted_;
}

void PyEventCategory::muted( bool newMute )
{
    BW_GUARD;
	muted_ = newMute;
	if (eventCategory_)
	{
		FMOD_RESULT result = eventCategory_->setMute( muted_ );
		SoundManager::FMOD_ErrCheck( result, "PyEventCategory::muted" );
	}
}

bool PyEventCategory::paused() const
{
    BW_GUARD;
    if (eventCategory_)
	{
		FMOD_RESULT result = eventCategory_->getPaused( &paused_ );
		SoundManager::FMOD_ErrCheck( result, "PyEventCategory::paused" );
	}

    return paused_;
}

void PyEventCategory::paused( bool newPaused )
{
    BW_GUARD;
	paused_ = newPaused;
	if (eventCategory_)
	{
		FMOD_RESULT result = eventCategory_->setPaused( paused_ );
		SoundManager::FMOD_ErrCheck( result, "PyEventCategory::paused" );
	}	
}

float PyEventCategory::volume() const
{
    BW_GUARD;
    if (eventCategory_)
	{
		FMOD_RESULT result = eventCategory_->getVolume( &volume_ );
		SoundManager::FMOD_ErrCheck( result, "PyEventCategory::volume" );
	}

    return volume_;
}

void PyEventCategory::volume( float newValue )
{
    BW_GUARD;
	volume_ = newValue;
	if (eventCategory_)
	{
		FMOD_RESULT result = eventCategory_->setVolume( volume_ );
		SoundManager::FMOD_ErrCheck( result, "PyEventCategory::volume" );
	}
}

float PyEventCategory::pitch() const
{
    BW_GUARD;
	if (eventCategory_)
	{
		FMOD_RESULT result = eventCategory_->getPitch( &pitch_ );
		SoundManager::FMOD_ErrCheck( result, "PyEventCategory::pitch" );
	}

    return pitch_;
}

void PyEventCategory::pitch( float newPitch )
{
    BW_GUARD;
	pitch_ = newPitch;
	if (eventCategory_)
	{
		FMOD_RESULT result = eventCategory_->setPitch( pitch_ );
		SoundManager::FMOD_ErrCheck(result, "PyEventCategory::pitch");
	}
}

PyObject *PyEventCategory::pyNew( PyObject *args )
{
	BW_GUARD;

	char* categoryPath;
	if (!PyArg_ParseTuple( args, "s", &categoryPath ))
	{
		PyErr_SetString( PyExc_TypeError, "FMOD.EventCategory() "
			"expects a string category path argument" );
		return NULL;
	}

	return SoundManager::pInstance()->pyEventCategory(categoryPath);
}

#endif // FMOD_SUPPORT

BW_END_NAMESPACE

// pyeventcategory.cpp
