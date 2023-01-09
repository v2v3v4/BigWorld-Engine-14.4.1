#include "pch.hpp"
#include "ps_tree.hpp"
#include "ps_node.hpp"
#include "resource.h"
#include "common/format.hpp"
#include "common/user_messages.hpp"
#include "resmgr/string_provider.hpp"

BW_BEGIN_NAMESPACE

BEGIN_MESSAGE_MAP(PSTree, TreeControl)
    ON_NOTIFY_REFLECT_EX(TVN_SELCHANGED, OnSelection)
    ON_WM_KEYDOWN()
    ON_WM_CREATE()
    ON_WM_RBUTTONDOWN()
    ON_WM_PAINT()
END_MESSAGE_MAP()

PSTree::PSTree():
	TreeControl(),
	m_dir()
{
}

bool PSTree::Load(BW::string const &dir)
{
	BW_GUARD;

    m_dir = dir;

    // Clear items from the tree:
    RemoveNode(NULL);

    // Create the appropriate datasection and iterate through its entries:
    bool haveOne = false;
    DataSectionPtr dataSection = BWResource::openSection(m_dir);
    if (dataSection)
    {
		set< BW::string > addedFiles;
        for
        (
            DataSectionIterator it = dataSection->begin();
            it != dataSection->end();
            ++it
        )
        {
            // Add entries that are xml files:
            BW::string filename = (*it)->sectionName();
            if (MetaNode::IsMetaParticleFile(filename))
            {
				if ( addedFiles.find( filename ) == addedFiles.end() )
				{
					MetaNode *mn = (MetaNode *)AddNode(new MetaNode(dir, filename));
					addedFiles.insert( filename );
				}
            }
        }
    }

    return haveOne;
}

MetaParticleSystemPtr PSTree::GetSelectedMetaParticleSystem() const
{
	BW_GUARD;

    TreeNode const *node = GetSelectedNode();
    while (node != NULL)
    {
        MetaNode const *metanode = dynamic_cast<MetaNode const *>(node);
        if (metanode != 0)
            return metanode->GetMetaParticleSystem();
        node = node->GetParent();
    }
    return NULL;
}

ParticleSystemPtr PSTree::GetSelectedParticleSystem() const
{
	BW_GUARD;

    TreeNode const *node = GetSelectedNode();
    while (node != NULL)
    {
        PSNode const *psnode = dynamic_cast<PSNode const *>(node);
        if (psnode != 0)
            return psnode->getParticleSystem();
        node = node->GetParent();
    }
    return NULL;
}

ParticleSystemActionPtr PSTree::GetSelectedAction() const
{
	BW_GUARD;

    TreeNode const *node = GetSelectedNode();
    ActionNode const *anode = dynamic_cast<ActionNode const *>(node);
    if (anode != NULL)
        return (const_cast<ActionNode *>(anode))->getAction();
    else
        return NULL;
}

void PSTree::updateCheckBoxes( MetaNode *metaNode )
{
	BW_GUARD;

	if ( metaNode != NULL )
	{
		bool metaChecked = false;
		bool noChildren = true;
		bool selected = metaNode->IsSelected();
		for ( size_t i = 0; i < NumberChildren( metaNode ); i++ )
		{
			noChildren = false;
			PSNode* ps = dynamic_cast<PSNode*>( GetChild( metaNode, i ) );
			if ( ps == NULL )
				continue;
			metaChecked |= ps->getParticleSystem()->enabled();
			selected |= ps->IsSelected();
			SetCheck( *ps, ps->getParticleSystem()->enabled() );
			for ( size_t j = 0; j < NumberChildren( ps ); j++)
			{
				ActionNode* psa = dynamic_cast<ActionNode*>( GetChild( ps, j ) );
				if ( psa == NULL )
					continue;
				selected |= psa->IsSelected();
				if ( ( psa->getActionType() != ActionNode::AT_SYS_PROP  ) 
				  && ( psa->getActionType() != ActionNode::AT_REND_PROP ) )
					SetCheck( *psa, psa->getAction()->enabled() );
				else
					SetItemState( *psa, 0, TVIS_STATEIMAGEMASK ); 
			}
		}
		if ( selected && ( metaChecked || noChildren ))
			SetCheck( *metaNode );
		else
			SetCheck( *metaNode, BST_UNCHECKED );
	}
}

void PSTree::updateEnabledState( MetaNode *metaNode )
{
	BW_GUARD;

	if ( metaNode != NULL )
	{
		for ( size_t i = 0; i < NumberChildren( metaNode ); i++ )
		{
			PSNode* ps = dynamic_cast<PSNode*>( GetChild( metaNode, i ) );
			if ( ps == NULL )
				continue;
			ps->getParticleSystem()->enabled( GetCheck( *ps ) == BST_CHECKED );
			for ( size_t j = 0; j < NumberChildren( ps ); j++)
			{
				ActionNode* psa = dynamic_cast<ActionNode*>( GetChild( ps, j ) );
				if ( psa == NULL )
					continue;
				if ( ( psa->getActionType() != ActionNode::AT_SYS_PROP  ) 
				  && ( psa->getActionType() != ActionNode::AT_REND_PROP ) )
					psa->getAction()->enabled( GetCheck( *psa ) == BST_CHECKED );
			}
		}
	}
}

