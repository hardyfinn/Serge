#include "../plugin.hpp"
#include "../dsp/Filters.hpp"
#include "../generated/Layout.hpp"

// FREQ knobs: 20 Hz to 20 kHz, exponential.
static float freqKnob(float k) { return 20.f * std::pow(1000.f, k); }
static const float FREQ_1K = std::log(50.f) / std::log(1000.f);

// ---------------------------------------------------------------- VCFQ ------
struct VCFQ : Module {
	enum ParamId { FREQ_PARAM, Q_PARAM, GAIN_PARAM, VCF_PARAM, VCQ_PARAM, PARAMS_LEN };
	enum InputId { FCV_INPUT, QCV_INPUT, VOCT_INPUT, IN_INPUT, AGC_INPUT, TRIG_INPUT, INPUTS_LEN };
	enum OutputId { LOW_OUTPUT, BAND_OUTPUT, HIGH_OUTPUT, NOTCH_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

	serge::Vcfq f;
	OsState os;
	serge::Upsampler up[2];
	serge::Decimator dec[4];
	serge::Smoother smGain;

	VCFQ() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQ_PARAM, 0.f, 1.f, FREQ_1K, "FREQ", " Hz", 1000.f, 20.f);
		configParam(Q_PARAM, 0.f, 1.f, 0.2f, "Q", "", 200.f, 1.f);
		configParam(GAIN_PARAM, 0.f, 2.f, 1.f, "GAIN (level of IN)", "x");
		configParam(VCF_PARAM, -1.f, 1.f, 0.f, "VC F (F CV amount, oct/V)");
		configParam(VCQ_PARAM, 0.f, 1.f, 0.f, "VC Q (Q CV amount)");
		configInput(FCV_INPUT, "F CV");
		configInput(QCV_INPUT, "Q CV");
		configInput(VOCT_INPUT, "1V/OCT");
		configInput(IN_INPUT, "IN (through GAIN)");
		configInput(AGC_INPUT, "AGC IN (level falls as Q rises)");
		configInput(TRIG_INPUT, "TRIG (pings the filter)");
		configOutput(LOW_OUTPUT, "LOW");
		configOutput(BAND_OUTPUT, "BAND");
		configOutput(HIGH_OUTPUT, "HIGH");
		configOutput(NOTCH_OUTPUT, "NOTCH");
		os.setRate(48000.f);
	}
	void onSampleRateChange(const SampleRateChangeEvent& e) override {
		os.setRate(e.sampleRate);
		for (auto& u : up) u.reset();
		for (auto& d : dec) d.reset();
		f.svf.reset();
	}
	void process(const ProcessArgs& args) override {
		float fc = serge::expFreq(freqKnob(params[FREQ_PARAM].getValue()),
		                          inputs[VOCT_INPUT].getVoltage() + params[VCF_PARAM].getValue() * inputs[FCV_INPUT].getVoltage());
		float q = params[Q_PARAM].getValue() + params[VCQ_PARAM].getValue() * inputs[QCV_INPUT].getVoltage() / 5.f;
		f.setup(fc, os.fsOs(), q, os.dtOs());
		f.trigger(inputs[TRIG_INPUT].getVoltage());
		float g = smGain.process(params[GAIN_PARAM].getValue(), args.sampleTime);
		float a[serge::OS_MAX], b[serge::OS_MAX], o[4][serge::OS_MAX];
		up[0].process(os.kernel, g * inputs[IN_INPUT].getVoltage(), a);
		up[1].process(os.kernel, inputs[AGC_INPUT].getVoltage(), b);
		for (int k = 0; k < os.factor; k++) {
			f.step(a[k], b[k]);
			o[0][k] = f.low(); o[1][k] = f.band(); o[2][k] = f.high(); o[3][k] = f.notch();
		}
		for (int j = 0; j < 4; j++)
			if (outputs[LOW_OUTPUT + j].isConnected()) outputs[LOW_OUTPUT + j].setVoltage(dec[j].process(os.kernel, o[j]));
	}
};

struct VCFQWidget : ModuleWidget {
	VCFQWidget(VCFQ* m) { sergeScaffold(this, m, "VCFQ"); placeVCFQ(this, m); }
};
Model* modelVCFQ = createModel<VCFQ, VCFQWidget>("VCFQ");

// ---------------------------------------------------------------- VCFS ------
struct VCFS : Module {
	enum ParamId { FREQ_PARAM, SLOPE_PARAM, MIX_PARAM, VCF_PARAM, VCSLOPE_PARAM, PARAMS_LEN };
	enum InputId { FCV_INPUT, SLOPECV_INPUT, VOCT_INPUT, IN1_INPUT, IN2_INPUT, INPUTS_LEN };
	enum OutputId { LOW_OUTPUT, BAND_OUTPUT, HIGH_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

	serge::Vcfs f;
	OsState os;
	serge::Upsampler up;
	serge::Decimator dec[3];
	serge::Smoother smMix;

