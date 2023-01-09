
/*
        These classes make it easier to get info.
        
        The wavebank information involves a dynamic array which could otherwise cause
        memory leaks.
*/



class EventSystemInfo
{
private:    
    FMOD_EVENT_SYSTEMINFO    sysInfo_;
    FMOD_EVENT_WAVEBANKINFO *wavebankInfo_;
    FMOD::EventSystem       *eventSystem_;

    EventSystemInfo(); //hidden
public:
    EventSystemInfo(FMOD::EventSystem *eventSystem) :   
      wavebankInfo_( NULL ),
      eventSystem_( eventSystem )

    {
        ZeroMemory( &sysInfo_, sizeof( sysInfo_ ) );
        FMOD_RESULT result = eventSystem_->getInfo( &sysInfo_ );
        SoundManager::FMOD_ErrCheck(result, "EventSystemInfo::EventSystemInfo()");
        
        if (sysInfo_.maxwavebanks)
        {
            int maxWaveBanks = sysInfo_.maxwavebanks;
            wavebankInfo_ = new FMOD_EVENT_WAVEBANKINFO[maxWaveBanks];    
            ZeroMemory( &sysInfo_, sizeof( sysInfo_ ) );
            sysInfo_.wavebankinfo = wavebankInfo_;
            sysInfo_.maxwavebanks = maxWaveBanks;
            result = eventSystem_->getInfo( &sysInfo_ );
            SoundManager::FMOD_ErrCheck(result, "EventSystemInfo::EventSystemInfo()");
        }
    }

    const int numWavebanks() const
    {
        return sysInfo_.maxwavebanks;
    }

    const FMOD_EVENT_WAVEBANKINFO wavebankInfo(int index) const
    {
        MF_ASSERT(index >= 0 && index < sysInfo_.maxwavebanks)
        return wavebankInfo_[index];
    }

    const FMOD_EVENT_SYSTEMINFO &info() const
    {
        return sysInfo_;
    }    

    ~EventSystemInfo()
    {
        // Clean up dynamic array       
        if (wavebankInfo_)
            delete [] wavebankInfo_;
    }
};


class EventInfo
{
private:    
    FMOD_EVENT_INFO          eventInfo_;
    FMOD_EVENT_WAVEBANKINFO *wavebankInfo_;
    FMOD::Event             *event_;

    EventInfo(); //hidden
public:
    EventInfo(FMOD::Event *event) :   
      wavebankInfo_( NULL ),
      event_( event )
    {
        ZeroMemory( &eventInfo_, sizeof( eventInfo_ ) );
        FMOD_RESULT result = event_->getInfo( NULL, NULL, &eventInfo_ );
        SoundManager::FMOD_ErrCheck(result, "EventInfo::EventInfo()");
        
        if (eventInfo_.maxwavebanks)
        {
            int maxWaveBanks = eventInfo_.maxwavebanks;
            wavebankInfo_ = new FMOD_EVENT_WAVEBANKINFO[maxWaveBanks];    
            ZeroMemory( &eventInfo_, sizeof( eventInfo_ ) );
            eventInfo_.wavebankinfo = wavebankInfo_;
            eventInfo_.maxwavebanks = maxWaveBanks;
            result = event_->getInfo( NULL, NULL, &eventInfo_ );
            SoundManager::FMOD_ErrCheck(result, "EventInfo::EventInfo()");
        }
    }

    const int numWavebanks() const
    {
        return eventInfo_.maxwavebanks;
    }

    const FMOD_EVENT_WAVEBANKINFO wavebankInfo(int index) const
    {
        MF_ASSERT(index >= 0 && index < eventInfo_.maxwavebanks)
        return wavebankInfo_[index];
    }

    const FMOD_EVENT_INFO &info() const
    {
        return eventInfo_;
    }    

    ~EventInfo()
    {
        // Clean up dynamic array       
        if (wavebankInfo_)
            delete [] wavebankInfo_;
    }
};
