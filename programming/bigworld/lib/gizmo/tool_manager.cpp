#include "pch.hpp"
#include "tool_manager.hpp"

DECLARE_DEBUG_COMPONENT2( "Tool", 2 );

BW_BEGIN_NAMESPACE

extern int ChunkItemGroup_token;
extern int ChunkItemView_token;
extern int Tool_token;
static int s_toolTokenSet = ChunkItemGroup_token |
ChunkItemView_token |
Tool_token;

extern int DynamicFloatDevice_token;
extern int MatrixMover_token;
extern int MatrixPositioner_token;
extern int MatrixRotator_token;
extern int MatrixScaler_token;
extern int WheelRotator_token;

static int s_toolFunctorTokenSet = DynamicFloatDevice_token |
MatrixMover_token |
MatrixPositioner_token |
MatrixRotator_token |
MatrixScaler_token |
WheelRotator_token;

extern int AdaptivePlaneToolLocator_token;
extern int LineToolLocator_token;
extern int OriginLocator_token;
extern int PlaneChunkLocator_token;
extern int PlaneToolLocator_token;
extern int TriPlaneToolLocator_token;
static int s_toolLocatorTokenSet = AdaptivePlaneToolLocator_token |
LineToolLocator_token |
OriginLocator_token |
PlaneChunkLocator_token |
PlaneToolLocator_token |
TriPlaneToolLocator_token;

extern int FloatVisualiser_token;
extern int MeshToolView_token;
extern int ModelToolView_token;
extern int TeeView_token;
extern int Vector2Visualiser_token;
static int s_toolViewTokenSet = FloatVisualiser_token |
MeshToolView_token |
ModelToolView_token |
TeeView_token |
Vector2Visualiser_token;

ToolManager::ToolManager() : reentry_( false )
{
}

ToolManager & ToolManager::instance()
{
	static ToolManager s_instance;

	return s_instance;
}

void ToolManager::pushTool( ToolPtr pNewTool )
{
	BW_GUARD;

	// Can't push while pushing
	MF_ASSERT( !reentry_ );
	reentry_ = true;

	MF_ASSERT( pNewTool.exists() );
	MF_ASSERT( !pNewTool->applying() );

	// Notify current tool that it is no longer being used
	ToolPtr pCurrentTool = this->tool();
	if (pCurrentTool.exists())
	{
		pCurrentTool->onEndUsing();
		MF_ASSERT( !pCurrentTool->applying() );
	}

	// Add new tool
	tools_.push_back( pNewTool );

	// Notify new tool
	pNewTool->onPush();
	pNewTool->onBeginUsing();

	reentry_ = false;
}


void ToolManager::popTool()
{
	BW_GUARD;

	// Can't pop while popping
	MF_ASSERT( !reentry_ );
	reentry_ = true;

	ToolPtr pOldTool = this->tool();
	if (pOldTool.exists())
	{	
		// Notify old tool
		pOldTool->onEndUsing();
		pOldTool->onPop();

		// Should have stopped applying
		MF_ASSERT( !pOldTool->applying() );

		// Remove old tool
		tools_.pop_back();

		// Notify new tool
		ToolPtr pCurrentTool = this->tool();
		if (pCurrentTool.exists())
		{
			MF_ASSERT( !pCurrentTool->applying() );
			pCurrentTool->onBeginUsing();
		}
	}

	reentry_ = false;
}

ToolPtr ToolManager::tool()
{
	BW_GUARD;

	if ( !tools_.empty() )
	{
		return (*tools_.rbegin());
	}

	return NULL;
}

const ToolPtr ToolManager::tool() const
{
	BW_GUARD;

	if ( !tools_.empty() )
	{
		return (*tools_.rbegin());
	}

	return NULL;
}


bool ToolManager::isToolApplying() const
{
	BW_GUARD;
	const ToolPtr pTool = this->tool();
	return (pTool.exists() && pTool->applying());
}


void ToolManager::changeSpace( const Vector3& worldRay )
{
	BW_GUARD;

	ToolStack ts = tools_;
	for( ToolStack::iterator iter = ts.begin(); iter != ts.end(); ++iter )
	{
		(*iter)->calculatePosition( worldRay );
		(*iter)->update( 0.f );
	}
}
BW_END_NAMESPACE

