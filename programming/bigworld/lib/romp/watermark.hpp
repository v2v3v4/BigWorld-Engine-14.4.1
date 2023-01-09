#ifndef WATERMARK_HPP
#define WATERMARK_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

BW::string decodeWaterMark (const BW::string& encodedData, int width, int height);

BW::string encodeWaterMark(const char* data, size_t size);

BW_END_NAMESPACE

#endif // WATERMARK_HPP


