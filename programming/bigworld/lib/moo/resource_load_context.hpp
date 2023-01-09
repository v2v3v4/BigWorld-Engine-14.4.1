#ifndef RESOURCE_LOAD_CONTEXT_HPP
#define RESOURCE_LOAD_CONTEXT_HPP

BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	This class keeps track of the resource currently being loaded, useful for
 *	when reporting errors for example.
 */
class ResourceLoadContext
{
public:
	static void push( const BW::StringRef & requester );
	static void pop();

	static BW::string formattedRequesterString();
};


/**
 *	This class allows for easy scoped setup of a resource load context.
 */
class ScopedResourceLoadContext
{
public:
	ScopedResourceLoadContext( const BW::StringRef & requester )
	{
		ResourceLoadContext::push( requester );
	}
	~ScopedResourceLoadContext()
	{
		ResourceLoadContext::pop();
	}
};


} // namespace Moo

BW_END_NAMESPACE

#endif // RESOURCE_LOAD_CONTEXT_HPP
