#include "batch_slice.h"

namespace NKikimr::NOlap {


bool TBatchSerializedSlice::GroupBlobs(std::vector<TSplittedBlob>& blobs) {
    std::vector<TSplittedColumnChunk> chunksInProgress;
    std::vector<TSplittedColumnChunk> chunksReady;
    std::sort(Columns.begin(), Columns.end());
    for (auto&& i : Columns) {
        for (auto&& p : i.GetChunks()) {
            chunksInProgress.emplace_back(p);
        }
    }
    std::vector<TSplittedBlob> result;
    Y_VERIFY(Settings.GetMaxBlobSize() >= 2 * Settings.GetMinBlobSize());
    while (chunksInProgress.size()) {
        i64 fullSize = 0;
        for (auto&& i : chunksInProgress) {
            fullSize += i.GetSize();
        }
        if (fullSize < Settings.GetMaxBlobSize()) {
            result.emplace_back(TSplittedBlob());
            for (auto&& i : chunksInProgress) {
                result.back().Take(i);
            }
            chunksInProgress.clear();
            break;
        }
        bool hasNoSplitChanges = true;
        while (hasNoSplitChanges) {
            hasNoSplitChanges = false;
            i64 partSize = 0;
            for (ui32 i = 0; i < chunksInProgress.size(); ++i) {
                const i64 nextPartSize = partSize + chunksInProgress[i].GetSize();
                const i64 nextOtherSize = fullSize - nextPartSize;
                const i64 otherSize = fullSize - partSize;
                if (nextPartSize >= Settings.GetMaxBlobSize() || nextOtherSize < Settings.GetMinBlobSize()) {
                    Y_VERIFY(otherSize >= Settings.GetMinBlobSize());
                    Y_VERIFY(partSize < Settings.GetMaxBlobSize());
                    if (partSize >= Settings.GetMinBlobSize()) {
                        result.emplace_back(TSplittedBlob());
                        for (ui32 chunk = 0; chunk < i; ++chunk) {
                            result.back().Take(chunksInProgress[chunk]);
                        }
                        Counters->BySizeSplitter.OnCorrectSerialized(result.back().GetSize());
                        chunksInProgress.erase(chunksInProgress.begin(), chunksInProgress.begin() + i);
                        hasNoSplitChanges = true;
                    } else {
                        Y_VERIFY(chunksInProgress[i].GetSize() > Settings.GetMinBlobSize() - partSize);
                        Y_VERIFY(otherSize - (Settings.GetMinBlobSize() - partSize) >= Settings.GetMinBlobSize());
                        chunksInProgress[i].AddSplit(Settings.GetMinBlobSize() - partSize);

                        Counters->BySizeSplitter.OnTrashSerialized(chunksInProgress[i].GetSize());
                        std::vector<TSplittedColumnChunk> newChunks = chunksInProgress[i].InternalSplit(Schema->GetColumnSaver(chunksInProgress[i].GetColumnId()), Counters);
                        chunksInProgress.erase(chunksInProgress.begin() + i);
                        chunksInProgress.insert(chunksInProgress.begin() + i, newChunks.begin(), newChunks.end());

                        TSplittedBlob newBlob;
                        for (ui32 chunk = 0; chunk <= i; ++chunk) {
                            newBlob.Take(chunksInProgress[chunk]);
                        }
                        if (newBlob.GetSize() < Settings.GetMaxBlobSize()) {
                            chunksInProgress.erase(chunksInProgress.begin(), chunksInProgress.begin() + i + 1);
                            result.emplace_back(std::move(newBlob));
                            Counters->BySizeSplitter.OnCorrectSerialized(result.back().GetSize());
                        }
                    }
                    break;
                }
                partSize = nextPartSize;
            }
        }
    }
    std::swap(blobs, result);
    return true;
}

TBatchSerializedSlice::TBatchSerializedSlice(std::shared_ptr<arrow::RecordBatch> batch, ISchemaDetailInfo::TPtr schema, std::shared_ptr<NColumnShard::TSplitterCounters> counters, const TSplitSettings& settings)
    : Schema(schema)
    , Batch(batch)
    , Counters(counters)
    , Settings(settings)
{
    Y_VERIFY(batch);
    RecordsCount = batch->num_rows();
    Columns.reserve(batch->num_columns());
    for (auto&& i : batch->schema()->fields()) {
        TSplittedColumn c(i, schema->GetColumnId(i->name()));
        Columns.emplace_back(std::move(c));
    }

    ui32 idx = 0;
    for (auto&& i : batch->columns()) {
        auto& c = Columns[idx];
        auto columnSaver = schema->GetColumnSaver(c.GetColumnId());
        auto stats = schema->GetColumnSerializationStats(c.GetColumnId());
        TSimpleSplitter splitter(columnSaver, Counters);
        splitter.SetStats(stats);
        c.SetBlobs(splitter.Split(i, c.GetField(), Settings.GetMaxBlobSize()));
        Size += c.GetSize();
        ++idx;
    }
}

void TBatchSerializedSlice::MergeSlice(TBatchSerializedSlice&& slice) {
    Batch = NArrow::CombineBatches({Batch, slice.Batch});
    Y_VERIFY(Columns.size() == slice.Columns.size());
    RecordsCount += slice.GetRecordsCount();
    for (ui32 i = 0; i < Columns.size(); ++i) {
        Size += slice.Columns[i].GetSize();
        Columns[i].Merge(std::move(slice.Columns[i]));
    }
}

}
