QT       -= core
QT       -= gui
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

# DEFINES += TEAL_SINGLE_THREADED
# DEFINES += TEAL_DEBUGGING

# DEFINES += STR_UTIL_ENABLE_CUSTOM_UNICODE_OPERATIONS

# DEFINES += USE_CUSTOM_MEMORY_ALLOCATION
# DEFINES += RW_MUTEX_PRIORITIES

INCLUDEPATH += ../../src

SOURCES += main.cpp

HEADERS += \
    ../../src/ext/array_buffer_ext.hpp \
    ../../src/ext/containers_ext.hpp \
    ../../src/ext/eigen_ext.hpp \
    ../../src/ext/socket_ext.hpp \
    ../../src/inc/base16.hpp \
    ../../src/inc/base64.hpp \
    ../../src/inc/base85.hpp \
    ../../src/inc/binned_allocator.hpp \
    ../../src/inc/bit_util.hpp \
    ../../src/inc/command_queue.hpp \
    ../../src/inc/commondefs.hpp \
    ../../src/inc/containers/buffer_queue.hpp \
    ../../src/inc/containers/circular_buffer.hpp \
    ../../src/inc/containers/concurrentqueue.h \
    ../../src/inc/containers/static_buffer.hpp \
    ../../src/inc/crypto/gamma.hpp \
    ../../src/inc/crypto/keccak.hpp \
    ../../src/inc/crypto/sha256.hpp \
    ../../src/inc/crypto/sha3_512.hpp \
    ../../src/inc/crypto/sha512.hpp \
    ../../src/inc/crypto/threefish.hpp \
    ../../src/inc/emhash/hash_set2.hpp \
    ../../src/inc/emhash/hash_set3.hpp \
    ../../src/inc/emhash/hash_set4.hpp \
    ../../src/inc/emhash/hash_set8.hpp \
    ../../src/inc/emhash/hash_table5.hpp \
    ../../src/inc/emhash/hash_table6.hpp \
    ../../src/inc/emhash/hash_table7.hpp \
    ../../src/inc/emhash/hash_table8.hpp \
    ../../src/inc/emhash/lru_size.h \
    ../../src/inc/emhash/lru_time.h \
    ../../src/inc/file_util.hpp \
    ../../src/inc/fsm_tokenizer.hpp \
    ../../src/inc/hash/adler.hpp \
    ../../src/inc/hash/crc.hpp \
    ../../src/inc/hash/hash.hpp \
    ../../src/inc/http/httplib.h \
    ../../src/inc/json.hpp \
    ../../src/inc/lzari.hpp \
    ../../src/inc/math/math_util.hpp \
    ../../src/inc/math/matrix4.hpp \
    ../../src/inc/math/vector4.hpp \
    ../../src/inc/mt_synchro.hpp \
    ../../src/inc/net/net_data_transfer.hpp \
    ../../src/inc/net/net_utils.hpp \
    ../../src/inc/net/socket_poller.hpp \
    ../../src/inc/net/socket_wrapper.hpp \
    ../../src/inc/net/tcp_client.hpp \
    ../../src/inc/net/tcp_server.hpp \
    ../../src/inc/net/udp_client_muxed.hpp \
    ../../src/inc/net/udp_server_muxed.hpp \
    ../../src/inc/net/url.hpp \
    ../../src/inc/sequence_generator.hpp \
    ../../src/inc/serialization.hpp \
    ../../src/inc/so.hpp \
    ../../src/inc/str_util.hpp \
    ../../src/inc/sys_util.hpp \
    ../../src/inc/terminable.hpp \
    ../../src/inc/threaded_queue_processing_base.hpp \
    ../../src/inc/timespec_wrapper.hpp \
    ../../src/inc/unicode_operations.hpp \
    ../../src/ext/cpu_ext.hpp \
    ../../src/ext/crypto_ext.hpp \
    ../../src/ext/file_ext.hpp \
    ../../src/ext/math_ext.hpp \
    ../../src/ext/rand_ext.hpp \
    ../../src/ext/time_ext.hpp \
    ../../src/tealscript_cells.hpp \
    ../../src/tealscript_net.hpp \
    ../../src/tealscript_codegen.hpp \
    ../../src/tealscript_console.hpp \
    ../../src/tealscript_exec_ctx.hpp \
    ../../src/tealscript_expr.hpp \
    ../../src/tealscript_interfaces.hpp \
    ../../src/tealscript_lexer.hpp \
    ../../src/tealscript_parser.hpp \
    ../../src/tealscript_runtime.hpp \
    ../../src/tealscript_statement.hpp \
    ../../src/tealscript_token.hpp \
    ../../src/tealscript_util.hpp \
    ../../src/tealscript_value.hpp

QMAKE_CXXFLAGS += -std=c++20 -march=native -Wno-unused-parameter -Wno-unused-function -Wl,-rpath,.
QMAKE_CXXFLAGS += -ftree-vectorize -mavx2 -ftree-vectorizer-verbose=5
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3
QMAKE_CFLAGS_RELEASE -= -O2
QMAKE_CFLAGS_RELEASE += -O3

LIBS += -lpthread -ldl

DISTFILES += \
    ../../README.md \
    ../alu74181.teal \
    ../alu74181_model.teal \
    ../alu74181_view.teal \
    ../array_buffer_test.teal \
    ../draft.teal \
    ../ex_cli.teal \
    ../ex_srv.teal \
    ../alu74181.png \
    ../example.teal \
    ../extending_example.teal \
    ../external_client.teal \
    ../external_server.teal \
    ../external_value.teal \
    ../kalman_1D_example.teal \
    ../lights.teal \
    ../one_second_limit.teal \
    ../perf_limit.teal \
    ../pid_regulator.teal \
    ../quad_eq.teal \
    ../sockets_server.teal \
    ../tbbt_2cola.teal \
    ../tests.teal \
    ../unix_socket.teal
