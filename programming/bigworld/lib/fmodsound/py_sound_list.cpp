#include "pch.hpp"
#include "py_sound_list.hpp"

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/guard.hpp"

#if FMOD_SUPPORT


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PySoundList
// -----------------------------------------------------------------------------

PySoundList::PySoundList() :
	BW::list< PySound * >(),
	stopOnDestroy_( true )
{
	BW_GUARD;	
}

/**
 *  If required, the destructor cleans up any sounds still remaining in the
 *  list.
 */
PySoundList::~PySoundList()
{
	BW_GUARD;
	if (stopOnDestroy_)
	{
		this->stopAll();
	}
	else
	{
		for (iterator it = this->begin(); it != this->end(); ++it)
		{
            (*it)->decRef();
		}
	}
}

bool PySoundList::removeSound( PySound *pPySound )
{    
    BW_GUARD;
	PySoundList::iterator itr;

	for(itr = this->begin(); itr != this->end(); itr++)
    {
	    // It's fine for an event to not be in the list ... that just means it
	    // was erased during an update(), probably because its Event* had been
	    // stolen by a newer Event.
        if (pPySound == *itr)
        {
            (*itr)->decRef();
		    this->erase( itr );
            return true;
        }
    }

    return false;
}


bool PySoundList::removeSound( SoundManager::Event *pEvent )
{    
    BW_GUARD;
	PySoundList::iterator itr;

    void *data;
    FMOD_RESULT result = pEvent->getUserData(&data);
    SoundManager::FMOD_ErrCheck(result, "PySoundList::removeSound: Couldn't retrieve user data");
    PySound *pySound = static_cast< PySound * >( data );

	for(itr = this->begin(); itr != this->end(); itr++)
    {
	    // It's fine for an event to not be in the list ... that just means it
	    // was erased during an update(), probably because its Event* had been
	    // stolen by a newer Event.
        if (pySound == *itr)
        {
		    this->erase( itr );
            result = pEvent->stop();
            SoundManager::FMOD_ErrCheck(result, "PySoundList::removeSound: Couldn't stop event");
            pySound->decRef();
            return true;
        }
    }

    return false;
}


/**
 *  Appends an Event to this list.
 */
void PySoundList::push_back( PySound *pySound )
{
	BW_GUARD;
    pySound->incRef();
	BW::list< PySound * >::push_back( pySound );
}


/**
 *  Update positions for any sounds that are still playing.
 */
bool PySoundList::update( const Vector3 &position, const Vector3 &orientation, float deltaTime )
{
	BW_GUARD;
	bool ok = true;

	iterator it = this->begin();
	while (it != this->end())
	{
		PySound *pySound = *it;
        if ( !pySound->unloaded() && pySound->update( position, orientation, deltaTime ))
		{
			++it;
		}
		else
		{
			// If we get to here, the event must have had it's channel stolen.
			it = this->erase( it );
            pySound->decRef();
			/*
			 technically we should notice FootTrigger::pSound_ if it pointers to this pySound,
			 but given the current structure there is no way that we can 
			 access the FootTrigger::pSound_ from here, so at this moment,
			 we might just leave it as it is, FootTrigger::pSound_ will check if
			 the sound is still valid, if not it will recreate another one
			*/

		}
	}

	return ok;
}

/**
 *  Stop and clear all sound events.
 */
bool PySoundList::stopAll()
{
	BW_GUARD;
	bool ok = true;

	for (iterator it = this->begin(); it != this->end(); ++it)
	{
		ok &= (*it)->stop();
        (*it)->decRef();
	}

	if (!ok)
	{
		ERROR_MSG( "PySoundList::stopAll: "
			"Some events failed to stop\n" );
	}

	this->clear();

	return ok;
}

BW_END_NAMESPACE

#endif // FMOD_SUPPORT

// pysoundlist.cpp
