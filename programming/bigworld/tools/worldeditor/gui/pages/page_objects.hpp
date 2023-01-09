#ifndef PAGE_OBJECTS_HPP
#define PAGE_OBJECTS_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "controls/edit_numeric.hpp"
#include "guitabs/guitabs_content.hpp"
#include "controls/auto_tooltip.hpp"
#include "controls/search_field.hpp"
#include <afxwin.h>

BW_BEGIN_NAMESPACE

class PageObjects : public CFormView, public GUITABS::Content
{
	IMPLEMENT_BASIC_CONTENT( Localise(L"WORLDEDITOR/GUI/PAGE_OBJECTS/SHORT_NAME"),
		Localise(L"WORLDEDITOR/GUI/PAGE_OBJECTS/LONG_NAME"), 290, 330, NULL )

	DECLARE_AUTO_TOOLTIP( PageObjects, CFormView )

public:
	PageObjects();
	virtual ~PageObjects();

// Dialog Data
	enum { IDD = IDD_PAGE_OBJECTS };

private:
	afx_msg LRESULT OnShowTooltip(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnHideTooltip(WPARAM wParam, LPARAM lParam);

private:
	bool resizeReady_;
	bool pageReady_;
	void InitPage();
	void stretchToRight( CWnd& widget, int pageWidth, int border );

	CComboBox selectionFilter_;

	CButton worldCoords_;
	CButton localCoords_;
	CButton viewCoords_;

	CButton freeSnap_;
	CButton terrainSnap_;
	CButton obstacleSnap_;

	CButton gridSnap_;
	controls::EditNumeric snapsX_;
	controls::EditNumeric snapsY_;
	controls::EditNumeric snapsZ_;
	controls::EditNumeric snapsAngle_;

	CComboBox placementMethod_;

	CButton dragOnSelect_;

	SearchField ctlEditSelByName_;
	SearchField ctlEditReplaceWith_;
	CButton ctlCheckFilterByName_;
	CButton ctlCheckSelectWholeFences_;
	CButton ctlBtnReplace_;

	BW::string lastCoordType_;
	int lastSnapType_;
	int lastDragOnSelect_;
	int lastGridSnap_;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg LRESULT OnActivateTool(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEditFieldClear( WPARAM wParam, LPARAM lParam );
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCbnSelchangeObjectsSelectionFilter();

	afx_msg void OnBnClickedObjectsCoordsWorld();
	afx_msg void OnBnClickedObjectsCoordsLocal();
	afx_msg void OnBnClickedObjectsCoordsView();

	afx_msg void OnBnClickedObjectsLockFree();
	afx_msg void OnBnClickedObjectsLockTerrain();
	afx_msg void OnBnClickedObjectsLockObstacle();

	afx_msg void OnBnClickedObjectsDragOnSelect();
	afx_msg void OnBnClickedObjectsLockToGrid();

	afx_msg void OnBnClickedOptionsShellsnaps();
	afx_msg void OnBnClickedOptionsUnitsnaps();
	afx_msg void OnBnClickedOptionsPointonesnaps();
	afx_msg LRESULT OnChangeEditNumeric(WPARAM mParam, LPARAM lParam);
	afx_msg void OnCbnSelchangeObjectsSelectPlacementSetting();
	afx_msg void OnBnClickedObjectsEditPlacementSettings();

	afx_msg void OnBnClickedSelectByName();
	afx_msg void OnBnClickedReplace();
	afx_msg void OnBnClickedSelectionToNameField();
	afx_msg void OnBnClickedSelectionToReplaceWith();
	afx_msg void OnBnClickedCheckboxFilterByName();
	afx_msg LRESULT OnEditFieldChanged( WPARAM wParam, LPARAM lParam );
	afx_msg void OnBnClickedCheckboxSelectWholeFences();

private:
	void updateSelectionFilter();
	void replaceSelectedItems(const BW::string& resourceID);
	BW::wstring getSelectedAssetName(bool inScene);
	bool useNameAsFilter_;

	class UndoRedoReplace : public UndoRedo::Operation
	{
	public:
		UndoRedoReplace(PageObjects *pOwner,
			const BW::vector<ChunkItemPtr>& itemsRemoved,
			const BW::vector<Chunk*>& itemsRemovedChunks,
			const BW::vector<ChunkItemPtr>& itemsAdded);

		void undo();

		bool iseq(UndoRedo::Operation const &other) const { return false; }

	private:
		BW::vector<ChunkItemPtr> itemsRemoved_;
		BW::vector<ChunkItemPtr> itemsAdded_;
		BW::vector<Chunk*> itemsRemovedChunks_;
		PageObjects *pOwner_;
	};

};

IMPLEMENT_BASIC_CONTENT_FACTORY( PageObjects )

BW_END_NAMESPACE

#endif // PAGE_OBJECTS_HPP
