#include <npp.h>
#include <opencv2/core.hpp>
#include <CuCtxSynchronize.h>
#include "AffineTransform.h"
#include "FrameMetadata.h"
#include "Frame.h"
#include "Logger.h"
#include "Utils.h"
#include "AIPExceptions.h"
#include "math.h"
#include "ImageMetadata.h"
#include "RawImagePlanarMetadata.h"
#include "DMAFDWrapper.h"
#include "DMAAllocator.h"
// #include <cuda_runtime.h>
#include <fstream>

#define PI 3.14159265

class AffineTransform::Detail
{
public:
	Detail(AffineTransformProps &_props) : props(_props), shiftX(0), shiftY(0), mFrameType(FrameMetadata::GENERAL), mFrameLength(0)
	{
		nppStreamCtx.hStream = props.stream->getCudaStream();
		rowSize[4] = 0;
		height[4] = 0;
	}
	int setInterPolation(AffineTransformProps::Interpolation eInterpolation)
	{
		switch (props.eInterpolation)
		{
		case AffineTransformProps::NN:
			return NppiInterpolationMode::NPPI_INTER_NN;
		case AffineTransformProps::LINEAR:
			return NppiInterpolationMode::NPPI_INTER_LINEAR;
		case AffineTransformProps::CUBIC:
			return NppiInterpolationMode::NPPI_INTER_CUBIC;
		case AffineTransformProps::UNDEFINED:
			return NppiInterpolationMode::NPPI_INTER_UNDEFINED; // not supported
		case AffineTransformProps::CUBIC2P_BSPLINE:
			return NppiInterpolationMode::NPPI_INTER_CUBIC2P_BSPLINE; // not supported
		case AffineTransformProps::CUBIC2P_CATMULLROM:
			return NppiInterpolationMode::NPPI_INTER_CUBIC2P_CATMULLROM;
		case AffineTransformProps::CUBIC2P_B05C03:
			return NppiInterpolationMode::NPPI_INTER_CUBIC2P_B05C03; // not supported
		case AffineTransformProps::SUPER:
			return NppiInterpolationMode::NPPI_INTER_SUPER; // not supported
		case AffineTransformProps::LANCZOS:
			return NppiInterpolationMode::NPPI_INTER_LANCZOS; // not supported
		case AffineTransformProps::LANCZOS3_ADVANCED:
			return NppiInterpolationMode::NPPI_INTER_LANCZOS3_ADVANCED; // not supported
		default:
			throw new AIPException(AIP_NOTEXEPCTED, "Unknown value for Interpolation!");
		}
	}
	~Detail()
	{
	}

