#pragma once
#include "abstract/abstract.h"

namespace NKikimr::NOlap {

class TCleanupColumnEngineChanges: public TColumnEngineChanges {
private:
    THashMap<TString, THashSet<NOlap::TEvictedBlob>> BlobsToForget;
protected:
    virtual void DoStart(NColumnShard::TColumnShard& self) override;
    virtual void DoOnFinish(NColumnShard::TColumnShard& self, TChangesFinishContext& context) override;
    virtual bool DoApplyChanges(TColumnEngineForLogs& self, TApplyChangesContext& context) override;
    virtual void DoDebugString(TStringOutput& out) const override;
    virtual void DoWriteIndexComplete(NColumnShard::TColumnShard& self, TWriteIndexCompleteContext& context) override;
    virtual void DoWriteIndex(NColumnShard::TColumnShard& self, TWriteIndexContext& context) override;
    virtual void DoCompile(TFinalizationContext& /*context*/) override {
    }
    virtual TConclusionStatus DoConstructBlobs(TConstructionContext& /*context*/) noexcept override {
        return TConclusionStatus::Success();
    }
    virtual bool NeedConstruction() const override {
        return false;
    }
    virtual NColumnShard::ECumulativeCounters GetCounterIndex(const bool isSuccess) const override;
public:
    virtual THashSet<ui64> GetTouchedGranules() const override {
        THashSet<ui64> result;
        for (const auto& portionInfo : PortionsToDrop) {
            result.emplace(portionInfo.GetGranule());
        }
        return result;
    }

    std::vector<TPortionInfo> PortionsToDrop;
    bool NeedRepeat = false;

    virtual THashMap<TUnifiedBlobId, std::vector<TBlobRange>> GetGroupedBlobRanges() const override {
        return {};
    }

    virtual ui32 GetWritePortionsCount() const override {
        return 0;
    }
    virtual TPortionInfoWithBlobs* GetWritePortionInfo(const ui32 /*index*/) override {
        return nullptr;
    }
    virtual bool NeedWritePortion(const ui32 /*index*/) const override {
        return true;
    }

    virtual TString TypeString() const override {
        return "CLEANUP";
    }
};

}
