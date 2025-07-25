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

#include "generateDetectionPlugin.h"
#include "common/plugin.h"
#include <algorithm>
#include <cuda_runtime_api.h>

using namespace nvinfer1;
using namespace plugin;
using nvinfer1::plugin::GenerateDetection;
using nvinfer1::plugin::GenerateDetectionPluginCreator;

#include <fstream>

namespace
{
char const* const kGENERATEDETECTION_PLUGIN_VERSION{"1"};
char const* const kGENERATEDETECTION_PLUGIN_NAME{"GenerateDetection_TRT"};
} // namespace

GenerateDetectionPluginCreator::GenerateDetectionPluginCreator() noexcept
{
    mPluginAttributes.clear();
    mPluginAttributes.emplace_back(PluginField("num_classes", nullptr, PluginFieldType::kINT32, 1));
    mPluginAttributes.emplace_back(PluginField("keep_topk", nullptr, PluginFieldType::kINT32, 1));
    mPluginAttributes.emplace_back(PluginField("score_threshold", nullptr, PluginFieldType::kFLOAT32, 1));
    mPluginAttributes.emplace_back(PluginField("iou_threshold", nullptr, PluginFieldType::kFLOAT32, 1));
    mPluginAttributes.emplace_back(PluginField("image_size", nullptr, PluginFieldType::kINT32, 3));

    mFC.nbFields = mPluginAttributes.size();
    mFC.fields = mPluginAttributes.data();
}

char const* GenerateDetectionPluginCreator::getPluginName() const noexcept
{
    return kGENERATEDETECTION_PLUGIN_NAME;
}

char const* GenerateDetectionPluginCreator::getPluginVersion() const noexcept
{
    return kGENERATEDETECTION_PLUGIN_VERSION;
}

PluginFieldCollection const* GenerateDetectionPluginCreator::getFieldNames() noexcept
{
    return &mFC;
}

IPluginV2Ext* GenerateDetectionPluginCreator::createPlugin(char const* name, PluginFieldCollection const* fc) noexcept
{
    try
    {
        auto image_size = TLTMaskRCNNConfig::IMAGE_SHAPE;
        PluginField const* fields = fc->fields;
        plugin::validateRequiredAttributesExist({"num_classes", "keep_topk", "score_threshold", "iou_threshold"}, fc);

        for (int32_t i = 0; i < fc->nbFields; ++i)
        {
            char const* attrName = fields[i].name;
            if (!strcmp(attrName, "num_classes"))
            {
                PLUGIN_VALIDATE(fields[i].type == PluginFieldType::kINT32);
                mNbClasses = *(static_cast<int32_t const*>(fields[i].data));
            }
            if (!strcmp(attrName, "keep_topk"))
            {
                PLUGIN_VALIDATE(fields[i].type == PluginFieldType::kINT32);
                mKeepTopK = *(static_cast<int32_t const*>(fields[i].data));
            }
            if (!strcmp(attrName, "score_threshold"))
            {
                PLUGIN_VALIDATE(fields[i].type == PluginFieldType::kFLOAT32);
                mScoreThreshold = *(static_cast<float const*>(fields[i].data));
            }
            if (!strcmp(attrName, "iou_threshold"))
            {
                PLUGIN_VALIDATE(fields[i].type == PluginFieldType::kFLOAT32);
                mIOUThreshold = *(static_cast<float const*>(fields[i].data));
            }
            if (!strcmp(attrName, "image_size"))
            {
                PLUGIN_VALIDATE(fields[i].type == PluginFieldType::kINT32);
                auto const dims = static_cast<int32_t const*>(fields[i].data);
                std::copy_n(dims, 3, image_size.d);
            }
        }
        return new GenerateDetection(mNbClasses, mKeepTopK, mScoreThreshold, mIOUThreshold, image_size);
    }
    catch (std::exception const& e)
    {
        caughtError(e);
    }
    return nullptr;
}

IPluginV2Ext* GenerateDetectionPluginCreator::deserializePlugin(
    char const* name, void const* data, size_t length) noexcept
{
    try
    {
        return new GenerateDetection(data, length);
    }
    catch (std::exception const& e)
    {
        caughtError(e);
    }
    return nullptr;
}

