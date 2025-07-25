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
#ifndef TRT_EMB_LAYER_NORM_VARSEQ_PLUGIN_LEGACY_H
#define TRT_EMB_LAYER_NORM_VARSEQ_PLUGIN_LEGACY_H

#include <cuda.h>

#include "NvInferPlugin.h"
#include "NvInferRuntime.h"

#include "common/bertCommon.h"
#include <string>
#include <vector>

namespace nvinfer1
{
namespace plugin
{
namespace bert
{

template <typename T>
int32_t embSkipLayerNormHFace(cudaStream_t stream, int32_t ld, int32_t B, int32_t S, int32_t const* inputIds,
    int32_t const* tokenIds, int32_t const* cuSeqlens, float const* beta, float const* gamma, T const* wordEmb,
    T const* posEmb, T const* tokEmb, int32_t const wordSize, int32_t const tokSize, T* output);

template <typename T>
int32_t embSkipLayerNormMTron(cudaStream_t stream, int32_t ld, int32_t B, int32_t S, int32_t const* inputIds,
    int32_t const* tokenIds, int32_t const* cuSeqlens, float const* beta, float const* gamma, T const* wordEmb,
    T const* posEmb, T const* tokEmb, int32_t const wordSize, int32_t const tokSize, T* output, T* skip);

class EmbLayerNormVarSeqlenPluginLegacyBase : public nvinfer1::IPluginV2DynamicExt
{
public:
    EmbLayerNormVarSeqlenPluginLegacyBase(std::string const& name, DataType type, Weights const& beta,
        Weights const& gamma, Weights const& word_emb, Weights const& pos_emb, Weights const& tok_emb,
        DataType maskType);

    EmbLayerNormVarSeqlenPluginLegacyBase(std::string const& name, void const* data, size_t length);

    // It doesn't make sense to make EmbLayerNormVarSeqlenPluginLegacy without arguments, so we
    // delete default constructor.
    EmbLayerNormVarSeqlenPluginLegacyBase() = delete;

    // IPluginV2DynamicExt Methods
    bool supportsFormatCombination(
        int32_t pos, nvinfer1::PluginTensorDesc const* inOut, int32_t nbInputs, int32_t nbOutputs) noexcept override;
    size_t getWorkspaceSize(nvinfer1::PluginTensorDesc const* inputs, int32_t nbInputs,
        nvinfer1::PluginTensorDesc const* outputs, int32_t nbOutputs) const noexcept override;

    // IPluginV2Ext Methods
    nvinfer1::DataType getOutputDataType(
        int32_t index, nvinfer1::DataType const* inputTypes, int32_t nbInputs) const noexcept override;

    // IPluginV2 Methods
    char const* getPluginType() const noexcept override;
    int32_t getNbOutputs() const noexcept override;
    size_t getSerializationSize() const noexcept override;
    void serialize(void* buffer) const noexcept override;
    void destroy() noexcept override;
    char const* getPluginNamespace() const noexcept override;
    void setPluginNamespace(char const* pluginNamespace) noexcept override;

protected:
    std::string const mLayerName;
    std::string mNamespace;

    bert::cuda_unique_ptr<float> mGammaDev;
    bert::cuda_unique_ptr<float> mBetaDev;
    bert::cuda_unique_ptr<void> mWordEmbDev;
    bert::cuda_unique_ptr<void> mTokEmbDev;
    bert::cuda_unique_ptr<void> mPosEmbDev;
    size_t mLd; // leading dim = hidden size
    size_t mWordVocabSize;
    size_t mPosVocabSize;
    size_t mTokVocabSize;
    bert::WeightsWithOwnership mBeta;
    bert::WeightsWithOwnership mGamma;
    bert::WeightsWithOwnership mWordEmb;
    bert::WeightsWithOwnership mTokEmb;
    bert::WeightsWithOwnership mPosEmb;
    DataType mType{};
    DataType mMaskType{};
};

class EmbLayerNormVarSeqlenPluginLegacyHFace : public EmbLayerNormVarSeqlenPluginLegacyBase
{
public:
    EmbLayerNormVarSeqlenPluginLegacyHFace(std::string const& name, nvinfer1::DataType const type,
        nvinfer1::Weights const& beta, nvinfer1::Weights const& gamma, nvinfer1::Weights const& word_emb,
        nvinfer1::Weights const& pos_emb, nvinfer1::Weights const& tok_emb);

    EmbLayerNormVarSeqlenPluginLegacyHFace(std::string const& name, void const* data, size_t length);

    // It doesn't make sense to make EmbLayerNormVarSeqlenPluginLegacy without arguments, so we
    // delete default constructor.
    EmbLayerNormVarSeqlenPluginLegacyHFace() = delete;

