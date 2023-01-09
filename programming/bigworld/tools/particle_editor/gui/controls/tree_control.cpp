#include "pch.hpp"
#include "tree_control.hpp"
#include "common/user_messages.hpp"

BW_BEGIN_NAMESPACE

namespace
{
    UINT         TC_CFORMAT     = 0;    // TreeControl's clipboard format
    BW::string const TREE_NODE_STR  = "treenode";
}

TreeNode::TreeNode() :
    parent_(NULL),
    htreeitem_(NULL),
    tree_(NULL),
    expanded_(false)
{
}

/*explicit*/ TreeNode::TreeNode(char const *label) :
    parent_(NULL),
    htreeitem_(NULL),
    label_(label),
    tree_(NULL),
    expanded_(false)
{
}

/*virtual*/ TreeNode::~TreeNode()
{
	BW_GUARD;

    for (size_t i = 0; i < children_.size(); ++i)
        delete children_[i];
    children_.clear();
    parent_    = NULL;
    htreeitem_ = NULL;
    tree_      = NULL;
}

TreeNode::operator HTREEITEM() const
{
    return htreeitem_;
}

/*virtual*/ bool TreeNode::CanEditLabel() const
{
    return true;
}

void TreeNode::SetLabel(BW::string const &label)
{
	BW_GUARD;

    label_ = label;
    if (htreeitem_ != NULL)
        tree_->SetItemText(htreeitem_, bw_utf8tow( label_ ).c_str());
}

BW::string const &TreeNode::GetLabel() const
{
	BW_GUARD;

    SyncLabels();
    return label_;
}

bool TreeNode::IsVirtualNode() const
{
    return false;
}

bool TreeNode::DeleteNeedsConfirm() const
{
    return false;
}

DROPEFFECT TreeNode::CanDragDrop() const
{
    return DROPEFFECT_COPY | DROPEFFECT_MOVE;
}

TreeNode *TreeNode::GetParent()
{
    return parent_;
}

TreeNode const *TreeNode::GetParent() const
{
    return parent_;
}

size_t TreeNode::NumberChildren() const
{
	BW_GUARD;

    return children_.size();
}

TreeNode *TreeNode::GetChild(size_t idx)
{
	BW_GUARD;

    return children_[idx];
}

TreeNode const *TreeNode::GetChild(size_t idx) const
{
	BW_GUARD;

    return children_[idx];
}

bool TreeNode::IsChecked() const
{
	BW_GUARD;

	if (tree_ != NULL && htreeitem_ != NULL)
    {
		return tree_->GetCheck(htreeitem_) == BST_CHECKED;
	}
	else
	{
		return false;
	}
}

bool TreeNode::IsExpanded() const
{
	BW_GUARD;

    if (tree_ != NULL && htreeitem_ != NULL)
    {
        UINT state = tree_->GetItemState(htreeitem_, TVIS_EXPANDED);
        return (state & TVIS_EXPANDED) != 0;
    }
    else
    {
        return false;
    }
}

bool TreeNode::IsSelected() const
{
	BW_GUARD;

    if (tree_ != NULL && htreeitem_ != NULL)
    {
        UINT state = tree_->GetItemState(htreeitem_, TVIS_SELECTED);
        return (state & TVIS_SELECTED) != 0;
    }
    else
    {
        return false;
    }
}

