#pragma once

#include <JuceHeader.h>
#include <RTNeural/RTNeural.h>

class RTNeuralExamplePlugin  : public AudioProcessor
{
public:
    //==============================================================================
    RTNeuralExamplePlugin();
    ~RTNeuralExamplePlugin();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    //==============================================================================
    AudioProcessorValueTreeState parameters;

    // input gain
    std::atomic<float>* inGainDbParam = nullptr;
    dsp::Gain<float> inputGain;
    
    // models
    std::atomic<float>* modelTypeParam = nullptr;

    // example of model defined at run-time
    std::unique_ptr<RTNeural::Model<float>> neuralNet[2];

    // example of model defined at compile-time
    RTNeural::ModelT<float, 1, 1,
        RTNeural::DenseT<float, 1, 8>,
        RTNeural::TanhActivationT<float, 8>,
        RTNeural::Conv1DT<float, 8, 4, 3, 2>,
        RTNeural::TanhActivationT<float, 4>,
        RTNeural::GRULayerT<float, 4, 8>,
        RTNeural::DenseT<float, 8, 1>
    > neuralNetT[2];

    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> dcBlocker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RTNeuralExamplePlugin)
};
