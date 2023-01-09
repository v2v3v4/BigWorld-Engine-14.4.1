#ifndef BASE_PANEL_MANAGER_HPP
#define BASE_PANEL_MANAGER_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class PanelManagerFunctor;

class BasePanelManager
{
public:
	BasePanelManager();
	virtual ~BasePanelManager();

private:
	class PanelManagerFunctor * pFunctor_;
};

BW_END_NAMESPACE

#endif // PANEL_MANAGER_HPP