TreeNode *
TreeNode::Serialise
(
    DataSectionPtr  data, 
    bool            load
)
{
	BW_GUARD;

    TreeNode *result = NULL;
    
    if (load)
    {
		bool checked = false;
        bool expanded = false; 
        bool selected = false;
        BW::string label;
		SERIALISE(data, checked , Bool  , true)
        SERIALISE(data, expanded, Bool  , true);
        SERIALISE(data, selected, Bool  , true);
        SERIALISE(data, label   , String, true);
        SetLabel(label);
		if (tree_ != NULL && htreeitem_ != NULL)
		{
			tree_->SetCheck(htreeitem_, checked ? BST_CHECKED : BST_UNCHECKED);
		}
        if (expanded && tree_ != NULL && htreeitem_ != NULL)
        {
            tree_->Expand(htreeitem_, TVE_EXPAND);
        }
        if (selected)
            result = this;

        for (DataSectionIterator it = data->begin(); it != data->end(); ++it)
        {
            DataSectionPtr ds = *it;

			if (ds->sectionName() == TREE_NODE_STR)
            {
				size_t nodeidx = 0;

				SERIALISE(ds, label   , String, true);

				for (; nodeidx < children_.size(); ++nodeidx)
				{
					if (children_[ nodeidx ]->label_ == label)
					{
						break;
					}
				}

				if (nodeidx != children_.size())
				{
					TreeNode *selChild = children_[nodeidx]->Serialise(ds, load);

					if (selChild != NULL)
						result = selChild;
				}
            }
        }
    }
    else
    {
		bool checked  = IsChecked();
        bool expanded = IsExpanded();
        bool selected = IsSelected();
        BW::string label = GetLabel();
		SERIALISE(data, checked,  Bool  , false);
        SERIALISE(data, expanded, Bool  , false);
        SERIALISE(data, selected, Bool  , false);
        SERIALISE(data, label   , String, false);
        if (selected)
            result = this;
        for (size_t i = 0; i < children_.size(); ++i)
        {
            DataSectionPtr childSection = 
                data->newSection(TREE_NODE_STR);
            TreeNode *selChild = children_[i]->Serialise(childSection, load);
            if (selChild != NULL)
                result = selChild;
        }
    }

    return result;
}

TreeNode *TreeNode::CopyExpandState(TreeNode const*other)
{
	BW_GUARD;

    if (other == NULL)
        return NULL;

    // Expand/collapse as appropriate:
    if (tree_ != NULL && htreeitem_ != NULL)
    {
        tree_->Expand
        (
            htreeitem_, 
            other->IsExpanded() 
                ? TVE_EXPAND
                : TVE_COLLAPSE
        );
    }

    // Setup selection:
    TreeNode *result = NULL;
    if (other->IsSelected())
        result = this;

    // For each child, find the corresponding child in other and copy
    // the expansion state.  This assumes unique names.
    for (size_t i = 0; i < children_.size(); ++i)
    {
        BW::string const &mylabel = children_[i]->GetLabel();
        for (size_t j = 0; j < other->children_.size(); ++j)
        {
            if (other->children_[j]->GetLabel() == mylabel)
            {
                TreeNode *sel =
                    children_[i]->CopyExpandState(other->children_[j]);
                if (sel != NULL)
                    result = sel;
                break;
            }
        }
    }

    return result;
}

void TreeNode::CopyCheckBoxState(TreeNode const*other)
{
	BW_GUARD;

    if (other == NULL)
        return;

    // Copy checkbox
    if ((tree_ != NULL) && 
		(htreeitem_ != NULL) && 
		(tree_->GetItemState( htreeitem_, TVIS_STATEIMAGEMASK ) != 0))
    {
        tree_->SetCheck( htreeitem_, tree_->GetCheck( *other ) );
    }

	// For each child, find the corresponding child in other and copy
    // the checkbox state.  This assumes unique names.
    for (size_t i = 0; i < children_.size(); ++i)
    {
        BW::string const &mylabel = children_[i]->GetLabel();
        for (size_t j = 0; j < other->children_.size(); ++j)
        {
            if (other->children_[j]->GetLabel() == mylabel)
            {
                children_[i]->CopyCheckBoxState(other->children_[j]);
            }
        }
    }
}

/*static*/ BW::string const &
TreeNode::DefaultSerializeName()
{
    return TREE_NODE_STR;
}

