#pragma once
#include "portion_info.h"
#include <ydb/library/accessor/accessor.h>
#include <ydb/core/tx/columnshard/splitter/chunks.h>
#include <ydb/core/tx/columnshard/blob.h>

namespace NKikimr::NOlap {

class TPortionInfoWithBlobs {
public:
    class TBlobInfo {
    private:
        using TBlobChunks = std::map<TChunkAddress, TSimpleOrderedColumnChunk>;
        YDB_READONLY(ui64, Size, 0);
        YDB_READONLY_DEF(TBlobChunks, Chunks);
        std::vector<const TSimpleOrderedColumnChunk*> ChunksOrdered;
        mutable std::optional<TString> ResultBlob;
        const TColumnRecord& AddChunk(TPortionInfoWithBlobs& owner, TOrderedColumnChunk&& chunk, const TIndexInfo& info);
        void RestoreChunk(const TPortionInfoWithBlobs& owner, TSimpleOrderedColumnChunk&& chunk);
    public:
        class TBuilder {
        private:
            TBlobInfo* OwnerBlob;
            TPortionInfoWithBlobs* OwnerPortion;
        public:
            TBuilder(TBlobInfo& blob, TPortionInfoWithBlobs& portion)
                : OwnerBlob(&blob)
                , OwnerPortion(&portion)
            {

            }
            const TColumnRecord& AddChunk(TOrderedColumnChunk&& chunk, const TIndexInfo& info) {
                return OwnerBlob->AddChunk(*OwnerPortion, std::move(chunk), info);
            }
            void RestoreChunk(TSimpleOrderedColumnChunk&& chunk) {
                OwnerBlob->RestoreChunk(*OwnerPortion, std::move(chunk));
            }
        };

        const TString& GetBlob() const {
            if (!ResultBlob) {
                TString result;
                result.reserve(Size);
                for (auto&& i : ChunksOrdered) {
                    result.append(i->GetData());
                }
                ResultBlob = std::move(result);
            }
            return *ResultBlob;
        }

        void RegisterBlobId(TPortionInfoWithBlobs& owner, const TUnifiedBlobId& blobId);
    };
private:
    TPortionInfo PortionInfo;
    YDB_READONLY_DEF(std::vector<TBlobInfo>, Blobs);
    mutable std::optional<std::shared_ptr<arrow::RecordBatch>> CachedBatch;

    explicit TPortionInfoWithBlobs(TPortionInfo&& portionInfo, std::optional<std::shared_ptr<arrow::RecordBatch>> batch = {})
        : PortionInfo(std::move(portionInfo))
        , CachedBatch(batch) {
    }

    explicit TPortionInfoWithBlobs(const TPortionInfo& portionInfo, std::optional<std::shared_ptr<arrow::RecordBatch>> batch = {})
        : PortionInfo(portionInfo)
        , CachedBatch(batch) {
    }

    void SetPortionInfo(const TPortionInfo& portionInfo) {
        PortionInfo = portionInfo;
    }

    TBlobInfo::TBuilder StartBlob() {
        Blobs.emplace_back(TBlobInfo());
        return TBlobInfo::TBuilder(Blobs.back(), *this);
    }

public:
    static std::vector<TPortionInfoWithBlobs> RestorePortions(const std::vector<TPortionInfo>& portions, const THashMap<TBlobRange, TString>& blobs);
    static TPortionInfoWithBlobs RestorePortion(const TPortionInfo& portions, const THashMap<TBlobRange, TString>& blobs);

    std::shared_ptr<arrow::RecordBatch> GetBatch(const ISnapshotSchema& data, const ISnapshotSchema& result) const;

    ui64 GetSize() const {
        return PortionInfo.BlobsBytes();
    }

    static TPortionInfoWithBlobs BuildByBlobs(std::vector<std::vector<TOrderedColumnChunk>>& chunksByBlobs, std::shared_ptr<arrow::RecordBatch> batch,
        const ui64 granule, const TSnapshot& snapshot, const TIndexInfo& info);

    std::optional<TPortionInfoWithBlobs> ChangeSaver(ISnapshotSchema::TPtr currentSchema, const TSaverContext& saverContext) const;

    const TString& GetBlobByRangeVerified(const ui32 columnId, const ui32 chunkId) const {
        for (auto&& b : Blobs) {
            auto it = b.GetChunks().find(TChunkAddress(columnId, chunkId));
            if (it == b.GetChunks().end()) {
                continue;
            } else {
                return it->second.GetData();
            }
        }
        Y_VERIFY(false);
    }

    ui64 GetBlobFullSizeVerified(const ui32 columnId, const ui32 chunkId) const {
        for (auto&& b : Blobs) {
            auto it = b.GetChunks().find(TChunkAddress(columnId, chunkId));
            if (it == b.GetChunks().end()) {
                continue;
            } else {
                return b.GetSize();
            }
        }
        Y_VERIFY(false);
    }

    std::vector<TBlobInfo>& GetBlobs() {
        return Blobs;
    }

    TString DebugString() const {
        return TStringBuilder() << PortionInfo.DebugString() << ";blobs_count=" << Blobs.size() << ";";
    }

    const TPortionInfo& GetPortionInfo() const {
        return PortionInfo;
    }

    TPortionInfo& GetPortionInfo() {
        return PortionInfo;
    }

    friend IOutputStream& operator << (IOutputStream& out, const TPortionInfoWithBlobs& info) {
        out << info.DebugString();
        return out;
    }
};

} // namespace NKikimr::NOlap