	VCFS() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQ_PARAM, 0.f, 1.f, FREQ_1K, "FREQ", " Hz", 1000.f, 20.f);
		configParam(SLOPE_PARAM, 0.f, 1.f, 0.5f, "SLOPE (6 dB/oct, flat 12 dB/oct at centre, resonant)");
		configParam(MIX_PARAM, 0.f, 1.f, 0.5f, "MIX (IN 1 to IN 2)", "%", 0.f, 100.f);
		configParam(VCF_PARAM, -1.f, 1.f, 0.f, "VC F (F CV amount, oct/V)");
		configParam(VCSLOPE_PARAM, -1.f, 1.f, 0.f, "VC SLOPE (SLOPE CV amount)");
		configInput(FCV_INPUT, "F CV");
		configInput(SLOPECV_INPUT, "SLOPE CV");
		configInput(VOCT_INPUT, "1V/OCT");
		configInput(IN1_INPUT, "IN 1");
		configInput(IN2_INPUT, "IN 2");
		configOutput(LOW_OUTPUT, "LOW");
		configOutput(BAND_OUTPUT, "BAND");
		configOutput(HIGH_OUTPUT, "HIGH");
		os.setRate(48000.f);
	}
	void onSampleRateChange(const SampleRateChangeEvent& e) override {
		os.setRate(e.sampleRate);
		up.reset();
		for (auto& d : dec) d.reset();
		f.svf.reset();
	}
	void process(const ProcessArgs& args) override {
		float fc = serge::expFreq(freqKnob(params[FREQ_PARAM].getValue()),
		                          inputs[VOCT_INPUT].getVoltage() + params[VCF_PARAM].getValue() * inputs[FCV_INPUT].getVoltage());
		float s = params[SLOPE_PARAM].getValue() + params[VCSLOPE_PARAM].getValue() * inputs[SLOPECV_INPUT].getVoltage() / 5.f;
		f.setup(fc, os.fsOs(), s);
		float mix = smMix.process(params[MIX_PARAM].getValue(), args.sampleTime);
		float in = (1.f - mix) * inputs[IN1_INPUT].getVoltage() + mix * inputs[IN2_INPUT].getVoltage();
		float a[serge::OS_MAX], o[3][serge::OS_MAX];
		up.process(os.kernel, in, a);
		for (int k = 0; k < os.factor; k++) {
			f.step(a[k]);
			o[0][k] = f.low(); o[1][k] = f.band(); o[2][k] = f.high();
		}
		for (int j = 0; j < 3; j++)
			if (outputs[LOW_OUTPUT + j].isConnected()) outputs[LOW_OUTPUT + j].setVoltage(dec[j].process(os.kernel, o[j]));
	}
};

struct VCFSWidget : ModuleWidget {
	VCFSWidget(VCFS* m) { sergeScaffold(this, m, "VCFS"); placeVCFS(this, m); }
};
Model* modelVCFS = createModel<VCFS, VCFSWidget>("VCFS");

// ---------------------------------------------------------------- RESEQ -----
struct RESEQ : Module {
	enum ParamId { BAND0_PARAM, INPUT_PARAM = BAND0_PARAM + 10, PARAMS_LEN };
	enum InputId { IN_INPUT, INPUTS_LEN };
	enum OutputId { OUT_OUTPUT, COMBUP_OUTPUT, COMBDOWN_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };
	// Named ids for the generated layout.
	enum { BAND1_PARAM = 1, BAND2_PARAM, BAND3_PARAM, BAND4_PARAM, BAND5_PARAM, BAND6_PARAM, BAND7_PARAM, BAND8_PARAM, BAND9_PARAM };

	serge::ResEq eq;
	OsState os;
	serge::Upsampler up;
	serge::Decimator dec[3];
	float last[10];
	bool dirty = true;
	serge::Smoother smIn;

	RESEQ() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		const char* names[10] = {"29 Hz", "61 Hz", "115 Hz", "218 Hz", "411 Hz", "777 Hz", "1.5 kHz", "2.8 kHz", "5.2 kHz", "11 kHz"};
		for (int i = 0; i < 10; i++) {
			configParam(BAND0_PARAM + i, -1.f, 1.f, 0.f, string::f("%s band (centre flat, +-12 dB at 9 and 3 o'clock, resonant beyond 3)", names[i]));
			last[i] = 99.f;
		}
		configParam(INPUT_PARAM, 0.f, 1.f, 0.5f, "INPUT level", "%", 0.f, 100.f);
		configInput(IN_INPUT, "IN");
		configOutput(OUT_OUTPUT, "OUT (all bands)");
		configOutput(COMBUP_OUTPUT, "COMB UP (61, 218, 777, 2.8k, 11k)");
		configOutput(COMBDOWN_OUTPUT, "COMB DOWN (29, 115, 411, 1.5k, 5.2k)");
		os.setRate(48000.f);
	}
	void onSampleRateChange(const SampleRateChangeEvent& e) override {
		os.setRate(e.sampleRate);
		up.reset();
		for (auto& d : dec) d.reset();
		for (auto& b : eq.band) b.reset();
		dirty = true;
	}
	void process(const ProcessArgs& args) override {
		float k[10];
		for (int i = 0; i < 10; i++) {
			k[i] = params[BAND0_PARAM + i].getValue();
			if (k[i] != last[i]) { dirty = true; last[i] = k[i]; }
		}
		if (dirty) { eq.setup(k, os.fsOs()); dirty = false; }
		float g = smIn.process(params[INPUT_PARAM].getValue(), args.sampleTime);
		float x[serge::OS_MAX], o[3][serge::OS_MAX];
		up.process(os.kernel, g * g * 4.f * inputs[IN_INPUT].getVoltage(), x);
		for (int j = 0; j < os.factor; j++) {
			eq.step(x[j]);
			o[0][j] = eq.out; o[1][j] = eq.combUp; o[2][j] = eq.combDown;
		}
		for (int j = 0; j < 3; j++)
			if (outputs[OUT_OUTPUT + j].isConnected()) outputs[OUT_OUTPUT + j].setVoltage(dec[j].process(os.kernel, o[j]));
	}
};

struct RESEQWidget : ModuleWidget {
	RESEQWidget(RESEQ* m) { sergeScaffold(this, m, "RESEQ"); placeRESEQ(this, m); }
};
Model* modelRESEQ = createModel<RESEQ, RESEQWidget>("RESEQ");
