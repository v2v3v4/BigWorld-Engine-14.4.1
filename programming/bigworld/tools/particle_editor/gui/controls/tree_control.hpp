#ifndef TREECONTROL_HPP
#define TREECONTROL_HPP

#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE
// Forward declarations.
class TreeControl;

//
// This represents a node in a TreeControl.  The tree structure should
// be manipulated via TreeControl as there are windows that need to be
// maintained.
//
class TreeNode
{
public:
    //
    // Default constructor.
    //
    TreeNode();

    //
    // Explicit constructor.
    //
    // @param label     The item's label.
    //
    explicit TreeNode(char const *label);

    //
    // Destructor.
    //
    virtual ~TreeNode();

    //
    // Get the underlying HTREEITEM. 
    //
    // @returns         The underlying HTREEITEM.   This will be NULL if 
    //                  the node is hidden.
    //
    operator HTREEITEM() const;
    
    //
    // Can the label be edited?
    //
    // @returns         True if the label can be edited.  The default value
    //                  is true.
    //
    virtual bool CanEditLabel() const;

    //
    // Set the label.
    //
    // @param label     The new label.
    //
    virtual void SetLabel(BW::string const &label);

    //
    // Get the label.
    //
    // @returns         The label.
    //
    BW::string const &GetLabel() const;

    //
    // Is the node virtual (will OnExpand add in new nodes).
    //
    // @returns         True if the node will add new nodes upon
    //                  expansion.
    //
    virtual bool IsVirtualNode() const;

    //
    // Does deleting the node require confirmation?
    //
    // @returns         True if deletion of the node should be confirmed.
    //
    virtual bool DeleteNeedsConfirm() const;

    //
    // Can the node be dragged and dropped?
    //
    // @returns         The drop effects allowed.
    //
    virtual DROPEFFECT CanDragDrop() const;

    //
    // Get the parent node.
    //
    // @returns         The parent node. 
    //
    TreeNode *GetParent();

    //
    // Get the parent node.
    //
    // @returns         The parent node. 
    //
    TreeNode const *GetParent() const;

    //
    // Get the number of children.
    //
    // @returns         The number of children nodes.
    //
    size_t NumberChildren() const;

    //
    // Get a child node.
    //
    // @param idx       The index of the children nodes to get.
    // @returns         The idx'th child node.
    //
    TreeNode *GetChild(size_t idx);

    //
    // Get a child node.
    //
    // @param idx       The index of the children nodes to get.
    // @returns         The idx'th child node.
    //
    TreeNode const *GetChild(size_t idx) const;

	//
    // Is the node checked?
    //
    // @returns         True if the node is checked.
    //
    bool IsChecked() const;

    //
    // Is the node expanded?
    //
    // @returns         True if the node is expanded.
    //
    bool IsExpanded() const;

    //
    // Is the node selected?
    //
    bool IsSelected() const;

    //
    // Serialize expansion/contraction data etc to the given data section.
    //
    // @param data      The DataSection to load/save the data.
    // @param load      True if loading.
    // @returns         The selected child (can be NULL).
    //
    virtual TreeNode *
    Serialise
    (
        DataSectionPtr  data, 
        bool            load
    );

    //
    // The default name for a TreeNode when serializing.
    //
    // @returns         The default name for a tree node.
    //
    static BW::string const &
    DefaultSerializeName();

    //
    // Copy the expansion state from other as much as possible.
    //
    // @param other     The other node.
    // @returns         The selected (sub) node of this that corresponds to
    //                  any selected node in other.
    //
    TreeNode *CopyExpandState(TreeNode const*other);

	//
    // Copy the CheckBox state from other as much as possible.
    //
    // @param other     The other node.
    // @returns         The selected (sub) node of this that corresponds to
    //                  any selected node in other.
    //
    void CopyCheckBoxState(TreeNode const*other);

protected:
    //
    // Called when a node is hidden.  Derived classes should call the
    // base class version.  The base class calls this on children.
    //
    virtual void OnHide();

    //
    // Called when a node is shown.  Derived classes should call the
    // base class version.  The base class calls this on children.
    //
    virtual void OnShow();

    //
    // Called when the node is expanded.
    //
    virtual void OnExpand();

    //
    // Called when the node is collapsed.
    //
    virtual void OnCollapse();

    //
    // Called when the label is edited.
    //
    // @param newLabel      The new label.
    //
    virtual void OnEditLabel(BW::string const &newLabel);

