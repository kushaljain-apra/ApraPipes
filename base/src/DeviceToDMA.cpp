#include "DeviceToDMA.h"
#include "FrameMetadata.h"
#include "Frame.h"
#include "Logger.h"
#include "Utils.h"
#include "AIPExceptions.h"
#include "DMAFDWrapper.h"

DeviceToDMA::DeviceToDMA(DeviceToDMAProps _props) : Module(TRANSFORM, "DeviceToDMA", _props), props(_props), mFrameLength(0)
{
}

DeviceToDMA::~DeviceToDMA() {}

bool DeviceToDMA::validateInputPins()
{
	if (getNumberOfInputPins() != 1)
	{
		LOG_ERROR << "<" << getId() << ">::validateInputPins size is expected to be 1. Actual<" << getNumberOfInputPins() << ">";
		return false;
	}

	return true;
}

bool DeviceToDMA::validateOutputPins()
{
	if (getNumberOfOutputPins() != 1)
	{
		LOG_ERROR << "<" << getId() << ">::validateOutputPins size is expected to be 1. Actual<" << getNumberOfOutputPins() << ">";
		return false;
	}

	framemetadata_sp metadata = getFirstOutputMetadata();
	mOutputFrameType = metadata->getFrameType();

	FrameMetadata::MemType memType = metadata->getMemType();
	if (memType != FrameMetadata::MemType::DMABUF)
	{
		LOG_ERROR << "<" << getId() << ">::validateOutputPins input memType is expected to be DMABUF. Actual<" << memType << ">";
		return false;
	}

	return true;
}

void DeviceToDMA::addInputPin(framemetadata_sp &metadata, string &pinId)
{
	Module::addInputPin(metadata, pinId);
	auto inputRawMetadata = FrameMetadataFactory::downcast<RawImagePlanarMetadata>(metadata);
	mOutputMetadata = framemetadata_sp(new RawImagePlanarMetadata(FrameMetadata::MemType::DMABUF));
	mOutputPinId = addOutputPin(mOutputMetadata);
}

bool DeviceToDMA::init()
{
	if (!Module::init())
	{
		return false;
	}

	return true;
}

bool DeviceToDMA::term()
{
	return Module::term();
}

bool DeviceToDMA::process(frame_container &frames)
{
	auto frame = frames.cbegin()->second;
	LOG_ERROR << "Printing Data Size"
	auto outFrame = makeFrame(mOutputMetadata->getDataSize());
	uint8_t * outBuffer = static_cast<uint8_t *>(static_cast<DMAFDWrapper *>(outFrame->data())->getCudaPtr());
			
	for (auto i = 0; i < mChannels; i++)
	{
		auto src = static_cast<uint8_t *>(frame->data()) + mSrcNextPtrOffset[i];
		auto dst = outBuffer + mDstNextPtrOffset[i];

		LOG_ERROR << "Printing Src Offset" << mSrcNextPtrOffset[i] << "Printing Destination Ptr Offset" << mDstNextPtrOffset[i];
		LOG_ERROR << "Dst Pitch is " << mDstPitch[i] << "Src Pitch is " << mSrcPitch[i] << "Row Size " << mRowSize[i] << "MHeight "<< mHeight[i];
	
		auto cudaStatus = cudaMemcpy2DAsync(dst, mDstPitch[i], src, mSrcPitch[i], mRowSize[i], mHeight[i], cudaMemcpyHostToDevice, props.stream);
		if (cudaStatus != cudaSuccess)
		{
			LOG_ERROR << "Cuda Operation Failed";
			break;
		}
	}

	frames.insert(make_pair(mOutputPinId, outFrame));
	send(frames);
	return true;
}

bool DeviceToDMA::processSOS(frame_sp &frame)
{
	auto metadata = frame->getMetadata();
	setMetadata(metadata);
	return true;
}

void DeviceToDMA::setMetadata(framemetadata_sp &metadata)
{
	mInputFrameType = metadata->getFrameType();

	auto rawImagePlanarMetadata = FrameMetadataFactory::downcast<RawImagePlanarMetadata>(metadata);	
	auto width = rawImagePlanarMetadata->getWidth(0);
	auto height = rawImagePlanarMetadata->getHeight(0);
	// auto type = rawPlanarMetadata->getType();
	auto depth = rawImagePlanarMetadata->getDepth();
	auto inputImageType = rawImagePlanarMetadata->getImageType();
	LOG_ERROR << width;
	LOG_ERROR << height;

	auto rawOutMetadata = FrameMetadataFactory::downcast<RawImagePlanarMetadata>(mOutputMetadata);
	RawImagePlanarMetadata outputMetadata(width, height, inputImageType, 512, depth, FrameMetadata::MemType::DMABUF);
	rawOutMetadata->setData(outputMetadata);

	mFrameLength = mOutputMetadata->getDataSize();

	mChannels = rawImagePlanarMetadata->getChannels();
	for (auto i = 0; i < mChannels; i++)
	{
		mSrcPitch[i] = rawImagePlanarMetadata->getStep(i);
		mSrcNextPtrOffset[i] = rawImagePlanarMetadata->getNextPtrOffset(i);
		mRowSize[i] = rawImagePlanarMetadata->getRowSize(i);
		mHeight[i] = rawImagePlanarMetadata->getHeight(i);
	}

	for (auto i = 0; i < mChannels; i++)
	{
		mDstPitch[i] = rawOutMetadata->getStep(i);
		mDstNextPtrOffset[i] = rawOutMetadata->getNextPtrOffset(i);
	}
}

bool DeviceToDMA::shouldTriggerSOS()
{
	return mFrameLength == 0;
}

bool DeviceToDMA::processEOS(string &pinId)
{
	mFrameLength = 0;
	return true;
}