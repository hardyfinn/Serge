#include "../plugin.hpp"
#include "../dsp/Nto.hpp"
#include "../generated/Layout.hpp"

// PORT shows its time constant, 2 s * knob^3.
struct PortQuantity : ParamQuantity {
	float getDisplayValue() override { float v = getValue(); return 2.f * v * v * v; }
	void setDisplayValue(float d) override { setValue(std::cbrt(std::max(d, 0.f) / 2.f)); }
};

struct NTO : Module {
	enum ParamId { FREQ_PARAM, FINE_PARAM, PORT_PARAM, FM_PARAM, VCFREQ_PARAM, WAVE_PARAM, VCWAVE_PARAM, PARAMS_LEN };
	enum InputId { FREQCV_INPUT, VOCT_INPUT, WAVECV_INPUT, SYNC_INPUT, PORTVOCT_INPUT, LINFM_INPUT, FMVC_INPUT, INPUTS_LEN };
	enum OutputId { SAW_OUTPUT, TRI_OUTPUT, SINE_OUTPUT, VAR_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

	serge::Nto osc;
	OsState os;
	serge::Upsampler upFm;
	serge::Decimator dec[4];
	float prevSync = 0.f;

	NTO() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQ_PARAM, 0.f, 1.f, std::log2(261.6256f / 16.f) / 10.f, "FREQ", " Hz", 1024.f, 16.f);
		configParam(FINE_PARAM, -0.5f, 0.5f, 0.f, "FINE", " oct");
		configParam<PortQuantity>(PORT_PARAM, 0.f, 1.f, 0.f, "PORT (glide time constant on 1V/OCT PORT)", " s");
		configParam(FM_PARAM, 0.f, 1.f, 0.f, "FM (linear FM depth)", "%", 0.f, 100.f);
		configParam(VCFREQ_PARAM, -1.f, 1.f, 0.f, "VC FREQ (FREQ CV amount, oct/V)");
		configParam(WAVE_PARAM, 0.f, 1.f, 0.f, "WAVE (VAR output, sine to saw)", "%", 0.f, 100.f);
		configParam(VCWAVE_PARAM, -1.f, 1.f, 0.f, "VC WAVE (WAVE CV amount)");
		configInput(FREQCV_INPUT, "FREQ CV");
		configInput(VOCT_INPUT, "1V/OCT");
		configInput(WAVECV_INPUT, "WAVE CV");
		configInput(SYNC_INPUT, "SYNC");
		configInput(PORTVOCT_INPUT, "1V/OCT PORT (with portamento)");
		configInput(LINFM_INPUT, "LIN FM");
		configInput(FMVC_INPUT, "FM VC (linear FM depth CV)");
		configOutput(SAW_OUTPUT, "SAW");
		configOutput(TRI_OUTPUT, "TRI");
		configOutput(SINE_OUTPUT, "SINE");
		configOutput(VAR_OUTPUT, "VAR");
		os.setRate(48000.f);
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override {
		os.setRate(e.sampleRate);
		upFm.reset();
		for (auto& d : dec) d.reset();
	}

	void process(const ProcessArgs& args) override {
		float port = osc.portamento(inputs[PORTVOCT_INPUT].getVoltage(), params[PORT_PARAM].getValue(), args.sampleTime);
		float pitch = params[FINE_PARAM].getValue() + inputs[VOCT_INPUT].getVoltage() + port
		              + params[VCFREQ_PARAM].getValue() * inputs[FREQCV_INPUT].getVoltage();
		float fexp = 16.f * std::pow(1024.f, params[FREQ_PARAM].getValue()) * std::exp2(clamp(pitch, -20.f, 20.f));
		float depth = clamp(params[FM_PARAM].getValue() + inputs[FMVC_INPUT].getVoltage() / 5.f, 0.f, 1.f);
		float w = clamp(params[WAVE_PARAM].getValue() + params[VCWAVE_PARAM].getValue() * inputs[WAVECV_INPUT].getVoltage() / 5.f, 0.f, 1.f);

		int F = os.factor;
		float fm[serge::OS_MAX];
		upFm.process(os.kernel, inputs[LINFM_INPUT].getVoltage(), fm);
		float sync = inputs[SYNC_INPUT].getVoltage();
		float b[4][serge::OS_MAX];
		for (int k = 0; k < F; k++) {
			float sk = prevSync + (sync - prevSync) * (k + 1) / F;
			osc.step(os.dtOs(), fexp, depth, fm[k], sk, w);
			b[0][k] = osc.saw; b[1][k] = osc.tri; b[2][k] = osc.sine; b[3][k] = osc.var;
		}
		prevSync = sync;
		for (int j = 0; j < 4; j++)
			outputs[SAW_OUTPUT + j].setVoltage(dec[j].process(os.kernel, b[j]));
	}
};

struct NTOWidget : ModuleWidget {
	NTOWidget(NTO* m) { sergeScaffold(this, m, "NTO"); placeNTO(this, m); }
};

Model* modelNTO = createModel<NTO, NTOWidget>("NTO");
