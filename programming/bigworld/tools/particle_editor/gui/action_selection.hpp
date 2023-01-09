#ifndef ACTIONSELECTION_HPP
#define ACTIONSELECTION_HPP

#include "../resource.h"
#include "ps_tree.hpp"
#include "fwd.hpp"
#include "gui/controls/drag_listbox.hpp"
#include "gui/controls/drop_target.hpp"
#include "controls/dialog_toolbar.hpp"
#include "controls/color_static.hpp"
#include "controls/image_button.hpp"
#include "controls/csearch_filter.hpp"
#include "controls/auto_tooltip.hpp"
#include "guitabs/guitabs_content.hpp"
#include "gizmo/gizmo_manager.hpp"
#include "gizmo/general_properties.hpp"
#include "resmgr/string_provider.hpp"

BW_BEGIN_NAMESPACE

//
// This is the main dialog of Particle Editor.
//
class ActionSelection : public CDialog, 
                        public GUITABS::Content,
                        public IDropTargetObj
{
	DECLARE_DYNCREATE(ActionSelection)

    IMPLEMENT_BASIC_CONTENT
    (
        Localise(L"PARTICLEEDITOR/GUI/ACTION_SELECTION/SHORT_NAME"),              // short name
        Localise(L"PARTICLEEDITOR/GUI/ACTION_SELECTION/LONG_NAME"),              // long name
        372,                    // default width
        550,                    // default height
        NULL                    // icon
    )

public:
    enum { IDD = IDD_ACTION_SELECTION };

    ActionSelection();

    /*virtual*/ ~ActionSelection();

    static ActionSelection *instance();

    /*virtual*/ BOOL OnInitDialog();

    bool isMetaParticleSystemSelected() const;

    MetaParticleSystemPtr getMetaParticleSystem();

    ParticleSystemPtr getCurrentParticleSystem();

    ParticleSystemActionPtr getSelectedAction();

    bool selectMetaParticleSystem(BW::string const &name);

	bool isSelectionReadOnly() const;

    bool save();

    void onNotSave();

    bool reload();    

    void reserialise(DataSectionPtr data, bool load, bool transient, bool undoing = true);

    void saveState(DataSectionPtr data);

    void restoreState(DataSectionPtr data);

    void setSubDlg(CView *subdlg);

    void addSystemOffsetGizmo();

    void removeSystemOffsetGizmo();

    void getChildDlgRect(CRect &rect) const;

    void appendOneShotPS();

    void clearAppendedPS();

    size_t numberAppendPS() const;

    MetaParticleSystem &getAppendedPS(size_t idx);

    void cleanupAppendPS();

	void refreshSelection();
protected:
    void initComponentList();
        
    /*virtual*/ void DoDataExchange(CDataExchange *dx);

    /*virtual*/ BOOL PreTranslateMessage(MSG *msg);

    afx_msg void OnPartTreeSel(NMHDR *nmhdr, LRESULT *result);

	void setSelectedMetaNode(TreeNode *newsel);

    bool updateSelection(TreeNode *newsel);

    afx_msg void OnPartTreeEditLabel(NMHDR *nmhdr, LRESULT *result);

	afx_msg void OnNMClickParticletree(NMHDR *nmhdr, LRESULT *result);

    void OnSelectComponent(TreeNode *node);

    afx_msg void OnAdd();

    afx_msg void OnCopy();

    afx_msg void OnDelete();

    afx_msg void OnAddAction();

    afx_msg void OnOpen();

    afx_msg void OnSave();

    afx_msg void OnOpenInExplorer();

    afx_msg void OnCopyPath();

    afx_msg void OnAddEnabled(CCmdUI *cmdui);

    afx_msg void OnCopyEnabled(CCmdUI *cmdui);

    afx_msg void OnDeleteEnabled(CCmdUI *cmdui);

    afx_msg void OnAddActionEnabled(CCmdUI *cmdui);

    afx_msg void OnEditFilter();

    afx_msg void OnSize(UINT type, int cx, int cy);

    afx_msg void OnClose();

    afx_msg LRESULT OnNodeDelete(WPARAM wparam, LPARAM lparam);

	afx_msg LRESULT OnClosing(WPARAM wparam, LPARAM lparam);

    BW::string 
    generateUniquePSName
    (
        MetaParticleSystemPtr   ms,
        BW::string             const &name,
        bool                    nameIsNew
    ) const;

    BW::string generateUniqueActionName(PSNode *ps, int id) const;

    BW::string 
    generateUniqueMetaSystemName
    (
        BW::string         const &name,
        bool                nameisNew,
        MetaNode            const *ignoreMN   = NULL
    ) const;

    ParticleSystemPtr generateNewParticleSystem() const;

    /*virtual*/ 
    DROPEFFECT
    OnDragEnter
    (
        CWnd                *window,
        COleDataObject      *dataObject,
        DWORD               keyState,
        CPoint              point
    );

	/*virtual*/
	void OnDragLeave(CWnd *window);

    /*virtual*/ 
    DROPEFFECT
    OnDragOver
    (
        CWnd                *window,
        COleDataObject      *dataObject,
        DWORD               keyState,
        CPoint              point
    );

    DROPEFFECT 
    OnDragTest
    (
        CWnd                *window,
        COleDataObject      *dataObject,
        DWORD               keyState,
        CPoint              point
    );

    DROPEFFECT 
    OnDragTestAction
    (
        CWnd                *window,
        COleDataObject      *dataObject,
        DWORD               keyState,
        CPoint              point
    );

    DROPEFFECT 
    OnDragTestTree
    (
        CWnd                *window,
        COleDataObject      *dataObject,
        DWORD               keyState,
        CPoint              point
    );

    /*virtual*/
    BOOL
    OnDrop
    (
        CWnd                *window,
        COleDataObject      *dataObject,
        DROPEFFECT          dropEffect,
        CPoint              point
    );

    /*virtual*/
    BOOL
    OnDropAction
    (
        CWnd                *window,
        COleDataObject      *dataObject,
        DROPEFFECT          dropEffect,
        CPoint              point
    );

    /*virtual*/
    BOOL
    OnDropTree
    (
        CWnd                *window,
        COleDataObject      *dataObject,
        DROPEFFECT          dropEffect,
        CPoint              point
    );

    void
    copySystemProperties
    (
        ParticleSystemPtr   source,
        ParticleSystemPtr   dest
    );

    void
    copyRendererProperties
    (
        ParticleSystemPtr   source,
        ParticleSystemPtr   dest
    );

    afx_msg LRESULT OnDragStart(WPARAM wparam, LPARAM lparam);

    afx_msg LRESULT OnDragDone(WPARAM wparam, LPARAM lparam);

    afx_msg LRESULT OnPartTreeMenu(WPARAM wparam, LPARAM lparam);

    afx_msg BOOL OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT *result);

    DECLARE_MESSAGE_MAP()

    DECLARE_AUTO_TOOLTIP_EX(ActionSelection)

    void removeTreeNode(TreeNode *tn, bool delFile);

    MetaNode * 
    addMetaParticleSystem
    (
        MetaParticleSystemPtr   mps,
        MetaNode                *prevSibling    = NULL,
        bool                    edit            = true,
        bool                    isNew           = true,
        BW::string             const &name     = "metasystem",
        bool                    keepName        = false,
		bool					readOnly		= false
    );

    PSNode *
    addParticleSystem
    (
        ParticleSystemPtr       ps,
        MetaNode                *parent,
        PSNode                  *prevSibling    = NULL,
        bool                    edit            = true,
        bool                    isNew           = true,
        BW::string             const &name     = "system"
    );

    ActionNode *
    addAction
    (
        ParticleSystemActionPtr  psa,
        PSNode                  *parent,
        ActionNode              *prevSibling    = NULL
    );

    BW::string directory() const;

    ActionNode *getActionInsert(ActionNode *anode) const;

    bool canInsertAction(PSNode const *node, int actionID) const;

    bool dropParticleSystem(UalItemInfo *ii);

    enum RenameResult
    {
        NAME_OK,
        NAME_INVALID_CHAR,
        NAME_INVALID_CHAR_INC_SPACE,
        NAME_NOT_UNIQUE
    };

    RenameResult checkUniqueName(TreeNode const *node, BW::string const &label) const;

    static void 
    fixupAction
    (
        ParticleSystemActionPtr newAction,
        ParticleSystemPtr       parent
    );

    static MetaNode *getMetaNode(TreeNode *node);