GenerateDetection::GenerateDetection(int32_t num_classes, int32_t keep_topk, float score_threshold, float iou_threshold,
    nvinfer1::Dims const& image_size)
    : mNbClasses(num_classes)
    , mKeepTopK(keep_topk)
    , mScoreThreshold(score_threshold)
    , mIOUThreshold(iou_threshold)
    , mImageSize(image_size)
{
    mBackgroundLabel = 0;
    PLUGIN_VALIDATE(mNbClasses > 0);
    PLUGIN_VALIDATE(mKeepTopK > 0);
    PLUGIN_VALIDATE(score_threshold >= 0.0F);
    PLUGIN_VALIDATE(iou_threshold > 0.0F);
    PLUGIN_VALIDATE(mImageSize.nbDims == 3);
    PLUGIN_VALIDATE(mImageSize.d[0] > 0 && mImageSize.d[1] > 0 && mImageSize.d[2] > 0);

    mParam.backgroundLabelId = 0;
    mParam.numClasses = mNbClasses;
    mParam.keepTopK = mKeepTopK;
    mParam.scoreThreshold = mScoreThreshold;
    mParam.iouThreshold = mIOUThreshold;

    mType = DataType::kFLOAT;
}

int32_t GenerateDetection::getNbOutputs() const noexcept
{
    return 1;
}

int32_t GenerateDetection::initialize() noexcept
{
    // Init the regWeight [10, 10, 5, 5]
    mRegWeightDevice = std::make_shared<CudaBind<float>>(4);
    PLUGIN_CUASSERT(cudaMemcpy(static_cast<void*>(mRegWeightDevice->mPtr),
        static_cast<void const*>(TLTMaskRCNNConfig::DETECTION_REG_WEIGHTS), sizeof(float) * 4, cudaMemcpyHostToDevice));

    //@Init the mValidCnt and mDecodedBboxes for max batch size
    std::vector<int32_t> tempValidCnt(mMaxBatchSize, mAnchorsCnt);

    mValidCnt = std::make_shared<CudaBind<int32_t>>(mMaxBatchSize);

    PLUGIN_CUASSERT(cudaMemcpy(mValidCnt->mPtr, static_cast<void*>(tempValidCnt.data()),
        sizeof(int32_t) * mMaxBatchSize, cudaMemcpyHostToDevice));

    return 0;
}

void GenerateDetection::terminate() noexcept {}

void GenerateDetection::destroy() noexcept
{
    delete this;
}

bool GenerateDetection::supportsFormat(DataType type, PluginFormat format) const noexcept
{
    return (type == DataType::kFLOAT && format == PluginFormat::kLINEAR);
}

char const* GenerateDetection::getPluginType() const noexcept
{
    return "GenerateDetection_TRT";
}

char const* GenerateDetection::getPluginVersion() const noexcept
{
    return "1";
}

IPluginV2Ext* GenerateDetection::clone() const noexcept
{
    try
    {
        return new GenerateDetection(*this);
    }
    catch (std::exception const& e)
    {
        caughtError(e);
    }
    return nullptr;
}

void GenerateDetection::setPluginNamespace(char const* libNamespace) noexcept
{
    mNameSpace = libNamespace;
}

char const* GenerateDetection::getPluginNamespace() const noexcept
{
    return mNameSpace.c_str();
}

size_t GenerateDetection::getSerializationSize() const noexcept
{
    return sizeof(int32_t) * 2 + sizeof(float) * 2 + sizeof(int32_t) * 2 + sizeof(nvinfer1::Dims);
}

void GenerateDetection::serialize(void* buffer) const noexcept
{
    char *d = reinterpret_cast<char*>(buffer), *a = d;
    write(d, mNbClasses);
    write(d, mKeepTopK);
    write(d, mScoreThreshold);
    write(d, mIOUThreshold);
    write(d, mMaxBatchSize);
    write(d, mAnchorsCnt);
    write(d, mImageSize);
    PLUGIN_ASSERT(d == a + getSerializationSize());
}

GenerateDetection::GenerateDetection(void const* data, size_t length)
{
    deserialize(static_cast<int8_t const*>(data), length);
}

void GenerateDetection::deserialize(int8_t const* data, size_t length)
{
    auto const* d{data};
    int32_t num_classes = read<int32_t>(d);
    int32_t keep_topk = read<int32_t>(d);
    float score_threshold = read<float>(d);
    float iou_threshold = read<float>(d);
    mMaxBatchSize = read<int32_t>(d);
    mAnchorsCnt = read<int32_t>(d);
    mImageSize = read<nvinfer1::Dims3>(d);
    PLUGIN_VALIDATE(d == data + length);

    mNbClasses = num_classes;
    mKeepTopK = keep_topk;
    mScoreThreshold = score_threshold;
    mIOUThreshold = iou_threshold;

    mParam.backgroundLabelId = 0;
    mParam.numClasses = mNbClasses;
    mParam.keepTopK = mKeepTopK;
    mParam.scoreThreshold = mScoreThreshold;
    mParam.iouThreshold = mIOUThreshold;

    mType = DataType::kFLOAT;
}

