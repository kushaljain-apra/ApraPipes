#include "Llava.h"
#include "FrameMetadata.h"
#include "FrameMetadataFactory.h"
#include "Frame.h"
#include "Logger.h"
#include "Utils.h"

#include "llama/common.h"
#include "llama/ggml.h"
#include "llama/llama.h"

class Llava::Detail {
public:
  Detail(LlavaProps &_props) : mProps(_props) {
    setContextSize(_props);
    setBatchSize(_props);
    setDegreeOfRandomness(_props);
    setGpuLayers(_props);
  }

  ~Detail();

  void setProps(LlavaProps &_props) {
    mProps = _props;
    updateProps(_props);
  }

  void updateProps(LlavaProps &_props) {
    setContextSize(_props);
    setBatchSize(_props);
    setDegreeOfRandomness(_props);
    setGpuLayers(_props);
  }

  void setContextSize(LlavaProps &_props) {
    mLlavaContextParams.n_ctx = _props.contextSize;
  }

  void setBatchSize(LlavaProps &_props) {
    mLlavaContextParams.n_batch = _props.batchSize;
  }

  void setDegreeOfRandomness(LlavaProps &_props) {
    mLlavaSamplingParams.temp = _props.degreeOfRandomness;
  }

  void setGpuLayers(LlavaProps &_props) {
    mLlavaModelParams.n_gpu_layers = _props.gpuLayers;
  }

public:
  LlavaProps mProps;
  llama_model *mLlavaModel;
  llama_context *mLlavaContext = NULL;
  llama_model_params mLlavaModelParams;
  llama_context_params mLlavaContextParams;
  llama_sampling_params mLlavaSamplingParams;
  llama_batch mLlavaBatch;
  frame_container storedFrames;
};

Llava::Llava(LlavaProps _props) : LlmModelAbstract(_props) {
  mDetail.reset(new Detail(_props));
}

Llava::~Llava() {}

bool Llava::validateInputPins() {
  if (getNumberOfInputPins() != 1)
	{
		LOG_ERROR << "<" << getId() << ">::validateInputPins size is expected to be 1. Actual<" << getNumberOfInputPins() << ">";
		return false;
	}

	framemetadata_sp metadata = getFirstInputMetadata();

	FrameMetadata::FrameType frameType = metadata->getFrameType();

	if (frameType != FrameMetadata::TEXT || frameType != FrameMetadata::IMAGE_EMBEDDING)
	{
		LOG_ERROR << "<" << getId() << ">::validateInputPins input frameType is expected to be Audio. Actual<" << frameType << ">";
		return false;
	}

	FrameMetadata::MemType memType = metadata->getMemType();
	if (memType != FrameMetadata::MemType::HOST)
	{
		LOG_ERROR << "<" << getId() << ">::validateInputPins input memType is expected to be HOST. Actual<" << memType << ">";
		return false;
	}
	return true;
}

bool Llava::validateOutputPins() {
  if (getNumberOfOutputPins() != 1)
	{
		LOG_ERROR << "<" << getId() << ">::validateOutputPins size is expected to be 1. Actual<" << getNumberOfOutputPins() << ">";
		return false;
	}

	framemetadata_sp metadata = getFirstOutputMetadata();
	FrameMetadata::FrameType frameType = metadata->getFrameType();
	if (frameType != FrameMetadata::TEXT)
	{
		LOG_ERROR << "<" << getId() << ">::validateOutputPins input frameType is expected to be TEXT. Actual<" << frameType << ">";
		return false;
	}

	return true;
}

bool Llava::validateUseCase(LlavaProps::UseCase useCase) {
  for(auto validUseCase: mDetail->mProps.useCases) {
    if(validUseCase == useCase) {
      return true;
    }
  }
  return false;
}

bool Llava::modelInit() {
  llama_backend_init(false);
  mDetail->mLlavaModelParams = llama_model_default_params();
  mDetail->mLlavaModel = llama_load_model_from_file(
      mDetail->mProps.modelPath.c_str(), mDetail->mLlavaModelParams);
  mDetail->mLlavaContextParams = llama_context_default_params();
  mDetail->mLlavaContext = llama_new_context_with_model(
      mDetail->mLlavaModel, mDetail->mLlavaContextParams);
  mDetail->mLlavaBatch = llama_batch_init(mDetail->mProps.batchSize, 0, 1);
  return LlmModelAbstract::init();
}

