#include "pch.hpp"
#include "worldeditor/gui/pages/page_terrain_base.hpp"


DECLARE_DEBUG_COMPONENT2("WorldEditor", 2)

BW_BEGIN_NAMESPACE

BEGIN_MESSAGE_MAP(PageTerrainBase, CDialog)
END_MESSAGE_MAP()


PageTerrainBase::PageTerrainBase(UINT nIDTemplate):
    CDialog(nIDTemplate)
{
}


BOOL PageTerrainBase::OnInitDialog()
{
	BW_GUARD;

	BOOL ret = CDialog::OnInitDialog();
	return ret;
}

BW_END_NAMESPACE

