#include "../plugin.hpp"
#include "../dsp/Shapers.hpp"
#include "../generated/Layout.hpp"

// ---------------------------------------------------------------- TWS -------
struct TWS : Module {
	enum ParamId { VC1_PARAM, VC2_PARAM, VC3_PARAM, PARAMS_LEN };
	enum InputId { IN1_INPUT, VC1_INPUT, IN2_INPUT, VC2_INPUT, IN3_INPUT, VC3_INPUT, INPUTS_LEN };
	enum OutputId { OUT1_OUTPUT, OUT2_OUTPUT, OUT3_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

	OsState os;
	serge::Upsampler up[3];
	serge::Decimator dec[3];
	serge::Smoother sm[3];

	TWS() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		for (int s = 0; s < 3; s++) {
			configParam(VC1_PARAM + s, -1.f, 1.f, 0.f, string::f("%d: VC (VC input amount)", s + 1));
			configInput(IN1_INPUT + 2 * s, string::f("%d: IN", s + 1));
			configInput(VC1_INPUT + 2 * s, string::f("%d: VC (offset into the shaper)", s + 1));
			configOutput(OUT1_OUTPUT + s, string::f("%d: OUT", s + 1));
		}
		os.setRate(48000.f);
	}
	void onSampleRateChange(const SampleRateChangeEvent& e) override {
		os.setRate(e.sampleRate);
		for (auto& u : up) u.reset();
		for (auto& d : dec) d.reset();
	}
	void process(const ProcessArgs& args) override {
		float x[serge::OS_MAX];
		for (int s = 0; s < 3; s++) {
			if (!outputs[OUT1_OUTPUT + s].isConnected()) continue;
			float k = sm[s].process(params[VC1_PARAM + s].getValue(), args.sampleTime);
			up[s].process(os.kernel, inputs[IN1_INPUT + 2 * s].getVoltage() + k * inputs[VC1_INPUT + 2 * s].getVoltage(), x);
			for (int j = 0; j < os.factor; j++) x[j] = serge::tws(x[j]);
			outputs[OUT1_OUTPUT + s].setVoltage(dec[s].process(os.kernel, x));
		}
	}
};
struct TWSWidget : ModuleWidget {
	TWSWidget(TWS* m) { sergeScaffold(this, m, "TWS"); placeTWS(this, m); }
};
Model* modelTWS = createModel<TWS, TWSWidget>("TWS");

// ---------------------------------------------------------------- RING ------
struct RING : Module {
	enum ParamId { MOD_PARAM, CARRIER_PARAM, VCMOD_PARAM, VCCARRIER_PARAM, PARAMS_LEN };
	enum InputId { MODCV_INPUT, CARRIERCV_INPUT, SIGNAL_INPUT, CARRIER_INPUT, INPUTS_LEN };
	enum OutputId { OUT_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

	OsState os;
	serge::Upsampler upS, upC;
	serge::Decimator dec;
	serge::Smoother smM, smC;

	RING() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MOD_PARAM, 0.f, 1.f, 1.f, "MOD (dry, AM at centre, ring)", "%", 0.f, 100.f);
		configParam(CARRIER_PARAM, 0.f, 1.f, 0.5f, "CARRIER level (rounded above centre)", "%", 0.f, 100.f);
		configParam(VCMOD_PARAM, -1.f, 1.f, 0.f, "VC MOD (MOD CV amount)");
		configParam(VCCARRIER_PARAM, -1.f, 1.f, 0.f, "VC CARRIER (CARRIER CV amount)");
		configInput(MODCV_INPUT, "MOD CV");
		configInput(CARRIERCV_INPUT, "CARRIER CV");
		configInput(SIGNAL_INPUT, "SIGNAL");
		configInput(CARRIER_INPUT, "CARRIER");
		configOutput(OUT_OUTPUT, "OUT");
		os.setRate(48000.f);
	}
	void onSampleRateChange(const SampleRateChangeEvent& e) override {
		os.setRate(e.sampleRate);
		upS.reset(); upC.reset(); dec.reset();
	}
	void process(const ProcessArgs& args) override {
		float m = smM.process(clamp(params[MOD_PARAM].getValue() + params[VCMOD_PARAM].getValue() * inputs[MODCV_INPUT].getVoltage() / 5.f, 0.f, 1.f), args.sampleTime);
		float c = smC.process(clamp(params[CARRIER_PARAM].getValue() + params[VCCARRIER_PARAM].getValue() * inputs[CARRIERCV_INPUT].getVoltage() / 5.f, 0.f, 1.f), args.sampleTime);
		float s[serge::OS_MAX], cr[serge::OS_MAX];
		upS.process(os.kernel, inputs[SIGNAL_INPUT].getVoltage(), s);
		upC.process(os.kernel, inputs[CARRIER_INPUT].getVoltage(), cr);
		for (int j = 0; j < os.factor; j++) s[j] = serge::ring(s[j], cr[j], m, c);
		outputs[OUT_OUTPUT].setVoltage(serge::railClip(dec.process(os.kernel, s)));
	}
};
struct RINGWidget : ModuleWidget {
	RINGWidget(RING* m) { sergeScaffold(this, m, "RING"); placeRING(this, m); }
};
Model* modelRING = createModel<RING, RINGWidget>("RING");
