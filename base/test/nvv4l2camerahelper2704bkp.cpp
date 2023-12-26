#include <boost/test/unit_test.hpp>

#include "PipeLine.h"
#include "NvV4L2Camera.h"
#include "NvTransform.h"
#include "VirtualCameraSink.h"
#include "FileWriterModule.h"
#include "DMAFDToHostCopy.h"
#include "StatSink.h"
#include "EglRenderer.h"
#include "ThumbnailListGenerator.h"
#include "ImageEncoderCV.h"
#include "ColorConversionXForm.h"
#include "AffineTransform.h"
#include "MaskNPPI.h"
#include "CudaStreamSynchronize.h"

BOOST_AUTO_TEST_SUITE(nvv4l2camera_tests)

BOOST_AUTO_TEST_CASE(basic, *boost::unit_test::disabled())
{
	Logger::setLogLevel(boost::log::trivial::severity_level::info);

	auto source = boost::shared_ptr<Module>(new NvV4L2Camera(NvV4L2CameraProps(640, 480, 10)));

	StatSinkProps sinkProps;
	sinkProps.logHealth = true;
	sinkProps.logHealthFrequency = 100;
	auto sink = boost::shared_ptr<Module>(new StatSink(sinkProps));
	source->setNext(sink);

	PipeLine p("test");
	p.appendModule(source);
	BOOST_TEST(p.init());

	p.run_all_threaded();
	boost::this_thread::sleep_for(boost::chrono::seconds(10));
	p.stop();
	p.term();
	p.wait_for_all();
}

BOOST_AUTO_TEST_CASE(save, *boost::unit_test::disabled())
{
	auto source = boost::shared_ptr<Module>(new NvV4L2Camera(NvV4L2CameraProps(640, 480, 10)));

	auto nv_transform = boost::shared_ptr<Module>(new NvTransform(NvTransformProps(ImageMetadata::YUV444)));
	source->setNext(nv_transform);

	auto copySource = boost::shared_ptr<Module>(new DMAFDToHostCopy);
	nv_transform->setNext(copySource);

	auto fileWriter = boost::shared_ptr<Module>(new FileWriterModule(FileWriterModuleProps("./data/testOutput/nvv4l22/frame_????.raw")));
	copySource->setNext(fileWriter);

	PipeLine p("test");
	p.appendModule(source);
	BOOST_TEST(p.init());

	Logger::setLogLevel(boost::log::trivial::severity_level::info);

	p.run_all_threaded();

	boost::this_thread::sleep_for(boost::chrono::seconds(4));
	Logger::setLogLevel(boost::log::trivial::severity_level::error);

	p.stop();
	p.term();

	p.wait_for_all();
}

BOOST_AUTO_TEST_CASE(thumbnail, *boost::unit_test::disabled())
{
	auto source = boost::shared_ptr<Module>(new NvV4L2Camera(NvV4L2CameraProps(640, 480, 10)));

	auto nv_transform = boost::shared_ptr<Module>(new NvTransform(NvTransformProps(ImageMetadata::RGBA)));
	source->setNext(nv_transform);

	auto copySource = boost::shared_ptr<Module>(new DMAFDToHostCopy);
	nv_transform->setNext(copySource);

	// auto conversionType = ColorConversionProps::ConversionType::YUV420PLANAR_TO_RGB;
	// auto colorchange = boost::shared_ptr<ColorConversion>(new ColorConversion(ColorConversionProps(conversionType)));
	// copySource->setNext(colorchange);

	// auto fileWriter = boost::shared_ptr<Module>(new FileWriterModule(FileWriterModuleProps("./data/testOutput/rgb/frame_????.raw", false)));
	// colorchange->setNext(fileWriter);

	// auto thumbnailgenerator = boost::shared_ptr<Module>(new ImageEncoderCV(ImageEncoderCVProps()));
	// copySource->setNext(thumbnailgenerator);

	auto thumbnailgenerator = boost::shared_ptr<ThumbnailListGenerator>(new ThumbnailListGenerator(ThumbnailListGeneratorProps(200, 200, "/home/developer/workspace/ApraPipes/test_1212.jpeg")));
	copySource->setNext(thumbnailgenerator);

	// auto fileWriter = boost::shared_ptr<Module>(new FileWriterModule(FileWriterModuleProps("./data/testOutput/th/frame_????.png")));
	// copySource->setNext(fileWriter);

	PipeLine p("test");
	p.appendModule(source);
	BOOST_TEST(p.init());

	Logger::setLogLevel(boost::log::trivial::severity_level::info);

	p.run_all_threaded();

	boost::this_thread::sleep_for(boost::chrono::seconds(4));

	// auto currThumbnailProps = thumbnailgenerator->getProps();
	// currThumbnailProps.fileToStore = "/home/developer/workspace/ApraPipes/test13.jpeg";
	// thumbnailgenerator->setProps(currThumbnailProps);
	boost::this_thread::sleep_for(boost::chrono::seconds(4));
	Logger::setLogLevel(boost::log::trivial::severity_level::error);

	p.stop();
	p.term();

	p.wait_for_all();
}

