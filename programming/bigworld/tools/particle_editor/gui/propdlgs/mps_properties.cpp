#include "pch.hpp"
#include "particle_editor.hpp"
#include "main_frame.hpp"
#include "gui/propdlgs/mps_properties.hpp"
#include "particle/meta_particle_system.hpp"
#include "common/editor_views.hpp"
#include "common/user_messages.hpp"
#include "shell/pe_module.hpp"
#include "appmgr/commentary.hpp"

DECLARE_DEBUG_COMPONENT2( "GUI", 0 )

BW_BEGIN_NAMESPACE

BEGIN_MESSAGE_MAP(MpsProperties, PropertyTable)
	ON_MESSAGE(WM_CHANGE_PROPERTYITEM, OnChangePropertyItem)
	ON_MESSAGE(WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI)
	ON_BN_CLICKED(IDC_CHECKBOX_DRAW_HELPER_MODEL, &MpsProperties::OnCbClickedDrawHelperModel)
	ON_BN_CLICKED(IDC_BTN_CHOOSE_HELPER_MODEL, &MpsProperties::OnBnClickedChooseHelperModel)
	ON_CBN_SELCHANGE(IDC_COMBO_CHOOSE_HELPER_MODEL_HARDPOINT, OnCbnSelchangeCenterOnHardpoint)
END_MESSAGE_MAP()

IMPLEMENT_DYNCREATE(MpsProperties, PropertyTable)

MpsProperties::MpsProperties(): 
	PropertyTable(MpsProperties::IDD),
	elected_( false )
{
}

MpsProperties::~MpsProperties()
{
	BW_GUARD;

	GeneralEditor::currentEditors( GeneralEditor::Editors() );

	PropTable::table( NULL );
}

void MpsProperties::DoDataExchange(CDataExchange* pDX)
{
    PropertyTable::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECKBOX_DRAW_HELPER_MODEL, ctlCheckDrawHelperModel_);
	DDX_Control(pDX, IDC_EDIT_HELPER_MODEL_NAME, ctlEditHelperModelName_);
	DDX_Control(pDX, IDC_COMBO_CHOOSE_HELPER_MODEL_HARDPOINT, ctlComboHelperModelHardpoint_);
}

MetaParticleSystemPtr MpsProperties::metaPS()
{
	BW_GUARD;

    return MainFrame::instance()->GetMetaParticleSystem();
}

void MpsProperties::OnInitialUpdate()
{
	BW_GUARD;

    PropertyTable::OnInitialUpdate();

	ctlComboHelperModelHardpoint_.ResetContent();
	ctlComboHelperModelHardpoint_.AddString(L"(none)");
	ctlEditHelperModelName_.SetWindowText(L"");
	ctlCheckDrawHelperModel_.SetCheck(BST_UNCHECKED);

	if ( !PeModule::instance().helperModelName().empty() )
	{
		ignoreControlChanges_ = true;
		
		BW::vector< BW::string > & hardPointNames = PeModule::instance().getHelperModelHardPointNames();

		for (uint i = 0; i < hardPointNames.size(); i++)
		{
			ctlComboHelperModelHardpoint_.AddString(bw_utf8tow(hardPointNames[i]).c_str());
		}

		ctlComboHelperModelHardpoint_.SetCurSel(PeModule::instance().helperModelCenterOnHardPoint() + 1);
		ctlEditHelperModelName_.SetWindowText(bw_utf8tow(PeModule::instance().helperModelName()).c_str());
		ctlCheckDrawHelperModel_.SetCheck(PeModule::instance().drawHelperModel() ? BST_CHECKED : BST_UNCHECKED);

		ignoreControlChanges_ = false;
	}
	INIT_AUTO_TOOLTIP();
}

afx_msg LRESULT MpsProperties::OnChangePropertyItem(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (lParam)
	{
		BaseView* relevantView = (BaseView*)lParam;
		bool transient = !!wParam;

		relevantView->onChange( transient );
	}

	return 0;
}

afx_msg LRESULT MpsProperties::OnIdleUpdateCmdUI(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	PropTable::table( this );

	if (metaPS())
	{
		if (!elected_)
		{
			editor_ = new GeneralEditor;
			GeneralEditor::Editors newEds;
			newEds.push_back( editor_ );
			GeneralEditor::currentEditors( newEds );

			metaPS()->metaData().edit( *editor_, "", false );
			editor_->elect();
			elected_ = true;
		}
	}
	else
	{
		if (elected_)
		{
			editor_ = NULL;
			GeneralEditor::Editors newEds;
			GeneralEditor::currentEditors( newEds );
			elected_ = false;
		}
	}

	if (elected_)
	{
		PropertyTable::update();
	}

	return 0;
}

void MpsProperties::OnCbClickedDrawHelperModel()
{
	BW_GUARD;

	PeModule::instance().drawHelperModel(ctlCheckDrawHelperModel_.GetCheck() == BST_CHECKED);
}

void MpsProperties::OnBnClickedChooseHelperModel()
{
	BW_GUARD;

	BW::string path;
	
	CString str;
	ctlEditHelperModelName_.GetWindowText(str);
	if (str.GetLength() == 0)
	{
		path = BWResource::resolveFilename( "models" );
	}
	else
	{
		path = bw_wtoutf8(str.GetString());
	}

	path = BWResolver::resolveFilename(path);
	std::replace(path.begin(), path.end(), '/', '\\');

	CFileDialog fileDlg(true, L".model", bw_utf8tow(path).c_str());
	OPENFILENAME &ofn = fileDlg.GetOFN();
	ofn.lpstrTitle = L"Choose a model file";
	ofn.lpstrFilter = L"Model Files (*.model)\0*.model\0\0";
	ofn.FlagsEx = OFN_EX_NOPLACESBAR;
	ofn.hwndOwner = AfxGetMainWnd()->m_hWnd;

	INT_PTR dlgResult = fileDlg.DoModal();
	if (dlgResult != IDOK)
		return;

	str = fileDlg.GetPathName();
	str.Replace(TCHAR('\\'), TCHAR('/'));
	path = bw_wtoutf8(str.GetString());
	path = BWResource::dissolveFilename(path);

	if (!PeModule::instance().helperModelName(path))
	{
		Commentary::instance().addMsg("Failed to load model from file: " + path, Commentary::ERROR_LEVEL);
		return;
	}

	ctlEditHelperModelName_.SetWindowText(bw_utf8tow(path).c_str());

	ignoreControlChanges_ = true;

	ctlComboHelperModelHardpoint_.ResetContent();
	ctlComboHelperModelHardpoint_.AddString(L"(none)");

	BW::vector<BW::string> &hardPointNames = PeModule::instance().getHelperModelHardPointNames();
	for (uint i = 0; i < hardPointNames.size(); i++)
	{
		ctlComboHelperModelHardpoint_.AddString(bw_utf8tow(hardPointNames[i]).c_str());
	}

	ignoreControlChanges_ = false;

	ctlComboHelperModelHardpoint_.SetCurSel(0);
}

void MpsProperties::OnCbnSelchangeCenterOnHardpoint()
{
	BW_GUARD;

	if (ignoreControlChanges_)
		return;

	uint idx = (uint)(-1);
	if (ctlComboHelperModelHardpoint_.GetCurSel() != -1)
		idx = (uint)(ctlComboHelperModelHardpoint_.GetCurSel() - 1);

	PeModule::instance().helperModelCenterOnHardPoint(idx);
}
BW_END_NAMESPACE

