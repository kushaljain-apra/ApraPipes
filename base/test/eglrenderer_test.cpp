#include <boost/test/unit_test.hpp>
#include "HostDMA.h"
#include "FileReaderModule.h"
#include "EglRenderer.h"
#include "PipeLine.h"
#include "StatSink.h"
#include "CudaMemCopy.h"
#include "DeviceToDMA.h"
#include "NvTransform.h"
#include "ResizeNPPI.h"

BOOST_AUTO_TEST_SUITE(eglrenderer_tests)

BOOST_AUTO_TEST_CASE(basic, *boost::unit_test::disabled())
{
	Logger::setLogLevel(boost::log::trivial::severity_level::debug);
	auto width = 640;
	auto height = 360;

	FileReaderModuleProps fileReaderProps("./data/Raw_YUV420_640x360");
	fileReaderProps.fps = 60;
	auto fileReader = boost::shared_ptr<FileReaderModule>(new FileReaderModule(fileReaderProps));
	auto metadata = framemetadata_sp(new RawImagePlanarMetadata(width, height, ImageMetadata::ImageType::YUV420, size_t(0), CV_8U, FrameMetadata::MemType::HOST));
	fileReader->addOutputPin(metadata);

	HostDMAProps hostdmaprops;
	hostdmaprops.qlen = 1;
	hostdmaprops.logHealth = true;
	hostdmaprops.logHealthFrequency = 100;

	auto hostdma = boost::shared_ptr<Module>(new HostDMA(hostdmaprops));
	fileReader->setNext(hostdma);

	auto nv_transform = boost::shared_ptr<Module>(new NvTransform(NvTransformProps(ImageMetadata::RGBA)));
	hostdma->setNext(nv_transform);

	auto sink = boost::shared_ptr<Module>(new EglRenderer(EglRendererProps(0, 0, 640, 360)));
	nv_transform->setNext(sink);

	PipeLine p("test");
	p.appendModule(fileReader);
	BOOST_TEST(p.init());


	p.run_all_threaded();

	boost::this_thread::sleep_for(boost::chrono::seconds(10));

	p.stop();
	p.term();

	p.wait_for_all();
}

BOOST_AUTO_TEST_CASE(basicnv12, *boost::unit_test::disabled())
{
	auto width = 640;
	auto height = 360;

	FileReaderModuleProps fileReaderProps("./data/Raw_YUV420_640x360"); // 800 x 800
	fileReaderProps.fps = 30;
	auto fileReader = boost::shared_ptr<FileReaderModule>(new FileReaderModule(fileReaderProps));
	auto metadata = framemetadata_sp(new RawImagePlanarMetadata(width, height, ImageMetadata::ImageType::YUV420, size_t(0), CV_8U, FrameMetadata::MemType::HOST));
	fileReader->addOutputPin(metadata);

	HostDMAProps hostdmaprops;
	hostdmaprops.qlen = 10;
	hostdmaprops.logHealth = true;
	hostdmaprops.logHealthFrequency = 100;

	auto hostdma = boost::shared_ptr<Module>(new HostDMA(hostdmaprops));
	fileReader->setNext(hostdma);

	auto nv_transform = boost::shared_ptr<Module>(new NvTransform(NvTransformProps(ImageMetadata::NV12)));
	hostdma->setNext(nv_transform);

	auto sink = boost::shared_ptr<Module>(new EglRenderer(EglRendererProps(0, 0, 640, 360)));
	nv_transform->setNext(sink);

	PipeLine p("test");
	p.appendModule(fileReader);
	BOOST_TEST(p.init());

	Logger::setLogLevel(boost::log::trivial::severity_level::info);

	p.run_all_threaded();

	boost::this_thread::sleep_for(boost::chrono::seconds(10));
	Logger::setLogLevel(boost::log::trivial::severity_level::error);

	p.stop();
	p.term();

	p.wait_for_all();
}

BOOST_AUTO_TEST_CASE(cudabasic, *boost::unit_test::disabled())
{
	auto width = 800;
	auto height = 800;

	FileReaderModuleProps fileReaderProps("/home/nvidia/Raw_YUV420");
	fileReaderProps.fps = 30;
	auto fileReader = boost::shared_ptr<FileReaderModule>(new FileReaderModule(fileReaderProps));
	auto metadata = framemetadata_sp(new RawImagePlanarMetadata(width, height, ImageMetadata::ImageType::YUV420, size_t(0), CV_8U, FrameMetadata::MemType::HOST));
	fileReader->addOutputPin(metadata);

	auto stream = cudastream_sp(new ApraCudaStream);
	auto copy = boost::shared_ptr<Module>(new CudaMemCopy(CudaMemCopyProps(cudaMemcpyHostToDevice, stream)));
	fileReader->setNext(copy);

	DeviceToDMAProps deviceTodmaprops;
	deviceTodmaprops.qlen = 1;
	deviceTodmaprops.logHealth = true;
	deviceTodmaprops.logHealthFrequency = 100;

	auto devicedma = boost::shared_ptr<Module>(new DeviceToDMA(deviceTodmaprops));
	copy->setNext(devicedma);

	auto nv_transform = boost::shared_ptr<Module>(new NvTransform(NvTransformProps(ImageMetadata::RGBA)));
	devicedma->setNext(nv_transform);

	// StatSinkProps sinkProps;
	// sinkProps.logHealth = true;
	// sinkProps.logHealthFrequency = 100;
	// auto sink = boost::shared_ptr<Module>(new StatSink(sinkProps));
	// hostdma->setNext(sink);

	auto sink = boost::shared_ptr<Module>(new EglRenderer(EglRendererProps(0, 0, 800, 800)));
	nv_transform->setNext(sink);

	PipeLine p("test");
	p.appendModule(fileReader);
	BOOST_TEST(p.init());

	Logger::setLogLevel(boost::log::trivial::severity_level::info);

	p.run_all_threaded();

	boost::this_thread::sleep_for(boost::chrono::seconds(10));
	Logger::setLogLevel(boost::log::trivial::severity_level::error);

	p.stop();
	p.term();

	p.wait_for_all();
}

