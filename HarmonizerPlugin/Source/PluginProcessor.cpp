/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
HarmonizerPlugin1AudioProcessor::HarmonizerPlugin1AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    pCHarmony = 0;
	CPpm::createInstance(m_pCPpm);
}

HarmonizerPlugin1AudioProcessor::~HarmonizerPlugin1AudioProcessor()
{
    pCHarmony->destroy(pCHarmony);

	CPpm::destroyInstance(m_pCPpm);
	m_pCPpm = 0;
	delete[] m_fPpmValue;
	delete[] m_fMaxPpmValue;
}

//==============================================================================
const String HarmonizerPlugin1AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool HarmonizerPlugin1AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool HarmonizerPlugin1AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool HarmonizerPlugin1AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double HarmonizerPlugin1AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int HarmonizerPlugin1AudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int HarmonizerPlugin1AudioProcessor::getCurrentProgram()
{
    return 0;
}

void HarmonizerPlugin1AudioProcessor::setCurrentProgram (int index)
{
}

const String HarmonizerPlugin1AudioProcessor::getProgramName (int index)
{
    return {};
}

void HarmonizerPlugin1AudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void HarmonizerPlugin1AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    auto totalNumInputChannels = getTotalNumInputChannels();
    pCHarmony->create(pCHarmony);
    pCHarmony->init((float)this->getSampleRate(), m_pitchShiftFac, totalNumInputChannels);
    pCHarmony->setParam(m_pitchShiftInit);
	m_iNumChannels = getTotalNumInputChannels();
	m_pCPpm->initInstance(sampleRate, m_iNumChannels);
	m_fPpmValue = new float[m_iNumChannels];
	m_fMaxPpmValue = new float[m_iNumChannels];
    ppfoldbuffer = new float*[totalNumInputChannels];

	
	for (int i = 0; i < m_iNumChannels; i++)
	{
		m_fPpmValue[i] = 0;
		m_fMaxPpmValue[i] = 0;
	}

    
    for (int i =0; i< totalNumInputChannels; i++) {
        ppfoldbuffer[i] = new float[samplesPerBlock];
    }
    
    for (int i =0; i< totalNumInputChannels; i++) {
        for ( int j =0; j< samplesPerBlock; j++) {
            ppfoldbuffer[i][j] = 0;
        }
    }
}

void HarmonizerPlugin1AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HarmonizerPlugin1AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void HarmonizerPlugin1AudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    //for (int channel = 0; channel < totalNumInputChannels; ++channel)
    //{
    //    auto* channelData = buffer.getWritePointer (channel);
    
    //    // ..do something to the data...
    //}
    
	for (int channel = 0; channel < totalNumInputChannels; ++channel)
	{
		
		m_pCPpm->process((float **)buffer.getArrayOfReadPointers(), m_fPpmValue, buffer.getNumSamples());


		if (m_fPpmValue[channel] > m_fMaxPpmValue[channel])
			m_fMaxPpmValue[channel] = m_fPpmValue[channel];
	}

    pCHarmony->processHarmony(ppfoldbuffer, (float**)buffer.getArrayOfReadPointers(), buffer.getArrayOfWritePointers(), buffer.getNumSamples());
    auto** doublechanneldata = (float**)buffer.getArrayOfReadPointers();
    //std::cout<<buffer.getNumSamples()<<"\n";
    for (int i=0; i< totalNumInputChannels; i++) {
        for (int j =0; j<buffer.getNumSamples(); j++) {
            ppfoldbuffer[i][j] = doublechanneldata[i][j];
        }
    }
}

//==============================================================================
bool HarmonizerPlugin1AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* HarmonizerPlugin1AudioProcessor::createEditor()
{
    return new HarmonizerPlugin1AudioProcessorEditor (*this);
}

//==============================================================================
void HarmonizerPlugin1AudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void HarmonizerPlugin1AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HarmonizerPlugin1AudioProcessor();
}

void HarmonizerPlugin1AudioProcessor::setParameter(int iParamIdx, float fNewValue)
{
    switch (iParamIdx)
    {
        case koutputGain:
            pCHarmony->outputGainDB = fNewValue;
            pCHarmony->ProcessGain();
            break;
        case kinputGain:
            pCHarmony->inputGainDB = fNewValue;
            pCHarmony->ProcessGain();
            break;
        case kpan:
            pCHarmony->panPer = fNewValue;
            pCHarmony->ProcessPan();
            break;
        case kpitchCombo:
            pCHarmony->pitchId = fNewValue;
            pCHarmony->ProcessPitchFactor();
            break;
        case kscaleCombo:
            pCHarmony->scaleId = fNewValue;
            pCHarmony->ProcessPitchFactor();
            break;
    }
}

float HarmonizerPlugin1AudioProcessor::getMaxPeakMeterValue(int channel)
{
	float iMaxPpmValue = 0;
	if (m_fMaxPpmValue[channel] == 0)
	{
		iMaxPpmValue = m_fPpmValue[channel];
	}
	else
	{
		iMaxPpmValue = m_fMaxPpmValue[channel];
		m_fMaxPpmValue[channel] = 0;

	}
	return iMaxPpmValue;
}