/*virtual*/ void TreeNode::OnHide()
{
	BW_GUARD;

    if (htreeitem_ != NULL)
    {
        expanded_ = 
            (tree_->GetItemState(htreeitem_, TVIS_EXPANDED) & TVIS_EXPANDED)
            == 
            TVIS_EXPANDED;
        SyncLabels();
        htreeitem_ = NULL;
    }
    for (size_t i = 0; i < children_.size(); ++i)
    {
        children_[i]->OnHide();
    }
}

/*virtual*/ void TreeNode::OnShow()
{
	BW_GUARD;

    for (size_t i = 0; i < children_.size(); ++i)
    {
        children_[i]->OnShow();
    }
}

/*virtual*/ void TreeNode::OnExpand()
{
}

/*virtual*/ void TreeNode::OnCollapse()
{
}

/*virtual*/ void TreeNode::OnEditLabel(BW::string const &/*newLabel*/)
{
}

void TreeNode::SyncLabels() const
{
	BW_GUARD;

    if (htreeitem_ != NULL)
    {
        CString label = tree_->GetItemText(htreeitem_);
        label_ = bw_wtoutf8( label.GetString() );
    }
}

TreeControl *TreeNode::GetTreeControl()
{
    return tree_;
}

TreeNode *TreeNode::Find(HTREEITEM hitem)
{
	BW_GUARD;

    if (htreeitem_ == hitem)
        return this;
    for (size_t i = 0; i < children_.size(); ++i)
    {
        TreeNode *result = children_[i]->Find(hitem);
        if (result != NULL)
            return result;
    }
    return NULL;
}

TreeNode const *TreeNode::Find(HTREEITEM hitem) const
{
	BW_GUARD;

    if (htreeitem_ == hitem)
        return this;
    for (size_t i = 0; i < children_.size(); ++i)
    {
        TreeNode *result = children_[i]->Find(hitem);
        if (result != NULL)
            return result;
    }
    return NULL;
}

BEGIN_MESSAGE_MAP(TreeControl, CTreeCtrl)
    ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING , OnItemExpanding )
    ON_NOTIFY_REFLECT(TVN_BEGINLABELEDIT, OnBeginLabelEdit)
    ON_NOTIFY_REFLECT_EX(TVN_ENDLABELEDIT, OnEndLabelEdit)
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_TIMER()
END_MESSAGE_MAP()

TreeControl::TreeControl() :
    CTreeCtrl(),
    root_(new TreeNode()),
    dragItem_(NULL),
    scrollTimerID_(0),
    timerticks_(0),
    largeOpCnt_(0)
{
}

TreeControl::~TreeControl()
{
	BW_GUARD;

    delete root_; root_ = NULL;
}

/*static*/ TreeNode *TreeControl::AtStart()
{
    return (TreeNode *)2;
}

/*static*/ TreeNode *TreeControl::AtEnd()
{
    return (TreeNode *)1;
}

