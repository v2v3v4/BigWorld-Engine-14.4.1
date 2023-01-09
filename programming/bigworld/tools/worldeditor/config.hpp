#ifndef WE_CONFIG_HPP
#define WE_CONFIG_HPP


/**
 *	This file is #included by all files within the WorldEditor project as the
 *	first file to include.  It allows WorldEditor specific compile-time options 
 *	to be turned on or off.
 */

#define MIN_MAP_SIZE 16
#define MIN_HOLE_MAP_SIZE 3
#define MAX_MAP_SIZE 1024
#define MAX_NO_RESPONDING_TIME 200

/**
 *	This turns off version control support in WorldEditor.
 */
//#define BIGWORLD_CLIENT_ONLY


#endif // WE_CONFIG_HPP
