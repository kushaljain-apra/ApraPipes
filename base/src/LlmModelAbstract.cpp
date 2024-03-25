#include "LlmModelAbstract.h"
#include "FrameContainerQueue.h"

LlmModelAbstract::~LlmModelAbstract() {

}

bool LlmModelAbstract::init() {
  return Module::init();
}

bool LlmModelAbstract::term() {
  return Module::term();
}

bool LlmModelAbstract::step() {
  return Module::step();
}

frame_sp LlmModelAbstract::makeFrame(size_t size) {
  return Module::makeFrame(size);
}

template<class T>
void LlmModelAbstract::addPropsToQueue(T& props) {
  return Module::addPropsToQueue(props);
}

boost::shared_ptr<FrameContainerQueue> LlmModelAbstract::getQue() {
  return Module::getQue();
}

bool LlmModelAbstract::push(frame_container frameContainer) {
  auto que = getQue();
  que->push(frameContainer);
  return true;
}

framemetadata_sp LlmModelAbstract::getFirstInputMetadata() {
  return Module::getFirstInputMetadata();
}

framemetadata_sp LlmModelAbstract::getFirstOutputMetadata() {
  return Module::getFirstOutputMetadata();
}

size_t LlmModelAbstract::getNumberOfInputPins() {
  return Module::getNumberOfInputPins();
}

size_t LlmModelAbstract::getNumberOfOutputPins(bool implicit = true) {
  return Module::getNumberOfOutputPins(implicit);
}