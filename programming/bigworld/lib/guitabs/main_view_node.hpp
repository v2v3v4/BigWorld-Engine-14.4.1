
#ifndef GUITABS_MAIN_VIEW_NODE_HPP
#define GUITABS_MAIN_VIEW_NODE_HPP

BW_BEGIN_NAMESPACE

namespace GUITABS
{


/**
 *  This class represents the main view window in the dock tree. It mantains a
 *	pointer to the actual main window in order to be able to return it when
 *  positioning it inside the appropriate splitter window. This class behaves
 *  as a leaf inside the dock tree.
 */
class MainViewNode : public DockNode
{
public:
	MainViewNode( CWnd* mainView );

	CWnd* getCWnd();

	bool load( DataSectionPtr section, CWnd* parent, int wndID );
	bool save( DataSectionPtr section );

private:
	CWnd* mainView_;
};


} // namespace
BW_END_NAMESPACE

#endif // GUITABS_MAIN_VIEW_NODE_HPP