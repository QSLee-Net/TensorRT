#
# SPDX-FileCopyrightText: Copyright (c) 1993-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
if (${TRT_BUILD_ENABLE_NEW_SAMPLES_FLOW})

add_executable(sample_onnx_mnist sampleOnnxMNIST.cpp)
target_link_libraries(sample_onnx_mnist PRIVATE trt_samples_common)
add_dependencies(tensorrt_samples sample_onnx_mnist)

installLibraries(
    TARGETS sample_onnx_mnist
    OPTIONAL
    COMPONENT full
)

else()

set(SAMPLE_SOURCES sampleOnnxMNIST.cpp)

set(SAMPLE_PARSERS "onnx")

include(../CMakeSamplesTemplate.txt)

endif()
