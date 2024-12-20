#ifndef __NeuralNetwork__
#define __NeuralNetwork__

// Globals, used for compatibility with Arduino-style sketches.

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

// input image size
#define INPUT_W 160
#define INPUT_H 60
#define INPUT_C 3
#define USE_CROP 0 // 1: use crop, 0: use resize

namespace tflite
{
    template <unsigned int tOpCount>
    class MicroMutableOpResolver;
    class Model;
    class MicroInterpreter;
} // namespace tflite

class NeuralNetwork
{
private:
#if USE_ALLOPS
    tflite::AllOpsResolver *resolver;
#else
    tflite::MicroMutableOpResolver<12> *resolver;
#endif
    const tflite::Model *model;
    tflite::MicroInterpreter *interpreter;
    TfLiteTensor *input;
    TfLiteTensor *output;
    uint8_t *tensor_arena;

public:
    NeuralNetwork();
    TfLiteTensor* getInput();
    TfLiteStatus predict();
    TfLiteTensor* getOutput();
};


#endif // __NeuralNetwork__