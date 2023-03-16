LIBRARY()

SRCS(
    bsc.cpp
    bsc.h
    cmds_box.cpp
    cmds_drive_status.cpp
    cmds_host_config.cpp
    cmds_storage_pool.cpp
    config_cmd.cpp
    config.cpp
    config_fit_groups.cpp
    config_fit_pdisks.cpp
    config.h
    defs.h
    diff.h
    disk_metrics.cpp
    drop_donor.cpp
    error.h
    get_group.cpp
    grouper.cpp
    grouper.h
    group_layout_checker.cpp
    group_layout_checker.h
    group_mapper.cpp
    group_mapper.h
    group_metrics_exchange.cpp
    group_reconfigure_wipe.cpp
    impl.h
    indir.h
    init_scheme.cpp
    load_everything.cpp
    migrate.cpp
    monitoring.cpp
    mood.h
    mv_object_map.h
    node_report.cpp
    propose_group_key.cpp
    register_node.cpp
    request_controller_info.cpp
    resources.h
    scheme.h
    scrub.cpp
    select_groups.cpp
    select_groups.h
    self_heal.cpp
    self_heal.h
    stat_processor.cpp
    stat_processor.h
    storage_pool_stat.h
    storage_stats_calculator.cpp
    sys_view.cpp
    sys_view.h
    table_merger.h
    types.h
    update_group_latencies.cpp
    update_last_seen_ready.cpp
    update_seen_operational.cpp
    vdisk_status_tracker.h
    virtual_group.cpp
)

PEERDIR(
    library/cpp/actors/core
    ydb/core/base
    ydb/core/base/services
    ydb/core/blobstorage
    ydb/core/blobstorage/base
    ydb/core/blobstorage/groupinfo
    ydb/core/blob_depot
    ydb/core/engine/minikql
    ydb/core/protos
    ydb/core/sys_view/common
    ydb/core/tablet
    ydb/core/tablet_flat
    ydb/core/tx/tx_proxy
)

END()

RECURSE_FOR_TESTS(
    ut
    ut_bscontroller
    ut_selfheal
)