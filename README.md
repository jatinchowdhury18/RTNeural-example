# RTNeural Example

![CI](https://github.com/jatinchowdhury18/RTNeural-example/workflows/CI/badge.svg)

This repository contains an example of how to use the 
[RTNeural](https://github.com/jatinchowdhury18/RTNeural)
library for running inference on a neural network
in realtime. The example case shown use here is in the
form of an audio plugin, built using the
[JUCE](https://github.com/juce-framework/JUCE) framework.

## Building from source
To build from source, you must have CMake installed.
```
$ git clone --recursive https://github.com/jatinchowdhury18/RTNeural-example
$ cd RTNeural-example
$ cmake -Bbuild
$ cmake --build build --config Release
```

Builds will be output to `build/RTNeuralExample_artefacts`.

## Creating the network

The audio plugin functions as a distortion effect,
using a neural network to generate the distorted
outut signal. The architecture and weights of the
neural network are defined in `neural_net.py`, and
then exported to `neural_net_weights.json`. The
plugin then stores the json file as binary data,
and loads the neural network at run-time.

## C++ Implementation

The C++ implementation is fairly simple, but there
are a few code snippets worth drawing attention to.
Note that the documentation below describes the
neural net implementation using RTNeural's run-time
API. The code in `plugin/` also contains example
code for using RTNeural's compile-time API.

### Linking to RTNeural with CMake

CMakeLists.txt:
```cmake
# Set your choice of RTNeural backends here!
set(RTNEURAL_STL ON CACHE BOOL "Use RTNeural with this backend" FORCE)
add_subdirectory(modules/RTNeural)
include_directories(modules/RTNeural)
...
target_link_libraries(RTNeuralExample PUBLIC
    ...
    RTNeural
)
```
Note that to choose the backend used by RTNeural,
you may instead set `RTNEURAL_XSIMD` or `RTNEURAL_EIGEN`.

### Adding RTNeural::model as a member variable

plugin/Plugin.h
```cpp
#include <RTNeural/RTNeural.h>

class RTNeuralExamplePlugin
{
...
private:
    std::unique_ptr<RTNeural::Model<float>> neuralNet[2];
};
```
Note that we create 2 models, since the plugin
accepts 2 channels of audio (stereo).

### Loading the network

```cpp
MemoryInputStream jsonStream (BinaryData::neural_net_weights_json, BinaryData::neural_net_weights_jsonSize, false);
auto jsonInput = nlohmann::json::parse (jsonStream.readEntireStreamAsString().toStdString());
neuralNet[0] = RTNeural::json_parser::parseJson<float> (jsonInput);
neuralNet[1] = RTNeural::json_parser::parseJson<float> (jsonInput);
```

### Preparing the network
```cpp
neuralNet[0]->reset();
neuralNet[1]->reset();
```

### Running inference
```cpp
for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
{
    auto* x = buffer.getWritePointer (ch);
    for (int n = 0; n < buffer.getNumSamples(); ++n)
    {
        float input[] = { x[n] };
        x[n] = neuralNet[ch]->forward (input);
    }
}
```

## License
The code in this repository is licensed under
the BSD 3-clause license. Enjoy!