	void setMetadata(framemetadata_sp &metadata)
	{
		if (mFrameType != metadata->getFrameType())
		{
			mFrameType = metadata->getFrameType();
			switch (mFrameType)
			{
			case FrameMetadata::RAW_IMAGE:
				mOutputMetadata = framemetadata_sp(new RawImageMetadata(FrameMetadata::MemType::DMABUF));
				break;
			case FrameMetadata::RAW_IMAGE_PLANAR:
				mOutputMetadata = framemetadata_sp(new RawImagePlanarMetadata(FrameMetadata::MemType::DMABUF));
				break;
			default:
				throw AIPException(AIP_FATAL, "Unsupported frameType<" + std::to_string(mFrameType) + ">");
			}
		}

		if (!metadata->isSet())
		{
			return;
		}

		ImageMetadata::ImageType imageType;
		if (mFrameType == FrameMetadata::RAW_IMAGE)
		{
			auto rawMetadata = FrameMetadataFactory::downcast<RawImageMetadata>(metadata);
			int x, y, w, h;
			w = rawMetadata->getWidth();
			h = rawMetadata->getHeight();
			RawImageMetadata outputMetadata(w * props.scale, h * props.scale, rawMetadata->getImageType(), rawMetadata->getType(), rawMetadata->getStep(), rawMetadata->getDepth(), FrameMetadata::DMABUF, true);
			auto rawOutMetadata = FrameMetadataFactory::downcast<RawImageMetadata>(mOutputMetadata);
			rawOutMetadata->setData(outputMetadata);
			imageType = rawMetadata->getImageType();
			depth = rawMetadata->getDepth();
		}

		if (mFrameType == FrameMetadata::RAW_IMAGE_PLANAR)
		{
			auto rawMetadata = FrameMetadataFactory::downcast<RawImagePlanarMetadata>(metadata);
			int x, y;
			float w, h;
			w = rawMetadata->getWidth(0);
			h = rawMetadata->getHeight(0);
			// RawImagePlanarMetadata outputMetadata(640, 480, ImageMetadata::YUV444, 768 , rawMetadata->getDepth(), FrameMetadata::DMABUF);
			// RawImagePlanarMetadata outputMetadata(w * props.scale, h * props.scale, rawMetadata->getImageType(), rawMetadata->getStep(0), rawMetadata->getDepth(), FrameMetadata::DMABUF); // rawMetadata->getStep(0)
			RawImagePlanarMetadata outputMetadata(w, h, rawMetadata->getImageType(), rawMetadata->getStep(0), rawMetadata->getDepth(), FrameMetadata::DMABUF); // rawMetadata->getStep(0)
			auto rawOutMetadata = FrameMetadataFactory::downcast<RawImagePlanarMetadata>(mOutputMetadata);
			rawOutMetadata->setData(outputMetadata);
			imageType = rawMetadata->getImageType();
			depth = rawMetadata->getDepth();
		}

		switch (imageType)
		{
		case ImageMetadata::MONO:
			if (depth != CV_8U)
			{
				throw AIPException(AIP_NOTIMPLEMENTED, "Rotate not supported for bit depth<" + std::to_string(depth) + ">");
			}
			break;
		case ImageMetadata::BGR:
		case ImageMetadata::RGB:
		case ImageMetadata::RGBA:
		case ImageMetadata::BGRA:
		case ImageMetadata::YUV444:
			if (depth != CV_8U)
			{
				throw AIPException(AIP_NOTIMPLEMENTED, "Rotate not supported for bit depth<" + std::to_string(depth) + ">");
			}
			break;
		}
		mFrameLength = mOutputMetadata->getDataSize();
		LOG_ERROR << "Output Metadata Size is" << mOutputMetadata->getDataSize();
		setMetadataHelper(metadata, mOutputMetadata);
		setAffineTransformMatrix();

	}
	bool isOnce = false;

