#pragma once
#ifndef BW_NONCOPYABLE_HPP
#define BW_NONCOPYABLE_HPP

namespace BW
{
	/**
	 *	This provides a class to inherit from to make the child class
	 *	noncopyable. It's easier and clearer to maintainers than doing it
	 *	manually.
	 */
	class NonCopyable
	{
	protected:
		NonCopyable() {}
		~NonCopyable() {}
	private:
		NonCopyable( const NonCopyable& );
		const NonCopyable& operator=( const NonCopyable& );
	};
}

#endif // BW_NONCOPYABLE_HPP
