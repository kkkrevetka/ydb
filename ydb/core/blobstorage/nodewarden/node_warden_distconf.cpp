#include "node_warden_distconf.h"
#include "node_warden_impl.h"

namespace NKikimr::NStorage {

    TDistributedConfigKeeper::TDistributedConfigKeeper(TIntrusivePtr<TNodeWardenConfig> cfg)
        : Cfg(std::move(cfg))
    {}

    void TDistributedConfigKeeper::Bootstrap() {
        STLOG(PRI_DEBUG, BS_NODE, NWDC00, "Bootstrap");
        Send(GetNameserviceActorId(), new TEvInterconnect::TEvListNodes(true));
        auto query = std::bind(&TThis::ReadConfig, TActivationContext::ActorSystem(), SelfId(), Cfg);
        Send(MakeIoDispatcherActorId(), new TEvInvokeQuery(std::move(query)));
        Become(&TThis::StateWaitForInit);
    }

    void TDistributedConfigKeeper::SendEvent(ui32 nodeId, ui64 cookie, TActorId sessionId, std::unique_ptr<IEventBase> ev) {
        Y_VERIFY(nodeId != SelfId().NodeId());
        auto handle = std::make_unique<IEventHandle>(MakeBlobStorageNodeWardenID(nodeId), SelfId(), ev.release(), 0, cookie);
        Y_VERIFY(sessionId);
        handle->Rewrite(TEvInterconnect::EvForward, sessionId);
        TActivationContext::Send(handle.release());
    }

    void TDistributedConfigKeeper::SendEvent(const TBinding& binding, std::unique_ptr<IEventBase> ev) {
        Y_VERIFY(binding.SessionId);
        SendEvent(binding.NodeId, binding.Cookie, binding.SessionId, std::move(ev));
    }

    void TDistributedConfigKeeper::SendEvent(const IEventHandle& handle, std::unique_ptr<IEventBase> ev) {
        SendEvent(handle.Sender.NodeId(), handle.Cookie, handle.InterconnectSession, std::move(ev));
    }

    void TDistributedConfigKeeper::SendEvent(ui32 nodeId, const TBoundNode& info, std::unique_ptr<IEventBase> ev) {
        SendEvent(nodeId, info.Cookie, info.SessionId, std::move(ev));
    }

#ifndef NDEBUG
    void TDistributedConfigKeeper::ConsistencyCheck() {
        THashMap<ui32, ui32> refAllBoundNodeIds;
        for (const auto& [nodeId, info] : DirectBoundNodes) {
            ++refAllBoundNodeIds[nodeId];
            for (const ui32 boundNodeId : info.BoundNodeIds) {
                ++refAllBoundNodeIds[boundNodeId];
            }
        }
        Y_VERIFY(AllBoundNodes == refAllBoundNodeIds);

        for (const auto& [nodeId, info] : DirectBoundNodes) {
            Y_VERIFY(std::binary_search(NodeIds.begin(), NodeIds.end(), nodeId));
        }
        if (Binding) {
            Y_VERIFY(std::binary_search(NodeIds.begin(), NodeIds.end(), Binding->NodeId));
        }

        for (const auto& [cookie, task] : ScatterTasks) {
            for (const ui32 nodeId : task.PendingNodes) {
                const auto it = DirectBoundNodes.find(nodeId);
                Y_VERIFY(it != DirectBoundNodes.end());
                TBoundNode& info = it->second;
                Y_VERIFY(info.ScatterTasks.contains(cookie));
            }
        }

        for (const auto& [nodeId, info] : DirectBoundNodes) {
            for (const ui64 cookie : info.ScatterTasks) {
                const auto it = ScatterTasks.find(cookie);
                Y_VERIFY(it != ScatterTasks.end());
                TScatterTask& task = it->second;
                Y_VERIFY(task.PendingNodes.contains(nodeId));
            }
        }

        for (const auto& [cookie, task] : ScatterTasks) {
            if (task.Origin) {
                Y_VERIFY(Binding);
                Y_VERIFY(task.Origin == Binding);
            } else { // locally-generated task
                Y_VERIFY(RootState != ERootState::INITIAL);
                Y_VERIFY(!Binding);
            }
        }

        for (const auto& [nodeId, sessionId] : SubscribedSessions) {
            bool okay = false;
            if (Binding && Binding->NodeId == nodeId) {
                Y_VERIFY(sessionId == Binding->SessionId);
                okay = true;
            }
            if (const auto it = DirectBoundNodes.find(nodeId); it != DirectBoundNodes.end()) {
                Y_VERIFY(!sessionId || sessionId == it->second.SessionId);
                okay = true;
            }
            if (!sessionId) {
                okay = true; // may be just obsolete subscription request
            }
            Y_VERIFY(okay);
        }

        if (Binding) {
            Y_VERIFY(SubscribedSessions.contains(Binding->NodeId));
        }
        for (const auto& [nodeId, info] : DirectBoundNodes) {
            Y_VERIFY(SubscribedSessions.contains(nodeId));
        }
    }
#endif