	bool setAffineTransformMatrix()
	{
		int inWidth = srcSize[0].width;
		int inHeight = srcSize[0].height;
		int outWidth = dstSize[0].width;
		int outHeight = dstSize[0].height;

		double inCenterX = inWidth / 2.0;
		double inCenterY = inHeight / 2.0;

		double outCenterX = outWidth / 2.0;
		double outCenterY = outHeight / 2.0;

		double tx = (outCenterX - inCenterX); // translation factor which is used to shift image to center in output image
		double ty = (outCenterY - inCenterY);

		double si, co;
		si = sin(props.angle * PI / 180);
		co = props.scale * cos(props.angle * PI / 180);

		double cx = props.x + (srcSize[0].width / 2); // rotating the image through its center
		double cy = props.y + (srcSize[0].height / 2);

		acoeff[0][0] = co;
		acoeff[0][1] = -si;
		acoeff[0][2] = ((1 - co) * cx + si * cy) + tx; // after rotation we translate it to center of output frame
		acoeff[1][0] = si;
		acoeff[1][1] = co;
		acoeff[1][2] = (-si * cx + (1 - co) * cy) + ty;

		// DMAAllocator::setMetadata(mOutputMetadata, dstSize[0].width, dstSize[0].height, ImageMetadata::YUV444);

	}
	bool compute(void *buffer, void *outBuffer)
	{
		auto status = NPP_SUCCESS;
		auto bufferNPP = static_cast<Npp8u *>(buffer);
		auto outBufferNPP = static_cast<Npp8u *>(outBuffer);

		if (mFrameType == FrameMetadata::RAW_IMAGE_PLANAR) // currently supported only for YUV444
		{
			// void *hostPtr2 = malloc(srcPitch[0] * srcSize[0].height * 3);
			for (auto i = 0; i < channels; i++) // works fine with y plane
			{
				src[i] = bufferNPP + srcNextPtrOffset[i];
				dst[i] = outBufferNPP + dstNextPtrOffset[i];

				// auto cudaStatus = cudaMemcpy2DAsync(dst[i], dstPitch[i], src[i], srcPitch[i], rowSize[i], height[i],
				// 									cudaMemcpyDeviceToDevice, props.stream->getCudaStream());

				// if (cudaStatus != cudaSuccess)
				// {
				// 	// not throwing error and not returning false - next frame will also be attempted
				// 	LOG_ERROR << "cudaMemcpy2DAsync failed <" << cudaStatus << "> Kind :" << cudaMemcpyDeviceToDevice
				// 			  << " from " << (uint64_t)src << " to " << (uint64_t)dst << " for " << dstPitch[i] << "," << srcPitch[i] << "," << rowSize[i] << " x " << height[i];
				// 	// return true;
				// }
				// else
				// {
				// 	LOG_ERROR << "Cuda Copy Passed";
				// 	LOG_ERROR << "cudaMemcpy2DAsync failed <" << cudaStatus << "> Kind :" << cudaMemcpyDeviceToDevice
				// 			  << " from " << (uint64_t)src << " to " << (uint64_t)dst << " for " << dstPitch[i] << "," << srcPitch[i] << "," << rowSize[i] << " x " << height[i];
				// }
			}

			status = nppiWarpAffine_8u_P3R_Ctx(src,
											   srcSize[0],
											   srcPitch[0],
											   srcRect[0],
											   dst,
											   dstPitch[0],
											   dstRect[0],
											   acoeff,
											   setInterPolation(props.eInterpolation),
											   nppStreamCtx);

			// if (!isOnce)
			// {
			// 	void *srchostPtrY = malloc(srcPitch[0] * srcSize[0].height);
			// 	void *srchostPtrU = malloc(srcPitch[0] * srcSize[0].height);
			// 	void *srchostPtrV = malloc(srcPitch[0] * srcSize[0].height);
			// 	void *dsthostPtrY = malloc(srcPitch[0] * srcSize[0].height);
			// 	void *dsthostPtrU = malloc(srcPitch[0] * srcSize[0].height);
			// 	void *dsthostPtrV = malloc(srcPitch[0] * srcSize[0].height);

			// 	cudaMemcpy(srchostPtrY, (void *)src[0], srcPitch[0] * srcSize[0].height, cudaMemcpyDeviceToHost);
			// 	std::ofstream file01("srchostPtrY.raw", std::ios::out | std::ios::binary);
			// 	file01.write((const char *)srchostPtrY, srcPitch[0] * srcSize[0].height);
			// 	file01.close();
			// 	free(srchostPtrY);

			// 	cudaMemcpy(srchostPtrU, (void *)src[1], srcPitch[0] * srcSize[0].height, cudaMemcpyDeviceToHost);
			// 	std::ofstream file02("srchostPtrU.raw", std::ios::out | std::ios::binary);
			// 	file02.write((const char *)srchostPtrU, srcSize[0].width * srcSize[0].height);
			// 	file02.close();
			// 	free(srchostPtrU);

			// 	cudaMemcpy(srchostPtrV, (void *)src[2], srcPitch[0] * srcSize[0].height, cudaMemcpyDeviceToHost);
			// 	std::ofstream file03("srchostPtrV.raw", std::ios::out | std::ios::binary);
			// 	file03.write((const char *)srchostPtrV, srcPitch[0] * srcSize[0].height);
			// 	file03.close();
			// 	free(srchostPtrV);

			// 	cudaMemcpy(dsthostPtrY, (void *)src[0], srcPitch[0] * srcSize[0].height, cudaMemcpyDeviceToHost);
			// 	std::ofstream file11("dsthostPtrY.raw", std::ios::out | std::ios::binary);
			// 	file11.write((const char *)dsthostPtrY, dstPitch[0] * dstSize[0].height);
			// 	file11.close();
			// 	free(dsthostPtrY);

			// 	cudaMemcpy(dsthostPtrU, (void *)src[1], srcPitch[0] * srcSize[0].height, cudaMemcpyDeviceToHost);
			// 	std::ofstream file12("dsthostPtrU.raw", std::ios::out | std::ios::binary);
			// 	file12.write((const char *)dsthostPtrU, dstPitch[0] * dstSize[0].height);
			// 	file12.close();
			// 	free(dsthostPtrU);

			// 	cudaMemcpy(dsthostPtrV, (void *)src[2], srcPitch[0] * srcSize[0].height, cudaMemcpyDeviceToHost);
			// 	std::ofstream file13("dsthostPtrV.raw", std::ios::out | std::ios::binary);
			// 	file13.write((const char *)dsthostPtrV, dstPitch[0] * dstSize[0].height);
			// 	file13.close();
			// 	free(dsthostPtrV);

			// 	isOnce = !isOnce;
			// }
		}

		if (mFrameType == FrameMetadata::RAW_IMAGE)
		{
			if (channels == 1 && depth == CV_8UC1)
			{
				status = nppiWarpAffine_8u_C1R_Ctx(const_cast<const Npp8u *>(bufferNPP),
												   srcSize[0],
												   srcPitch[0],
												   srcRect[0],
												   outBufferNPP,
												   dstPitch[0],
												   dstRect[0],
												   acoeff,
												   setInterPolation(props.eInterpolation),
												   nppStreamCtx);
			}

			else if (channels == 3)
			{
				status = nppiWarpAffine_8u_C3R_Ctx(const_cast<const Npp8u *>(bufferNPP),
												   srcSize[0],
												   srcPitch[0],
												   srcRect[0],
												   outBufferNPP,
												   dstPitch[0],
												   dstRect[0],
												   acoeff,
												   setInterPolation(props.eInterpolation),
												   nppStreamCtx);
			}

			else if (channels == 4)
			{
				status = nppiWarpAffine_8u_C4R_Ctx(const_cast<const Npp8u *>(bufferNPP),
												   srcSize[0],
												   srcPitch[0],
												   srcRect[0],
												   outBufferNPP,
												   dstPitch[0],
												   dstRect[0],
												   acoeff,
												   setInterPolation(props.eInterpolation),
												   nppStreamCtx);
			}
		}
		if (status != NPP_SUCCESS)
		{
			LOG_ERROR << "AffineTransform failed<" << status << ">";
			throw AIPException(AIP_FATAL, "Failed to tranform the image");
		}
		return true;
	}

