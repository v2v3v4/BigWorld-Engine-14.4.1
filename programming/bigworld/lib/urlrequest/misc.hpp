#ifndef URL_MISC_HPP
#define URL_MISC_HPP

#include "cstdmf/bw_namespace.hpp"

#include "cstdmf/bw_list.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

namespace URL
{

enum Method
{
	METHOD_GET,
	METHOD_PUT,
	METHOD_POST,
	METHOD_DELETE,
	METHOD_PATCH
};

typedef BW::list< BW::string > Headers;

enum PostDataEncodingType
{
	ENCODING_RAW,
	ENCODING_MULTIPART_FORM_DATA
};

typedef BW::map< BW::string, BW::string > FormData;

struct PostData
{
	PostDataEncodingType encodingType;
	BW::string rawData;
	FormData formData;
};

}

BW_END_NAMESPACE

#endif // URL_MISC_HPP
