#include "pch.hpp"

#include "tools_common.hpp"

BW_BEGIN_NAMESPACE

/*static*/ bool ToolsCommon::isEval()
{
#ifdef BW_EVALUATION
	return true;
#else//BW_EVALUATION
	return false;
#endif//BW_EVALUATION
}
BW_END_NAMESPACE

