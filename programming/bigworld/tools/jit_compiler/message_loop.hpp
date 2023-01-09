#ifndef MESSAGE_LOOP_HPP
#define MESSAGE_LOOP_HPP

#include "cstdmf/concurrency.hpp"
#include "cstdmf/bw_namespace.hpp"

#include <queue>
#include <functional>

BW_BEGIN_NAMESPACE

class MainMessageLoop
{
public:
	typedef std::function<void ()> Action;

	MainMessageLoop();
	~MainMessageLoop();

	int run();

	void addAction(Action action);

private:

	void processActions();

	std::queue<Action> queue_;
	BW::SimpleMutex guard_;

};

BW_END_NAMESPACE

#endif // MESSAGE_LOOP_HPP
