#ifndef LINK_PROPERTY_HPP
#define LINK_PROPERTY_HPP


#include "gizmo/general_properties.hpp"
#include "worldeditor/editor/link_proxy.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This property is used to link items.
 */
class LinkProperty : public GeneralProperty
{
public:
	LinkProperty
    (
        const Name&         name, 
        LinkProxyPtr        linkProxy,
        MatrixProxyPtr      matrix,   
		bool				alwaysShow = true
    );

	virtual const ValueType & valueType() const { RETURN_VALUETYPE( STRING ); }

	/*virtual*/ PyObject * pyGet();
	/*virtual*/ int pySet(PyObject * value, bool transient = false);

    MatrixProxyPtr matrix() const;
    LinkProxyPtr link() const;

	bool alwaysShow() const;

private:
    LinkProxyPtr            linkProxy_;
    MatrixProxyPtr          matrix_;
	bool					alwaysShow_;

private:
	GENPROPERTY_VIEW_FACTORY_DECLARE(LinkProperty)
};

BW_END_NAMESPACE

#endif // LINK_PROPERTY_HPP
