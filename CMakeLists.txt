cmake_minimum_required(VERSION 3.15)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment target")
project(ChowMatrix VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(modules/JUCE)

# Set your choice of RTNeural backends here!
set(RTNEURAL_STL ON CACHE BOOL "Use RTNeural with this backend" FORCE)
add_subdirectory(modules/RTNeural)
include_directories(modules/RTNeural)

juce_add_plugin(RTNeuralExample
    COMPANY_NAME chowdsp
    PLUGIN_MANUFACTURER_CODE Chow
    PLUGIN_CODE Rtn1
    FORMATS AU VST3 Standalone
    ProductName "RTNeuralExample"
)

# create JUCE header
juce_generate_juce_header(RTNeuralExample)

# add sources
target_sources(RTNeuralExample PRIVATE
    plugin/Plugin.cpp
)

juce_add_binary_data(BinaryData SOURCES
    neural_net_weights.json
)

# Need to build BinaryData with -fPIC flag on Linux
set_target_properties(BinaryData PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE)

target_compile_definitions(RTNeuralExample
    PUBLIC
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(RTNeuralExample PUBLIC
    BinaryData
    juce::juce_audio_utils
    juce::juce_dsp
    RTNeural
)
