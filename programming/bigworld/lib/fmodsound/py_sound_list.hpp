#ifndef PY_SOUND_LIST_HPP
#define PY_SOUND_LIST_HPP

#include "fmod_config.hpp"

#if FMOD_SUPPORT
#include "py_sound.hpp"
#include "sound_manager.hpp"


BW_BEGIN_NAMESPACE

class PySound;

class PySoundList : public BW::list< PySound * >
{
public:
    friend class PySound; // to access destructor


	PySoundList();
	~PySoundList();

#if 0
    bool createPySound();
#endif


	void push_back( PySound *pPySound );
	bool update( const Vector3 &pos, const Vector3 &orientation = Vector3::zero(), float deltaTime = 0.f);
    bool removeSound( PySound *pPySound);
    bool removeSound( SoundManager::Event *pEvent );
	bool stopAll();
	void stopOnDestroy( bool enable ) { stopOnDestroy_ = enable; }

protected:
	bool stopOnDestroy_;
};

BW_END_NAMESPACE

#endif // FMOD_SUPPORT

#endif // PY_SOUND_LIST_HPP