    //
    // Synchronize the control's label with our internal one.
    //
    void SyncLabels() const;

    //
    // The parent tree control.
    //
    TreeControl *GetTreeControl();

    //
    // Find the subnode (or this node) that has the given htreeitem.
    //
    // @param hitem         The item to search for.
    // @returns             The corresponding TreeNode or NULL if there
    //                      is no such TreeNode.
    //
    TreeNode *Find(HTREEITEM hitem);

    //
    // Find the subnode (or this node) that has the given htreeitem.
    //
    // @param hitem         The item to search for.
    // @returns             The corresponding TreeNode or NULL if there
    //                      is no such TreeNode.
    //
    TreeNode const *Find(HTREEITEM hitem) const;

private:
    // Not permitted:
    TreeNode(TreeNode const &);
    TreeNode &operator=(TreeNode const &);

private:
    friend class TreeControl;

    TreeNode                *parent_;       // The parent node.
    BW::vector<TreeNode *> children_;      // Children nodes.
    HTREEITEM               htreeitem_;     // HTREEITEM for this node.  NULL is node is hidden.
    mutable BW::string     label_;         // The text label.  This can be out of sync unless SyncLabels is called.
    TreeControl             *tree_;         // The parent tree control.
    mutable bool            expanded_;      // Expanded at point of being hidden?
};

//
// This class is like CTreeCtrl, but allows for items to be hidden,
// virtual nodes etc.
// Don't use InsertItem etc of CTreeCtrl but use the given functions
// below.
//
class TreeControl : public CTreeCtrl
{
public:
    //
    // Constructor.
    //
    TreeControl();

    //
    // Destructor.
    //
    ~TreeControl();

    //
    // A "TreeNode" used to denote insertion at the end.
    //
    // @returns             A TreeNode used to denote insertion at the end.
    //
    static TreeNode *AtStart();

    //
    // A "TreeNode" used to denote insertion at the end.
    //
    // @returns             A TreeNode used to denote insertion at the end.
    //
    static TreeNode *AtEnd();

    //
    // Insert a new TreeNode.
    //
    // @param   newNode     The new node to use.  You can provide your own
    //                      TreeNode derived class here.  If this is null then
    //                      a TreeNode is created for you.
    // @param   parent      The parent node.  If this is NULL then we insert 
    //                      into the root node.
    // @param   prevSibling The previous sibling to insert after.  If this is
    //                      AtStart() then we add at the start of the parent.
    //                      If this is AtEnd() then we add at the end of the
    //                      parent.
    // @returns             newNode.
    //
    TreeNode * 
    AddNode
    (
        TreeNode            *newNode, 
        TreeNode            *parent         = NULL,
        TreeNode            *prevSibling    = TreeControl::AtEnd()
    );

    //
    // Remove the given node and all of its subchildren.
    //
    // @param node          The node to remove.
    //
    void RemoveNode(TreeNode *node);

    //
    // Return the parent TreeNode of the given node.
    //
    // @param node          The node whose parent to get.
    // @returns             The parent node.  This will be null if the
    //                      parent is the root.
    //
    TreeNode *GetParent(TreeNode *node);

    //
    // Get the number of child nodes of the given node.
    //
    // @param node          The node to query.  If this is null then the
    //                      root node is used.
    // @returns             The number of children.
    //
    size_t NumberChildren(TreeNode *node) const;

    //
    // Get the idx'th child node.
    //
    // @param node          The node to query.  If this is null then the
    //                      root node is used.
    // @param idx           The index of the child to get.
    // @returns             The idx'th child of the given node.
    //
    TreeNode *GetChild(TreeNode *node, size_t idx);

    //
    // Get the idx'th child node.
    //
    // @param node          The node to query.  If this is null then the
    //                      root node is used.
    // @param idx           The index of the child to get.
    // @returns             The idx'th child of the given node.
    //
    TreeNode const *GetChild(TreeNode const *node, size_t idx) const;

    //
    // Hide/show the given node.
    //
    // @param node          The node to show/hide.
    // @param show          If true the given node is shown.
    //
    void ShowNode(TreeNode *node, bool show);

    //
    // Is the node shown?
    //
    // @param node          The node to query.
    // @returns             True if the node is in the shown state.  It still
    //                      may be scrolled off the screen and not actually
    //                      visible.
    //
    bool IsShown(TreeNode *node);

    //
    // Make the user edit the node's label.
    //
    void EditNodeLabel(TreeNode *node);

