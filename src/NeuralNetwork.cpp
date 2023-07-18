#include <ESP_TF.h>
#include "NeuralNetwork.h"
#include "opt-160x66x3.cc"

#include <esp_attr.h>
#include <Arduino.h>

int kTensorArenaSize = 1024 * 1024;

NeuralNetwork::NeuralNetwork()
{
    printf("Free heap: %d\n", ESP.getFreeHeap());
    printf("largest size (8bit): %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    printf("largest size (default): %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
    printf("largest size (spiram): %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
    printf("largest size (internal): %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));

    // get model (.tflite) from flash
    model = tflite::GetModel(models_opt_160x66x3_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        MicroPrintf("Model provided is schema version %d not equal to supported "
                    "version %d.",
                    model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }
    static tflite::MicroMutableOpResolver<12> micro_op_resolver;
    micro_op_resolver.AddFullyConnected();
    micro_op_resolver.AddConv2D();
    micro_op_resolver.AddDepthwiseConv2D();
    micro_op_resolver.AddMaxPool2D();
    micro_op_resolver.AddMul();
    micro_op_resolver.AddAdd();
    micro_op_resolver.AddLogistic();
    micro_op_resolver.AddTanh();
    micro_op_resolver.AddRelu();
    micro_op_resolver.AddReshape();
    micro_op_resolver.AddQuantize();
    micro_op_resolver.AddDequantize();

    tensor_arena = (uint8_t *)heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (tensor_arena == NULL)
    {
        printf("Couldn't allocate memory of %d bytes\n", kTensorArenaSize);
        return;
    }
    printf("Free heap: %d\n", ESP.getFreeHeap());

    // Build an interpreter to run the model with.
    static tflite::MicroInterpreter static_interpreter(
        model, micro_op_resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;

    MicroPrintf("interpreter initialization");
    // Allocate memory from the tensor_arena for the model's tensors.
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        MicroPrintf("AllocateTensors() failed");
        return;
    }

    size_t used_bytes = interpreter->arena_used_bytes();
    MicroPrintf("Used bytes %d\n", used_bytes);

    // Obtain pointers to the model's input and output tensors.
    input = interpreter->input(0);
    output = interpreter->output(0);

    printf("tensor_arena: %p, input: %p\n", tensor_arena, input->data.uint8);
} 

uint8 *NeuralNetwork::getInputBuffer()
{
    return input->data.uint8;
}

TfLiteStatus NeuralNetwork::predict()
{
    return interpreter->Invoke();
}

float NeuralNetwork::getOutput()
{
    return output->data.f[0];
}