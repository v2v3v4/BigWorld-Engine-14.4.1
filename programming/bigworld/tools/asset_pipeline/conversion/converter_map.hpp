#ifndef ASSET_PIPELINE_CONVERTER_MAP
#define ASSET_PIPELINE_CONVERTER_MAP

#include "cstdmf/bw_map.hpp"

#include "converter_info.hpp"

BW_BEGIN_NAMESPACE

/// map of ids to converterInfos
typedef BW::map< uint64, ConverterInfo* > ConverterMap;

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_CONVERTER_CREATOR