BOOST_AUTO_TEST_CASE(vcam, *boost::unit_test::disabled())
{
	Logger::setLogLevel(boost::log::trivial::severity_level::info);
	auto source = boost::shared_ptr<Module>(new NvV4L2Camera(NvV4L2CameraProps(1280, 720, 10)));

	auto copySource = boost::shared_ptr<Module>(new DMAFDToHostCopy);
	source->setNext(copySource);

	auto transform = boost::shared_ptr<Module>(new NvTransform(NvTransformProps(ImageMetadata::YUV420)));
	source->setNext(transform);

	auto copy = boost::shared_ptr<Module>(new DMAFDToHostCopy);
	transform->setNext(copy);

	auto transform2 = boost::shared_ptr<Module>(new NvTransform(NvTransformProps(ImageMetadata::NV12)));
	source->setNext(transform2);

	auto copy2 = boost::shared_ptr<Module>(new DMAFDToHostCopy);
	transform2->setNext(copy2);

	// VirtualCameraSinkProps sinkProps("/dev/video10");
	// sinkProps.logHealth = true;
	// sinkProps.logHealthFrequency = 100;
	// auto sink = boost::shared_ptr<Module>(new VirtualCameraSink(sinkProps));
	// copy->setNext(sink);

	auto fileWriter1 = boost::shared_ptr<Module>(new FileWriterModule(FileWriterModuleProps("./data/testOutput/nvv4l2/uyvy_????.raw")));
	copySource->setNext(fileWriter1);

	auto fileWriter2 = boost::shared_ptr<Module>(new FileWriterModule(FileWriterModuleProps("./data/testOutput/nvv4l2/yuv420_????.raw")));
	copy->setNext(fileWriter2);

	// auto sink = boost::shared_ptr<Module>(new EglRenderer(EglRendererProps(0, 0)));
	// transform->setNext(sink);

	PipeLine p("test");
	p.appendModule(source);
	BOOST_TEST(p.init());

	Logger::setLogLevel(boost::log::trivial::severity_level::info);

	p.run_all_threaded();
	boost::this_thread::sleep_for(boost::chrono::seconds(10));
	Logger::setLogLevel(boost::log::trivial::severity_level::error);

	p.stop();
	p.term();

	p.wait_for_all();
}

