#include "../plugin.hpp"
#include "../dsp/Noise.hpp"
#include "../dsp/Utility.hpp"
#include "../generated/Layout.hpp"

// ---------------------------------------------------------------- NOI -------
struct NOI : Module {
	enum ParamId { PARAMS_LEN };
	enum InputId { INPUTS_LEN };
	enum OutputId { WHITE_OUTPUT, PINK_OUTPUT, SH_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };
	serge::NoiseSource n;
	NOI() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configOutput(WHITE_OUTPUT, "WHITE");
		configOutput(PINK_OUTPUT, "PINK");
		configOutput(SH_OUTPUT, "S/H SOURCE (slow random 0 to 5 V, for sample and hold)");
		n.rng.seed(random::u64());
		n.setSampleRate(48000.f);
	}
	void onSampleRateChange(const SampleRateChangeEvent& e) override { n.setSampleRate(e.sampleRate); }
	void process(const ProcessArgs&) override {
		n.process();
		outputs[WHITE_OUTPUT].setVoltage(n.white);
		outputs[PINK_OUTPUT].setVoltage(n.pink);
		outputs[SH_OUTPUT].setVoltage(n.sh);
	}
};
struct NOIWidget : ModuleWidget {
	NOIWidget(NOI* m) { sergeScaffold(this, m, "NOI"); placeNOI(this, m); }
};
Model* modelNOI = createModel<NOI, NOIWidget>("NOI");

// ---------------------------------------------------------------- ASR -------
struct ASR : Module {
	enum ParamId { PARAMS_LEN };
	enum InputId { IN_INPUT, PULSE_INPUT, INPUTS_LEN };
	enum OutputId { OUT1_OUTPUT, OUT2_OUTPUT, OUT3_OUTPUT, PULSE_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };
	serge::Asr asr;
	ASR() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(IN_INPUT, "IN");
		configInput(PULSE_INPUT, "PULSE");
		configOutput(OUT1_OUTPUT, "OUT 1");
		configOutput(OUT2_OUTPUT, "OUT 2");
		configOutput(OUT3_OUTPUT, "OUT 3");
		configOutput(PULSE_OUTPUT, "PULSE OUT (for chaining)");
	}
	void process(const ProcessArgs&) override {
		float p = inputs[PULSE_INPUT].getVoltage();
		asr.process(inputs[IN_INPUT].getVoltage(), p);
		for (int i = 0; i < 3; i++) outputs[OUT1_OUTPUT + i].setVoltage(asr.out[i]);
		outputs[PULSE_OUTPUT].setVoltage(asr.pulse.high ? 5.f : 0.f);
	}
};
struct ASRWidget : ModuleWidget {
	ASRWidget(ASR* m) { sergeScaffold(this, m, "ASR"); placeASR(this, m); }
};
Model* modelASR = createModel<ASR, ASRWidget>("ASR");

// ---------------------------------------------------------------- COM -------
struct COM : Module {
	enum ParamId { THRESH_A_PARAM, THRESH_B_PARAM, PARAMS_LEN };
	enum InputId { PLUS_A_INPUT, MINUS_A_INPUT, PLUS_B_INPUT, MINUS_B_INPUT, INPUTS_LEN };
	enum OutputId { OUT_A_OUTPUT, OUT_B_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };
	serge::Comparator c[2];
	COM() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		for (int s = 0; s < 2; s++) {
			const char* n = s ? "B" : "A";
			configParam(THRESH_A_PARAM + s, -5.f, 5.f, 0.f, string::f("%s: THRESHOLD (added to the - input)", n), " V");
			configInput(PLUS_A_INPUT + 2 * s, string::f("%s: +", n));
			configInput(MINUS_A_INPUT + 2 * s, string::f("%s: -", n));
			configOutput(OUT_A_OUTPUT + s, string::f("%s: OUT", n));
		}
	}
	void process(const ProcessArgs&) override {
		for (int s = 0; s < 2; s++)
			outputs[OUT_A_OUTPUT + s].setVoltage(c[s].process(inputs[PLUS_A_INPUT + 2 * s].getVoltage(), inputs[MINUS_A_INPUT + 2 * s].getVoltage(), params[THRESH_A_PARAM + s].getValue()));
	}
};
struct COMWidget : ModuleWidget {
	COMWidget(COM* m) { sergeScaffold(this, m, "COM"); placeCOM(this, m); }
};
Model* modelCOM = createModel<COM, COMWidget>("COM");

