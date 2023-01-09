#ifndef CUETRACK_HPP
#define CUETRACK_HPP

#include <Max.h>
#include "cstdmf/bw_list.hpp"
#include "cstdmf/bw_map.hpp"
#include <string>

#include "cstdmf/binaryfile.hpp"

BW_BEGIN_NAMESPACE

class CueTrack
{
public:
	static void clear();
	static void addCue( TimeValue time, const char* name );
	static void writeFile( BinaryFile& file );

	static bool hasCues();

protected:
	static CueTrack _instance;

	// maps times the cues occur at to their identifiers
	BW::map< TimeValue, BW::list<BW::string> > _cues;
};

BW_END_NAMESPACE

#endif // CUETRACK_HPP