    //
    // Expand the given node.
    //
    // @param node          The node to expand, collapse.
    // @param code          The expansion code.
    //
    void ExpandNode(TreeNode *node, UINT code);

    //
    // Find the TreeNode with the given HTREEITEM.
    //
    // @param hitem         The item to seach for.
    // @returns             The corresponding TreeNode.
    //
    TreeNode *Find(HTREEITEM hitem);

    //
    // Find the TreeNode with the given HTREEITEM.
    //
    // @param hitem         The item to seach for.
    // @returns             The corresponding TreeNode.
    //
    TreeNode const *Find(HTREEITEM hitem) const;

    //
    // Get the selected node.
    //
    // @returns             The selected node or NULL if nothing is selected.
    //
    TreeNode *GetSelectedNode();

    //
    // Get the selected node.
    //
    // @returns             The selected node or NULL if nothing is selected.
    //
    TreeNode const *GetSelectedNode() const;

    //
    // Set the selected node.
    //
    // @param sel           The newly selected node.
    //
    void SetSelectedNode(TreeNode *sel);

    //
    // Perform a hit-test.
    //
    // @param point         The test point.
    // @param flags         Optional flags returning TVHT_ABOVE etc.
    // @returns             The TreeNode that was hit.  This can be NULL.
    //
    TreeNode *HitTest(CPoint const &point, UINT *flags = NULL);

    //
    // Return the item being dragged.  This can be NULL.
    //
    TreeNode *GetDraggedNode();

    //
    // Get the format id for drag and drop operations.
    //
    // @returns             The ID used for drag and drop of TreeNodes.
    //
    static UINT GetDragDropID();

    //
    // Call this before a drag operation onto the control. 
    //
    // @param timerID       The id of a timer to use for scrolling.
    //
    void BeginDrag(UINT_PTR timerID);

    //
    // Call this after a drag operation onto the control. 
    //
    void EndDrag();

    //
    // Call this before doing many operations on a TreeControl.
    //
    void BeginLargeOperation();

    //
    // Call this after doing the many operations on a TreeControl.
    //
    void EndLargeOperation();

    //
    // Are we in a large operation?
    //
    // @returns             True if inside a Begin/EndLargeOperation bracket.
    //
    bool InLargeOperation() const;

protected:
    //
    // Add node at the point of parent and prevSibling.  We assume that
    // node is already inserted into our local tree structure.  Children
    // nodes are also inserted.
    //
    // @param node          The node to insert.
    // @param parent        The parent HTREEITEM.
    // @param prevSibling   The previous sibling HTREEITEM.
    // @returns             The newly inserted item.
    //
    HTREEITEM 
    CreateHTreeItems
    (
        TreeNode            *node, 
        HTREEITEM           parent,
        HTREEITEM           prevSibling
    );

    //
    // Called upon item expansion.  This helper deals with virtual nodes.
    //
    // @param nmhdr         The notification data.
    // @param result        The return LRESULT.
    //
    afx_msg void OnItemExpanding(NMHDR* nmhdr, LRESULT* result);

    //
    // Called upon node label editing.  This helper allows nodes that
    // are not editable.
    //
    // @param nmhdr         The notification data.
    // @param result        The return LRESULT.
    //
    afx_msg void OnBeginLabelEdit(NMHDR* nmhdr, LRESULT* result);

    //
    // Called when node label editing is done.  This helper allows nodes that
    // are not editable and validates the label edit for those that are 
    // editable.
    //
    // @param nmhdr         The notification data.
    // @param result        The return LRESULT.
    // @returns             false.  This allows the parent to also receive the
    //                      notification.
    //
    afx_msg BOOL OnEndLabelEdit(NMHDR* nmhdr, LRESULT* result);

    //
    // Called when the left mouse button is pressed.
    //
    // @param flags         The key state flags.
    // @param point         The cursor location.
    //
    afx_msg void OnLButtonDown(UINT flags, CPoint point);

    //
    // Called when the mouse is moved.
    //
    afx_msg void OnMouseMove(UINT flags, CPoint point);

    //
    // Called to handle scrolling during drag and drop.
    //
    afx_msg void OnTimer(UINT_PTR timerID);

    DECLARE_MESSAGE_MAP()

    void ExpandInternal(TreeNode *node);

private:
    TreeNode                *root_;
    TreeNode                *dragItem_;
    UINT_PTR                scrollTimerID_;
    size_t                  timerticks_;
    size_t                  largeOpCnt_;
};

BW_END_NAMESPACE
#endif // TREECONTROL_HPP
