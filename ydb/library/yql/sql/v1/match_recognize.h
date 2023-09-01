#pragma once
#include "node.h"
#include <ydb/library/yql/core/sql_types/match_recognize.h>
#include <util/generic/ptr.h>

namespace NSQLTranslationV1 {

struct TPartitioner {
    TNodePtr callable; // a callable with one free variable: row
    TString name;
};

struct TNamedLambda {
    TNodePtr lambda;
    TString name;
};

enum class ERowsPerMatch {
    OneRow,
    AllRows
};

enum class EAfterMatchSkipTo {
    NextRow,
    PastLastRow,
    ToFirst,
    ToLast,
    To
};

struct TAfterMatchSkipTo {
    TAfterMatchSkipTo(EAfterMatchSkipTo to, const TStringBuf var = TStringBuf())
        : To(to)
        , Var(var)
    {}
    EAfterMatchSkipTo To;
    TString Var;
};

class TMatchRecognizeBuilder: public TSimpleRefCount<TMatchRecognizeBuilder> {
public:
    TMatchRecognizeBuilder(
            TPosition clausePos,
            std::pair<TPosition, TVector<TPartitioner>>&& partitioners,
            std::pair<TPosition, TVector<TSortSpecificationPtr>>&& sortSpecs,
            std::pair<TPosition, TVector<TNamedLambda>>&& measures,
            std::pair<TPosition, ERowsPerMatch>&& rowsPerMatch,
            std::pair<TPosition, TAfterMatchSkipTo>&& skipTo,
            std::pair<TPosition, NYql::NMatchRecognize::TRowPattern>&& pattern,
            std::pair<TPosition, TNodePtr>&& subset,
            std::pair<TPosition, TVector<TNamedLambda>>&& definitions
            )
            : Pos(clausePos)
            , Partitioners(std::move(partitioners))
            , SortSpecs(std::move(sortSpecs))
            , Measures(std::move(measures))
            , RowsPerMatch(std::move(rowsPerMatch))
            , SkipTo(std::move(skipTo))
            , Pattern(std::move(pattern))
            , Subset(std::move(subset))
            , Definitions(definitions)

    {}
    TNodePtr Build(TContext& ctx, TString&& inputTable, ISource* source);
private:
    TPosition Pos;
    std::pair<TPosition, TVector<TPartitioner>> Partitioners;
    std::pair<TPosition, TVector<TSortSpecificationPtr>> SortSpecs;
    std::pair<TPosition, TVector<TNamedLambda>> Measures;
    std::pair<TPosition, ERowsPerMatch> RowsPerMatch;
    std::pair<TPosition, TAfterMatchSkipTo> SkipTo;
    std::pair<TPosition, NYql::NMatchRecognize::TRowPattern> Pattern;
    std::pair<TPosition, TNodePtr> Subset;
    std::pair<TPosition, TVector<TNamedLambda>> Definitions;
};

using TMatchRecognizeBuilderPtr=TIntrusivePtr<TMatchRecognizeBuilder> ;

} // namespace NSQLTranslationV1
