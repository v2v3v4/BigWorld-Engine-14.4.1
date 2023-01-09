#include "cuetrack.hpp"

#include "expsets.hpp"

#include <stdio.h>

BW_BEGIN_NAMESPACE

CueTrack CueTrack::_instance;

void CueTrack::clear()
{
	_instance._cues.clear();
}

void CueTrack::addCue( TimeValue time, const char* name )
{
	// Check if the cue is among the frames being exported
	int cueFrame = time / GetTicksPerFrame();
	if (cueFrame < ExportSettings::instance().firstFrame() ||
		cueFrame > ExportSettings::instance().lastFrame())
	{
		return;
	}

	// Adjust the time based on the first frame
	time = time - (ExportSettings::instance().firstFrame() * GetTicksPerFrame());
	
	BW::string note( name );
	BW::string type, cue;

	size_t splitPosition = note.find( ':' );

	// string for the note track is expected to be of the form cue:name or sound:name
	if( splitPosition == BW::string::npos )
		return;

	type = note.substr( 0, splitPosition );

	if( type == "sound" )
		cue = BW::string( "s" ) + note.substr( splitPosition + 1, note.length() - splitPosition - 1 );
	else if( type == "cue" )
		cue = BW::string( "c" ) + note.substr( splitPosition + 1, note.length() - splitPosition - 1 );
	else
		return; // only cue: and sound: are supported at the moment

	BW::list<BW::string>::iterator it = _instance._cues[time].begin();

	for( ; it != _instance._cues[time].end(); ++it )
		if( (*it) == cue )
			return; // already have the cue for that time

	// add the cue
	_instance._cues[time].push_back( cue );
}

void CueTrack::writeFile( BinaryFile& file )
{
	int num = 0; // number of cues

	BW::map< TimeValue, BW::list<BW::string> >::iterator it = _instance._cues.begin();

	for( ; it != _instance._cues.end(); ++it )
	{
		BW::list<BW::string>::iterator jt = it->second.begin();

		for( ; jt != it->second.end(); ++jt )
			++num;
	}

	// if no cues, don't even bother writing the cue track
	if( num == 0 )
		return;

	// write channel identifier
	file << (int)6;

	// write number of cues
	file << num;

	// write cue data
	it = _instance._cues.begin();

	for( ; it != _instance._cues.end(); ++it )
	{
		BW::list<BW::string>::iterator jt = it->second.begin();

		for( ; jt != it->second.end(); ++jt )
		{
			file << (float)it->first / (float)GetTicksPerFrame();
			file << *jt;

			// no additional args
			file << (int)0;
		}
	}
}

bool CueTrack::hasCues()
{
	// check for a cue
	BW::map< TimeValue, BW::list<BW::string> >::iterator it = _instance._cues.begin();

	for( ; it != _instance._cues.end(); ++it )
	{
		BW::list<BW::string>::iterator jt = it->second.begin();

		for( ; jt != it->second.end(); ++jt )
			return true; // found one
	}

	return false;
}

BW_END_NAMESPACE

