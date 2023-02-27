#pragma once

#include <ydb/core/kqp/query_data/kqp_prepared_query.h>
#include <ydb/library/yql/public/udf/udf_data_type.h>
#include <ydb/library/yql/minikql/mkql_node.h>
#include <ydb/library/yql/minikql/computation/mkql_computation_node_holders.h>
#include <ydb/library/mkql_proto/protos/minikql.pb.h>

#include <library/cpp/random_provider/random_provider.h>
#include <library/cpp/time_provider/time_provider.h>

#include <util/generic/ptr.h>
#include <util/generic/guid.h>
#include <google/protobuf/arena.h>

#include <unordered_map>
#include <vector>

namespace NKqpProto {
class TKqpPhyParamBinding;
}

namespace Ydb {
class TypedValue;
}

namespace NYql {
namespace NDqProto {
class TData;
}
}

namespace NKikimr::NKqp {

using TTypedUnboxedValue = std::pair<NKikimr::NMiniKQL::TType*, NUdf::TUnboxedValue>;
using TNamedUnboxedValue = std::pair<const TString, TTypedUnboxedValue>;

using TUnboxedParamsMap = std::unordered_map<
    TString,
    TTypedUnboxedValue,
    std::hash<TString>,
    std::equal_to<TString>,
    NKikimr::NMiniKQL::TMKQLAllocator<TNamedUnboxedValue>
>;

using TTypedUnboxedValueVector = std::vector<
    TTypedUnboxedValue,
    NKikimr::NMiniKQL::TMKQLAllocator<TTypedUnboxedValue>
>;

using TTxResultVector = std::vector<
    TTypedUnboxedValueVector,
    NKikimr::NMiniKQL::TMKQLAllocator<TTypedUnboxedValueVector>
>;

struct TKqpExecuterTxResult {
    bool IsStream = true;
    NKikimr::NMiniKQL::TType* MkqlItemType;
    const TVector<ui32>* ColumnOrder = nullptr;
    NKikimr::NMiniKQL::TUnboxedValueVector Rows;

    explicit TKqpExecuterTxResult(
        bool isStream,
        NKikimr::NMiniKQL::TType* mkqlItemType,
        const TVector<ui32>* сolumnOrder = nullptr)
        : IsStream(isStream)
        , MkqlItemType(mkqlItemType)
        , ColumnOrder(сolumnOrder)
    {}

    TTypedUnboxedValue GetUV(const NKikimr::NMiniKQL::TTypeEnvironment& typeEnv,
        const NKikimr::NMiniKQL::THolderFactory& factory);
    NKikimrMiniKQL::TResult* GetMkql(google::protobuf::Arena* arena);
    NKikimrMiniKQL::TResult GetMkql();
    void FillMkql(NKikimrMiniKQL::TResult* mkqlResult);
};

struct TTimeAndRandomProvider {
    TIntrusivePtr<ITimeProvider> TimeProvider;
    TIntrusivePtr<IRandomProvider> RandomProvider;

    std::optional<ui64> CachedNow;
    std::tuple<std::optional<ui64>, std::optional<double>, std::optional<TGUID>> CachedRandom;

    ui64 GetCachedNow() {
        if (!CachedNow) {
            CachedNow = TimeProvider->Now().GetValue();
        }

        return *CachedNow;
    }

    ui64 GetCachedDate() {
        return std::min<ui64>(NYql::NUdf::MAX_DATE - 1u, GetCachedNow() / 86400000000ul);
    }

    ui64 GetCachedDatetime() {
        return std::min<ui64>(NYql::NUdf::MAX_DATETIME - 1u, GetCachedNow() / 1000000ul);
    }

    ui64 GetCachedTimestamp() {
        return std::min<ui64>(NYql::NUdf::MAX_TIMESTAMP - 1u, GetCachedNow());
    }

    template <typename T>
    T GetRandom() const {
        if constexpr (std::is_same_v<T, double>) {
            return RandomProvider->GenRandReal2();
        }
        if constexpr (std::is_same_v<T, ui64>) {
            return RandomProvider->GenRand64();
        }
        if constexpr (std::is_same_v<T, TGUID>) {
            return RandomProvider->GenUuid4();
        }
    }

