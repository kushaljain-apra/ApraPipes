#pragma once

#include "LlmModelAbstract.h"

class LlavaProps : public LlmModelProps {
public:
  LlavaProps(std::string _modelPath, std::string _prompt, int _modelSize,
             int _contextSize, int _batchSize, float _degreeOfRandomness, int _gpuLayers, int _predictionLength) {
    
    /* Set LLM Model Base Class Properties for each model*/
    modelArchitecture = ModelArchitectureType::TRANSFORMER;
    inputTypes = {InputType::TEXT, InputType::IMAGE_EMBEDDING};
    outputTypes = {OutputType::TEXT};
    useCases = {UseCase::TEXT_TO_TEXT, UseCase::OCR, UseCase::SCENE_DESCRIPTOR};

    /*Unique Model Properties*/
    modelPath = _modelPath;
    prompt = _prompt;
    modelSize = _modelSize;
    degreeOfRandomness = _degreeOfRandomness;
    contextSize = _contextSize;
    batchSize = _batchSize;
    gpuLayers = _gpuLayers;
    predictionLength = _predictionLength;
  }

  std::string modelPath;
  std::string prompt;
  int modelSize;
  int contextSize;
  int batchSize;
  float degreeOfRandomness;
  int gpuLayers;
  int predictionLength;

  size_t getSerializeSize() {
    return LlmModelProps::getSerializeSize() + sizeof(modelPath) +
           sizeof(prompt) + sizeof(float) + 5 * sizeof(int);
  }

private:
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive &ar, const unsigned int version) {
    ar &boost::serialization::base_object<ModuleProps>(*this);
    ar & modelPath & prompt;
    ar & degreeOfRandomness;
    ar & modelSize & contextSize & batchSize & gpuLayers & predictionLength;
  }
};

class Llava : public LlmModelAbstract {
public:
  Llava(LlavaProps _props);
  virtual ~Llava();
  bool modelInit() override;
  bool modelTerm() override;
  bool processPrompt(frame_sp& frame, std::string& frameId) override;
  bool validateUseCase(LlmModelProps::UseCase useCase) override;
  frame_container getFrames() override;

  frame_sp processImageEmbedding(frame_sp& frame);
  void setProps(LlavaProps &props);
  LlavaProps getProps();

protected:
  bool process(frame_container &frames);
  bool validateInputPins();
  bool validateOutputPins();
  bool handlePropsChange(frame_sp &frame);

private:
  class Detail;
  boost::shared_ptr<Detail> mDetail;
};