TreeNode * 
TreeControl::AddNode
(   
    TreeNode            *newNode ,
    TreeNode            *parent         /*= NULL*/,
    TreeNode            *prevSibling    /*= NULL*/
)
{
	BW_GUARD;

    // Handle the degenerate case:
    if (newNode == NULL)
        return NULL;
    // Make sense of the parent:
    if (parent == NULL)
        parent = root_;
    // Insert into our tree structure:
    newNode->parent_ = parent;
    if (prevSibling == AtEnd())
    {
        parent->children_.push_back(newNode);
    }
    else if (prevSibling == AtStart())
    {
        parent->children_.insert(parent->children_.begin(), newNode);
    }
    else
    {
        size_t idx = 0;
        for (;idx < parent->children_.size(); ++idx)
        {
            if (parent->children_[idx] == prevSibling)
                break;
        }
        if (idx != parent->children_.size())
        {
            parent->children_.insert
            (
                parent->children_.begin() + idx + 1, 
                newNode
            );
        }
        else
        {
            // prevSibling not found, add at end
            parent->children_.push_back(newNode);
            prevSibling = AtEnd(); // flag this failure
        }
    }
    newNode->tree_ = this;
    // Insert into the CTreeCtrl's structure:
    if (IsShown(parent))
    {
        HTREEITEM hparent = TVI_ROOT;
        if (parent != root_ && parent->htreeitem_ != NULL)
            hparent = parent->htreeitem_;
        HTREEITEM hsibling = (prevSibling == AtStart()) ? TVI_FIRST : TVI_LAST;
        if
        (
            prevSibling != AtStart() 
            && 
            prevSibling != AtEnd()
            && 
            prevSibling->htreeitem_ != NULL
        )
        {
            hsibling = prevSibling->htreeitem_;
        }
        HTREEITEM hitem = 
            InsertItem
            (
                bw_utf8tow( newNode->label_ ).c_str(), 
                hparent, 
                hsibling
            );
        newNode->htreeitem_ = hitem;

        // Add the plus sign if the node is virtual:
        if (newNode->IsVirtualNode())
        {
            TVITEM item;
            ::ZeroMemory(&item, sizeof(item));
            item.hItem     = hitem;
            item.mask      = TVIF_HANDLE | TVIF_CHILDREN;
            item.cChildren = 1;
            SetItem(&item);
        }
    }
    // If there are children, recursively add them:
    for (size_t i = 0; i < newNode->children_.size(); ++i)
        AddNode(newNode->children_[i], newNode, AtEnd());
    return newNode;
}

void TreeControl::RemoveNode(TreeNode *node)
{
	BW_GUARD;

    // Removing the root is like removing everything:
    if (node == NULL)
    {
        BeginLargeOperation();
        DeleteAllItems();
        delete root_;        
        root_ = new TreeNode();
        EndLargeOperation();
    }
    // Not the root node case:
    else
    {
        // Remove from parent's child list:
        TreeNode *parent = node->parent_;
        for (size_t i = 0; i < parent->children_.size(); ++i)
        {
            if (parent->children_[i] == node)
            {
                parent->children_.erase(parent->children_.begin() + i);
                break;
            }
        }
        // We need to retain the hitem beyond deletion:
        HTREEITEM hitem = node->htreeitem_;

        // Get the next selected item:
        HTREEITEM nextItem = GetNextSiblingItem(hitem);
        HTREEITEM prevItem = GetPrevSiblingItem(hitem);

        // Remove the item:
        delete node;
        if (hitem != NULL)
            DeleteItem(hitem);

        // Update the selection:
        if (prevItem != NULL)
            SelectItem(prevItem);
        else if (nextItem != NULL)
            SelectItem(nextItem);
    }
    dragItem_ = NULL;
}

TreeNode *TreeControl::GetParent(TreeNode *node)
{
	BW_GUARD;

    // We have to be careful about the root case. 
    if (node == NULL)
        return NULL;
    TreeNode *result = node->parent_;
    if (result == root_)
        return NULL;
    else
        return result;
}

size_t TreeControl::NumberChildren(TreeNode *node) const
{
	BW_GUARD;

    if (node == NULL)
        node = root_;
    return node->children_.size();
}

TreeNode *TreeControl::GetChild(TreeNode *node, size_t idx)
{
	BW_GUARD;

    if (node == NULL)
        node = root_;
    return node->children_[idx];
}

TreeNode const *TreeControl::GetChild(TreeNode const *node, size_t idx) const
{
	BW_GUARD;

    if (node == NULL)
        node = root_;
    return node->children_[idx];
}

