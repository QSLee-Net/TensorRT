/*
 * SPDX-FileCopyrightText: Copyright (c) 1993-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef TRT_ROIALIGN_PLUGIN_H
#define TRT_ROIALIGN_PLUGIN_H

#include "common/plugin.h"
#include <cuda_runtime_api.h>
#include <stdint.h>
#include <vector>

#include "NvInfer.h"
#include "NvInferPlugin.h"

namespace nvinfer1
{
namespace plugin
{

class ROIAlignV3PluginCreator : public nvinfer1::IPluginCreatorV3One
{
public:
    ROIAlignV3PluginCreator();

    ~ROIAlignV3PluginCreator() override = default;

    char const* getPluginName() const noexcept override;

    char const* getPluginVersion() const noexcept override;

    PluginFieldCollection const* getFieldNames() noexcept override;

    IPluginV3* createPlugin(char const* name, PluginFieldCollection const* fc, TensorRTPhase phase) noexcept override;

    void setPluginNamespace(char const* libNamespace) noexcept;

    char const* getPluginNamespace() const noexcept override;

private:
    PluginFieldCollection mFC;
    std::vector<PluginField> mPluginAttributes;
    std::string mNamespace;
};

class ROIAlignV3 : public IPluginV3, public IPluginV3OneCore, public IPluginV3OneBuild, public IPluginV3OneRuntime
{
public:
    ROIAlignV3(int32_t outputHeight, int32_t outputWidth, int32_t samplingRatio, int32_t mode, float spatialScale,
        int32_t aligned);
    ROIAlignV3(ROIAlignV3 const&) = default;
    ~ROIAlignV3() override = default;

    IPluginCapability* getCapabilityInterface(PluginCapabilityType type) noexcept override;

    IPluginV3* clone() noexcept override;

    char const* getPluginName() const noexcept override;

    char const* getPluginVersion() const noexcept override;

    char const* getPluginNamespace() const noexcept override;

    int32_t getNbOutputs() const noexcept override;

    int32_t configurePlugin(DynamicPluginTensorDesc const* in, int32_t nbInputs, DynamicPluginTensorDesc const* out,
        int32_t nbOutputs) noexcept override;

    bool supportsFormatCombination(
        int32_t pos, DynamicPluginTensorDesc const* inOut, int32_t nbInputs, int32_t nbOutputs) noexcept override;

    int32_t getOutputDataTypes(
        DataType* outputTypes, int32_t nbOutputs, DataType const* inputTypes, int32_t nbInputs) const noexcept override;

    int32_t getOutputShapes(DimsExprs const* inputs, int32_t nbInputs, DimsExprs const* shapeInputs,
        int32_t nbShapeInputs, DimsExprs* outputs, int32_t nbOutputs, IExprBuilder& exprBuilder) noexcept override;

    int32_t enqueue(PluginTensorDesc const* inputDesc, PluginTensorDesc const* outputDesc, void const* const* inputs,
        void* const* outputs, void* workspace, cudaStream_t stream) noexcept override;

    int32_t onShapeChange(
        PluginTensorDesc const* in, int32_t nbInputs, PluginTensorDesc const* out, int32_t nbOutputs) noexcept override;

    IPluginV3* attachToContext(IPluginResourceContext* context) noexcept override;

    PluginFieldCollection const* getFieldsToSerialize() noexcept override;

    size_t getWorkspaceSize(DynamicPluginTensorDesc const* inputs, int32_t nbInputs,
        DynamicPluginTensorDesc const* outputs, int32_t nbOutputs) const noexcept override;

    void setPluginNamespace(char const* libNamespace) noexcept;

private:
    int32_t mOutputHeight{};
    int32_t mOutputWidth{};
    int32_t mSamplingRatio{};
    float mSpatialScale{};
    int32_t mMode{};
    int32_t mAligned{};

    int32_t mROICount{};
    int32_t mFeatureLength{}; // number of channels
    int32_t mHeight{};
    int32_t mWidth{};

    int32_t mMaxThreadsPerBlock{};

    std::string mNameSpace{};

    std::vector<nvinfer1::PluginField> mDataToSerialize;
    nvinfer1::PluginFieldCollection mFCToSerialize;
};

} // namespace plugin
} // namespace nvinfer1
#endif // TRT_ROIALIGN_PLUGIN_H
