#ifndef CELL_APP_CHANNEL_HPP
#define CELL_APP_CHANNEL_HPP

#include "network/channel_owner.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used to represent a connection between two cell applications.
 */
class CellAppChannel : public Mercury::ChannelOwner
{
public:
	int mark() const				{ return mark_; }
	void mark( int v )				{ mark_ = v; }

	GameTime lastReceivedTime() const { return lastReceivedTime_; }
	void lastReceivedTime( GameTime time ) { lastReceivedTime_ = time; }

	bool isGood() const 			{ return !this->channel().hasRemoteFailed(); }

private:
	explicit CellAppChannel( const Mercury::Address & addr );

	int		mark_;
	GameTime lastReceivedTime_;

	friend class CellAppChannels;
};

BW_END_NAMESPACE

#endif // CELL_APP_CHANNEL_HPP