BOOST_AUTO_TEST_CASE(atlcam, *boost::unit_test::disabled()) // Getting latency of 130ms, previously we have got around range og 60 to 130
{
	Logger::setLogLevel(boost::log::trivial::severity_level::debug);
	NvV4L2CameraProps nvCamProps(400, 400, 2);
	nvCamProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;

	auto source = boost::shared_ptr<Module>(new NvV4L2Camera(nvCamProps));

	// auto copySource = boost::shared_ptr<Module>(new DMAFDToHostCopy);
	// source->setNext(copySource);

	// auto fileWriter1 = boost::shared_ptr<Module>(new FileWriterModule(FileWriterModuleProps("./data/testOutput/nvv4l2/Frame_????.raw")));
	// copySource->setNext(fileWriter1);

	NvTransformProps nvprops(ImageMetadata::YUV444);
	nvprops.qlen = 1;
	nvprops.logHealth = true;
	nvprops.logHealthFrequency = 100;
	nvprops.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto transform = boost::shared_ptr<Module>(new NvTransform(nvprops));
	source->setNext(transform);

	// auto copySource1 = boost::shared_ptr<Module>(new DMAFDToHostCopy);
	// transform->setNext(copySource1);

	// auto fileWriter11 = boost::shared_ptr<Module>(new FileWriterModule(FileWriterModuleProps("./data/testOutput/nvv4l2232/Frame2_????.raw")));
	// copySource1->setNext(fileWriter11);

	EglRendererProps eglProps(0, 0);
	eglProps.qlen = 1;
	eglProps.fps = 60;
	eglProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	eglProps.logHealth = true;
	eglProps.logHealthFrequency = 100;
	auto sink = boost::shared_ptr<Module>(new EglRenderer(eglProps));
	transform->setNext(sink);

	PipeLine p("test");
	p.appendModule(source);
	BOOST_TEST(p.init());

	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

	p.run_all_threaded();
	boost::this_thread::sleep_for(boost::chrono::seconds(1000000));
	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

	p.stop();
	p.term();

	p.wait_for_all();
}

// BOOST_AUTO_TEST_CASE(atlcamwithaffine, *boost::unit_test::disabled())
// {
// 	Logger::setLogLevel(boost::log::trivial::severity_level::debug);
// 	NvV4L2CameraProps nvCamProps(400, 400, 2);
// 	nvCamProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
// 	auto source = boost::shared_ptr<Module>(new NvV4L2Camera(nvCamProps));

// 	NvTransformProps nvprops(ImageMetadata::YUV444);
// 	nvprops.qlen = 1;
// 	nvprops.logHealth = true;
// 	nvprops.logHealthFrequency = 100;
// 	nvprops.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
// 	auto transform = boost::shared_ptr<Module>(new NvTransform(nvprops));
// 	source->setNext(transform);

// 	auto stream = cudastream_sp(new ApraCudaStream);

// 	AffineTransformProps affineProps(AffineTransformProps::CUBIC, stream, 0, 0, 0, 2);
// 	affineProps.qlen = 1;
// 	affineProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
// 	auto m_affineTransform = boost::shared_ptr<AffineTransform>(new AffineTransform(affineProps));
// 	transform->setNext(m_affineTransform);

// 	NvTransformProps nvprops1(ImageMetadata::RGBA);
// 	nvprops1.qlen = 2;
// 	nvprops1.logHealth = true;
// 	nvprops1.logHealthFrequency = 100;
// 	nvprops1.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
// 	auto m_rgbaTransform = boost::shared_ptr<NvTransform>(new NvTransform(nvprops));
// 	m_affineTransform->setNext(m_rgbaTransform);

// 	// auto sync = boost::shared_ptr<CudaStreamSynchronize>(new CudaStreamSynchronize(CudaStreamSynchronizeProps(stream)));
// 	// m_affineTransform->setNext(sync);

// 	EglRendererProps eglProps(0, 0);
// 	eglProps.qlen = 1;
// 	eglProps.fps = 60;
// 	eglProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
// 	eglProps.logHealth = true;
// 	eglProps.logHealthFrequency = 100;
// 	auto sink = boost::shared_ptr<Module>(new EglRenderer(eglProps));
// 	m_rgbaTransform->setNext(sink);

// 	PipeLine p("test");
// 	p.appendModule(source);
// 	BOOST_TEST(p.init());

// 	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

// 	p.run_all_threaded();
// 	boost::this_thread::sleep_for(boost::chrono::seconds(1000000));
// 	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

// 	p.stop();
// 	p.term();

// 	p.wait_for_all();
// }