bool Llava::modelTerm() {
  llama_batch_free(mDetail->mLlavaBatch);
  llama_free(mDetail->mLlavaContext);
  llama_free_model(mDetail->mLlavaModel);
  llama_backend_free();
  return LlmModelAbstract::term();
}

bool Llava::processPrompt(frame_sp &frame, std::string& frameId) {

  char* userText = static_cast<char*>(frame->data());
  std::string combinedPrompt(userText);
  combinedPrompt = combinedPrompt + mDetail->mProps.prompt;

  int n_predict = mDetail->mProps.predictionLength;
  int n_batch = llama_n_batch(mDetail->mLlavaContext);

  std::vector<llama_token> tokens_list;
  tokens_list = ::llama_tokenize(mDetail->mLlavaContext, combinedPrompt, true);

  for (size_t i = 0; i < tokens_list.size(); i++) {
    llama_batch_add(mDetail->mLlavaBatch, tokens_list[i], i, {0}, false);
  }

  mDetail->mLlavaBatch.logits[mDetail->mLlavaBatch.n_tokens - 1] = true;

  if (llama_decode(mDetail->mLlavaContext, mDetail->mLlavaBatch) != 0) {
    LOG_ERROR << "LLAMA DECODE ERROR/n" ;
  }

  std::string output = "";

  for(size_t i = 0; i < n_predict; i++) {
    auto n_vocab = llama_n_vocab(mDetail->mLlavaModel);
    auto *logits = llama_get_logits_ith(mDetail->mLlavaContext,
                                        mDetail->mLlavaBatch.n_tokens - 1);

    std::vector<llama_token_data> candidates;
    candidates.reserve(n_vocab);

    for (llama_token token_id = 0; token_id < n_vocab; token_id++) {
      candidates.emplace_back(
          llama_token_data{token_id, logits[token_id], 0.0f});
    }

    llama_token_data_array candidates_p = {candidates.data(),
                                            candidates.size(), false};
    const llama_token new_token_id =
        llama_sample_token_greedy(mDetail->mLlavaContext, &candidates_p);

    if(new_token_id == llama_token_eos(llama_get_model(mDetail->mLlavaContext))) break;
    
    output += llama_token_to_piece(mDetail->mLlavaContext, new_token_id).c_str();
    
    llama_batch_clear(mDetail->mLlavaBatch);
    llama_batch_add(mDetail->mLlavaBatch, new_token_id, i, {0}, true);

    if (llama_decode(mDetail->mLlavaContext, mDetail->mLlavaBatch)) {
      LOG_ERROR << "LLAMA DECODE ERROR/n" ;
    }
  }

  frame_container frames;
  auto outFrame = makeFrame(output.length());
	memcpy(outFrame->data(), output.c_str(), output.length());
  frames.insert(make_pair(frameId, outFrame));
  mDetail->storedFrames = frames;
  return true;
}

// frame_sp Llava::processImageEmbedding(frame_sp& frame){

// }

frame_container Llava::getFrames() {
  return mDetail->storedFrames;
}

bool Llava::process(frame_container &frames) {
  std::string frameId = frames.begin()->first;
  auto frame = frames.begin()->second;
  auto frameType = frame->getMetadata()->getFrameType();

  // if (frameType == FrameMetadata::FrameType::IMAGE_EMBEDDING) {
  //   processImageEmbedding(frame);
  // }
  return processPrompt(frame, frameId);
}

LlavaProps Llava::getProps() {
  fillProps(mDetail->mProps);
  return mDetail->mProps;
}

bool Llava::handlePropsChange(frame_sp &frame) {
  LlavaProps props(mDetail->mProps.modelPath, mDetail->mProps.prompt,
                   mDetail->mProps.modelSize, mDetail->mProps.contextSize,
                   mDetail->mProps.batchSize,
                   mDetail->mProps.degreeOfRandomness,
                   mDetail->mProps.gpuLayers, mDetail->mProps.predictionLength);
  auto ret = LlmModelAbstract::handlePropsChange(frame, props);
  mDetail->setProps(props);
  return ret;
}

void Llava::setProps(LlavaProps &props) {
  if (props.modelPath != mDetail->mProps.modelPath) {
    throw AIPException(AIP_FATAL, "Model Path dynamic change not handled");
  }
  LlmModelAbstract::addPropsToQueue(props);
}