// ---------------------------------------------------------------- STR -------
struct STR : Module {
	enum ParamId { PARAMS_LEN };
	enum InputId { INA_INPUT, INB_INPUT, INPUTS_LEN };
	enum OutputId { OUTA_OUTPUT, OUTB_OUTPUT, RS_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };
	serge::Str str;
	STR() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(INA_INPUT, "IN A");
		configInput(INB_INPUT, "IN B");
		configOutput(OUTA_OUTPUT, "OUT A");
		configOutput(OUTB_OUTPUT, "OUT B");
		configOutput(RS_OUTPUT, "R/S (set by A, reset by B)");
	}
	void process(const ProcessArgs&) override {
		str.process(inputs[INA_INPUT].getVoltage(), inputs[INB_INPUT].getVoltage());
		outputs[OUTA_OUTPUT].setVoltage(str.outA);
		outputs[OUTB_OUTPUT].setVoltage(str.outB);
		outputs[RS_OUTPUT].setVoltage(str.outRs);
	}
};
struct STRWidget : ModuleWidget {
	STRWidget(STR* m) { sergeScaffold(this, m, "STR"); placeSTR(this, m); }
};
Model* modelSTR = createModel<STR, STRWidget>("STR");

// ---------------------------------------------------------------- PRC -------
struct PRC : Module {
	enum ParamId { K1_A_PARAM, K2_A_PARAM, K3_A_PARAM, OFFSET_A_PARAM, K1_B_PARAM, K2_B_PARAM, K3_B_PARAM, OFFSET_B_PARAM, PARAMS_LEN };
	enum InputId { IN1_A_INPUT, IN2_A_INPUT, IN3_A_INPUT, IN1_B_INPUT, IN2_B_INPUT, IN3_B_INPUT, INPUTS_LEN };
	enum OutputId { OUT_A_OUTPUT, OUT_B_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };
	PRC() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		for (int s = 0; s < 2; s++) {
			const char* n = s ? "B" : "A";
			for (int i = 0; i < 3; i++) {
				configParam(K1_A_PARAM + 4 * s + i, -1.f, 1.f, 0.f, string::f("%s: IN %d level", n, i + 1), "%", 0.f, 100.f);
				configInput(IN1_A_INPUT + 3 * s + i, string::f("%s: IN %d", n, i + 1));
			}
			configParam(OFFSET_A_PARAM + 4 * s, -5.f, 5.f, 0.f, string::f("%s: OFFSET", n), " V");
			configOutput(OUT_A_OUTPUT + s, string::f("%s: OUT", n));
		}
	}
	void process(const ProcessArgs&) override {
		for (int s = 0; s < 2; s++) {
			float in[3], k[3];
			for (int i = 0; i < 3; i++) { in[i] = inputs[IN1_A_INPUT + 3 * s + i].getVoltage(); k[i] = params[K1_A_PARAM + 4 * s + i].getValue(); }
			outputs[OUT_A_OUTPUT + s].setVoltage(serge::processor(in, k, params[OFFSET_A_PARAM + 4 * s].getValue()));
		}
	}
};
struct PRCWidget : ModuleWidget {
	PRCWidget(PRC* m) { sergeScaffold(this, m, "PRC"); placePRC(this, m); }
};
Model* modelPRC = createModel<PRC, PRCWidget>("PRC");

// ---------------------------------------------------------------- XFAD ------
struct XFAD : Module {
	enum ParamId { FADE_PARAM, GAIN_PARAM, PARAMS_LEN };
	enum InputId { FADECV_INPUT, GAINCV_INPUT, IN1_INPUT, IN2_INPUT, INPUTS_LEN };
	enum OutputId { OUT_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };
	serge::Smoother smF, smG;
	XFAD() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FADE_PARAM, 0.f, 1.f, 0.5f, "FADE (IN 1 to IN 2, equal power)", "%", 0.f, 100.f);
		configParam(GAIN_PARAM, 0.f, 1.f, 1.f, "GAIN", "%", 0.f, 100.f)->randomizeEnabled = false;
		configInput(FADECV_INPUT, "FADE CV");
		configInput(GAINCV_INPUT, "GAIN CV (0 to 5 V, scaled by GAIN)");
		configInput(IN1_INPUT, "IN 1");
		configInput(IN2_INPUT, "IN 2");
		configOutput(OUT_OUTPUT, "OUT");
	}
	void process(const ProcessArgs& args) override {
		float x = smF.process(clamp(params[FADE_PARAM].getValue() + inputs[FADECV_INPUT].getVoltage() / 5.f, 0.f, 1.f), args.sampleTime);
		float g = params[GAIN_PARAM].getValue();
		if (inputs[GAINCV_INPUT].isConnected()) g *= inputs[GAINCV_INPUT].getVoltage() / 5.f;
		g = smG.process(clamp(g, 0.f, 1.f), args.sampleTime);
		outputs[OUT_OUTPUT].setVoltage(serge::railClip(serge::crossfade(inputs[IN1_INPUT].getVoltage(), inputs[IN2_INPUT].getVoltage(), x, g)));
	}
};
struct XFADWidget : ModuleWidget {
	XFADWidget(XFAD* m) { sergeScaffold(this, m, "XFAD"); placeXFAD(this, m); }
};
Model* modelXFAD = createModel<XFAD, XFADWidget>("XFAD");