BOOST_AUTO_TEST_CASE(cudabasicresize, *boost::unit_test::disabled())
{
	auto width = 800;
	auto height = 800;

	FileReaderModuleProps fileReaderProps("/home/nvidia/Raw_YUV420");
	fileReaderProps.fps = 30;
	auto fileReader = boost::shared_ptr<FileReaderModule>(new FileReaderModule(fileReaderProps));
	auto metadata = framemetadata_sp(new RawImagePlanarMetadata(width, height, ImageMetadata::ImageType::YUV420, size_t(0), CV_8U, FrameMetadata::MemType::HOST));
	fileReader->addOutputPin(metadata);

	auto stream = cudastream_sp(new ApraCudaStream);
	auto copy = boost::shared_ptr<Module>(new CudaMemCopy(CudaMemCopyProps(cudaMemcpyHostToDevice, stream)));
	fileReader->setNext(copy);

	auto resize = boost::shared_ptr<Module>(new ResizeNPPI(ResizeNPPIProps(960, 960, stream)));
	copy->setNext(resize);

	DeviceToDMAProps deviceTodmaprops;
	deviceTodmaprops.qlen = 1;
	deviceTodmaprops.logHealth = true;
	deviceTodmaprops.logHealthFrequency = 100;

	auto devicedma = boost::shared_ptr<Module>(new DeviceToDMA(deviceTodmaprops));
	resize->setNext(devicedma);

	auto nv_transform = boost::shared_ptr<Module>(new NvTransform(NvTransformProps(ImageMetadata::RGBA)));
	devicedma->setNext(nv_transform);

	// StatSinkProps sinkProps;
	// sinkProps.logHealth = true;
	// sinkProps.logHealthFrequency = 100;
	// auto sink = boost::shared_ptr<Module>(new StatSink(sinkProps));
	// hostdma->setNext(sink);

	auto sink = boost::shared_ptr<Module>(new EglRenderer(EglRendererProps(0, 0, 960, 960)));
	nv_transform->setNext(sink);

	PipeLine p("test");
	p.appendModule(fileReader);
	BOOST_TEST(p.init());

	Logger::setLogLevel(boost::log::trivial::severity_level::info);

	p.run_all_threaded();

	boost::this_thread::sleep_for(boost::chrono::seconds(10));
	Logger::setLogLevel(boost::log::trivial::severity_level::error);

	p.stop();
	p.term();

	p.wait_for_all();
}

BOOST_AUTO_TEST_CASE(cudabasicimp1rgbarend, *boost::unit_test::disabled())
{
	auto width = 640;
	auto height = 360;

	FileReaderModuleProps fileReaderProps("/home/developer/ApraPipes/data/Raw_YUV420_640x360");
	fileReaderProps.fps = 30;
	auto fileReader = boost::shared_ptr<FileReaderModule>(new FileReaderModule(fileReaderProps));
	auto metadata = framemetadata_sp(new RawImagePlanarMetadata(width, height, ImageMetadata::ImageType::YUV420, size_t(0), CV_8U, FrameMetadata::MemType::HOST));
	fileReader->addOutputPin(metadata);

	auto stream = cudastream_sp(new ApraCudaStream);

	DeviceToDMAProps deviceTodmaprops(stream);
	deviceTodmaprops.qlen = 1;
	deviceTodmaprops.logHealth = true;
	deviceTodmaprops.logHealthFrequency = 100;

	auto devicedma = boost::shared_ptr<Module>(new DeviceToDMA(deviceTodmaprops));
	fileReader->setNext(devicedma);

	auto nv_transform = boost::shared_ptr<Module>(new NvTransform(NvTransformProps(ImageMetadata::RGBA)));
	devicedma->setNext(nv_transform);

	auto sink = boost::shared_ptr<Module>(new EglRenderer(EglRendererProps(0, 0, 640, 360)));
	nv_transform->setNext(sink);

	PipeLine p("test");
	p.appendModule(fileReader);
	BOOST_TEST(p.init());

	Logger::setLogLevel(boost::log::trivial::severity_level::info);

	p.run_all_threaded();

	boost::this_thread::sleep_for(boost::chrono::seconds(10));
	Logger::setLogLevel(boost::log::trivial::severity_level::error);

	p.stop();
	p.term();

	p.wait_for_all();
}

BOOST_AUTO_TEST_SUITE_END()