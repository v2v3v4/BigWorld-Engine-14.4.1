#ifndef ASSET_PIPELINE_CONVERSION_TASK_QUEUE
#define ASSET_PIPELINE_CONVERSION_TASK_QUEUE

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_deque.hpp"

BW_BEGIN_NAMESPACE

struct ConversionTask;
/// double ended queue of conversion tasks
typedef BW::deque<ConversionTask*> ConversionTaskQueue;

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_CONVERSION_TASK_QUEUE
