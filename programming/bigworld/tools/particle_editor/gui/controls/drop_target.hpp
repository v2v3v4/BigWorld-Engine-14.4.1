#ifndef DROPTARGET_HPP
#define DROPTARGET_HPP

#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

//
// This is an interface for objects that can be drop targets.
// Derive from this class (typically in a CWnd derived class) and handle the
// functions below.  You must also include a DropTarget as given below and
// called DropTarget::Register.
//
//
class IDropTargetObj
{
public:
    //
    // Destructor.
    //
    virtual ~IDropTargetObj() {}

    //
    // Called when the user drags into a window.
    //
    // @param window        The window dragged into.
    // @param dataObject    The dragged data.
    // @param keyState      The state of the keys.
    // @param point         The mouse cursor location.
    // @returns             DROPEFFECT_NONE etc.
    //
    virtual 
    DROPEFFECT
    OnDragEnter
    (
        CWnd                *window,
        COleDataObject      *dataObject,
        DWORD               keyState,
        CPoint              point
    ) 
    {
        return DROPEFFECT_NONE; 
    }

    //
    // Called when the user drags out of a window.
    //
    // @param window        The window dragged out of.
    //
    virtual void OnDragLeave(CWnd *window) {}

    //
    // Called when the user drags over a window.
    //
    // @param window        The window dragged into.
    // @param dataObject    The dragged data.
    // @param keyState      The state of the keys.
    // @param point         The mouse cursor location.
    // @returns             DROPEFFECT_NONE etc.
    //
    virtual 
    DROPEFFECT
    OnDragOver
    (
        CWnd                *window,
        COleDataObject      *dataObject,
        DWORD               keyState,
        CPoint              point
    ) 
    {
        return DROPEFFECT_NONE; 
    }

    //
    // Called to determine whether the drag is over a scrolling region.
    //
    // @param window        The window dragged into.
    // @param keyState      The state of the keys.
    // @param point         The mouse cursor location.
    // @returns             DROPEFFECT_NONE etc.
    //
    virtual 
    DROPEFFECT 
    OnDragScroll
    (
        CWnd                *window, 
        DWORD               keyState, 
        CPoint              point
    )
    {
        return DROPEFFECT_NONE;
    }

    //
    // Called when a drop occurs.
    //
    // @param window        The window dragged into.
    // @param dataObject    The dragged data.
    // @param dropEffect    DROPEFFECT_COPY etc.
    // @param point         The mouse cursor location.
    // @returns             TRUE if successful, FALSE if failed.
    //
    virtual
    BOOL
    OnDrop
    (
        CWnd                *window,
        COleDataObject      *dataObject,
        DROPEFFECT          dropEffect,
        CPoint              point
    )
    {
        return FALSE;
    }
};

//
// Objects that wish to manage OLE dropping can include one of these classes
// as a member and derive from IDropTarget.  You need to call Register to
// register interest in dragging and dropping on a particular window.
//
class DropTarget : public COleDropTarget
{
public:
    //
    // Default constructor.
    //
    DropTarget();

    //
    // Register/unregister a drop target.
    //
    // @param target        The drop target that does the actual handling.
    // @param window        The window for dragging and dropping.
    //
    void Register(IDropTargetObj *target, CWnd *window);

    //
    // Called when the user drags into a window.
    //
    // @param window        The window dragged into.
    // @param dataObject    The dragged data.
    // @param keyState      The state of the keys.
    // @param point         The mouse cursor location.
    // @returns             DROPEFFECT_NONE etc.
    //
    /*virtual*/ 
    DROPEFFECT
    OnDragEnter
    (
        CWnd                *window,
        COleDataObject      *dataObject,
        DWORD               keyState,
        CPoint              point
    );

    //
    // Called when the user drags out of a window.
    //
    // @param window        The window dragged out of.
    //
    /*virtual*/ void OnDragLeave(CWnd *window);

    //
    // Called when the user drags over a window.
    //
    // @param window        The window dragged into.
    // @param dataObject    The dragged data.
    // @param keyState      The state of the keys.
    // @param point         The mouse cursor location.
    // @returns             DROPEFFECT_NONE etc.
    //
    /*virtual*/ 
    DROPEFFECT
    OnDragOver
    (
        CWnd                *window,
        COleDataObject      *dataObject,
        DWORD               keyState,
        CPoint              point
    );

    //
    // Called to determine whether the drag is over a scrolling region.
    //
    // @param window        The window dragged into.
    // @param keyState      The state of the keys.
    // @param point         The mouse cursor location.
    // @returns             DROPEFFECT_NONE etc.
    //
    /*virtual*/ 
    DROPEFFECT 
    OnDragScroll
    (
        CWnd                *window, 
        DWORD               keyState, 
        CPoint              point
    );

    //
    // Called when a drop occurs.
    //
    // @param window        The window dragged into.
    // @param dataObject    The dragged data.
    // @param dropEffect    DROPEFFECT_COPY etc.
    // @param point         The mouse cursor location.
    // @returns             TRUE if successful, FALSE if failed.
    //
    /*virtual*/
    BOOL
    OnDrop
    (
        CWnd                *window,
        COleDataObject      *dataObject,
        DROPEFFECT          dropEffect,
        CPoint              point
    );  

    //
    // Given the COleDataObject try and convert it to a string.
    // 
    // @param dataObject    The dataObject.
    // @returns             The text of the dataObject (if in the format 
    //                      CF_UNICODE).  The result is not defined otherwise.
    //
    static BW::wstring GetText(COleDataObject *dataObject);

private:
    // Not permitted:
    DropTarget(DropTarget const &);
    DropTarget &operator=(DropTarget const &);

private:
    IDropTargetObj          *m_dropTarget;
};

BW_END_NAMESPACE
#endif // DROPTARGET_HPP
