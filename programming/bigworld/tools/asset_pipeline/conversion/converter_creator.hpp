#ifndef ASSET_PIPELINE_CONVERTER_CREATOR
#define ASSET_PIPELINE_CONVERTER_CREATOR

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

class Converter;
/// function pointer for creating an instance of a converter
typedef Converter * ( *ConverterCreator )( const BW::string& );

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_CONVERTER_CREATOR