	void setProps(AffineTransformProps &mprops)
	{
		if (!mOutputMetadata.get())
		{
			return;
		}
		auto rawMetadata = FrameMetadataFactory::downcast<RawImageMetadata>(mOutputMetadata);
		props = mprops;
		LOG_ERROR << "Coming here to set angle" << props.angle;
		setAffineTransformMatrix();
	}

public:
	size_t mFrameLength;
	framemetadata_sp mOutputMetadata;
	std::string mOutputPinId;
	AffineTransformProps props;
	bool setMetadataHelper(framemetadata_sp &input, framemetadata_sp &output)
	{
		if (mFrameType == FrameMetadata::RAW_IMAGE)
		{
			auto inputRawMetadata = FrameMetadataFactory::downcast<RawImageMetadata>(input);
			auto outputRawMetadata = FrameMetadataFactory::downcast<RawImageMetadata>(output);

			channels = inputRawMetadata->getChannels();

			srcSize[0] = {inputRawMetadata->getWidth(), inputRawMetadata->getHeight()};
			srcRect[0] = {0, 0, inputRawMetadata->getWidth(), inputRawMetadata->getHeight()};
			srcPitch[0] = static_cast<int>(inputRawMetadata->getStep());
			srcNextPtrOffset[0] = 0;
			dstSize[0] = {outputRawMetadata->getWidth(), outputRawMetadata->getHeight()};
			dstRect[0] = {0, 0, outputRawMetadata->getWidth(), outputRawMetadata->getHeight()};
			dstPitch[0] = static_cast<int>(outputRawMetadata->getStep());
			dstNextPtrOffset[0] = 0;
			rowSize[0] = inputRawMetadata->getRowSize();
			height[0] = inputRawMetadata->getHeight();
		}
		else if (mFrameType == FrameMetadata::RAW_IMAGE_PLANAR)
		{
			auto inputRawMetadata = FrameMetadataFactory::downcast<RawImagePlanarMetadata>(input);
			auto outputRawMetadata = FrameMetadataFactory::downcast<RawImagePlanarMetadata>(output);

			channels = inputRawMetadata->getChannels();

			for (auto i = 0; i < channels; i++)
			{
				srcSize[i] = {inputRawMetadata->getWidth(i), inputRawMetadata->getHeight(i)};
				srcRect[i] = {0, 0, inputRawMetadata->getWidth(i), inputRawMetadata->getHeight(i)};
				srcPitch[i] = static_cast<int>(inputRawMetadata->getStep(i));
				srcNextPtrOffset[i] = inputRawMetadata->getNextPtrOffset(i);
				rowSize[i] = inputRawMetadata->getRowSize(i);
				height[i] = inputRawMetadata->getHeight(i);
				;

				dstSize[i] = {outputRawMetadata->getWidth(i), outputRawMetadata->getHeight(i)};
				dstRect[i] = {0, 0, outputRawMetadata->getWidth(i), outputRawMetadata->getHeight(i)};
				dstPitch[i] = static_cast<int>(outputRawMetadata->getStep(i));
				dstNextPtrOffset[i] = outputRawMetadata->getNextPtrOffset(i);
			}
		}
		return true;
	}

