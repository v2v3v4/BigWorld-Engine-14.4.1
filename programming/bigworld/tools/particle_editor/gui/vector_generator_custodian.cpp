#include "pch.hpp"
#include "resource.h"
#include "gui/vector_generator_custodian.hpp"
#include "resmgr/string_provider.hpp"

BW_BEGIN_NAMESPACE


BW::string VecGenGUIStrFromID(BW::string const &str)
{
	BW_GUARD;

    if (str == PointVectorGenerator::nameID_.c_str())
        return LocaliseUTF8(L"`RCS_IDS_POINT");
    else if (str == LineVectorGenerator::nameID_.c_str())
        return LocaliseUTF8(L"`RCS_IDS_LINE");
    else if (str == CylinderVectorGenerator::nameID_.c_str())
        return LocaliseUTF8(L"`RCS_IDS_CYLINDER");
    else if (str == SphereVectorGenerator::nameID_.c_str())
        return LocaliseUTF8(L"`RCS_IDS_SPHERE");
    else if (str == BoxVectorGenerator::nameID_.c_str())
        return LocaliseUTF8(L"`RCS_IDS_BOX");
    else
        return BW::string();
}

BW::string VecGenIDFromGuiStr(BW::string const &str)
{
	BW_GUARD;

    if (str == LocaliseUTF8(L"`RCS_IDS_POINT"))
        return PointVectorGenerator::nameID_.c_str();
    else if (str == LocaliseUTF8(L"`RCS_IDS_LINE"))
        return LineVectorGenerator::nameID_.c_str();
    else if (str == LocaliseUTF8(L"`RCS_IDS_CYLINDER"))
        return CylinderVectorGenerator::nameID_.c_str();
    else if (str == LocaliseUTF8(L"`RCS_IDS_SPHERE"))
        return SphereVectorGenerator::nameID_.c_str();
    else if (str == LocaliseUTF8(L"`RCS_IDS_BOX"))
        return BoxVectorGenerator::nameID_.c_str();
    else
        return BW::string();
}

BW_END_NAMESPACE