    STFUNC(TDistributedConfigKeeper::StateWaitForInit) {
        auto processPendingEvent = [&] {
            Y_VERIFY(!PendingEvents.empty());
            StateFunc(PendingEvents.front());
            PendingEvents.pop_front();
            if (PendingEvents.empty()) {
                Become(&TThis::StateFunc);
            } else {
                TActivationContext::Send(new IEventHandle(TEvPrivate::EvProcessPendingEvent, 0, SelfId(), {}, nullptr, 0));
            }
        };

        switch (ev->GetTypeRewrite()) {
            case TEvInterconnect::TEvNodesInfo::EventType:
                PendingEvents.push_front(std::move(ev));
                NodeListObtained = true;
                if (StorageConfigLoaded) {
                    processPendingEvent();
                }
                break;

            case TEvPrivate::EvStorageConfigLoaded:
                if (auto *msg = ev->Get<TEvPrivate::TEvStorageConfigLoaded>(); msg->Success) {
                    StorageConfig.Swap(&msg->StorageConfig);
                }
                StorageConfigLoaded = true;
                if (NodeListObtained) {
                    processPendingEvent();
                }
                break;

            case TEvPrivate::EvProcessPendingEvent:
                processPendingEvent();
                break;

            default:
                PendingEvents.push_back(std::move(ev));
                break;
        }
    }

    STFUNC(TDistributedConfigKeeper::StateFunc) {
        STRICT_STFUNC_BODY(
            hFunc(TEvNodeConfigPush, Handle);
            hFunc(TEvNodeConfigReversePush, Handle);
            hFunc(TEvNodeConfigUnbind, Handle);
            hFunc(TEvNodeConfigScatter, Handle);
            hFunc(TEvNodeConfigGather, Handle);
            hFunc(TEvInterconnect::TEvNodesInfo, Handle);
            hFunc(TEvInterconnect::TEvNodeConnected, Handle);
            hFunc(TEvInterconnect::TEvNodeDisconnected, Handle);
            hFunc(TEvents::TEvUndelivered, Handle);
            cFunc(TEvPrivate::EvQuorumCheckTimeout, HandleQuorumCheckTimeout);
            hFunc(TEvPrivate::TEvStorageConfigLoaded, Handle);
            hFunc(TEvPrivate::TEvStorageConfigStored, Handle);
            hFunc(NMon::TEvHttpInfo, Handle);
            cFunc(TEvents::TSystem::Wakeup, HandleWakeup);
            cFunc(TEvents::TSystem::Poison, PassAway);
        )
        ConsistencyCheck();
    }

    void TNodeWarden::StartDistributedConfigKeeper() {
        DistributedConfigKeeperId = Register(new TDistributedConfigKeeper(Cfg));
    }

    void TNodeWarden::ForwardToDistributedConfigKeeper(STATEFN_SIG) {
        ev->Rewrite(ev->GetTypeRewrite(), DistributedConfigKeeperId);
        TActivationContext::Send(ev.Release());
    }

} // NKikimr::NStorage

template<>
void Out<NKikimr::NStorage::TDistributedConfigKeeper::ERootState>(IOutputStream& s, NKikimr::NStorage::TDistributedConfigKeeper::ERootState state) {
    using E = decltype(state);
    switch (state) {
        case E::INITIAL:                    s << "INITIAL";                    return;
        case E::QUORUM_CHECK_TIMEOUT:       s << "QUORUM_CHECK_TIMEOUT";       return;
        case E::COLLECT_CONFIG:             s << "COLLECT_CONFIG";             return;
        case E::PROPOSE_NEW_STORAGE_CONFIG: s << "PROPOSE_NEW_STORAGE_CONFIG"; return;
    }
    Y_FAIL();
}
