#include "pch.hpp"
#include "worldeditor/import/import_codec.hpp"

BW_BEGIN_NAMESPACE

/*virtual*/ ImportCodec::~ImportCodec()
{
}


/*virtual*/ bool ImportCodec::canLoad() const
{
    return false;
}


/*virtual*/ bool ImportCodec::canSave() const
{
    return false;
}

BW_END_NAMESPACE

