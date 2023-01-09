#ifndef CONTROLS_DEFS_HPP
#define CONTROLS_DEFS_HPP

//
// Common defines for the control library.
//

// DLL defines:
#ifdef CONTROLS_IMPORT
#define CONTROLS_DLL __declspec(dllimport)
#endif
#ifdef CONTROLS_EXPORT
#define CONTROLS_DLL __declspec(dllexport)
#endif
#ifndef CONTROLS_DLL
#define CONTROLS_DLL
#endif

#endif // CONTROLS_DEFS_HPP