BOOST_AUTO_TEST_CASE(atlcamwithaffine, *boost::unit_test::disabled())
{
	Logger::setLogLevel(boost::log::trivial::severity_level::debug);
	NvV4L2CameraProps nvCamProps(400, 400, 2);
	nvCamProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto source = boost::shared_ptr<Module>(new NvV4L2Camera(nvCamProps));

	NvTransformProps nvprops(ImageMetadata::YUV444);
	nvprops.qlen = 1;
	nvprops.logHealth = true;
	nvprops.logHealthFrequency = 100;
	nvprops.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto transform = boost::shared_ptr<Module>(new NvTransform(nvprops));
	source->setNext(transform);

	auto stream = cudastream_sp(new ApraCudaStream);

	AffineTransformProps affineProps(AffineTransformProps::CUBIC, stream, 0, 0, 0, 1);
	affineProps.qlen = 1;
	affineProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto m_affineTransform = boost::shared_ptr<AffineTransform>(new AffineTransform(affineProps));
	transform->setNext(m_affineTransform);

	NvTransformProps nvprops1(ImageMetadata::RGBA);
	nvprops1.qlen = 2;
	nvprops1.logHealth = true;
	nvprops1.logHealthFrequency = 100;
	nvprops1.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto m_rgbaTransform = boost::shared_ptr<NvTransform>(new NvTransform(nvprops));
	m_affineTransform->setNext(m_rgbaTransform);

	// auto sync = boost::shared_ptr<CudaStreamSynchronize>(new CudaStreamSynchronize(CudaStreamSynchronizeProps(stream)));
	// m_affineTransform->setNext(sync);

	EglRendererProps eglProps(0, 0);
	eglProps.qlen = 10;
	eglProps.fps = 30;
	eglProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	eglProps.logHealth = true;
	eglProps.logHealthFrequency = 100;
	auto sink = boost::shared_ptr<Module>(new EglRenderer(eglProps));
	m_rgbaTransform->setNext(sink);

	PipeLine p("test");
	p.appendModule(source);
	BOOST_TEST(p.init());

	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

	p.run_all_threaded();
	boost::this_thread::sleep_for(boost::chrono::seconds(1000000));
	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

	p.stop();
	p.term();

	p.wait_for_all();
}
BOOST_AUTO_TEST_CASE(atlcamwithaffine1, *boost::unit_test::disabled())
{
	Logger::setLogLevel(boost::log::trivial::severity_level::debug);
	NvV4L2CameraProps nvCamProps(400, 400, 2);
	nvCamProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto source = boost::shared_ptr<Module>(new NvV4L2Camera(nvCamProps));

	NvTransformProps nvprops(ImageMetadata::YUV444);
	nvprops.qlen = 1;
	nvprops.logHealth = true;
	nvprops.logHealthFrequency = 100;
	nvprops.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto transform = boost::shared_ptr<Module>(new NvTransform(nvprops));
	source->setNext(transform);

	auto stream = cudastream_sp(new ApraCudaStream);

	AffineTransformProps affineProps(AffineTransformProps::CUBIC, stream, 0, 0, 0, 1);
	affineProps.qlen = 1;
	affineProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto m_affineTransform = boost::shared_ptr<AffineTransform>(new AffineTransform(affineProps));
	transform->setNext(m_affineTransform);

	// auto sync = boost::shared_ptr<CudaStreamSynchronize>(new CudaStreamSynchronize(CudaStreamSynchronizeProps(stream)));
	// m_affineTransform->setNext(sync);

	EglRendererProps eglProps(0, 0);
	eglProps.qlen = 1;
	eglProps.fps = 30;
	eglProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	eglProps.logHealth = true;
	eglProps.logHealthFrequency = 100;
	auto sink = boost::shared_ptr<Module>(new EglRenderer(eglProps));
	m_affineTransform->setNext(sink);

	PipeLine p("test");
	p.appendModule(source);
	BOOST_TEST(p.init());

	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

	p.run_all_threaded();
	boost::this_thread::sleep_for(boost::chrono::seconds(1000000));
	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

	p.stop();
	p.term();

	p.wait_for_all();
}