// ---------------------------------------------------------------- VCA2 ------
struct VCA2 : Module {
	enum ParamId { GAIN_A_PARAM, GAIN_B_PARAM, PARAMS_LEN };
	enum InputId { IN_A_INPUT, VC_A_INPUT, IN_B_INPUT, VC_B_INPUT, INPUTS_LEN };
	enum OutputId { OUT_A_OUTPUT, OUT_B_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };
	serge::Smoother sm[2];
	VCA2() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		for (int s = 0; s < 2; s++) {
			const char* n = s ? "B" : "A";
			configParam(GAIN_A_PARAM + s, 0.f, 1.f, 0.f, string::f("%s: GAIN (adds to VC)", n), " V", 0.f, 5.5f)->randomizeEnabled = false;
			configInput(IN_A_INPUT + 2 * s, string::f("%s: IN", n));
			configInput(VC_A_INPUT + 2 * s, string::f("%s: VC (12 dB/V, unity at 5 V)", n));
			configOutput(OUT_A_OUTPUT + s, string::f("%s: OUT", n));
		}
	}
	void process(const ProcessArgs& args) override {
		for (int s = 0; s < 2; s++) {
			float v = inputs[VC_A_INPUT + 2 * s].getVoltage() + 5.5f * params[GAIN_A_PARAM + s].getValue();
			float g = sm[s].process(serge::vca2Gain(v), args.sampleTime);
			outputs[OUT_A_OUTPUT + s].setVoltage(serge::railClip(g * inputs[IN_A_INPUT + 2 * s].getVoltage()));
		}
	}
};
struct VCA2Widget : ModuleWidget {
	VCA2Widget(VCA2* m) { sergeScaffold(this, m, "VCA2"); placeVCA2(this, m); }
};
Model* modelVCA2 = createModel<VCA2, VCA2Widget>("VCA2");

// ---------------------------------------------------------------- MIX -------
struct MIX : Module {
	enum ParamId { L1_A_PARAM, L2_A_PARAM, L3_A_PARAM, L1_B_PARAM, L2_B_PARAM, L3_B_PARAM, PARAMS_LEN };
	enum InputId { IN1_A_INPUT, IN2_A_INPUT, IN3_A_INPUT, IN4_A_INPUT, IN1_B_INPUT, IN2_B_INPUT, IN3_B_INPUT, IN4_B_INPUT, INPUTS_LEN };
	enum OutputId { OUT_A_OUTPUT, OUT_B_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };
	MIX() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		for (int s = 0; s < 2; s++) {
			const char* n = s ? "B" : "A";
			for (int i = 0; i < 3; i++) {
				configParam(L1_A_PARAM + 3 * s + i, 0.f, 1.f, 0.f, string::f("%s: IN %d level", n, i + 1), "%", 0.f, 100.f)->randomizeEnabled = false;
				configInput(IN1_A_INPUT + 4 * s + i, string::f("%s: IN %d", n, i + 1));
			}
			configInput(IN4_A_INPUT + 4 * s, string::f("%s: IN 4 (unity)", n));
			configOutput(OUT_A_OUTPUT + s, string::f("%s: OUT", n));
		}
	}
	void process(const ProcessArgs&) override {
		for (int s = 0; s < 2; s++) {
			float sum = inputs[IN4_A_INPUT + 4 * s].getVoltage();
			for (int i = 0; i < 3; i++) {
				float k = params[L1_A_PARAM + 3 * s + i].getValue();
				sum += k * k * inputs[IN1_A_INPUT + 4 * s + i].getVoltage();
			}
			outputs[OUT_A_OUTPUT + s].setVoltage(serge::railClip(sum));
		}
	}
};
struct MIXWidget : ModuleWidget {
	MIXWidget(MIX* m) { sergeScaffold(this, m, "MIX"); placeMIX(this, m); }
};
Model* modelMIX = createModel<MIX, MIXWidget>("MIX");

