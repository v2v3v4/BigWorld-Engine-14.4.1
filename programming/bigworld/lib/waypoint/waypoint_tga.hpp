#ifndef _WAYPOINT_TGA_HEADER
#define _WAYPOINT_TGA_HEADER

#ifdef _WIN32
#define PACKED
#pragma pack(1)
#else
#define PACKED __attribute__((packed))
#endif

BW_BEGIN_NAMESPACE

struct WaypointTGAHeader
{
	unsigned char   extraLength 		PACKED;
	unsigned char   colourMapType		PACKED;
	unsigned char   imageType			PACKED;
	unsigned short  colourMapStart		PACKED;
	unsigned short  colourMapLength		PACKED;
	unsigned char   colourMapDepth		PACKED;
	unsigned short  x					PACKED;
	unsigned short  y					PACKED;
	unsigned short  width				PACKED;
	unsigned short  height				PACKED;
	unsigned char   bpp					PACKED;
	unsigned char   imageDescriptor		PACKED;

	// These fields stored in the 'extra data' part of the header.

	Vector3			gridMin				PACKED;
	float			gridResolution		PACKED;
};

BW_END_NAMESPACE

#endif

