#ifndef USER_LOG_BUFFER_MONGODB_HPP
#define USER_LOG_BUFFER_MONGODB_HPP

#include "types.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

namespace MongoDB
{

/**
 *  This class represents the information required to create a user log
 *  collection and flush logs to it - username and a log buffer.
 */
class UserLogBuffer : public SafeReferenceCount
{
public:
	UserLogBuffer( const BW::string & username ) :
		logBuffer_(),
		username_( username )
	{
		logBuffer_.reserve( RESERVE_LOG_BUFFER_SIZE );
	};

	const BW::string & getUsername() { return username_; }

	void append( mongo::BSONObj logBson )
	{
		logBuffer_.push_back( logBson );
	}

	void clear()
	{
		logBuffer_.clear();
	}

	BSONObjBuffer::size_type size()
	{
		return logBuffer_.size();
	}

	// Must be publicly visible as non-const so that it can be swapped into a
	// flush task.
	BSONObjBuffer logBuffer_;

private:
	BW::string username_;
};

typedef SmartPointer< MongoDB::UserLogBuffer > UserLogBufferPtr;

} // namespace MongoDB

BW_END_NAMESPACE

#endif // USER_LOG_BUFFER_MONGODB_HPP
