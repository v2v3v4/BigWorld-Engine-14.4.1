#ifndef BUFFERED_GHOST_MESSAGE_HPP
#define BUFFERED_GHOST_MESSAGE_HPP

#include "cstdmf/bw_namespace.hpp"


BW_BEGIN_NAMESPACE

class BufferedGhostMessage
{
public:
	BufferedGhostMessage( bool isSubsequenceStart,
			bool isSubsequenceEnd ) :
		isSubsequenceStart_( isSubsequenceStart ),
		isSubsequenceEnd_( isSubsequenceEnd )
	{
	}

	virtual ~BufferedGhostMessage() {};

	virtual void play() = 0;
	virtual bool isCreateGhostMessage() const { return false; }

	bool isSubsequenceStart() const { return isSubsequenceStart_; }
	bool isSubsequenceEnd() const { return isSubsequenceEnd_; }

private:
	bool isSubsequenceStart_;
	bool isSubsequenceEnd_;
};

BW_END_NAMESPACE

#endif // BUFFERED_GHOST_MESSAGE_HPP
