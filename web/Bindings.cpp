#include <emscripten/bind.h>
#include "../OptMeta.h"

static void setAllow(Settings &settings, int index, bool allow) {
  settings.allowedCoolers[index] = allow;
}

static void setRate(Settings &settings, int index, double rate) {
  settings.coolingRate[index] = rate;
}

static emscripten::val getData(const OptMetaIndividual &x) {
  return emscripten::val(emscripten::typed_memory_view(x.state.size(), x.state.data()));
}

static int getShape(const OptMetaIndividual &x, int i) {
  return x.state.shape(i);
}

static int getStride(const OptMetaIndividual &x, int i) {
  return x.state.strides()[i];
}

static double getPower(const OptMetaIndividual &x) {
  return x.value.power;
}

static double getHeat(const OptMetaIndividual &x) {
  return x.value.heat;
}

static double getCooling(const OptMetaIndividual &x) {
  return x.value.cooling;
}

static double getNetHeat(const OptMetaIndividual &x) {
  return x.value.netHeat();
}

static double getDutyCycle(const OptMetaIndividual &x) {
  return x.value.dutyCycle();
}

static double getEffPower(const OptMetaIndividual &x) {
  return x.value.effPower();
}

EMSCRIPTEN_BINDINGS(FissionOpt) {
  emscripten::class_<Settings>("Settings")
    .constructor<>()
    .property("sizeX", &Settings::sizeX)
    .property("sizeY", &Settings::sizeY)
    .property("sizeZ", &Settings::sizeZ)
    .property("fuelBasePower", &Settings::fuelBasePower)
    .property("fuelBaseHeat", &Settings::fuelBaseHeat)
    .function("setAllow", &setAllow)
    .function("setRate", &setRate);
  emscripten::class_<OptMetaIndividual>("OptMetaIndividual")
    .function("getData", &getData)
    .function("getShape", &getShape)
    .function("getStride", &getStride)
    .function("getPower", &getPower)
    .function("getHeat", &getHeat)
    .function("getCooling", &getCooling)
    .function("getNetHeat", &getNetHeat)
    .function("getDutyCycle", &getDutyCycle)
    .function("getEffPower", &getEffPower);
  emscripten::class_<OptMeta>("OptMeta")
    .constructor<const Settings&>()
    .function("step", &OptMeta::step)
    .function("getBest", &OptMeta::getBest)
    .function("getBestNoNetHeat", &OptMeta::getBestNoNetHeat);
}
