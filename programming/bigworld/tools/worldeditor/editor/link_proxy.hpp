#ifndef LINK_PROXY_HPP
#define LINK_PROXY_HPP


#include "gizmo/general_properties.hpp"
#include "gizmo/tool_locator.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This is base class for objects that can link together or create new 
 *  linkable objects.
 */
class LinkProxy : public ReferenceCount
{
public:
    /**
     *  This bitfield determines the allowed link types.
     */
    enum LinkType
    {
        LT_ADD  = 1,    // Add a new node for linking
        LT_LINK = 2     // Link nodes, entities etc
    };

    enum TargetState
    {
        TS_NO_TARGET = 0,   // No linkable object under the cursor
        TS_CAN_LINK  = 1,   // There's an object, and can link to it
        TS_CANT_LINK = 2    // There's an object, but can't link to it
    };

    LinkProxy();
    virtual ~LinkProxy();

    /**
     *  What type of linking is supported?
     *
     *  @returns        The supported linking operations.
     */
    virtual LinkType linkType() const = 0;

    /**
     *  Create a copy of the object that the LinkProxy is working on,
     *  link this copy to the original and return a MatrixProxyPtr that can
     *  set the position/orientation etc of the copy.
     *
     *  @returns        A proxy to set the position/orientation of the linked
     *                  item.
     */
    virtual MatrixProxyPtr createCopyForLink() = 0;

    /**
     *  Return true if a link can be created from the object that the LinkProxy
     *  represents to something at the tool locator.
     *
     *  @param locator  The tool locator used to select something.
     *  @returns        True if a link is possible.
     */
    virtual TargetState canLinkAtPos(ToolLocatorPtr locator) const = 0;

    /**
     *  Create a link from something at pos to the object the proxy represents.
     *
     *  @param locator  The tool locator used to select something.
     */
    virtual void createLinkAtPos(ToolLocatorPtr locator) = 0;

    /**
     *  Create the locator appropriate for the linker.
     */
    virtual ToolLocatorPtr createLocator() const = 0;
    
	BW::wstring& linkValue() {
		return linkValue_;
	}
	protected:
		BW::wstring linkValue_;
};


typedef SmartPointer<LinkProxy> LinkProxyPtr;

BW_END_NAMESPACE

#endif // LINK_PROXY_HPP
