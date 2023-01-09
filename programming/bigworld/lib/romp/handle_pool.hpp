#ifndef HANDLE_POOL_HPP
#define HANDLE_POOL_HPP

#include <stack>


BW_BEGIN_NAMESPACE

class HandlePool
{
public:
	typedef uint16 Handle;
	static const Handle INVALID_HANDLE = 0xffff;

	HandlePool( uint16 numHandles );
	Handle handleFromId( size_t id );
	void beginFrame();
	void endFrame();
	void reset();
	uint16 numHandles() const { return numHandles_; }

	struct Info
	{
		Handle handle_;
		bool used_;
	};

	typedef BW::map<size_t,Info>	HandleMap;
	HandleMap::iterator begin() { return handleMap_.begin(); }
	HandleMap::iterator end()	{ return handleMap_.end(); }

private:
	HandleMap						handleMap_;
	std::stack<Handle>				unusedHandles_;
	uint16		numHandles_;
};

BW_END_NAMESPACE

#endif // HANDLE_POOL_HPP