void TreeControl::ShowNode(TreeNode *node, bool show)
{
	BW_GUARD;

    // Make sure that this operation makes sense:
    if (node != NULL)
    {
        if (show)
        {
            // This node should not already be shown:
            if (node->htreeitem_ != NULL)
                return;
            // The parent, if it exists, must be shown:
            TreeNode *parent = node->parent_;
            if (parent != NULL && parent != root_ && parent->htreeitem_ == NULL)
                return;
        }
        else
        {
            // This node should not be already hidden:
            if (node->htreeitem_ == NULL)
                return;
        }
    }

    // The root is a special case:
    if (node == NULL)
    {
        if (show)
        {            
            HTREEITEM lastHItem = TVI_LAST;
            for (size_t i = 0; i < root_->children_.size(); ++i)
            {         
                lastHItem =
                    CreateHTreeItems
                    (
                        root_->children_[i],
                        TVI_ROOT,
                        lastHItem
                    );
            }
            root_->OnShow();
        }
        else
        {            
            DeleteAllItems();
            root_->OnHide();
        }
    }
    else
    {
        if (show)
        {
            // Find the position to insert at:
            // The HTREEITEM of the parent is easy since above we
            // guarantee the parent is visible:
            TreeNode *parent = node->parent_;
            HTREEITEM hparent = 
                parent == root_ 
                    ? TVI_ROOT
                    : parent->htreeitem_;
            // Find the previous sibling that is visible.  If one is
            // not visible then it must be the first shown child of the 
            // parent and so we use TVI_FIRST.
            HTREEITEM hprevsibling = TVI_FIRST;
            int idx;
            for (idx = 0; idx < (int)parent->children_.size(); ++idx)
                if (parent->children_[idx] == node)
                    break;
            for (idx = idx - 1; idx >= 0; --idx)
            {
                if (parent->children_[idx]->htreeitem_ != NULL)
                {
                    hprevsibling = parent->children_[idx]->htreeitem_;
                    break;
                }
            }
            // Create the controls:
            CreateHTreeItems(node, hparent, hprevsibling);
            // Inform the node:
            node->OnShow();
        }
        else
        {          
            // Save the HTREEITEM of the node (OnHide sets it to
            // NULL).
            HTREEITEM hitem = node->htreeitem_;
            // Inform the node:
            node->OnHide();
            // Remove the nodes from the Window's control:
            DeleteItem(hitem);
        }
    }
}

bool TreeControl::IsShown(TreeNode *node)
{
    if (node == NULL || node == root_)
        return true; // the root is always shown?
    else
        return node->htreeitem_ != NULL;
}

void TreeControl::EditNodeLabel(TreeNode *node)
{
	BW_GUARD;

    if (node != NULL && node->CanEditLabel() && node->htreeitem_ != NULL)
    {
        SetFocus();
        EditLabel(node->htreeitem_);
    }
}

void TreeControl::ExpandNode(TreeNode *node, UINT code)
{
	BW_GUARD;

    if (node != NULL)
    {        
        if (code == TVE_EXPAND)
            ExpandInternal(node);
        else
            Expand(node->htreeitem_, code);
        EnsureVisible(node->htreeitem_);
    }
}

TreeNode *TreeControl::Find(HTREEITEM hitem)
{
	BW_GUARD;

    return root_->Find(hitem);
}

TreeNode const *TreeControl::Find(HTREEITEM hitem) const
{
	BW_GUARD;

    return root_->Find(hitem);
}

TreeNode *TreeControl::GetDraggedNode()
{
    return dragItem_;
}

UINT TreeControl::GetDragDropID()
{
	BW_GUARD;

    if (TC_CFORMAT == 0)
    {
        TC_CFORMAT = ::RegisterClipboardFormat(L"TreeControlClipboard");
    }
    return TC_CFORMAT;
}

void TreeControl::BeginDrag(UINT_PTR timerID)
{
	BW_GUARD;

    scrollTimerID_ = SetTimer(timerID, 75, NULL);
    timerticks_ = 0;
}

void TreeControl::EndDrag()
{
	BW_GUARD;

    KillTimer(scrollTimerID_);
    CWnd *parent = CTreeCtrl::GetParent();
}

