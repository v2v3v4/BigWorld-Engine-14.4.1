#ifndef __boneset_hpp__
#define __boneset_hpp__

BW_BEGIN_NAMESPACE

class BoneSet
{
public:
	void addBone( BW::string bone );

	BW::map<BW::string, uint32> indexes;
	BW::vector<BW::string> bones;
};

BW_END_NAMESPACE

#endif // __boneset_hpp__
