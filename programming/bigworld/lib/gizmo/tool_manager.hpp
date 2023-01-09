#ifndef TOOL_MANAGER_HPP
#define TOOL_MANAGER_HPP

#include "tool.hpp"
#include <stack>

BW_BEGIN_NAMESPACE

class ToolManager
{
public:
	static ToolManager & instance();

	void pushTool( ToolPtr pNewTool );
	void popTool();
	ToolPtr tool();
	const ToolPtr tool() const;

	bool isToolApplying() const;

	void changeSpace( const Vector3& worldRay );

private:
	ToolManager();

	ToolManager( const ToolManager& );
	ToolManager& operator=( const ToolManager& );

	typedef BW::vector<ToolPtr>	ToolStack;
	ToolStack tools_;
	bool reentry_;
};

BW_END_NAMESPACE
#endif // TOOL_MANAGER_HPP
