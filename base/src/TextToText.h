#include "Module.h"
#include "ModelStrategy.h"

class TextToTextProps : public ModuleProps {
public:
	TextToTextProps(ModelStrategy::ModelStrategyType _modelStrategyType, std::string _modelPrompt) {
		modelStrategyType = _modelStrategyType;
		modelPrompt = _modelPrompt;
	};

	ModelStrategy::ModelStrategyType modelStrategyType;
	std::string modelPrompt;
	size_t getSerializeSize() {
		return ModuleProps::getSerializeSize() + sizeof(modelStrategyType) + sizeof(modelPrompt);
	}
private:
	friend class boost::serialization::access;
	
	template <class Archive>
	void serialize(Archive& ar, const unsigned int version) {
		ar &boost::serialization::base_object<ModuleProps>(*this);
		ar & modelStrategyType;
		ar & modelPrompt;
	}
};

class TextToText : public Module {
public:
	TextToText(TextToTextProps _props);
	virtual ~TextToText();
	bool init();
	bool term();
	void setProps(TextToTextProps& props);
	TextToTextProps getProps();

protected:
	bool process(frame_container& frames);
	bool processSOS(frame_sp& frame);
	bool validateInputPins();
	bool validateOutputPins();
	void addInputPin(framemetadata_sp& metadata, string& pinId);
	bool handlePropsChange(frame_sp& frame);

private:
	void setMetadata(framemetadata_sp& metadata);
	class Detail;
	boost::shared_ptr<Detail> mDetail;
};