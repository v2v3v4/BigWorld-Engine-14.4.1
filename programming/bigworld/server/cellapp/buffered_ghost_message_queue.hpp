#ifndef BUFFERED_GHOST_MESSAGE_QUEUE_HPP
#define BUFFERED_GHOST_MESSAGE_QUEUE_HPP

#include "cstdmf/smartpointer.hpp"
#include "network/basictypes.hpp"

#include "cstdmf/bw_list.hpp"


BW_BEGIN_NAMESPACE

class BufferedGhostMessage;

/**
 *  This class represents a stream of buffered real->ghost messages from a
 *  single CellApp destined for a single entity. It owns the
 *	BufferedGhostMessages that it contains.
 *
 *	BufferedGhostMessagesForEntity contains a collection of these mapped by
 *	source address.
 */
class BufferedGhostMessageQueue: public ReferenceCount
{
public:
	BufferedGhostMessageQueue();
	~BufferedGhostMessageQueue();

	void add( BufferedGhostMessage * pMessage );

	bool isFrontCreateGhost() const;
	bool isEmpty() const				{ return messages_.empty(); }
	bool isDelaying() const;

	bool playSubsequence();

	void delaySubsequence( BufferedGhostMessage * pFirstMessage );
	void promoteDelayedSubsequences();

private:
	bool playFront();
	BufferedGhostMessage * popFront();

	bool isFrontSubsequenceStart() const;
	bool isFrontSubsequenceEnd() const;

	typedef BW::list< BufferedGhostMessage* > Messages;
	typedef Messages::iterator iterator;

	Messages messages_;
	Messages * pDelayedMessages_;
};

typedef SmartPointer<BufferedGhostMessageQueue> BufferedGhostMessageQueuePtr;

BW_END_NAMESPACE

#endif // BUFFERED_GHOST_MESSAGE_QUEUE_HPP
