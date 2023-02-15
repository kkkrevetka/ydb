#pragma once

#include <util/datetime/base.h>


namespace NKikimr::NPQ {
    class TQuotaTracker {
    public:
        TQuotaTracker(const ui64 maxBurst, const ui64 speedPerSecond, const TInstant timestamp);

        void UpdateConfig(const ui64 maxBurst, const ui64 speedPerSecond);
        void Update(const TInstant timestamp);

        bool CanExaust() const;
        void Exaust(const ui64 size, const TInstant timestamp);

        ui64 GetQuotedTime() const;
        ui64 GetTotalSpeed() const;

    private:
        i64 AvailableSize;
        ui64 SpeedPerSecond;
        TInstant LastUpdateTime;
        ui64 MaxBurst;

        ui64 QuotedTime;
    };

} // NKikimr::NPQ