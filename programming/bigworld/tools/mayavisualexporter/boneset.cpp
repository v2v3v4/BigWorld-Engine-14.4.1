#include "pch.hpp"

#include "boneset.hpp"

BW_BEGIN_NAMESPACE

void BoneSet::addBone( BW::string bone )
{
	if( indexes.find( bone ) != indexes.end() )
		return; // already added
	
	bones.push_back( bone );
	indexes[bone] = static_cast<uint32>(bones.size() - 1);
}

BW_END_NAMESPACE

