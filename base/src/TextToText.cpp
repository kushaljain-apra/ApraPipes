#include "TextToText.h"
#include "ModelStrategy.h"

class TextToText::Detail {
public:
  Detail(TextToTextProps& _props) : mProps(_props) { 

  }
  ~Detail() { }

  void setProps(TextToTextProps& props)
	{
		mProps = props;
	}

public:
  framemetadata_sp mOutputMetadata;
	std::string mOutputPinId;
  ModelStrategy modelStrategy;
  TextToTextProps mProps;
};

TextToText::TextToText(TextToTextProps _props) : Module(TRANSFORM, "TextToText", _props)
{
	mDetail.reset(new Detail(_props));
}

TextToText::~TextToText() {}

bool TextToText::validateInputPins()
{
	if (getNumberOfInputPins() != 1)
	{
		LOG_ERROR << "<" << getId() << ">::validateInputPins size is expected to be 1. Actual<" << getNumberOfInputPins() << ">";
		return false;
	}

	framemetadata_sp metadata = getFirstInputMetadata();

	FrameMetadata::FrameType frameType = metadata->getFrameType();

	if (frameType != FrameMetadata::TEXT)
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

bool TextToText::validateOutputPins()
{
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

void TextToText::addInputPin(framemetadata_sp& metadata, string& pinId)
{
	Module::addInputPin(metadata, pinId);
	mDetail->mOutputMetadata = framemetadata_sp(new FrameMetadata(FrameMetadata::FrameType::TEXT));
	mDetail->mOutputMetadata->copyHint(*metadata.get());
	mDetail->mOutputPinId = addOutputPin(mDetail->mOutputMetadata);
}

bool TextToText::init()
{
	// Call model initialization here
	return Module::init();
}

bool TextToText::term()
{
	// Call model termination here
	return Module::term();
}

bool TextToText::process(frame_container& frames)
{	
	// Send frames to the model, process it and get the frames back from the model
	// send it to the next module
	return true;
}

void TextToText::setMetadata(framemetadata_sp& metadata)
{
	if (!metadata->isSet())
	{
		return;
	}
}

bool TextToText::processSOS(frame_sp& frame)
{
	auto metadata = frame->getMetadata();
	setMetadata(metadata);
	return true;
}

TextToTextProps TextToText::getProps()
{
	fillProps(mDetail->mProps);
	return mDetail->mProps;
}

bool TextToText::handlePropsChange(frame_sp& frame)
{
	TextToTextProps props(mDetail->mProps.modelStrategyType, mDetail->mProps.modelPrompt);
	auto ret = Module::handlePropsChange(frame, props);
	mDetail->setProps(props);
	return ret;
}

void TextToText::setProps(TextToTextProps& props)
{
	Module::addPropsToQueue(props);
}