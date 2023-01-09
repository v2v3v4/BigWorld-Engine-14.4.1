#include "pch.hpp"
#include "link_property.hpp"

BW_BEGIN_NAMESPACE

GENPROPERTY_VIEW_FACTORY(LinkProperty)


/**
 *  LinkProperty constructor.
 *
 *  @param name         The name of the property.
 *  @param linkProxy    A proxy object used to test linkages, to create
 *                      linkages etc.
 *  @param matrix       The position/orientation of the item being linked.
 */
LinkProperty::LinkProperty
(
	const Name&			name,
    LinkProxyPtr        linkProxy,
    MatrixProxyPtr      matrix,
	bool				alwaysShow /*= true*/
)
:
GeneralProperty(name),
linkProxy_(linkProxy),
matrix_(matrix),
alwaysShow_(alwaysShow)
{
	BW_GUARD;

    GENPROPERTY_MAKE_VIEWS()
}


/**
 *  Not implemented.
 */
/*virtual*/ PyObject * LinkProperty::pyGet()
{
    return NULL;
}


/**
 *  Not implemented.
 */
/*virtual*/ int LinkProperty::pySet
(
    PyObject        *value, 
    bool            transient   /*= false*/
)
{
    return 0;
}


/**
 *  Get the position/orientation proxy for the item being linked.
 *
 *  @returns        The position/orientation proxy for the item being linked.
 */
MatrixProxyPtr LinkProperty::matrix() const 
{ 
    return matrix_; 
}


/**
 *  Get the link proxy for the item being linked.
 *
 *  @returns        The link proxy for the item being linked.
 */
LinkProxyPtr LinkProperty::link() const
{
    return linkProxy_;
}


/**
 *  This method returns true if the link gizmo should be displayed always, or
 *	false if it should only be displayed when the it's property is selected.
 *
 *  @returns        true if the link gizmo should be displayed always, or
 *					false if it should only be displayed when the relevant
 *					property is selected.
 */
bool LinkProperty::alwaysShow() const
{
    return alwaysShow_;
}
BW_END_NAMESPACE

