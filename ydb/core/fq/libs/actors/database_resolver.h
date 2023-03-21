#pragma once

#include <library/cpp/actors/core/actor.h>
#include <ydb/library/yql/providers/common/token_accessor/client/factory.h>

namespace NFq {

TString TransformMdbHostToCorrectFormat(const TString& mdbHost);

NActors::IActor* CreateDatabaseResolver(NActors::TActorId httpProxy, NYql::ISecuredServiceAccountCredentialsFactory::TPtr credentialsFactory);

} /* namespace NFq */