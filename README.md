# DeepPicar-ESP32S3

TFLite model viewer
https://netron.app

## Models


| Model	         |  #params      |  flops       |  accuracy  |
|----------------|---------------|--------------|-------------
| 160x66x3(full) |	220219	     |              |  82%       |
| 160x66x3(depth)|  102641       |              |  7X%      |

## Performance Analysis

| Model	         |  idf | pio(unopt) | pio(opt) |
|----------------|---------------|--------------|-------------|
| 160x66x3(full) |	115	         |   1793       | 205       |

(*) all numbers are in *ms* units.

## Design considerations

### full vs. depthwise
full is much better than depthwise in terms of accuracy. also, it's faster to train. 

### int8 vs. float input image format

int8 and float achieve similar accuracy (float is slightly better)

float execution time profile:  
311ms (3.2fps): cap=0ms, pre=86ms, dnn=201ms, enc=19ms

int8  execution time profile: 
223ms (4.5fps): cap=0ms, pre=3ms, dnn=198ms, enc=19ms

