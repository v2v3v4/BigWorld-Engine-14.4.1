#include "signal.hpp"

BW_BEGIN_NAMESPACE

Connection::Connection()
{
}

Connection::Connection(SignalHolderPtr entry) :
	entry_(entry)
{
}

Connection::Connection(Connection && other) : 
	entry_(std::move(other.entry_))
{
}

Connection::~Connection()
{
	entry_.reset();
}

Connection & Connection::operator=(Connection && other)
{
	entry_ = std::move(other.entry_);
	return *this;
}

void Connection::enable()
{
	if (entry_)
	{
		SimpleMutexHolder lock(entry_->guard_);
		entry_->enabled_ = true;
	}
}

void Connection::disable()
{
	if (entry_)
	{
		SimpleMutexHolder lock(entry_->guard_);
		entry_->enabled_ = false;
	}
}

void Connection::disconnect()
{
	entry_.reset();
}

bool Connection::enabled() const
{
	return entry_ && entry_->enabled_;
}

bool Connection::connected() const
{
	return static_cast<bool>(entry_);
}

BW_END_NAMESPACE
