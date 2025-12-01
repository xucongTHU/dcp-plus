# Third-party dependencies for ad_shadowmode project

set(SENSEAUTO_RSCL /opt/senseauto/senseauto-rscl/active)
set(SENSEAUTO_MSG /opt/senseauto/senseauto-msgs/active)
set(SENSEAUTO_3RDPARTY /opt/senseauto/senseauto-3rdparty/active/3rdparty)

# Include directories
include_directories(
    ${SENSEAUTO_RSCL}
    ${SENSEAUTO_RSCL}/include
    ${SENSEAUTO_MSG}/include
    ${SENSEAUTO_3RDPARTY}/boost/include
    ${SENSEAUTO_3RDPARTY}/capnp/include
    ${SENSEAUTO_3RDPARTY}/gflags/include
    ${SENSEAUTO_3RDPARTY}/glog/include
    ${SENSEAUTO_3RDPARTY}/gtest/include
    ${SENSEAUTO_3RDPARTY}/jsoncpp/include
    ${SENSEAUTO_3RDPARTY}/lz4/include
    ${SENSEAUTO_3RDPARTY}/opencv/include
    ${SENSEAUTO_3RDPARTY}/openssl/include
    ${SENSEAUTO_3RDPARTY}/protobuf/include
    ${SENSEAUTO_3RDPARTY}/uuid/include
    ${SENSEAUTO_3RDPARTY}/yaml-cpp/include
    ${SENSEAUTO_3RDPARTY}/zmq/include
    ${SENSEAUTO_3RDPARTY}/zstd/include
    ${SENSEAUTO_3RDPARTY}/curl/include
)

# Library files
file(GLOB SENSEAUTO_RSCL_LIBS ${SENSEAUTO_RSCL}/lib/*.so ${SENSEAUTO_RSCL}/lib/*.a)
file(GLOB SENSEAUTO_MSG_LIBS ${SENSEAUTO_MSG}/lib/*.so ${SENSEAUTO_MSG}/lib/*.a)

# Third-party libraries
set(SENSEAUTO_LIBS
    ${SENSEAUTO_RSCL_LIBS}
    ${SENSEAUTO_MSG_LIBS}
)
set(THIRD_PARTY_LIBS
    ${SENSEAUTO_3RDPARTY}/curl/lib/libcurl.so
    ${SENSEAUTO_3RDPARTY}/openssl/lib64/libssl.so
    ${SENSEAUTO_3RDPARTY}/openssl/lib64/libcrypto.so
    ${SENSEAUTO_3RDPARTY}/uuid/lib/libuuid.so
    ${SENSEAUTO_3RDPARTY}/capnp/lib/libcapnp.so
    ${SENSEAUTO_3RDPARTY}/lib/libkj.so
    ${SENSEAUTO_3RDPARTY}/yaml-cpp/lib/libyaml-cpp.so
    ${SENSEAUTO_3RDPARTY}/glog/lib/libglog.so
    ${SENSEAUTO_3RDPARTY}/gflags/lib/libgflags.so
    ${SENSEAUTO_3RDPARTY}/protobuf/lib/libprotobuf.so.25
    ${SENSEAUTO_3RDPARTY}/zmq/lib/libzmq.so
    ${SENSEAUTO_3RDPARTY}/zstd/lib/libzstd.so
    ${SENSEAUTO_3RDPARTY}/lz4/lib/liblz4.so
)

# Boost libraries
set(BOOST_LIBS
    ${SENSEAUTO_3RDPARTY}/boost/lib/libboost_filesystem.so
    ${SENSEAUTO_3RDPARTY}/boost/lib/libboost_system.so
    ${SENSEAUTO_3RDPARTY}/boost/lib/libboost_regex.so
)