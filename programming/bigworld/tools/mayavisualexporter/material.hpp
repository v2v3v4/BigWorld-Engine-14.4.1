#ifndef __material_hpp__
#define __material_hpp__

BW_BEGIN_NAMESPACE

struct material
{
	BW::string name;		// identifier
	BW::string mapFile;	// texture or mfm file
	BW::string fxFile;		// .fx file for shader
	
	int mapIdMeaning;	// 0 = none, 1 = bitmap, 2 = mfm
};

BW_END_NAMESPACE

#endif // __material_hpp__