BOOST_AUTO_TEST_CASE(atlcamwithaffinergba, *boost::unit_test::disabled())
{
	Logger::setLogLevel(boost::log::trivial::severity_level::debug);
	NvV4L2CameraProps nvCamProps(640, 480, 2);
	nvCamProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto source = boost::shared_ptr<Module>(new NvV4L2Camera(nvCamProps));

	NvTransformProps nvprops(ImageMetadata::RGBA);
	nvprops.qlen = 2;
	nvprops.logHealth = true;
	nvprops.logHealthFrequency = 100;
	nvprops.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto transform = boost::shared_ptr<Module>(new NvTransform(nvprops));
	source->setNext(transform);

	auto stream = cudastream_sp(new ApraCudaStream);

	AffineTransformProps affineProps(AffineTransformProps::CUBIC, stream, 0, 0, 0, 2.0);
	affineProps.qlen = 1;
	affineProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto m_affineTransform = boost::shared_ptr<AffineTransform>(new AffineTransform(affineProps));
	transform->setNext(m_affineTransform);

    CudaStreamSynchronizeProps cuctxprops(stream);
	cuctxprops.qlen = 1;
	cuctxprops.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto sync = boost::shared_ptr<CudaStreamSynchronize>(new CudaStreamSynchronize(cuctxprops));
	m_affineTransform->setNext(sync);

	EglRendererProps eglProps(0, 0);
	eglProps.qlen = 1;
	eglProps.fps = 20;
	eglProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	eglProps.logHealth = true;
	eglProps.logHealthFrequency = 100;
	auto sink = boost::shared_ptr<Module>(new EglRenderer(eglProps));
	m_affineTransform->setNext(sink);

	PipeLine p("test");
	p.appendModule(source);
	BOOST_TEST(p.init());

	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

	p.run_all_threaded();
	boost::this_thread::sleep_for(boost::chrono::seconds(1000000));
	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

	p.stop();
	p.term();

	p.wait_for_all();
}

BOOST_AUTO_TEST_CASE(atlcamwithaffinewithsync, *boost::unit_test::disabled())
{
	Logger::setLogLevel(boost::log::trivial::severity_level::debug);
	NvV4L2CameraProps nvCamProps(400, 400, 2);
	nvCamProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto source = boost::shared_ptr<Module>(new NvV4L2Camera(nvCamProps));

	NvTransformProps nvprops(ImageMetadata::YUV444);
	nvprops.qlen = 1;
	nvprops.logHealth = true;
	nvprops.logHealthFrequency = 100;
	nvprops.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto transform = boost::shared_ptr<Module>(new NvTransform(nvprops));
	source->setNext(transform);

	auto stream = cudastream_sp(new ApraCudaStream);

	AffineTransformProps affineProps(AffineTransformProps::CUBIC, stream, 0, 0, 0, 1);
	affineProps.qlen = 1;
	affineProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto m_affineTransform = boost::shared_ptr<AffineTransform>(new AffineTransform(affineProps));
	transform->setNext(m_affineTransform);

	NvTransformProps nvprops1(ImageMetadata::RGBA);
	nvprops1.qlen = 2;
	nvprops1.logHealth = true;
	nvprops1.logHealthFrequency = 100;
	nvprops1.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto m_rgbaTransform = boost::shared_ptr<NvTransform>(new NvTransform(nvprops));
	m_affineTransform->setNext(m_rgbaTransform);

	auto sync = boost::shared_ptr<CudaStreamSynchronize>(new CudaStreamSynchronize(CudaStreamSynchronizeProps(stream)));
	m_rgbaTransform->setNext(sync);

	EglRendererProps eglProps(0, 0);
	eglProps.qlen = 1;
	eglProps.fps = 30;
	eglProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	eglProps.logHealth = true;
	eglProps.logHealthFrequency = 100;
	auto sink = boost::shared_ptr<Module>(new EglRenderer(eglProps));
	sync->setNext(sink);

	PipeLine p("test");
	p.appendModule(source);
	BOOST_TEST(p.init());

	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

	p.run_all_threaded();
	boost::this_thread::sleep_for(boost::chrono::seconds(1000000));
	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

	p.stop();
	p.term();

	p.wait_for_all();
}