// ---------------------------------------------------------------- DCSM ------
struct DCSM : Module {
	enum ParamId { GAIN1_PARAM, PAN1_PARAM, GAIN2_PARAM, PAN2_PARAM, PARAMS_LEN };
	enum InputId { GAINCV1_INPUT, IN1_INPUT, PANCV1_INPUT, GAINCV2_INPUT, IN2_INPUT, PANCV2_INPUT, AUXL_INPUT, AUXR_INPUT, INPUTS_LEN };
	enum OutputId { LEFT_OUTPUT, RIGHT_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };
	serge::Smoother smG[2], smP[2];
	DCSM() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		for (int s = 0; s < 2; s++) {
			configParam(GAIN1_PARAM + 2 * s, 0.f, 1.f, 1.f, string::f("%d: GAIN", s + 1), "%", 0.f, 100.f)->randomizeEnabled = false;
			configParam(PAN1_PARAM + 2 * s, 0.f, 1.f, 0.5f, string::f("%d: PAN (left to right)", s + 1), "%", 0.f, 100.f);
			configInput(GAINCV1_INPUT + 3 * s, string::f("%d: GAIN CV (0 to 5 V)", s + 1));
			configInput(IN1_INPUT + 3 * s, string::f("%d: IN", s + 1));
			configInput(PANCV1_INPUT + 3 * s, string::f(s ? "%d: PAN CV (0 to 5 V pans right to left)" : "%d: PAN CV (0 to 5 V pans left to right)", s + 1));
		}
		configInput(AUXL_INPUT, "AUX L");
		configInput(AUXR_INPUT, "AUX R");
		configOutput(LEFT_OUTPUT, "LEFT");
		configOutput(RIGHT_OUTPUT, "RIGHT");
	}
	void process(const ProcessArgs& args) override {
		float L = inputs[AUXL_INPUT].getVoltage(), R = inputs[AUXR_INPUT].getVoltage();
		for (int s = 0; s < 2; s++) {
			float g = params[GAIN1_PARAM + 2 * s].getValue();
			if (inputs[GAINCV1_INPUT + 3 * s].isConnected()) g *= clamp(inputs[GAINCV1_INPUT + 3 * s].getVoltage() / 5.f, 0.f, 1.f);
			g = smG[s].process(g, args.sampleTime);
			float cv = inputs[PANCV1_INPUT + 3 * s].getVoltage() / 5.f;
			float pan = smP[s].process(clamp(params[PAN1_PARAM + 2 * s].getValue() + (s ? -cv : cv), 0.f, 1.f), args.sampleTime);
			float l, r;
			serge::panLaw(pan, l, r);
			float in = inputs[IN1_INPUT + 3 * s].getVoltage();
			L += g * l * in;
			R += g * r * in;
		}
		outputs[LEFT_OUTPUT].setVoltage(serge::railClip(L));
		outputs[RIGHT_OUTPUT].setVoltage(serge::railClip(R));
	}
};
struct DCSMWidget : ModuleWidget {
	DCSMWidget(DCSM* m) { sergeScaffold(this, m, "DCSM"); placeDCSM(this, m); }
};
Model* modelDCSM = createModel<DCSM, DCSMWidget>("DCSM");

// ---------------------------------------------------------------- PT --------
struct PT : Module {
	enum ParamId { PARAMS_LEN };
	enum InputId { A_INPUT, B_INPUT, INPUTS_LEN };
	enum OutputId { PEAK_OUTPUT, TROUGH_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };
	PT() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(A_INPUT, "A");
		configInput(B_INPUT, "B");
		configOutput(PEAK_OUTPUT, "PEAK (higher of A and B)");
		configOutput(TROUGH_OUTPUT, "TROUGH (lower of A and B)");
	}
	void process(const ProcessArgs&) override {
		bool a = inputs[A_INPUT].isConnected(), b = inputs[B_INPUT].isConnected();
		float va = inputs[A_INPUT].getVoltage(), vb = inputs[B_INPUT].getVoltage();
		float pk = 0.f, tr = 0.f;
		if (a && b) { pk = std::max(va, vb); tr = std::min(va, vb); }
		else if (a) { pk = tr = va; }
		else if (b) { pk = tr = vb; }
		outputs[PEAK_OUTPUT].setVoltage(pk);
		outputs[TROUGH_OUTPUT].setVoltage(tr);
	}
};
struct PTWidget : ModuleWidget {
	PTWidget(PT* m) { sergeScaffold(this, m, "PT"); placePT(this, m); }
};
Model* modelPT = createModel<PT, PTWidget>("PT");