	FrameMetadata::FrameType mFrameType;
	int depth;
	int channels;
	NppiSize srcSize[4];
	NppiRect srcRect[4];
	int srcPitch[4];
	size_t srcNextPtrOffset[4];
	NppiSize dstSize[4];
	NppiRect dstRect[4];
	int dstPitch[4];
	size_t dstNextPtrOffset[4];
	size_t rowSize[4];
	size_t height[4];

	double shiftX;
	double shiftY;
	void *ctx;
	NppStreamContext nppStreamCtx;

	const Npp8u *src[3];
	Npp8u *dst[3];
	double acoeff[3][3] = {{-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}};
};

AffineTransform::AffineTransform(AffineTransformProps props) : Module(TRANSFORM, "AffineTransform", props)
{
	mDetail.reset(new Detail(props));
}

AffineTransform::~AffineTransform() {}

bool AffineTransform::validateInputPins()
{
	if (getNumberOfInputPins() != 1)
	{
		LOG_ERROR << "<" << getId() << ">::validateInputPins size is expected to be 1. Actual<" << getNumberOfInputPins() << ">";
		return false;
	}

	framemetadata_sp metadata = getFirstInputMetadata();
	FrameMetadata::FrameType frameType = metadata->getFrameType();
	if (frameType != FrameMetadata::RAW_IMAGE && frameType != FrameMetadata::RAW_IMAGE_PLANAR)
	{
		LOG_ERROR << "<" << getId() << ">::validateInputPins input frameType is expected to be RAW_IMAGE or RAW_IMAGE_PLANAR. Actual<" << frameType << ">";
		return false;
	}

	FrameMetadata::MemType memType = metadata->getMemType();
	if (memType != FrameMetadata::MemType::DMABUF)
	{
		LOG_ERROR << "<" << getId() << ">::validateInputPins input memType is expected to be DMABUF. Actual<" << memType << ">";
		return false;
	}

	return true;
}