void TreeControl::BeginLargeOperation()
{
	BW_GUARD;

    if (largeOpCnt_++ == 0)
        SetRedraw(FALSE);
}

void TreeControl::EndLargeOperation()
{
	BW_GUARD;

    if (--largeOpCnt_ == 0)
    {
        SetRedraw(TRUE);
        Invalidate();
        UpdateWindow();
    }
}

bool TreeControl::InLargeOperation() const
{
    return largeOpCnt_ != 0;
}

TreeNode *TreeControl::GetSelectedNode()
{
	BW_GUARD;

    HTREEITEM hitem = GetSelectedItem();
    if (hitem == NULL)
        return NULL;
    return root_->Find(hitem);
}

TreeNode const *TreeControl::GetSelectedNode() const
{
	BW_GUARD;

    HTREEITEM hitem = GetSelectedItem();
    if (hitem == NULL)
        return NULL;
    return root_->Find(hitem);
}

void TreeControl::SetSelectedNode(TreeNode *sel)
{
	BW_GUARD;

    if (sel == NULL)
        return;
    HTREEITEM hitem = sel->htreeitem_;
    if (hitem == NULL)
        return;
    Select(hitem, TVGN_CARET);
}

TreeNode *TreeControl::HitTest(CPoint const &point, UINT *flags /* = NULL*/)
{
	BW_GUARD;

    HTREEITEM hitem = CTreeCtrl::HitTest(point, flags);
    if (hitem == NULL)
        return NULL;
    else
        return Find(hitem);
}

HTREEITEM 
TreeControl::CreateHTreeItems
(
    TreeNode            *node, 
    HTREEITEM           parent,
    HTREEITEM           prevSibling
)
{
	BW_GUARD;

    HTREEITEM newItem = 
        InsertItem
        (
            bw_utf8tow( node->label_ ).c_str(),
            parent,
            prevSibling
        );
    node->htreeitem_ = newItem;
    if (node->IsVirtualNode())
    {
        TVITEM item;
        ::ZeroMemory(&item, sizeof(item));
        item.hItem     = newItem;
        item.mask      = TVIF_HANDLE | TVIF_CHILDREN;
        item.cChildren = 1;
        SetItem(&item);
    }
    HTREEITEM subPrevSibling = TVI_LAST;
    for (size_t i = 0; i < node->children_.size(); ++i)
    {
        subPrevSibling =
            CreateHTreeItems
            (
                node->children_[i], 
                newItem, 
                subPrevSibling
            );
    }
    if (node->expanded_)
        Expand(newItem, TVE_EXPAND);
    return newItem;
}

void TreeControl::OnItemExpanding(NMHDR* nmhdr, LRESULT* result)
{
	BW_GUARD;

    NM_TREEVIEW *treeview = (NM_TREEVIEW*)nmhdr;
    TreeNode * node = root_->Find(treeview->itemNew.hItem);
    if (node != NULL)
    {
        if (treeview->action == TVE_EXPAND)
            node->OnExpand();
        else if (treeview->action == TVE_COLLAPSE)
            node->OnCollapse();
    }
    if (result != NULL)
        *result = 0;
}

void TreeControl::OnBeginLabelEdit(NMHDR* nmhdr, LRESULT* presult)
{
	BW_GUARD;

    LRESULT result = 0;
    NMTVDISPINFO *dispInfo = (NMTVDISPINFO*)nmhdr;
    TreeNode * node = root_->Find(dispInfo->item.hItem);
    if (node != NULL)
    {
        if (node->CanEditLabel())
            result = FALSE; // This looks wrong but is correct
        else
            result = TRUE;
    }
    if (presult != NULL)
        *presult = result;
}

