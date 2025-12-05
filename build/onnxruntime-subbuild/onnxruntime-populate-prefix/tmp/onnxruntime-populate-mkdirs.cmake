# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/workspaces/ad_data_closed_loop/3rdparty/onnxruntime"
  "/workspaces/ad_data_closed_loop/build/onnxruntime-build"
  "/workspaces/ad_data_closed_loop/build/onnxruntime-subbuild/onnxruntime-populate-prefix"
  "/workspaces/ad_data_closed_loop/build/onnxruntime-subbuild/onnxruntime-populate-prefix/tmp"
  "/workspaces/ad_data_closed_loop/build/onnxruntime-subbuild/onnxruntime-populate-prefix/src/onnxruntime-populate-stamp"
  "/workspaces/ad_data_closed_loop/build/onnxruntime-subbuild/onnxruntime-populate-prefix/src"
  "/workspaces/ad_data_closed_loop/build/onnxruntime-subbuild/onnxruntime-populate-prefix/src/onnxruntime-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/workspaces/ad_data_closed_loop/build/onnxruntime-subbuild/onnxruntime-populate-prefix/src/onnxruntime-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/workspaces/ad_data_closed_loop/build/onnxruntime-subbuild/onnxruntime-populate-prefix/src/onnxruntime-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
