#include "message_loop.hpp"

BW_BEGIN_NAMESPACE

MainMessageLoop::MainMessageLoop()
{
}

MainMessageLoop::~MainMessageLoop()
{
}

void MainMessageLoop::addAction(Action action)
{
	BW::SimpleMutexHolder lock(guard_);
	queue_.push(action);
}

int MainMessageLoop::run()
{
	MSG msg;

	for (;;)
	{
		while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) != FALSE)
		{
			if (::GetMessage(&msg, NULL, 0, 0) == 0)
			{
				return static_cast<int>(msg.wParam);
			}
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

		processActions();

		::Sleep(10);
	}

	return -1;
}

void MainMessageLoop::processActions()
{
	BW::SimpleMutexHolder lock(guard_);

	while (!queue_.empty())
	{
		auto& action = queue_.front();
		action();
		queue_.pop();
	}
}

BW_END_NAMESPACE
