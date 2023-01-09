#ifndef PAGE_TERRAIN_BASE_HPP
#define PAGE_TERRAIN_BASE_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "controls/auto_tooltip.hpp"

BW_BEGIN_NAMESPACE

class PageTerrainBase : public CDialog
{
public:
	PageTerrainBase(UINT nIDTemplate);

protected:
	BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
};

BW_END_NAMESPACE

#endif // PAGE_TERRAIN_BASE_HPP
