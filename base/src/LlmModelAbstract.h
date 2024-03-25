#pragma once

#include "Module.h"

class LlmModelProps : public ModuleProps {
public:
  enum ModelArchitectureType {
    TRANSFORMER = 0,
    ENCODERDECODER,
    CASUALDECODER,
    PREFIXDECODER
  };

  enum InputType { TEXT = 0, IMAGE, IMAGE_EMBEDDING };

  enum OutputType { TEXT = 0, IMAGE, IMAGE_EMBEDDING };

  enum UseCase { TEXT_TO_TEXT = 0, SCENE_DESCRIPTOR, OCR };

  LlmModelProps() {
    modelArchitecture = ModelArchitectureType::TRANSFORMER;
    inputTypes = {InputType::TEXT};
    outputTypes = {OutputType::TEXT};
    useCases = {UseCase::TEXT_TO_TEXT};
  }

  LlmModelProps(ModelArchitectureType _modelArchitecture,
                std::vector<InputType> _inputTypes,
                std::vector<OutputType> _outputTypes,
                std::vector<UseCase> _useCases) {
    modelArchitecture = _modelArchitecture;
    inputTypes = _inputTypes;
    outputTypes = _outputTypes;
    useCases = _useCases;
  }

  size_t getSerializeSize() {
    return ModuleProps::getSerializeSize() + sizeof(modelArchitecture) +
           sizeof(inputTypes) + sizeof(outputTypes) + sizeof(useCases);
  }

  ModelArchitectureType modelArchitecture;
  std::vector<InputType> inputTypes;
  std::vector<OutputType> outputTypes;
  std::vector<UseCase> useCases;

private:
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive &ar, const unsigned int version) {
    ar &boost::serialization::base_object<ModuleProps>(*this);
    ar & modelArchitecture;
    ar & inputTypes;
    ar & outputTypes;
    ar & useCases;
  }
};

class LlmModelAbstract : protected Module {
public:
  LlmModelAbstract(LlmModelProps props);
  ~LlmModelAbstract();

	virtual bool modelInit() = 0;
	virtual bool modelTerm() = 0;
  virtual bool processPrompt(frame_sp& props, std::string& frameId) = 0;
  virtual bool validateUseCase(LlmModelProps::UseCase useCase) = 0;
  virtual frame_container getFrames() = 0;

	bool init();
	bool term();
  bool step();
	frame_sp makeFrame(size_t size);
	template<class T>
	void addPropsToQueue(T& props);
  boost::shared_ptr<FrameContainerQueue> getQue();
  bool push(frame_container frameContainer);

  framemetadata_sp getFirstInputMetadata();
	framemetadata_sp getFirstOutputMetadata();
  size_t getNumberOfInputPins();
  size_t getNumberOfOutputPins(bool implicit = true);
};