#ifndef INPUT_MANAGER_HPP
#define INPUT_MANAGER_HPP

#include <iostream>

#include "input/input.hpp"

BW_BEGIN_NAMESPACE

/**
 * This class provides a high-level input routing service
 */
class InputManager : public InputHandler
{
public:
	InputManager();
	~InputManager();

	//input handler methods
	bool handleKeyEvent( const KeyEvent & /*event*/ );
	bool handleMouseEvent( const MouseEvent & /*event*/ );

private:
	InputManager(const InputManager&);
	InputManager& operator=(const InputManager&);

	friend std::ostream& operator<<(std::ostream&, const InputManager&);
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "input_manager.ipp"
#endif

#endif
/*input_manager.hpp*/