BOOL TreeControl::OnEndLabelEdit(NMHDR* nmhdr, LRESULT* presult)
{
	BW_GUARD;

    LRESULT result = TRUE;
    NMTVDISPINFO *dispInfo = (NMTVDISPINFO*)nmhdr;

    TreeNode * node = root_->Find(dispInfo->item.hItem);
    if (node != NULL)
    {
        if (node->CanEditLabel())
        {
            if (dispInfo->item.pszText != NULL)
                node->OnEditLabel(bw_wtoutf8( dispInfo->item.pszText ));
            result = TRUE;  // accept changes
        }
        else
        {
            result = FALSE; // reject changes
        }
    }

    if (presult != NULL)
        *presult = result;

    return FALSE; // allow the parent to get the message too
}

void TreeControl::OnLButtonDown(UINT flags, CPoint point)
{
	BW_GUARD;

    CTreeCtrl::OnLButtonDown(flags, point);
    dragItem_ = HitTest(point);
}

void TreeControl::OnMouseMove(UINT flags, CPoint point)
{
	BW_GUARD;

    if 
    (
        dragItem_ != NULL 
        && 
        dragItem_->CanDragDrop() != DROPEFFECT_NONE
        &&
        (flags & MK_LBUTTON) != 0
    )
    {
        DROPEFFECT dropEffect = dragItem_->CanDragDrop();
        if (dropEffect != DROPEFFECT_NONE)
        {
            CWnd *parent = CTreeCtrl::GetParent();
            if (parent != NULL)
            {
                parent->SendMessage(WM_DRAG_START, 0, 0);
            }
            COleDataSource dataSource;
            HGLOBAL hglobal = ::GlobalAlloc(GMEM_MOVEABLE, sizeof(HTREEITEM));
            HTREEITEM *hitem = (HTREEITEM *)::GlobalLock(hglobal);
            *hitem = dragItem_->htreeitem_;
            ::GlobalUnlock(hglobal);
            dataSource.CacheGlobalData(GetDragDropID(), hglobal);
            dataSource.DoDragDrop(dropEffect);
            dragItem_ = NULL;
            if (parent != NULL)
            {
                parent->SendMessage(WM_DRAG_DONE, 0, 0);
            }
        }
    }
}

void TreeControl::OnTimer(UINT_PTR timerID)
{
	BW_GUARD;

    if (timerID != scrollTimerID_)
    {
        CTreeCtrl::OnTimer(timerID);
        return;
    }

    POINT pt;
    GetCursorPos(&pt);
    RECT rect;
    GetClientRect(&rect);
    ClientToScreen(&rect);

    ++timerticks_;

    // NOTE: Screen coordinate is being used because the call
    // to DragEnter had used the Desktop window.
    CImageList::DragMove(pt);

    if (pt.y < rect.top + 10)
    {
        // We need to scroll up
        // Scroll slowly if cursor near the treeview control
        int slowscroll = 6 - (rect.top + 10 - pt.y)/20;
        if( 0 == (timerticks_ % (slowscroll > 0? slowscroll : 1)))
        {
            CImageList::DragShowNolock(FALSE);
            SendMessage(WM_VSCROLL, SB_LINEUP);
            CImageList::DragShowNolock(TRUE);
        }
    }
    else if (pt.y > rect.bottom - 10)
    {
        // We need to scroll down
        // Scroll slowly if cursor near the treeview control
        int slowscroll = 6 - (pt.y - rect.bottom + 10 ) / 20;
        if (0 == (timerticks_ % (slowscroll > 0? slowscroll : 1)))
        {
            CImageList::DragShowNolock(FALSE);
            SendMessage(WM_VSCROLL, SB_LINEDOWN);
            CImageList::DragShowNolock(TRUE);
        }
    }
}

void TreeControl::ExpandInternal(TreeNode *node)
{
	BW_GUARD;

    if (node == NULL)
        return;

    Expand(node->htreeitem_, TVE_EXPAND);
    for (size_t i = 0; i < node->NumberChildren(); ++i)
    {
        ExpandInternal(node->GetChild(i));
    }
}
BW_END_NAMESPACE