bool AffineTransform::validateOutputPins()
{
	if (getNumberOfOutputPins() != 1)
	{
		LOG_ERROR << "<" << getId() << ">::validateOutputPins size is expected to be 1. Actual<" << getNumberOfOutputPins() << ">";
		return false;
	}

	framemetadata_sp metadata = getFirstOutputMetadata();
	FrameMetadata::FrameType frameType = metadata->getFrameType();
	if (frameType != FrameMetadata::RAW_IMAGE && frameType != FrameMetadata::RAW_IMAGE_PLANAR)
	{
		LOG_ERROR << "<" << getId() << ">::validateOutputPins input frameType is expected to be RAW_IMAGE or RAW_IMAGE_PLANAR. Actual<" << frameType << ">";
		return false;
	}

	FrameMetadata::MemType memType = metadata->getMemType();
	if (memType != FrameMetadata::MemType::DMABUF)
	{
		LOG_ERROR << "<" << getId() << ">::validateOutputPins input memType is expected to be DMABUF. Actual<" << memType << ">";
		return false;
	}

	return true;
}

void AffineTransform::addInputPin(framemetadata_sp &metadata, string &pinId)
{
	Module::addInputPin(metadata, pinId);

	mDetail->setMetadata(metadata);

	mDetail->mOutputMetadata->copyHint(*metadata.get());
	mDetail->mOutputPinId = addOutputPin(mDetail->mOutputMetadata);
}

bool AffineTransform::init()
{
	if (!Module::init())
	{
		return false;
	}

	return true;
}

bool AffineTransform::term()
{
	mDetail.reset();
	return Module::term();
}

bool AffineTransform::process(frame_container &frames)
{
	LOG_ERROR << "Latest Build";
	auto frame = frames.cbegin()->second;
	auto outFrame = makeFrame();
	cudaMemset(outFrame->data(), 0, outFrame->size());
	mDetail->compute(static_cast<DMAFDWrapper *>(frame->data())->getCudaPtr(), static_cast<DMAFDWrapper *>(outFrame->data())->getCudaPtr());
	frames.insert(make_pair(mDetail->mOutputPinId, outFrame));
	send(frames);

	return true;
}

bool AffineTransform::processSOS(frame_sp &frame)
{
	auto metadata = frame->getMetadata();
	mDetail->setMetadata(metadata);
	return true;
}

bool AffineTransform::shouldTriggerSOS()
{
	return mDetail->mFrameLength == 0;
}

bool AffineTransform::processEOS(string &pinId)
{
	mDetail->mFrameLength = 0;
	return true;
}

void AffineTransform::setProps(AffineTransformProps &props)
{
	LOG_ERROR << "CHANGING ANGLE TO " << props.angle;
	Module::addPropsToQueue(props);
	LOG_ERROR << "CHANGING ANGLE TO " << props.angle;
}

AffineTransformProps AffineTransform::getProps()
{
	fillProps(mDetail->props);
	return mDetail->props;
}

bool AffineTransform::handlePropsChange(frame_sp &frame)
{

	AffineTransformProps props(mDetail->props.stream, 0);
	bool ret = Module::handlePropsChange(frame, props);
	mDetail->setProps(props);
	LOG_ERROR << "CHANGING ANGLE TO ================>>>>>>>>>>>>>>" << props.angle;
	return ret;
}