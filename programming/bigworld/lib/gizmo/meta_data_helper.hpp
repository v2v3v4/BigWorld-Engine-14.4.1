#ifndef META_DATA_HELPER_HPP
#define META_DATA_HELPER_HPP


#include "meta_data.hpp"

BW_BEGIN_NAMESPACE

namespace MetaData
{

void updateCreationInfo( MetaData& metaData,
	time_t time = Environment::instance().time(),
	const BW::string& username = Environment::instance().username() );

void updateModificationInfo( MetaData& metaData,
	time_t time = Environment::instance().time(),
	const BW::string& username = Environment::instance().username() );

}

BW_END_NAMESPACE
#endif//META_DATA_HELPER_HPP