void GenerateDetection::check_valid_inputs(nvinfer1::Dims const* inputs, int32_t nbInputDims) noexcept
{
    // classifier_delta_bbox[N, anchors, num_classes*4, 1, 1]
    // classifier_class[N, anchors, num_classes, 1, 1]
    // rpn_rois[N, anchors, 4]
    PLUGIN_ASSERT(nbInputDims == 3);

    // score
    PLUGIN_ASSERT(inputs[1].nbDims == 4 && inputs[1].d[1] == mNbClasses);
    // delta_bbox
    PLUGIN_ASSERT(inputs[0].nbDims == 4 && inputs[0].d[1] == mNbClasses * 4);
    // roi
    PLUGIN_ASSERT(inputs[2].nbDims == 2 && inputs[2].d[1] == 4);
}

size_t GenerateDetection::getWorkspaceSize(int32_t batch_size) const noexcept
{
    RefineDetectionWorkSpace refine(batch_size, mAnchorsCnt, mParam, mType);
    return refine.totalSize;
}

Dims GenerateDetection::getOutputDimensions(int32_t index, Dims const* inputs, int32_t nbInputDims) noexcept
{

    check_valid_inputs(inputs, nbInputDims);
    PLUGIN_ASSERT(index == 0);

    return {2, {mKeepTopK, 6}};
}

int32_t GenerateDetection::enqueue(
    int32_t batch_size, void const* const* inputs, void* const* outputs, void* workspace, cudaStream_t stream) noexcept
{

    void* detections = outputs[0];

    // refine detection
    RefineDetectionWorkSpace refDetcWorkspace(batch_size, mAnchorsCnt, mParam, mType);
    cudaError_t status
        = DetectionPostProcess(stream, batch_size, mAnchorsCnt, static_cast<float*>(mRegWeightDevice->mPtr),
            static_cast<float>(mImageSize.d[1]), // Image Height
            static_cast<float>(mImageSize.d[2]), // Image Width
            DataType::kFLOAT,                    // mType,
            mParam, refDetcWorkspace, workspace,
            inputs[1],       // inputs[InScore]
            inputs[0],       // inputs[InDelta],
            mValidCnt->mPtr, // inputs[InCountValid],
            inputs[2],       // inputs[ROI]
            detections);

    PLUGIN_ASSERT(status == cudaSuccess);
    return status;
}

DataType GenerateDetection::getOutputDataType(
    int32_t index, nvinfer1::DataType const* inputTypes, int32_t nbInputs) const noexcept
{
    // Only DataType::kFLOAT is acceptable by the plugin layer
    return DataType::kFLOAT;
}

// Return true if output tensor is broadcast across a batch.
bool GenerateDetection::isOutputBroadcastAcrossBatch(
    int32_t outputIndex, bool const* inputIsBroadcasted, int32_t nbInputs) const noexcept
{
    return false;
}

// Return true if plugin can use input that is broadcast across batch without replication.
bool GenerateDetection::canBroadcastInputAcrossBatch(int32_t inputIndex) const noexcept
{
    return false;
}

// Configure the layer with input and output data types.
void GenerateDetection::configurePlugin(Dims const* inputDims, int32_t nbInputs, Dims const* outputDims,
    int32_t nbOutputs, DataType const* inputTypes, DataType const* outputTypes, bool const* inputIsBroadcast,
    bool const* outputIsBroadcast, PluginFormat floatFormat, int32_t maxBatchSize) noexcept
{
    check_valid_inputs(inputDims, nbInputs);
    PLUGIN_ASSERT(inputDims[0].d[0] == inputDims[1].d[0] && inputDims[1].d[0] == inputDims[2].d[0]);

    mAnchorsCnt = inputDims[2].d[0];
    mType = inputTypes[0];
    mMaxBatchSize = maxBatchSize;
}

// Attach the plugin object to an execution context and grant the plugin the access to some context resource.
void GenerateDetection::attachToContext(
    cudnnContext* cudnnContext, cublasContext* cublasContext, IGpuAllocator* gpuAllocator) noexcept
{
}

// Detach the plugin object from its execution context.
void GenerateDetection::detachFromContext() noexcept {}
