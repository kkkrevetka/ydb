#include "yql_dq_integration_impl.h"

namespace NYql {

ui64 TDqIntegrationBase::Partition(const TDqSettings&, size_t, const TExprNode&,
    TVector<TString>&, TString*, TExprContext&, bool) {
    return 0;
}

bool TDqIntegrationBase::CheckPragmas(const TExprNode& node, TExprContext& ctx, bool skipIssues) {
    Y_UNUSED(skipIssues);
    Y_UNUSED(node);
    Y_UNUSED(ctx);
    return true;
}

bool TDqIntegrationBase::CanRead(const TExprNode&, TExprContext&, bool) {
    return false;
}

TMaybe<ui64> TDqIntegrationBase::EstimateReadSize(ui64, ui32, const TVector<const TExprNode*> &, TExprContext&) {
    return Nothing();
}

TExprNode::TPtr TDqIntegrationBase::WrapRead(const TDqSettings&, const TExprNode::TPtr& read, TExprContext&) {
    return read;
}

TMaybe<TOptimizerStatistics> TDqIntegrationBase::ReadStatistics(const TExprNode::TPtr& readWrap, TExprContext& ctx) {
    Y_UNUSED(readWrap);
    Y_UNUSED(ctx);
    return Nothing();
}

TMaybe<bool> TDqIntegrationBase::CanWrite(const TExprNode&, TExprContext&) {
    return Nothing();
}

TExprNode::TPtr TDqIntegrationBase::WrapWrite(const TExprNode::TPtr& write, TExprContext&) {
    return write;
}

void TDqIntegrationBase::RegisterMkqlCompiler(NCommon::TMkqlCallableCompilerBase&)  {
}

bool TDqIntegrationBase::CanFallback() {
    return false;
}

void TDqIntegrationBase::FillSourceSettings(const TExprNode&, ::google::protobuf::Any&, TString&) {
}

void TDqIntegrationBase::FillSinkSettings(const TExprNode&, ::google::protobuf::Any&, TString&) {
}

void TDqIntegrationBase::FillTransformSettings(const TExprNode&, ::google::protobuf::Any&) {
}

void TDqIntegrationBase::Annotate(const TExprNode&, THashMap<TString, TString>&) {
}

bool TDqIntegrationBase::PrepareFullResultTableParams(const TExprNode&, TExprContext&, THashMap<TString, TString>&, THashMap<TString, TString>&) {
    return false;
}

void TDqIntegrationBase::WriteFullResultTableRef(NYson::TYsonWriter&, const TVector<TString>&, const THashMap<TString, TString>&) {
}

} // namespace NYql