private:
    controls::CSearchFilter searchFilter_;          // The search filter control.
    PSTree                  particleTree_;          // Particle tree control.
    DragListBox             componentList_;         // Component list control.
    controls::DialogToolbar actionBar_;             // The action toolbar.
    controls::ImageButton   addActBtn_;             // Button for adding an action.
    controls::ImageButton   saveBtn_;               // Button for saving.
    controls::ImageButton   openBtn_;               // Button for directory open.
    CView	               *subDlg_;               // Component dialog.
    GizmoPtr                sysOffsGizmo_;          // System offset Gizmo
    GizmoPtr                sysPosGizmo_;           // System position Gizmo
    DropTarget              dropTarget_;            // Drop target for particle tree.
    TreeNode                *selection_;            // The selected metanode.
    bool                    filterPromptSave_;      // Filter prompting of saving
    int                     pauseState_;            // State of app before dragging.
    size_t                  filterFocusSelection_;  // Filter set focus of part. selection.
    BW::vector<MetaParticleSystemPtr> appendedPS_; // Appended one shot particle systems.
    static ActionSelection  *s_instance_;
};

IMPLEMENT_CDIALOG_CONTENT_FACTORY(ActionSelection, IDD_ACTION_SELECTION)

BW_END_NAMESPACE

#endif // ACTIONSELECTION_HPP