BOOST_AUTO_TEST_CASE(atlcamwithmask, *boost::unit_test::disabled())
{
	Logger::setLogLevel(boost::log::trivial::severity_level::debug);
	NvV4L2CameraProps nvCamProps(400, 400, 2);
	nvCamProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto source = boost::shared_ptr<Module>(new NvV4L2Camera(nvCamProps));

	NvTransformProps nvprops(ImageMetadata::YUV444);
	nvprops.qlen = 1;
	nvprops.logHealth = true;
	nvprops.logHealthFrequency = 100;
	nvprops.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto transform = boost::shared_ptr<Module>(new NvTransform(nvprops));
	source->setNext(transform);

	auto stream = cudastream_sp(new ApraCudaStream);

	MaskNPPIProps maskProps(200, 200, 100, MaskNPPIProps::AVAILABLE_MASKS::NONE, stream);
	maskProps.qlen = 1;
	maskProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto m_mask = boost::shared_ptr<MaskNPPI>(new MaskNPPI(maskProps));
	transform->setNext(m_mask);

	// auto sync = boost::shared_ptr<CudaStreamSynchronize>(new CudaStreamSynchronize(CudaStreamSynchronizeProps(stream)));
	// m_mask->setNext(sync);

	EglRendererProps eglProps(0, 0);
	eglProps.qlen = 1;
	eglProps.fps = 60;
	eglProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	eglProps.logHealth = true;
	eglProps.logHealthFrequency = 100;
	auto sink = boost::shared_ptr<Module>(new EglRenderer(eglProps));
	m_mask->setNext(sink);

	PipeLine p("test");
	p.appendModule(source);
	BOOST_TEST(p.init());

	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

	p.run_all_threaded();
	boost::this_thread::sleep_for(boost::chrono::seconds(1000000));
	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

	p.stop();
	p.term();

	p.wait_for_all();
}

BOOST_AUTO_TEST_CASE(atlcamwithaffinewithmask, *boost::unit_test::disabled())
{
	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

	NvV4L2CameraProps nvCamProps(400, 400, 2);
	nvCamProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto source = boost::shared_ptr<Module>(new NvV4L2Camera(nvCamProps));

	NvTransformProps nvprops(ImageMetadata::YUV444);
	nvprops.qlen = 1;
	nvprops.logHealth = true;
	nvprops.logHealthFrequency = 100;
	nvprops.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto transform = boost::shared_ptr<Module>(new NvTransform(nvprops));
	source->setNext(transform);

	auto stream = cudastream_sp(new ApraCudaStream);
	AffineTransformProps affineProps(AffineTransformProps::CUBIC, stream, 0, 0, 0, 1);
	affineProps.qlen = 1;
	affineProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto m_affineTransform = boost::shared_ptr<AffineTransform>(new AffineTransform(affineProps));
	transform->setNext(m_affineTransform);

	MaskNPPIProps maskProps(200, 200, 100, MaskNPPIProps::AVAILABLE_MASKS::NONE, stream);
	maskProps.qlen = 1;
	maskProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto m_mask = boost::shared_ptr<MaskNPPI>(new MaskNPPI(maskProps));
	m_affineTransform->setNext(m_mask);

	auto sync = boost::shared_ptr<CudaStreamSynchronize>(new CudaStreamSynchronize(CudaStreamSynchronizeProps(stream)));
	m_mask->setNext(sync);

	EglRendererProps eglProps(0, 0);
	eglProps.qlen = 1;
	eglProps.fps = 60;
	eglProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	eglProps.logHealth = true;
	eglProps.logHealthFrequency = 100;
	auto sink = boost::shared_ptr<Module>(new EglRenderer(eglProps));
	sync->setNext(sink);

	PipeLine p("test");
	p.appendModule(source);
	BOOST_TEST(p.init());

	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

	p.run_all_threaded();
	boost::this_thread::sleep_for(boost::chrono::seconds(1000000));
	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

	p.stop();
	p.term();

	p.wait_for_all();
}

