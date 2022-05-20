#pragma once

#include "Module.h"
#include "CudaCommon.h"

class AffineTransformProps : public ModuleProps
{
public:
	AffineTransformProps(cudastream_sp &_stream, double _angle, int _x=0, int _y=0, float _scale=1.0f)
	{
		stream = _stream;
		angle = _angle;
		x = _x;
		y = _y;
		scale = _scale;
	}

	int x=0;
	int y = 0;
	float scale = 1.0f;
	double angle;
	cudastream_sp stream;
	size_t getSerializeSize()
	{
		return ModuleProps::getSerializeSize() + sizeof(int) * 2 + sizeof(double) + sizeof(float) ;
	}

private:
	friend class boost::serialization::access;

	template <class Archive>
	void serialize(Archive &ar, const unsigned int version)
	{
		ar &boost::serialization::base_object<ModuleProps>(*this);
		ar &angle &x &y &scale ;
	}
};

class AffineTransform : public Module
{

public:
	AffineTransform(AffineTransformProps props);
	virtual ~AffineTransform();
	bool init();
	bool term();
	void setProps(AffineTransformProps &props);
	AffineTransformProps getProps();

protected:
	bool process(frame_container &frames);
	bool processSOS(frame_sp &frame);
	bool validateInputPins();
	bool validateOutputPins();
	void addInputPin(framemetadata_sp &metadata, string &pinId); // throws exception if validation fails
	bool shouldTriggerSOS();
	bool processEOS(string &pinId);
	void setProps(AffineTransform);
	bool handlePropsChange(frame_sp &frame);

private:
	class Detail;
	boost::shared_ptr<Detail> mDetail;
};