    // IPluginV2DynamicExt Methods
    nvinfer1::IPluginV2DynamicExt* clone() const noexcept override;
    nvinfer1::DimsExprs getOutputDimensions(int32_t outputIndex, nvinfer1::DimsExprs const* inputs, int32_t nbInputs,
        nvinfer1::IExprBuilder& exprBuilder) noexcept override;
    void configurePlugin(nvinfer1::DynamicPluginTensorDesc const* in, int32_t nbInputs,
        nvinfer1::DynamicPluginTensorDesc const* out, int32_t nbOutputs) noexcept override;
    int32_t enqueue(nvinfer1::PluginTensorDesc const* inputDesc, nvinfer1::PluginTensorDesc const* outputDesc,
        void const* const* inputs, void* const* outputs, void* workspace, cudaStream_t stream) noexcept override;

    // IPluginV2 Methods
    int32_t initialize() noexcept override;
    void terminate() noexcept override;
    void destroy() noexcept override;
    char const* getPluginVersion() const noexcept override;
};

class EmbLayerNormVarSeqlenPluginLegacyMTron : public EmbLayerNormVarSeqlenPluginLegacyBase
{
public:
    EmbLayerNormVarSeqlenPluginLegacyMTron(std::string const& name, nvinfer1::DataType const type,
        nvinfer1::Weights const& beta, nvinfer1::Weights const& gamma, nvinfer1::Weights const& word_emb,
        nvinfer1::Weights const& pos_emb, nvinfer1::Weights const& tok_emb);

    EmbLayerNormVarSeqlenPluginLegacyMTron(std::string const& name, void const* data, size_t length);

    // It doesn't make sense to make EmbLayerNormVarSeqlenPluginLegacy without arguments, so we
    // delete default constructor.
    EmbLayerNormVarSeqlenPluginLegacyMTron() = delete;

    // IPluginV2DynamicExt Methods
    nvinfer1::IPluginV2DynamicExt* clone() const noexcept override;
    nvinfer1::DimsExprs getOutputDimensions(int32_t outputIndex, nvinfer1::DimsExprs const* inputs, int32_t nbInputs,
        nvinfer1::IExprBuilder& exprBuilder) noexcept override;
    void configurePlugin(nvinfer1::DynamicPluginTensorDesc const* in, int32_t nbInputs,
        nvinfer1::DynamicPluginTensorDesc const* out, int32_t nbOutputs) noexcept override;
    int32_t enqueue(nvinfer1::PluginTensorDesc const* inputDesc, nvinfer1::PluginTensorDesc const* outputDesc,
        void const* const* inputs, void* const* outputs, void* workspace, cudaStream_t stream) noexcept override;

    // IPluginV2 Methods
    int32_t initialize() noexcept override;
    void terminate() noexcept override;
    void destroy() noexcept override;
    char const* getPluginVersion() const noexcept override;
};

class EmbLayerNormVarSeqlenPluginLegacyBaseCreator : public nvinfer1::IPluginCreator
{
public:
    EmbLayerNormVarSeqlenPluginLegacyBaseCreator();

    char const* getPluginName() const noexcept override;

    nvinfer1::PluginFieldCollection const* getFieldNames() noexcept override;

    void setPluginNamespace(char const* pluginNamespace) noexcept override;

    char const* getPluginNamespace() const noexcept override;

protected:
    nvinfer1::PluginFieldCollection mFC;
    std::vector<nvinfer1::PluginField> mPluginAttributes;
    std::string mNamespace;
};

class EmbLayerNormVarSeqlenPluginLegacyHFaceCreator : public EmbLayerNormVarSeqlenPluginLegacyBaseCreator
{
public:
    nvinfer1::IPluginV2* createPlugin(char const* name, nvinfer1::PluginFieldCollection const* fc) noexcept override;
    char const* getPluginVersion() const noexcept override;
    nvinfer1::IPluginV2* deserializePlugin(
        char const* name, void const* serialData, size_t serialLength) noexcept override;
};

class EmbLayerNormVarSeqlenPluginLegacyMTronCreator : public EmbLayerNormVarSeqlenPluginLegacyBaseCreator
{
public:
    nvinfer1::IPluginV2* createPlugin(char const* name, nvinfer1::PluginFieldCollection const* fc) noexcept override;
    char const* getPluginVersion() const noexcept override;
    nvinfer1::IPluginV2* deserializePlugin(
        char const* name, void const* serialData, size_t serialLength) noexcept override;
};

} // namespace bert
} // namespace plugin
} // namespace nvinfer1
#endif // TRT_EMB_LAYER_NORM_VARSEQ_PLUGIN_LEGACY_H
