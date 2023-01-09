#ifndef SCISSORS_SETTER_HPP
#define SCISSORS_SETTER_HPP

BW_BEGIN_NAMESPACE

namespace Moo
{
/**
 *	This class is a scoped scissors setter, allowing you to
 *	easily set a new scissors rectangle on the device.
 *
 *	The previous settings are restored automatically when the
 *	class goes out of scope.
 */
	class ScissorsSetter
	{
	public:
		ScissorsSetter();
		ScissorsSetter( uint32 x, uint32 y, uint32 width, uint32 height );
		~ScissorsSetter();

		static bool isAvailable();
	private:		
		RECT oldRect_;
		RECT newRect_;
	};
}	//namespace Moo

BW_END_NAMESPACE

#endif	//Scissors_SETTER_HPP