    template <typename T>
    T GetCachedRandom() {
        auto& cached = std::get<std::optional<T>>(CachedRandom);
        if (!cached) {
            cached = GetRandom<T>();
        }

        return *cached;
    }

    void Reset() {
        CachedNow.reset();
        std::get<0>(CachedRandom).reset();
        std::get<1>(CachedRandom).reset();
        std::get<2>(CachedRandom).reset();
    }
};

class TTxAllocatorState: public TTimeAndRandomProvider {
public:
    NKikimr::NMiniKQL::TScopedAlloc Alloc;
    NKikimr::NMiniKQL::TTypeEnvironment TypeEnv;
    NKikimr::NMiniKQL::TMemoryUsageInfo MemInfo;
    NKikimr::NMiniKQL::THolderFactory HolderFactory;

    using TPtr = std::shared_ptr<TTxAllocatorState>;

    TTxAllocatorState(const NKikimr::NMiniKQL::IFunctionRegistry* functionRegistry,
        TIntrusivePtr<ITimeProvider> timeProvider, TIntrusivePtr<IRandomProvider> randomProvider);
    ~TTxAllocatorState();
    std::pair<NKikimr::NMiniKQL::TType*, NUdf::TUnboxedValue> GetInternalBindingValue(const NKqpProto::TKqpPhyParamBinding& paramBinding);
};

class TQueryData {
private:
    using TTypedUnboxedValue = std::pair<NKikimr::NMiniKQL::TType*, NUdf::TUnboxedValue>;
    using TNamedUnboxedValue = std::pair<const TString, TTypedUnboxedValue>;

    using TParamMap = std::unordered_map<
        TString,
        NKikimrMiniKQL::TParams
    >;

    TParamMap Params;
    TUnboxedParamsMap UnboxedData;
    TVector<TVector<TKqpExecuterTxResult>> TxResults;
    TVector<TVector<TKqpPhyTxHolder::TConstPtr>> TxHolders;
    TTxAllocatorState::TPtr AllocState;

public:
    using TPtr = std::shared_ptr<TQueryData>;

    TQueryData(const NKikimr::NMiniKQL::IFunctionRegistry* functionRegistry,
        TIntrusivePtr<ITimeProvider> timeProvider, TIntrusivePtr<IRandomProvider> randomProvider);
    TQueryData(TTxAllocatorState::TPtr allocatorState);
    ~TQueryData();

    const TParamMap& GetParams();

    const NKikimr::NMiniKQL::TTypeEnvironment& TypeEnv();

    TTxAllocatorState::TPtr GetAllocState() { return AllocState; }
    NKikimr::NMiniKQL::TType* GetParameterType(const TString& name);
    bool AddUVParam(const TString& name, NKikimr::NMiniKQL::TType* type, const NUdf::TUnboxedValue& value);
    bool AddMkqlParam(const TString& name, const NKikimrMiniKQL::TType& t, const NKikimrMiniKQL::TValue& v);
    bool AddTypedValueParam(const TString& name, const Ydb::TypedValue& p);

    bool MaterializeParamValue(bool ensure, const NKqpProto::TKqpPhyParamBinding& paramBinding);
    void AddTxResults(TVector<NKikimr::NKqp::TKqpExecuterTxResult>&& results);
    void AddTxHolders(TVector<TKqpPhyTxHolder::TConstPtr>&& holders);

    bool HasResult(ui32 txIndex, ui32 resultIndex) {
        if (txIndex >= TxResults.size())
            return false;

        return resultIndex < TxResults[txIndex].size();
    }

    TTypedUnboxedValue GetTxResult(ui32 txIndex, ui32 resultIndex);
    NKikimrMiniKQL::TResult* GetMkqlTxResult(ui32 txIndex, ui32 resultIndex, google::protobuf::Arena* arena);

    std::pair<NKikimr::NMiniKQL::TType*, NUdf::TUnboxedValue> GetInternalBindingValue(const NKqpProto::TKqpPhyParamBinding& paramBinding);
    TTypedUnboxedValue& GetParameterUnboxedValue(const TString& name);
    const NKikimrMiniKQL::TParams* GetParameterMiniKqlValue(const TString& name);
    NYql::NDqProto::TData SerializeParamValue(const TString& name);
    void Clear();
};

} // namespace NKikimr::NKqp