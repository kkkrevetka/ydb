#include "read_context.h"
#include "read_metadata.h"
#include <library/cpp/actors/core/events.h>

namespace NKikimr::NOlap {

TReadContext::TReadContext(const NColumnShard::TDataTasksProcessorContainer& processor,
    const NColumnShard::TConcreteScanCounters& counters,
    std::shared_ptr<NOlap::TActorBasedMemoryAccesor> memoryAccessor, const bool isInternalRead)
    : Processor(processor)
    , Counters(counters)
    , MemoryAccessor(memoryAccessor)
    , IsInternalRead(isInternalRead)
{

}

void TActorBasedMemoryAccesor::DoOnBufferReady() {
    OwnerId.Send(OwnerId, new NActors::TEvents::TEvWakeup(1));
}


IDataReader::IDataReader(const TReadContext& context, NOlap::TReadMetadata::TConstPtr readMetadata)
    : Context(context)
    , ReadMetadata(readMetadata)
{
    Y_VERIFY(ReadMetadata);
    Y_VERIFY(ReadMetadata->SelectInfo);
}

}
