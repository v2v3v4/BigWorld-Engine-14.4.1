#ifndef FMOD_CONFIG_HPP
#define FMOD_CONFIG_HPP

// Set BW_FMOD_SUPPORT to ON in CMake to enable FMOD_SUPPORT.
// Please see http://www.fmod.org for tools downloads and licensing details.
#define ENABLE_JFIQ_HANDLING 1

#if FMOD_VERSION > 0x00042000
# define FMOD_SUPPORT_CUES       1
# define FMOD_SUPPORT_MEMORYINFO 1
#else
# define FMOD_SUPPORT_CUES       0
# define FMOD_SUPPORT_MEMORYINFO 0
#endif

#endif // FMOD_CONFIG_HPP
