
#ifndef GUITABS_DOCKED_PANEL_NODE_HPP
#define GUITABS_DOCKED_PANEL_NODE_HPP

BW_BEGIN_NAMESPACE

namespace GUITABS
{


/**
 *  This class represents a docked panel in the dock tree. It mantains a
 *	pointer to the actual panel in order to be able to return it's CWnd
 *  pointer, which will permit its correct positioning it inside the
 *  appropriate splitter window. This class behaves as a leaf inside the dock
 *  tree.
 */
class DockedPanelNode : public DockNode
{
public:
	DockedPanelNode();
	DockedPanelNode( PanelPtr dockedPanel );

	void init( PanelPtr dockedPanel );

	CWnd* getCWnd();

	bool load( DataSectionPtr section, CWnd* parent, int wndID );
	bool save( DataSectionPtr section );

	void getPreferredSize( int& w, int& h );

	bool isExpanded();

private:
	PanelPtr dockedPanel_;
};


} // namespace
BW_END_NAMESPACE

#endif // GUITABS_DOCKED_PANEL_NODE_HPP