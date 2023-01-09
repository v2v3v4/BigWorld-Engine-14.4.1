#pragma once
#ifndef MOO_TEST_WINDOW_HPP
#define MOO_TEST_WINDOW_HPP

#include "cstdmf/cstdmf_windows.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
	/**
	 *	This class will setup a Moo window for testing. This is intended to
	 *	be used during unit tests that rely on having a Moo device
	 *	initialised and require a simple way of doing it.
	 */
	class TestWindow
	{
	public:
		TestWindow();
		~TestWindow();
		bool init();
		void destroy();
		HWND hwnd() const;

	private:
		///Handle of the window created.
		HWND hwnd_;
		///Should only ever be one instance initialised at a time
		///for this class.
		static bool isAnInstanceCurrentlyInitialised_;
	};
} // namespace Moo

BW_END_NAMESPACE

#endif // MOO_TEST_WINDOW_HPP