BOOST_AUTO_TEST_CASE(atlcamwithaffinewithmasknosync, *boost::unit_test::disabled())
{
	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

	NvV4L2CameraProps nvCamProps(400, 400, 2);
	nvCamProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto source = boost::shared_ptr<Module>(new NvV4L2Camera(nvCamProps));

	NvTransformProps nvprops(ImageMetadata::YUV444);
	nvprops.qlen = 1;
	nvprops.logHealth = true;
	nvprops.logHealthFrequency = 100;
	nvprops.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto transform = boost::shared_ptr<Module>(new NvTransform(nvprops));
	source->setNext(transform);

	auto stream = cudastream_sp(new ApraCudaStream);
	AffineTransformProps affineProps(AffineTransformProps::CUBIC, stream, 0, 0, 0, 1);
	affineProps.qlen = 1;
	affineProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto m_affineTransform = boost::shared_ptr<AffineTransform>(new AffineTransform(affineProps));
	transform->setNext(m_affineTransform);

	MaskNPPIProps maskProps(200, 200, 100, MaskNPPIProps::AVAILABLE_MASKS::NONE, stream);
	maskProps.qlen = 1;
	maskProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto m_mask = boost::shared_ptr<MaskNPPI>(new MaskNPPI(maskProps));
	m_affineTransform->setNext(m_mask);

	// auto sync = boost::shared_ptr<CudaStreamSynchronize>(new CudaStreamSynchronize(CudaStreamSynchronizeProps(stream)));
	// m_mask->setNext(sync);

	EglRendererProps eglProps(0, 0);
	eglProps.qlen = 1;
	eglProps.fps = 60;
	eglProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	eglProps.logHealth = true;
	eglProps.logHealthFrequency = 100;
	auto sink = boost::shared_ptr<Module>(new EglRenderer(eglProps));
	m_mask->setNext(sink);

	PipeLine p("test");
	p.appendModule(source);
	BOOST_TEST(p.init());

	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

	p.run_all_threaded();
	boost::this_thread::sleep_for(boost::chrono::seconds(1000000));
	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

	p.stop();
	p.term();

	p.wait_for_all();
}

BOOST_AUTO_TEST_CASE(atlcam1, *boost::unit_test::disabled())
{
	Logger::setLogLevel(boost::log::trivial::severity_level::debug);
	NvV4L2CameraProps nvCamProps(400, 400, 2);
	nvCamProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;

	auto source = boost::shared_ptr<Module>(new NvV4L2Camera(nvCamProps));

	// auto copySource = boost::shared_ptr<Module>(new DMAFDToHostCopy);
	// source->setNext(copySource);

	// auto fileWriter1 = boost::shared_ptr<Module>(new FileWriterModule(FileWriterModuleProps("./data/testOutput/nvv4l2/Frame_????.raw")));
	// copySource->setNext(fileWriter1);

	NvTransformProps nvprops(ImageMetadata::RGBA);
	nvprops.qlen = 1;
	nvprops.logHealth = true;
	nvprops.logHealthFrequency = 100;
	nvprops.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	auto transform = boost::shared_ptr<Module>(new NvTransform(nvprops));
	source->setNext(transform);

	// auto copySource1 = boost::shared_ptr<Module>(new DMAFDToHostCopy);
	// transform->setNext(copySource1);

	// auto fileWriter11 = boost::shared_ptr<Module>(new FileWriterModule(FileWriterModuleProps("./data/testOutput/nvv4l2232/Frame2_????.raw")));
	// copySource1->setNext(fileWriter11);

	EglRendererProps eglProps(0, 0);
	eglProps.qlen = 1;
	eglProps.fps = 60;
	eglProps.quePushStrategyType = QuePushStrategy::NON_BLOCKING_ANY;
	eglProps.logHealth = true;
	eglProps.logHealthFrequency = 100;
	auto sink = boost::shared_ptr<Module>(new EglRenderer(eglProps));
	transform->setNext(sink);

	PipeLine p("test");
	p.appendModule(source);
	BOOST_TEST(p.init());

	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

	p.run_all_threaded();
	boost::this_thread::sleep_for(boost::chrono::seconds(1000000));
	Logger::setLogLevel(boost::log::trivial::severity_level::debug);

	p.stop();
	p.term();

	p.wait_for_all();
}

BOOST_AUTO_TEST_SUITE_END()
