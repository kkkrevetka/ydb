#include <ydb/core/grpc_services/local_rpc/local_rpc.h>
#include <ydb/public/api/grpc/ydb_auth_v1.grpc.pb.h>
#include <ydb/core/base/ticket_parser.h>
#include <ydb/core/kafka_proxy/kafka_events.h>
#include <library/cpp/actors/core/actor.h>

#include "kafka_sasl_auth_actor.h"

namespace NKafka {

static constexpr char ERROR_AUTH_BYTES[] = "";

NActors::IActor* CreateKafkaSaslAuthActor(const TContext::TPtr context, const ui64 correlationId, const NKikimr::NRawSocket::TSocketDescriptor::TSocketAddressType address, const TSaslAuthenticateRequestData* message) {
    return new TKafkaSaslAuthActor(context, correlationId, address, message);
}    

void TKafkaSaslAuthActor::Bootstrap(const NActors::TActorContext& ctx) {
    if (Context->AuthenticationStep != EAuthSteps::WAIT_AUTH) {
        SendAuthFailedAndDie(EKafkaErrors::ILLEGAL_SASL_STATE,
                             "Request is not valid given the current SASL state.",
                             TStringBuilder() << "Current step: " << static_cast<int>(Context->AuthenticationStep),
                             ctx);
        return; 
    }
    if (Context->SaslMechanism != "PLAIN") {
        SendAuthFailedAndDie(EKafkaErrors::UNSUPPORTED_SASL_MECHANISM, 
                             "Does not support the requested SASL mechanism.", 
                             TStringBuilder() << "Requested mechanism '" << Context->SaslMechanism << "'",
                             ctx);
        return;
    }
    Become(&TKafkaSaslAuthActor::StateWork);
    StartPlainAuth(ctx);
}

void TKafkaSaslAuthActor::StartPlainAuth(const NActors::TActorContext& ctx) {
    TAuthData authData;
    if (!TryParseAuthDataTo(authData, ctx)) {
        return;
    }
    Database = authData.Database;
    SendLoginRequest(authData, ctx);
}

void TKafkaSaslAuthActor::Handle(NKikimr::TEvTicketParser::TEvAuthorizeTicketResult::TPtr& ev, const NActors::TActorContext& ctx) {
    if (ev->Get()->Error) {
        SendAuthFailedAndDie(EKafkaErrors::SASL_AUTHENTICATION_FAILED, "", ev->Get()->Error.Message, ctx);
        return;
    }

    auto responseToClient = std::make_shared<TSaslAuthenticateResponseData>();
    responseToClient->ErrorCode = EKafkaErrors::NONE_ERROR;
    responseToClient->ErrorMessage = "";
    responseToClient->AuthBytes = TKafkaRawBytes(ERROR_AUTH_BYTES, sizeof(ERROR_AUTH_BYTES));

    auto evResponse = std::make_shared<TEvKafka::TEvResponse>(CorrelationId, responseToClient);

    auto authResult = new TEvKafka::TEvAuthResult(EAuthSteps::SUCCESS, evResponse, ev->Get()->Token, Database);
    Send(Context->ConnectionId, authResult);  
    
    Die(ctx);
}

void TKafkaSaslAuthActor::Handle(TEvPrivate::TEvTokenReady::TPtr& ev, const NActors::TActorContext& /*ctx*/) {
    Send(NKikimr::MakeTicketParserID(), new NKikimr::TEvTicketParser::TEvAuthorizeTicket({
        .Database = ev->Get()->Database,
        .Ticket = ev->Get()->LoginResult.token(),
        .PeerName = TStringBuilder() << Address,
    }));
}

void TKafkaSaslAuthActor::Handle(TEvPrivate::TEvAuthFailed::TPtr& ev, const NActors::TActorContext& ctx) {
    SendAuthFailedAndDie(EKafkaErrors::SASL_AUTHENTICATION_FAILED, "", ev->Get()->ErrorMessage, ctx);
}

bool TKafkaSaslAuthActor::TryParseAuthDataTo(TKafkaSaslAuthActor::TAuthData& authData, const NActors::TActorContext& ctx) {
    if (!AuthenticateRequestData->AuthBytes.has_value()) { 
        SendAuthFailedAndDie(EKafkaErrors::SASL_AUTHENTICATION_FAILED, "", "AuthBytes is empty.",  ctx);
        return false;
    }

    TKafkaRawBytes rawAuthBytes = AuthenticateRequestData->AuthBytes.value();
    TString auth(rawAuthBytes.data(), rawAuthBytes.size());
    TVector<TString> tokens = StringSplitter(auth).Split('\0');
    if (tokens.size() != 3) {
        SendAuthFailedAndDie(EKafkaErrors::SASL_AUTHENTICATION_FAILED, TStringBuilder() << "Invalid SASL/PLAIN response: expected 3 tokens, got " << tokens.size(), "", ctx);
        return false;
    }

    // tokens[0] is authorizationIdFromClient. Ignored
    auto userAndDatabase = tokens[1];
    auto password = tokens[2];
    size_t atPos = userAndDatabase.rfind('@');
    if (atPos == TString::npos) {
        SendAuthFailedAndDie(EKafkaErrors::SASL_AUTHENTICATION_FAILED, "Database not provided.", "", ctx);
        return false;
    }
    
    authData.UserName = userAndDatabase.substr(0, atPos);
    authData.Database = userAndDatabase.substr(atPos + 1);
    authData.Password = password;
    return true;
}

void TKafkaSaslAuthActor::SendAuthFailedAndDie(EKafkaErrors errorCode, const TString& errorMessage, const TString& details, const NActors::TActorContext& ctx) {
    KAFKA_LOG_ERROR("Authentication failure. " << errorMessage << " " << details);

    auto responseToClient = std::make_shared<TSaslAuthenticateResponseData>();
    responseToClient->ErrorCode = errorCode;
    responseToClient->ErrorMessage = TStringBuilder() << "Authentication failure. " << errorMessage; 
    responseToClient->AuthBytes = TKafkaRawBytes(ERROR_AUTH_BYTES, sizeof(ERROR_AUTH_BYTES));

    auto evResponse = std::make_shared<TEvKafka::TEvResponse>(CorrelationId, responseToClient);
    auto authResult = new TEvKafka::TEvAuthResult(EAuthSteps::FAILED, evResponse, nullptr, "", errorMessage);
    Send(Context->ConnectionId, authResult);
   
    Die(ctx);
}

void TKafkaSaslAuthActor::SendLoginRequest(TKafkaSaslAuthActor::TAuthData authData, const NActors::TActorContext& ctx) {
    Ydb::Auth::LoginRequest request;
    request.set_user(authData.UserName);
    request.set_password(authData.Password);
    auto* actorSystem = ctx.ActorSystem();

    using TRpcEv = NKikimr::NGRpcService::TGRpcRequestWrapperNoAuth<NKikimr::NGRpcService::TRpcServices::EvLogin, Ydb::Auth::LoginRequest, Ydb::Auth::LoginResponse>;
    auto rpcFuture = NKikimr::NRpcService::DoLocalRpc<TRpcEv>(std::move(request), authData.Database, {}, actorSystem);

    rpcFuture.Subscribe([authData, actorSystem, selfId = SelfId()](const NThreading::TFuture<Ydb::Auth::LoginResponse>& future) {
        auto& response = future.GetValueSync();

        if (response.operation().status() == Ydb::StatusIds::SUCCESS) {
            auto tokenReadyEvent = std::make_unique<TEvPrivate::TEvTokenReady>();
            response.operation().result().UnpackTo(&(tokenReadyEvent->LoginResult));
            tokenReadyEvent->Database = authData.Database;
            actorSystem->Schedule(TDuration::Seconds(1), new IEventHandle(selfId, selfId, tokenReadyEvent.release())); // FIXME(savnik): replace Schedule to Send
        } else {
            auto authFailedEvent = std::make_unique<TEvPrivate::TEvAuthFailed>();
            if (response.operation().issues_size() > 0) {
                authFailedEvent->ErrorMessage = response.operation().issues(0).message();
            } else {
                authFailedEvent->ErrorMessage = Ydb::StatusIds_StatusCode_Name(response.operation().status());
            }
            actorSystem->Send(selfId, authFailedEvent.release());
        }
    });
}

} // NKafka
