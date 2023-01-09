#ifndef LOGGING_TYPES_MONGODB_HPP
#define LOGGING_TYPES_MONGODB_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_list.hpp"
#include "cstdmf/stdmf.hpp"

#include "mongo/client/dbclient.h"

#include <list>
#include <string>
#include <vector>


BW_BEGIN_NAMESPACE

namespace MongoDB
{
/**
 * This type represents the MongoDB collection name list
 */
typedef BW::list< BW::string > CollList;


/**
 * This type represents the string type used by MongoDB
 */
typedef std::string string;


/**
 *  This type represents a batch of BSON Objects which can be written straight
 *  to the database.
 */
typedef std::vector< mongo::BSONObj > BSONObjBuffer;


} // namespace MongoDB

BW_END_NAMESPACE

#endif // LOGGING_TYPES_MONGODB_HPP
