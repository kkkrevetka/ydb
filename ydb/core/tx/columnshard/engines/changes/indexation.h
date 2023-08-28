#pragma once
#include "abstract/abstract.h"
#include "with_appended.h"
#include <ydb/core/tx/columnshard/engines/insert_table/data.h>
#include <util/generic/hash.h>

namespace NKikimr::NOlap {

class TInsertColumnEngineChanges: public TChangesWithAppend {
private:
    using TBase = TChangesWithAppend;
    std::shared_ptr<arrow::RecordBatch> AddSpecials(const std::shared_ptr<arrow::RecordBatch>& srcBatch,
        const TIndexInfo& indexInfo, const TInsertedData& inserted) const;
    std::vector<NOlap::TInsertedData> DataToIndex;

protected:
    virtual void DoStart(NColumnShard::TColumnShard& self) override;
    virtual void DoWriteIndexComplete(NColumnShard::TColumnShard& self, TWriteIndexCompleteContext& context) override;
    virtual bool DoApplyChanges(TColumnEngineForLogs& self, TApplyChangesContext& context) override;
    virtual void DoOnFinish(NColumnShard::TColumnShard& self, TChangesFinishContext& context) override;
    virtual void DoWriteIndex(NColumnShard::TColumnShard& self, TWriteIndexContext& context) override;
    virtual TConclusionStatus DoConstructBlobs(TConstructionContext& context) noexcept override;
    virtual NColumnShard::ECumulativeCounters GetCounterIndex(const bool isSuccess) const override;
public:
    const TMark DefaultMark;
    THashMap<ui64, std::vector<std::pair<TMark, ui64>>> PathToGranule; // pathId -> {mark, granule}
    THashMap<TUnifiedBlobId, std::shared_ptr<arrow::RecordBatch>> CachedBlobs;
public:
    TInsertColumnEngineChanges(const TMark& defaultMark, std::vector<NOlap::TInsertedData>&& dataToIndex, const TSplitSettings& splitSettings)
        : TBase(splitSettings)
        , DataToIndex(std::move(dataToIndex))
        , DefaultMark(defaultMark) {

    }

    const std::vector<NOlap::TInsertedData>& GetDataToIndex() const {
        return DataToIndex;
    }

    virtual THashSet<ui64> GetTouchedGranules() const override {
        THashSet<ui64> result = TBase::GetTouchedGranules();
        for (auto&& i : PathToGranule) {
            for (auto&& g : i.second) {
                result.emplace(g.second);
            }
        }
        return result;
    }

    virtual THashMap<TUnifiedBlobId, std::vector<TBlobRange>> GetGroupedBlobRanges() const override;
    virtual TString TypeString() const override {
        return "INSERT";
    }
    bool AddPathIfNotExists(ui64 pathId);

};

}
