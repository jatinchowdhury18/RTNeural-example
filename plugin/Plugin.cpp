#include "Plugin.h"

//==============================================================================
RTNeuralExamplePlugin::RTNeuralExamplePlugin() :
#if JUCE_IOS || JUCE_MAC
    AudioProcessor (juce::JUCEApplicationBase::isStandaloneApp() ?
        BusesProperties().withInput ("Input", juce::AudioChannelSet::mono(), true)
                         .withOutput ("Output", juce::AudioChannelSet::stereo(), true) :
        BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
#else
    AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
#endif
    parameters (*this, nullptr, Identifier ("Parameters"),
    {
        std::make_unique<AudioParameterFloat> ("gain_db", "Gain [dB]", -12.0f, 12.0f, 0.0f),
        std::make_unique<AudioParameterChoice> ("model_type", "Model Type", StringArray { "Run-Time", "Compile-Time" }, 0)
    })
{
    inGainDbParam = parameters.getRawParameterValue ("gain_db");
    modelTypeParam = parameters.getRawParameterValue ("model_type");

    MemoryInputStream jsonStream (BinaryData::neural_net_weights_json, BinaryData::neural_net_weights_jsonSize, false);
    auto jsonInput = nlohmann::json::parse (jsonStream.readEntireStreamAsString().toStdString());
    neuralNet[0] = RTNeural::json_parser::parseJson<float> (jsonInput);
    neuralNet[1] = RTNeural::json_parser::parseJson<float> (jsonInput);

    neuralNetT[0].parseJson (jsonInput);
    neuralNetT[1].parseJson (jsonInput);
}

RTNeuralExamplePlugin::~RTNeuralExamplePlugin()
{
}

//==============================================================================
const String RTNeuralExamplePlugin::getName() const
{
    return JucePlugin_Name;
}

bool RTNeuralExamplePlugin::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool RTNeuralExamplePlugin::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool RTNeuralExamplePlugin::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double RTNeuralExamplePlugin::getTailLengthSeconds() const
{
    return 0.0;
}

int RTNeuralExamplePlugin::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int RTNeuralExamplePlugin::getCurrentProgram()
{
    return 0;
}

void RTNeuralExamplePlugin::setCurrentProgram (int index)
{
}

const String RTNeuralExamplePlugin::getProgramName (int index)
{
    return {};
}

void RTNeuralExamplePlugin::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void RTNeuralExamplePlugin::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    *dcBlocker.state = *dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 35.0f);

    dsp::ProcessSpec spec { sampleRate, static_cast<uint32> (samplesPerBlock), 2 };
    inputGain.prepare (spec);
    inputGain.setRampDurationSeconds (0.05);
    dcBlocker.prepare (spec);

    neuralNet[0]->reset();
    neuralNet[1]->reset();

    neuralNetT[0].reset();
    neuralNetT[1].reset();
}

void RTNeuralExamplePlugin::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool RTNeuralExamplePlugin::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void RTNeuralExamplePlugin::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

    dsp::AudioBlock<float> block (buffer);
    dsp::ProcessContextReplacing<float> context (block);

    inputGain.setGainDecibels (inGainDbParam->load() + 25.0f);
    inputGain.process (context);

    if (static_cast<int> (modelTypeParam->load()) == 0)
    {
        // use run-time model
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* x = buffer.getWritePointer (ch);
            for (int n = 0; n < buffer.getNumSamples(); ++n)
            {
                float input[] = { x[n] };
                x[n] = neuralNet[ch]->forward (input);
            }
        }
    }
    else
    {
        // use compile-time model
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* x = buffer.getWritePointer (ch);
            for (int n = 0; n < buffer.getNumSamples(); ++n)
            {
                float input[] = { x[n] };
                x[n] = neuralNetT[ch].forward (input);
            }
        }
    }

    dcBlocker.process (context);
    buffer.applyGain (5.0f);

    ignoreUnused (midiMessages);
}

//==============================================================================
bool RTNeuralExamplePlugin::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* RTNeuralExamplePlugin::createEditor()
{
    return new GenericAudioProcessorEditor (*this);
}

//==============================================================================
void RTNeuralExamplePlugin::getStateInformation (MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void RTNeuralExamplePlugin::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
 
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RTNeuralExamplePlugin();
}
