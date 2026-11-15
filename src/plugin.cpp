#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
	pluginInstance = p;
	p->addModel(modelDSG);
	p->addModel(modelSSG);
	p->addModel(modelNTO);
	p->addModel(modelVCM);
	p->addModel(modelVCFQ);
	p->addModel(modelVCFS);
	p->addModel(modelRESEQ);
	p->addModel(modelTWS);
	p->addModel(modelRING);
	p->addModel(modelNOI);
	p->addModel(modelASR);
	p->addModel(modelCOM);
	p->addModel(modelSTR);
	p->addModel(modelPRC);
	p->addModel(modelXFAD);
	p->addModel(modelVCA2);
	p->addModel(modelMIX);
	p->addModel(modelDCSM);
	p->addModel(modelPT);
	p->addModel(modelWAD);
	p->addModel(modelFRS);
	p->addModel(modelBDR);
	p->addModel(modelPDIV);
}
