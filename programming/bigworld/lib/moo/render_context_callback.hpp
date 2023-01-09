#ifndef RENDER_CONTEXT_CALLBACK_HPP
#define RENDER_CONTEXT_CALLBACK_HPP


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	This class allows controlled release of resources allocated by derived
 *	classes.
 */
class RenderContextCallback
{
public:
	typedef BW::vector< RenderContextCallback * > Callbacks;

	/**
	 *	This method must be implemented in derived classes, and should release
	 *	all dynamic resources it has gathered.
	 */
	virtual void renderContextFini() = 0;


	RenderContextCallback();

	static void fini();

private:
	static bool s_finalised_;
	static Callbacks s_callbacks_;
};

} // namespace Moo

BW_END_NAMESPACE

#endif // RENDER_CONTEXT_CALLBACK_HPP