void PSTree::SetFilter(BW::string const &filter)
{
	BW_GUARD;

    BeginLargeOperation();

    // Display everthing?
    if (filter.empty())
    {
        size_t sz = NumberChildren(NULL);
        for (size_t i = 0; i < sz; ++i)
        {
            TreeNode *tn = GetChild(NULL, i);
            ShowNode(tn, true);
			updateCheckBoxes( dynamic_cast<MetaNode*>(tn) );
        }
    }
    // Don't display all, apply a case-insenstive filter:
    else
    {
        BW::string ifilter = filter; // case removed filter
        transform(ifilter.begin(), ifilter.end(), ifilter.begin(), tolower);
        size_t sz = NumberChildren(NULL);
        for (size_t i = 0; i < sz; ++i)
        {
            TreeNode *tn = GetChild(NULL, i);
            BW::string name = tn->GetLabel();
            transform(name.begin(), name.end(), name.begin(), tolower);
            bool visible = (name.find(ifilter) != BW::string::npos);
            ShowNode(tn, visible);
			if ( visible )
				updateCheckBoxes( dynamic_cast<MetaNode*>(tn)  );
        }
    }

    EndLargeOperation();
}

BW::string const &PSTree::GetDirectory() const
{
    return m_dir;
}

BOOL PSTree::OnSelection(NMHDR* nmhdr, LRESULT* result)
{
	BW_GUARD;

    if (!InLargeOperation())
    {
        // Make sure that if we selected a meta-particle system that it is loaded.
        TreeNode *node = GetSelectedNode();
        while (node != NULL)
        {
            MetaNode *metanode = dynamic_cast<MetaNode *>(node);
            if (metanode != 0)
            {
                metanode->EnsureLoaded();
                if (metanode->GetMetaParticleSystem() == NULL)
                {
                    AfxMessageBox
                    (
						Localise(L"RCS_IDS_BADPARTICLEFILE", metanode->GetFilename())
                    );
                }
                break;
            }
            node = node->GetParent();
        }

        if (result != NULL)
            *result = 0;
    }

    return FALSE; // allow any containing dialogs to process selections too
}

void PSTree::OnKeyDown(UINT key, UINT repcnt, UINT flags)
{
	BW_GUARD;

    if (key == VK_F2)
    {
        TreeNode *node = GetSelectedNode();
        EditNodeLabel(node);
    }
    else if (key == VK_DELETE)
    {
        CWnd *parent = CTreeCtrl::GetParent();
        HWND hmyself = *this;
        if (parent != NULL)
            parent->SendNotifyMessage(WM_DELETE_NODE, 0, (LPARAM)hmyself);
    }
    else
    {
        TreeControl::OnKeyDown(key, repcnt, flags);
    }
}

void PSTree::OnRButtonDown(UINT flags, CPoint point)
{
	BW_GUARD;

    TreeNode *hititem = HitTest(point);
    if (hititem != NULL)
    {
        SetSelectedNode(hititem);
        TreeNode *selitem = GetSelectedNode();
        // The user may have been prompted to save the current meta-particle
        // system and they said no.  In this case do not bring up the right
        // click menu.
        if (selitem == hititem)
        {
            CWnd *parent = CWnd::GetParent();
            if (parent != NULL)
                parent->SendMessage(WM_PSTREE_CMENU);
        }
    }
}

void PSTree::OnPaint()
{
	BW_GUARD;

    CPaintDC dc(this);

    // Draw into a back buffer:
    CDC memDC;
    memDC.CreateCompatibleDC(&dc);

	CRect rcClip, rcClient;
	dc.GetClipBox(&rcClip);
	GetClientRect(&rcClient);

	// Select a compatible bitmap into the memory DC
	CBitmap bitmap;
	bitmap.CreateCompatibleBitmap(&dc, rcClient.Width(), rcClient.Height());
	memDC.SelectObject(&bitmap);
	
	// Set clip region to be same as that in paint DC
	CRgn rgn;
	rgn.CreateRectRgnIndirect( &rcClip );
	memDC.SelectClipRgn(&rgn);
	rgn.DeleteObject();
	
	// First let the control do its default drawing.
	CWnd::DefWindowProc(WM_PAINT, (WPARAM)memDC.m_hDC, 0);

    // If we are editing the current label then it's been drawn correctly.
    CEdit *edit     = GetEditControl();
    CWnd  *focusWnd = CWnd::GetFocus();
    if (edit == NULL && focusWnd != this)
    {
        // Find the selected item and draw it as the solid selected colour:
	    HTREEITEM hitem = GetFirstVisibleItem();
	    int itemCnt = GetVisibleCount() + 1;
	    while (hitem != NULL && --itemCnt)
	    {	
            UINT selFlag = TVIS_DROPHILITED | TVIS_SELECTED;
            if ((GetItemState(hitem, selFlag) & selFlag) != 0)
            {
                CFont *font = GetFont();
                CFont *oldFont = memDC.SelectObject(font);
                memDC.SetTextColor(GetSysColor(COLOR_HIGHLIGHTTEXT));			
			    memDC.SetBkColor(GetSysColor(COLOR_HIGHLIGHT));
                CRect rect;
                GetItemRect(hitem, &rect, TRUE);
                CString itemTxt = GetItemText(hitem);
                memDC.FillSolidRect(rect, GetSysColor(COLOR_HIGHLIGHT));
			    memDC.TextOut(rect.left + 2, rect.top + 1, itemTxt);
                memDC.SelectObject(oldFont);
            }
            hitem = GetNextVisibleItem(hitem);
        }
    }

    // Blit the backbuffer:
	dc.BitBlt
    (
        rcClip.left, 
        rcClip.top, 
        rcClip.Width(), 
        rcClip.Height(), 
        &memDC, 
		rcClip.left, 
        rcClip.top, 
        SRCCOPY
    );

	memDC.DeleteDC();
}

BW_END_NAMESPACE

