#include <ydb/core/keyvalue/keyvalue_events.h>
#include <ydb/core/persqueue/events/internal.h>
#include <ydb/core/persqueue/partition.h>
#include <ydb/core/persqueue/ut/common/pq_ut_common.h>
#include <ydb/core/protos/counters_keyvalue.pb.h>
#include <ydb/core/protos/pqconfig.pb.h>
#include <ydb/core/tablet/tablet_counters_protobuf.h>
#include <ydb/core/tx/tx_processing.h>
#include <ydb/library/persqueue/topic_parser/topic_parser.h>
#include <ydb/public/api/protos/draft/persqueue_error_codes.pb.h>
#include <ydb/public/lib/base/msgbus_status.h>

#include <library/cpp/actors/core/actorid.h>
#include <library/cpp/actors/core/event.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/system/types.h>

#include "pqtablet_mock.h"

namespace NKikimr::NPQ {

Y_UNIT_TEST_SUITE(TPQTabletTests) {

namespace NHelpers {

struct TTxOperation {
    ui32 Partition;
    TString Consumer;
    ui64 Begin = 0;
    ui64 End = 0;
    TString Path;
};

struct TProposeTransactionParams {
    ui64 TxId = 0;
    TVector<ui64> Senders;
    TVector<ui64> Receivers;
    TVector<TTxOperation> TxOps;
};

struct TPlanStepParams {
    ui64 Step;
    TVector<ui64> TxIds;
};

struct TReadSetParams {
    ui64 Step = 0;
    ui64 TxId = 0;
    ui64 Source = 0;
    ui64 Target = 0;
    bool Predicate = false;
    ui64 SeqNo = 0;
};

using NKikimr::NPQ::NHelpers::CreatePQTabletMock;
using TPQTabletMock = NKikimr::NPQ::NHelpers::TPQTabletMock;

}

class TPQTabletFixture : public NUnitTest::TBaseFixture {
protected:
    struct TProposeTransactionResponseMatcher {
        TMaybe<ui64> TxId;
        TMaybe<NKikimrPQ::TEvProposeTransactionResult::EStatus> Status;
    };

    struct TTxOperationMatcher {
        TMaybe<ui32> Partition;
        TMaybe<TString> Consumer;
        TMaybe<ui64> Begin;
        TMaybe<ui64> End;
    };

    struct TCmdWriteTxMatcher {
        TMaybe<ui64> TxId;
        TMaybe<NKikimrPQ::TTransaction::EState> State;
        TVector<ui64> Senders;
        TVector<ui64> Receivers;
        TVector<TTxOperationMatcher> TxOps;
    };

    struct TPlanStepAckMatcher {
        TMaybe<ui64> Step;
        TVector<ui64> TxIds;
    };

    struct TPlanStepAcceptedMatcher {
        TMaybe<ui64> Step;
    };

    struct TReadSetMatcher {
        TMaybe<ui64> Step;
        TMaybe<ui64> TxId;
        TMaybe<ui64> Source;
        TMaybe<ui64> Target;
        TMaybe<NKikimrTx::TReadSetData::EDecision> Decision;
        TMaybe<ui64> Producer;
    };

    struct TReadSetAckMatcher {
        TMaybe<ui64> Step;
        TMaybe<ui64> TxId;
        TMaybe<ui64> Source;
        TMaybe<ui64> Target;
        TMaybe<ui64> Consumer;
    };

    using TProposeTransactionParams = NHelpers::TProposeTransactionParams;
    using TPlanStepParams = NHelpers::TPlanStepParams;
    using TReadSetParams = NHelpers::TReadSetParams;

    void SetUp(NUnitTest::TTestContext&) override;
    void TearDown(NUnitTest::TTestContext&) override;

    void SendProposeTransactionRequest(const TProposeTransactionParams& params);
    void WaitProposeTransactionResponse(const TProposeTransactionResponseMatcher& matcher = {});

    void SendPlanStep(const TPlanStepParams& params);
    void WaitPlanStepAck(const TPlanStepAckMatcher& matcher = {});
    void WaitPlanStepAccepted(const TPlanStepAcceptedMatcher& matcher = {});

    void WaitReadSet(NHelpers::TPQTabletMock& tablet, const TReadSetMatcher& matcher);
    void SendReadSet(NHelpers::TPQTabletMock& tablet, const TReadSetParams& params);

    void WaitReadSetAck(NHelpers::TPQTabletMock& tablet, const TReadSetAckMatcher& matcher);
    void SendReadSetAck(NHelpers::TPQTabletMock& tablet);

    //
    // TODO(abcdef): для тестирования повторных вызовов нужны примитивы Send+Wait
    //

    NHelpers::TPQTabletMock* CreatePQTabletMock(ui64 tabletId);

    TMaybe<TTestContext> Ctx;
    TMaybe<TFinalizer> Finalizer;

    TTestActorRuntimeBase::TEventObserver PrevEventObserver;

    TActorId Pipe;
};

void TPQTabletFixture::SetUp(NUnitTest::TTestContext&)
{
    Ctx.ConstructInPlace();
    Finalizer.ConstructInPlace(*Ctx);

    Ctx->Prepare();
    Ctx->Runtime->SetScheduledLimit(5'000);
}

void TPQTabletFixture::TearDown(NUnitTest::TTestContext&)
{
}

void TPQTabletFixture::SendProposeTransactionRequest(const TProposeTransactionParams& params)
{
    auto event = MakeHolder<TEvPersQueue::TEvProposeTransaction>();
    THashSet<ui32> partitions;

    //
    // Source
    //
    ActorIdToProto(Ctx->Edge, event->Record.MutableSource());

    //
    // TxBody
    //
    auto* body = event->Record.MutableTxBody();
    for (auto& txOp : params.TxOps) {
        auto* operation = body->MutableOperations()->Add();
        operation->SetPartitionId(txOp.Partition);
        operation->SetBegin(txOp.Begin);
        operation->SetEnd(txOp.End);
        operation->SetConsumer(txOp.Consumer);
        operation->SetPath(txOp.Path);

        partitions.insert(txOp.Partition);
    }
    for (ui64 tabletId : params.Senders) {
        body->AddSendingShards(tabletId);
    }
    for (ui64 tabletId : params.Receivers) {
        body->AddReceivingShards(tabletId);
    }
    body->SetImmediate(params.Senders.empty() && params.Receivers.empty() && (partitions.size() == 1));

    //
    // TxId
    //
    event->Record.SetTxId(params.TxId);

    if (Pipe == TActorId()) {
        Pipe = Ctx->Runtime->ConnectToPipe(Ctx->TabletId,
                                           Ctx->Edge,
                                           0,
                                           GetPipeConfigWithRetries());
    }

    Ctx->Runtime->SendToPipe(Pipe,
                             Ctx->Edge,
                             event.Release());
}

void TPQTabletFixture::WaitProposeTransactionResponse(const TProposeTransactionResponseMatcher& matcher)
{
    auto event = Ctx->Runtime->GrabEdgeEvent<TEvPersQueue::TEvProposeTransactionResult>();
    UNIT_ASSERT(event != nullptr);

    if (matcher.TxId) {
        UNIT_ASSERT(event->Record.HasTxId());
        UNIT_ASSERT_VALUES_EQUAL(*matcher.TxId, event->Record.GetTxId());
    }

    if (matcher.Status) {
        UNIT_ASSERT(event->Record.HasStatus());
        UNIT_ASSERT(*matcher.Status == event->Record.GetStatus());
    }
}

void TPQTabletFixture::SendPlanStep(const TPlanStepParams& params)
{
    auto event = MakeHolder<TEvTxProcessing::TEvPlanStep>();
    event->Record.SetStep(params.Step);
    for (ui64 txId : params.TxIds) {
        auto tx = event->Record.AddTransactions();

        tx->SetTxId(txId);
        ActorIdToProto(Ctx->Edge, tx->MutableAckTo());
    }

    Ctx->Runtime->SendToPipe(Pipe,
                             Ctx->Edge,
                             event.Release());
}

void TPQTabletFixture::WaitPlanStepAck(const TPlanStepAckMatcher& matcher)
{
    auto event = Ctx->Runtime->GrabEdgeEvent<TEvTxProcessing::TEvPlanStepAck>();
    UNIT_ASSERT(event != nullptr);

    if (matcher.Step.Defined()) {
        UNIT_ASSERT(event->Record.HasStep());
        UNIT_ASSERT_VALUES_EQUAL(*matcher.Step, event->Record.GetStep());
    }

    UNIT_ASSERT_VALUES_EQUAL(matcher.TxIds.size(), event->Record.TxIdSize());
    for (size_t i = 0; i < event->Record.TxIdSize(); ++i) {
        UNIT_ASSERT_VALUES_EQUAL(matcher.TxIds[i], event->Record.GetTxId(i));
    }
}

void TPQTabletFixture::WaitPlanStepAccepted(const TPlanStepAcceptedMatcher& matcher)
{
    auto event = Ctx->Runtime->GrabEdgeEvent<TEvTxProcessing::TEvPlanStepAccepted>();
    UNIT_ASSERT(event != nullptr);

    if (matcher.Step.Defined()) {
        UNIT_ASSERT(event->Record.HasStep());
        UNIT_ASSERT_VALUES_EQUAL(*matcher.Step, event->Record.GetStep());
    }
}

void TPQTabletFixture::WaitReadSet(NHelpers::TPQTabletMock& tablet, const TReadSetMatcher& matcher)
{
    if (!tablet.ReadSet.Defined()) {
        TDispatchOptions options;
        options.CustomFinalCondition = [&]() {
            return tablet.ReadSet.Defined();
        };
        UNIT_ASSERT(Ctx->Runtime->DispatchEvents(options));
    }

    if (matcher.Step.Defined()) {
        UNIT_ASSERT(tablet.ReadSet->HasStep());
        UNIT_ASSERT_VALUES_EQUAL(*matcher.Step, tablet.ReadSet->GetStep());
    }
    if (matcher.TxId.Defined()) {
        UNIT_ASSERT(tablet.ReadSet->HasTxId());
        UNIT_ASSERT_VALUES_EQUAL(*matcher.TxId, tablet.ReadSet->GetTxId());
    }
    if (matcher.Source.Defined()) {
        UNIT_ASSERT(tablet.ReadSet->HasTabletSource());
        UNIT_ASSERT_VALUES_EQUAL(*matcher.Source, tablet.ReadSet->GetTabletSource());
    }
    if (matcher.Target.Defined()) {
        UNIT_ASSERT(tablet.ReadSet->HasTabletDest());
        UNIT_ASSERT_VALUES_EQUAL(*matcher.Target, tablet.ReadSet->GetTabletDest());
    }
    if (matcher.Decision.Defined()) {
        UNIT_ASSERT(tablet.ReadSet->HasReadSet());

        NKikimrTx::TReadSetData data;
        Y_VERIFY(data.ParseFromString(tablet.ReadSet->GetReadSet()));

        UNIT_ASSERT(*matcher.Decision == data.GetDecision());
    }
    if (matcher.Producer.Defined()) {
        UNIT_ASSERT(tablet.ReadSet->HasTabletProducer());
        UNIT_ASSERT_VALUES_EQUAL(*matcher.Producer, tablet.ReadSet->GetTabletProducer());
    }
}

void TPQTabletFixture::WaitReadSetAck(NHelpers::TPQTabletMock& tablet, const TReadSetAckMatcher& matcher)
{
    if (!tablet.ReadSetAck.Defined()) {
        TDispatchOptions options;
        options.CustomFinalCondition = [&]() {
            return tablet.ReadSetAck.Defined();
        };
        UNIT_ASSERT(Ctx->Runtime->DispatchEvents(options));
    }

    if (matcher.Step.Defined()) {
        UNIT_ASSERT(tablet.ReadSetAck->HasStep());
        UNIT_ASSERT_VALUES_EQUAL(*matcher.Step, tablet.ReadSetAck->GetStep());
    }
    if (matcher.TxId.Defined()) {
        UNIT_ASSERT(tablet.ReadSetAck->HasTxId());
        UNIT_ASSERT_VALUES_EQUAL(*matcher.TxId, tablet.ReadSetAck->GetTxId());
    }
    if (matcher.Source.Defined()) {
        UNIT_ASSERT(tablet.ReadSetAck->HasTabletSource());
        UNIT_ASSERT_VALUES_EQUAL(*matcher.Source, tablet.ReadSetAck->GetTabletSource());
    }
    if (matcher.Target.Defined()) {
        UNIT_ASSERT(tablet.ReadSetAck->HasTabletDest());
        UNIT_ASSERT_VALUES_EQUAL(*matcher.Target, tablet.ReadSetAck->GetTabletDest());
    }
    if (matcher.Consumer.Defined()) {
        UNIT_ASSERT(tablet.ReadSetAck->HasTabletConsumer());
        UNIT_ASSERT_VALUES_EQUAL(*matcher.Consumer, tablet.ReadSetAck->GetTabletConsumer());
    }
}

NHelpers::TPQTabletMock* TPQTabletFixture::CreatePQTabletMock(ui64 tabletId)
{
    NHelpers::TPQTabletMock* mock = nullptr;
    auto wrapCreatePQTabletMock = [&](const NActors::TActorId& tablet, NKikimr::TTabletStorageInfo* info) -> IActor* {
        mock = NHelpers::CreatePQTabletMock(tablet, info);
        return mock;
    };

    CreateTestBootstrapper(*Ctx->Runtime,
                           CreateTestTabletInfo(tabletId, NKikimrTabletBase::TTabletTypes::Dummy, TErasureType::ErasureNone),
                           wrapCreatePQTabletMock);

    TDispatchOptions options;
    options.FinalEvents.push_back(TDispatchOptions::TFinalEventCondition(TEvTablet::EvBoot));
    Ctx->Runtime->DispatchEvents(options);

    return mock;
}

Y_UNIT_TEST_F(Multiple_PQTablets, TPQTabletFixture)
{
    NHelpers::TPQTabletMock* tablet = CreatePQTabletMock(22222);
    PQTabletPrepare({.partitions=1}, {}, *Ctx);

    const ui64 txId_1 = 67890;
    const ui64 txId_2 = 67891;

    SendProposeTransactionRequest({.TxId=txId_1,
                                  .Senders={22222}, .Receivers={22222},
                                  .TxOps={
                                  {.Partition=0, .Consumer="user", .Begin=0, .End=0, .Path="/topic"},
                                  }});
    WaitProposeTransactionResponse({.TxId=txId_1,
                                   .Status=NKikimrPQ::TEvProposeTransactionResult::PREPARED});

    SendProposeTransactionRequest({.TxId=txId_2,
                                  .Senders={22222}, .Receivers={22222},
                                  .TxOps={
                                  {.Partition=0, .Consumer="user", .Begin=0, .End=0, .Path="/topic"},
                                  }});
    WaitProposeTransactionResponse({.TxId=txId_2,
                                   .Status=NKikimrPQ::TEvProposeTransactionResult::PREPARED});

    SendPlanStep({.Step=100, .TxIds={txId_2}});
    SendPlanStep({.Step=200, .TxIds={txId_1}});

    //
    // TODO(abcdef): проверить, что в команде CmdWrite есть информация о транзакции
    //

    WaitPlanStepAck({.Step=100, .TxIds={txId_2}}); // TEvPlanStepAck для координатора
    WaitPlanStepAccepted({.Step=100});

    WaitPlanStepAck({.Step=200, .TxIds={txId_1}}); // TEvPlanStepAck для координатора
    WaitPlanStepAccepted({.Step=200});

    //
    // транзакция txId_2
    //
    WaitReadSet(*tablet, {.Step=100, .TxId=txId_2, .Source=Ctx->TabletId, .Target=22222, .Decision=NKikimrTx::TReadSetData::DECISION_COMMIT, .Producer=Ctx->TabletId});
    tablet->SendReadSet(*Ctx->Runtime, {.Step=100, .TxId=txId_2, .Target=Ctx->TabletId, .Decision=NKikimrTx::TReadSetData::DECISION_COMMIT});

    WaitProposeTransactionResponse({.TxId=txId_2,
                                   .Status=NKikimrPQ::TEvProposeTransactionResult::COMPLETE});

    WaitReadSetAck(*tablet, {.Step=100, .TxId=txId_2, .Source=22222, .Target=Ctx->TabletId, .Consumer=Ctx->TabletId});
    tablet->SendReadSetAck(*Ctx->Runtime, {.Step=100, .TxId=txId_2, .Source=Ctx->TabletId});

    //
    // TODO(abcdef): проверить, что удалена информация о транзакции
    //

    //
    // транзакция txId_1
    //
    WaitReadSet(*tablet, {.Step=200, .TxId=txId_1, .Source=Ctx->TabletId, .Target=22222, .Decision=NKikimrTx::TReadSetData::DECISION_COMMIT, .Producer=Ctx->TabletId});
}

Y_UNIT_TEST_F(Single_PQTablet_And_Multiple_Partitions, TPQTabletFixture)
{
    PQTabletPrepare({.partitions=2}, {}, *Ctx);

    const ui64 txId = 67890;

    SendProposeTransactionRequest({.TxId=txId,
                                  .TxOps={
                                  {.Partition=0, .Consumer="user", .Begin=0, .End=0, .Path="/topic"},
                                  {.Partition=1, .Consumer="user", .Begin=0, .End=0, .Path="/topic"},
                                  }});
    WaitProposeTransactionResponse({.TxId=txId,
                                   .Status=NKikimrPQ::TEvProposeTransactionResult::PREPARED});

    SendPlanStep({.Step=100, .TxIds={txId}});

    //
    // TODO(abcdef): проверить, что в команде CmdWrite есть информация о транзакции
    //

    WaitPlanStepAck({.Step=100, .TxIds={txId}}); // TEvPlanStepAck для координатора
    WaitPlanStepAccepted({.Step=100});

    WaitProposeTransactionResponse({.TxId=txId,
                                   .Status=NKikimrPQ::TEvProposeTransactionResult::COMPLETE});

    //
    // TODO(abcdef): проверить, что удалена информация о транзакции
    //
}

Y_UNIT_TEST_F(PQTablet_Send_RS_With_Abort, TPQTabletFixture)
{
    NHelpers::TPQTabletMock* tablet = CreatePQTabletMock(22222);
    PQTabletPrepare({.partitions=1}, {}, *Ctx);

    const ui64 txId = 67890;

    SendProposeTransactionRequest({.TxId=txId,
                                  .Senders={22222}, .Receivers={22222},
                                  .TxOps={
                                  {.Partition=0, .Consumer="user", .Begin=0, .End=0, .Path="/topic"},
                                  }});

    WaitProposeTransactionResponse({.TxId=txId,
                                   .Status=NKikimrPQ::TEvProposeTransactionResult::PREPARED});

    SendPlanStep({.Step=100, .TxIds={txId}});

    //
    // TODO(abcdef): проверить, что в команде CmdWrite есть информация о транзакции
    //

    WaitPlanStepAck({.Step=100, .TxIds={txId}}); // TEvPlanStepAck для координатора
    WaitPlanStepAccepted({.Step=100});

    WaitReadSet(*tablet, {.Step=100, .TxId=txId, .Source=Ctx->TabletId, .Target=22222, .Decision=NKikimrTx::TReadSetData::DECISION_COMMIT, .Producer=Ctx->TabletId});
    tablet->SendReadSet(*Ctx->Runtime, {.Step=100, .TxId=txId, .Target=Ctx->TabletId, .Decision=NKikimrTx::TReadSetData::DECISION_ABORT});

    WaitProposeTransactionResponse({.TxId=txId,
                                   .Status=NKikimrPQ::TEvProposeTransactionResult::ABORTED});

    WaitReadSetAck(*tablet, {.Step=100, .TxId=txId, .Source=22222, .Target=Ctx->TabletId, .Consumer=Ctx->TabletId});
    tablet->SendReadSetAck(*Ctx->Runtime, {.Step=100, .TxId=txId, .Source=Ctx->TabletId});

    //
    // TODO(abcdef): проверить, что удалена информация о транзакции
    //
}

Y_UNIT_TEST_F(Partition_Send_Predicate_With_False, TPQTabletFixture)
{
    NHelpers::TPQTabletMock* tablet = CreatePQTabletMock(22222);
    PQTabletPrepare({.partitions=1}, {}, *Ctx);

    const ui64 txId = 67890;

    SendProposeTransactionRequest({.TxId=txId,
                                  .Senders={22222}, .Receivers={22222},
                                  .TxOps={
                                  {.Partition=0, .Consumer="user", .Begin=0, .End=2, .Path="/topic"},
                                  }});

    WaitProposeTransactionResponse({.TxId=txId,
                                   .Status=NKikimrPQ::TEvProposeTransactionResult::PREPARED});

    SendPlanStep({.Step=100, .TxIds={txId}});

    //
    // TODO(abcdef): проверить, что в команде CmdWrite есть информация о транзакции
    //

    WaitPlanStepAck({.Step=100, .TxIds={txId}}); // TEvPlanStepAck для координатора
    WaitPlanStepAccepted({.Step=100});

    WaitReadSet(*tablet, {.Step=100, .TxId=txId, .Source=Ctx->TabletId, .Target=22222, .Decision=NKikimrTx::TReadSetData::DECISION_ABORT, .Producer=Ctx->TabletId});
    tablet->SendReadSet(*Ctx->Runtime, {.Step=100, .TxId=txId, .Target=Ctx->TabletId, .Decision=NKikimrTx::TReadSetData::DECISION_COMMIT});

    WaitProposeTransactionResponse({.TxId=txId,
                                   .Status=NKikimrPQ::TEvProposeTransactionResult::ABORTED});

    WaitReadSetAck(*tablet, {.Step=100, .TxId=txId, .Source=22222, .Target=Ctx->TabletId, .Consumer=Ctx->TabletId});
    tablet->SendReadSetAck(*Ctx->Runtime, {.Step=100, .TxId=txId, .Source=Ctx->TabletId});

    //
    // TODO(abcdef): проверить, что удалена информация о транзакции
    //
}

}

}