RECURSE(
    actorlib_impl
    base
    blobstorage
    blob_depot
    blockstore
    client
    cms
    control
    debug
    discovery
    engine
    erasure
    filestore
    formats
    grpc_caching
    grpc_services
    grpc_streaming
    health_check
    http_proxy
    io_formats
    kesus
    keyvalue
    kqp
    load_test
    log_backend
    metering
    mind
    mon
    mon_alloc
    node_whiteboard
    persqueue
    pgproxy
    protos
    public_http
    quoter
    scheme
    scheme_types
    security
    sys_view
    tablet
    tablet_flat
    test_tablet
    tracing
    tx
    util
    viewer
    wrappers
    ydb_convert
    ymq
    driver_lib
    yq
    yql_testlib
)

RECURSE_FOR_TESTS(
    testlib
)