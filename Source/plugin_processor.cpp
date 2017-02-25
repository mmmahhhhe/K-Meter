/* ----------------------------------------------------------------------------

   K-Meter
   =======
   Implementation of a K-System meter according to Bob Katz' specifications

   Copyright (c) 2010-2016 Martin Zuther (http://www.mzuther.de/)

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Thank you for using free software!

---------------------------------------------------------------------------- */

#include "plugin_processor.h"
#include "plugin_editor.h"


// To hear the audio source after average filtering, simply set
// DEBUG_FILTER to "true".  Please remember to revert this variable to
// "false" before committing your changes.
const bool DEBUG_FILTER = false;

/*==============================================================================

Flow of parameter processing:

  Editor:      buttonClicked(button) / sliderValueChanged(slider)
  Processor:   changeParameter(nIndex, fValue)
  Processor:   setParameter(nIndex, fValue)
  Parameters:  setFloat(nIndex, fValue)
  Editor:      actionListenerCallback(strMessage)
  Editor:      updateParameter(nIndex)

==============================================================================*/

#ifdef KMETER_SURROUND

KmeterAudioProcessor::KmeterAudioProcessor() :
#ifndef JucePlugin_PreferredChannelConfigurations
    AudioProcessor(BusesProperties()
                   .withInput("Main In",
                              AudioChannelSet::create5point1())
                   .withOutput("Main Out",
                               AudioChannelSet::create5point1())),
#endif
    nTrakmeterBufferSize(1024)

#else

KmeterAudioProcessor::KmeterAudioProcessor() :
#ifndef JucePlugin_PreferredChannelConfigurations
    AudioProcessor(BusesProperties()
                   .withInput("Main In",
                              AudioChannelSet::stereo())
                   .withOutput("Main Out",
                               AudioChannelSet::stereo())),
#endif
    nTrakmeterBufferSize(1024)

#endif
{
    frut::Frut::printVersionNumbers();

    if (DEBUG_FILTER)
    {
        Logger::outputDebugString("********************************************************************************");
        Logger::outputDebugString("** Debugging average filtering.  Please reset DEBUG_FILTER before committing! **");
        Logger::outputDebugString("********************************************************************************");
    }

    bSampleRateIsValid = false;

    setLatencySamples(nTrakmeterBufferSize);

    // depends on "KmeterPluginParameters"!
    nAverageAlgorithm = getRealInteger(KmeterPluginParameters::selAverageAlgorithm);

    fProcessedSeconds = 0.0f;
}


KmeterAudioProcessor::~KmeterAudioProcessor()
{
    removeAllActionListeners();
}


//==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
bool KmeterAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    // main bus: do not allow differing input and output layouts
    if (layouts.getMainInputChannelSet() != layouts.getMainOutputChannelSet())
    {
        return false;
    }

    // main bus: do not allow disabling channels
    if (layouts.getMainInputChannelSet().isDisabled())
    {
        return false;
    }

#ifdef KMETER_SURROUND

    // main bus with stereo input --> okay
    if (layouts.getMainInputChannelSet() == AudioChannelSet::stereo())
    {
        return true;
    }

    // main bus with 5.0 input --> okay
    if (layouts.getMainInputChannelSet() == AudioChannelSet::create5point0())
    {
        return true;
    }

    // main bus with 5.1 input --> okay
    if (layouts.getMainInputChannelSet() == AudioChannelSet::create5point1())
    {
        return true;
    }

#else

    // main bus with mono input --> okay
    if (layouts.getMainInputChannelSet() == AudioChannelSet::mono())
    {
        return true;
    }

    // main bus with stereo input --> okay
    if (layouts.getMainInputChannelSet() == AudioChannelSet::stereo())
    {
        return true;
    }

#endif

    // current channel layout is not allowed
    return false;
}
#endif


const String KmeterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}


int KmeterAudioProcessor::getNumParameters()
{
    return pluginParameters.getNumParameters(false);
}


const String KmeterAudioProcessor::getParameterName(int nIndex)
{
    return pluginParameters.getName(nIndex);
}


const String KmeterAudioProcessor::getParameterText(int nIndex)
{
    return pluginParameters.getText(nIndex);
}


float KmeterAudioProcessor::getParameter(int nIndex)
{
    // This method will be called by the host, probably on the audio
    // thread, so it's absolutely time-critical. Don't use critical
    // sections or anything GUI-related, or anything at all that may
    // block in any way!

    return pluginParameters.getFloat(nIndex);
}


void KmeterAudioProcessor::changeParameter(int nIndex, float fValue)
{
    // This method will be called by the host, probably on the audio
    // thread, so it's absolutely time-critical. Don't use critical
    // sections or anything GUI-related, or anything at all that may
    // block in any way!

    if (nIndex == KmeterPluginParameters::selMono)
    {
        // automatically enable "Mono" button for mono channels
        if (getMainBusNumInputChannels() == 1)
        {
            fValue = 1.0f;
        }
        // automatically disable "Mono" button for multi-channel audio
        else if (getMainBusNumInputChannels() > 2)
        {
            fValue = 0.0f;
        }
    }

    // notify host of parameter change (this will automatically call
    // "setParameter"!)
    beginParameterChangeGesture(nIndex);
    setParameterNotifyingHost(nIndex, fValue);
    endParameterChangeGesture(nIndex);
}


void KmeterAudioProcessor::setParameter(int nIndex, float fValue)
{
    // This method will be called by the host, probably on the audio
    // thread, so it's absolutely time-critical. Don't use critical
    // sections or anything GUI-related, or anything at all that may
    // block in any way!

    // Please only call this method directly for non-automatable
    // values!

    pluginParameters.setFloat(nIndex, fValue);

    // notify plug-in editor of parameter change
    if (pluginParameters.hasChanged(nIndex))
    {
        // for visible parameters, notify the editor of changes (this
        // will also clear the change flag)
        if (nIndex < pluginParameters.getNumParameters(false))
        {
            if (nIndex == KmeterPluginParameters::selCrestFactor)
            {
                if (audioFilePlayer)
                {
                    audioFilePlayer->setCrestFactor(getRealInteger(nIndex));
                }
            }
            else if (nIndex == KmeterPluginParameters::selAverageAlgorithm)
            {
                setAverageAlgorithm(getRealInteger(nIndex));
            }

            // "PC" --> parameter changed, followed by a hash and the
            // parameter's ID
            sendActionMessage("PC#" + String(nIndex));
        }
        // for hidden parameters, we only have to clear the change
        // flag
        else
        {
            pluginParameters.clearChangeFlag(nIndex);
        }
    }
}


void KmeterAudioProcessor::clearChangeFlag(int nIndex)
{
    pluginParameters.clearChangeFlag(nIndex);
}


bool KmeterAudioProcessor::hasChanged(int nIndex)
{
    return pluginParameters.hasChanged(nIndex);
}


void KmeterAudioProcessor::updateParameters(bool bIncludeHiddenParameters)
{
    int nNumParameters = pluginParameters.getNumParameters(false);

    for (int nIndex = 0; nIndex < nNumParameters; ++nIndex)
    {
        if (pluginParameters.hasChanged(nIndex))
        {
            float fValue = pluginParameters.getFloat(nIndex);
            changeParameter(nIndex, fValue);
        }
    }

    if (bIncludeHiddenParameters)
    {
        // handle hidden parameters here!

        // the following parameters need no updating:
        //
        // * selValidationFileName
        // * selValidationSelectedChannel
        // * selValidationAverageMeterLevel
        // * selValidationPeakMeterLevel
        // * selValidationMaximumPeakLevel
        // * selValidationStereoMeterValue
        // * selValidationPhaseCorrelation
        // * selValidationCSVFormat
        // * selSkinName
    }
}


bool KmeterAudioProcessor::getBoolean(int nIndex)
{
    // This method will be called by the host, probably on the audio
    // thread, so it's absolutely time-critical. Don't use critical
    // sections or anything GUI-related, or anything at all that may
    // block in any way!

    return pluginParameters.getBoolean(nIndex);
}


int KmeterAudioProcessor::getRealInteger(int nIndex)
{
    // This method will be called by the host, probably on the audio
    // thread, so it's absolutely time-critical. Don't use critical
    // sections or anything GUI-related, or anything at all that may
    // block in any way!

    return pluginParameters.getRealInteger(nIndex);
}


File KmeterAudioProcessor::getParameterValidationFile()
{
    // This method will be called by the host, probably on the audio
    // thread, so it's absolutely time-critical. Don't use critical
    // sections or anything GUI-related, or anything at all that may
    // block in any way!

    return pluginParameters.getValidationFile();
}


void KmeterAudioProcessor::setParameterValidationFile(const File &fileValidation)
{
    // This method will be called by the host, probably on the audio
    // thread, so it's absolutely time-critical. Don't use critical
    // sections or anything GUI-related, or anything at all that may
    // block in any way!

    pluginParameters.setValidationFile(fileValidation);
}


String KmeterAudioProcessor::getParameterSkinName()
{
    // This method will be called by the host, probably on the audio
    // thread, so it's absolutely time-critical. Don't use critical
    // sections or anything GUI-related, or anything at all that may
    // block in any way!

    return pluginParameters.getSkinName();
}


void KmeterAudioProcessor::setParameterSkinName(const String &strSkinName)
{
    // This method will be called by the host, probably on the audio
    // thread, so it's absolutely time-critical. Don't use critical
    // sections or anything GUI-related, or anything at all that may
    // block in any way!

    pluginParameters.setSkinName(strSkinName);
}


bool KmeterAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}


bool KmeterAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}


double KmeterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}


int KmeterAudioProcessor::getNumPrograms()
{
    return 0;
}


int KmeterAudioProcessor::getCurrentProgram()
{
    return 0;
}


void KmeterAudioProcessor::setCurrentProgram(int nIndex)
{
    UNUSED(nIndex);
}


const String KmeterAudioProcessor::getProgramName(int nIndex)
{
    UNUSED(nIndex);

    return String::empty;
}


void KmeterAudioProcessor::changeProgramName(int nIndex, const String &newName)
{
    UNUSED(nIndex);
    UNUSED(newName);
}


//==============================================================================

void KmeterAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    Logger::outputDebugString("[K-Meter] preparing to play");

    if ((sampleRate < 44100) || (sampleRate > 192000))
    {
        Logger::outputDebugString("[K-Meter] WARNING: sample rate of " + String(sampleRate) + " Hz not supported");
        bSampleRateIsValid = false;
        return;
    }
    else
    {
        bSampleRateIsValid = true;
    }

    isSilent = false;

    if (getBoolean(KmeterPluginParameters::selMute))
    {
        attenuationLevel_ = 0.0f;
    }
    else if (getBoolean(KmeterPluginParameters::selDim))
    {
        attenuationLevel_ = 0.1f;
    }
    else
    {
        attenuationLevel_ = 1.0f;
    }

    int numInputChannels = getMainBusNumInputChannels();

    Logger::outputDebugString("[K-Meter] number of input channels: " + String(numInputChannels));
    Logger::outputDebugString("[K-Meter] number of output channels: " + String(getMainBusNumOutputChannels()));

    isStereo = (numInputChannels == 2);

    pMeterBallistics = new MeterBallistics(numInputChannels, nAverageAlgorithm, false, false);

    arrPeakLevels.clear();
    arrRmsLevels.clear();
    arrAverageLevelsFiltered.clear();
    arrTruePeakLevels.clear();

    arrOverflows.clear();

    for (int nChannel = 0; nChannel < numInputChannels; ++nChannel)
    {
        arrPeakLevels.add(0.0f);
        arrRmsLevels.add(0.0f);
        arrAverageLevelsFiltered.add(MeterBallistics::getMeterMinimumDecibel());
        arrTruePeakLevels.add(0.0f);

        arrOverflows.add(0);
    }

    pAverageLevelFiltered = new AverageLevelFiltered(this, numInputChannels, (int) sampleRate, nTrakmeterBufferSize, nAverageAlgorithm);

    // maximum under-read of true peak measurement is 0.169 dB (see
    // Annex 2 of ITU-R BS.1770-4)
    int nOversamplingRate = 8;

    if (sampleRate >= 176400)
    {
        nOversamplingRate /= 4;
    }
    else if (sampleRate >= 88200)
    {
        nOversamplingRate /= 2;
    }

    pTruePeakMeter = new TruePeakMeter(nOversamplingRate,
                                       numInputChannels,
                                       nTrakmeterBufferSize);

    // make sure that ring buffer can hold at least nTrakmeterBufferSize
    // samples and is large enough to receive a full block of audio
    nSamplesInBuffer = 0;
    unsigned int uRingBufferSize = (samplesPerBlock > nTrakmeterBufferSize) ? samplesPerBlock : nTrakmeterBufferSize;

    pRingBufferInput = new frut::audio::RingBuffer("Input ring buffer", numInputChannels, uRingBufferSize, nTrakmeterBufferSize, nTrakmeterBufferSize);
    pRingBufferInput->setCallbackClass(this);

    pRingBufferOutput = new frut::audio::RingBuffer("Output ring buffer", numInputChannels, uRingBufferSize, nTrakmeterBufferSize, nTrakmeterBufferSize);
}


void KmeterAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free
    // up any spare memory, etc.

    Logger::outputDebugString("[K-Meter] releasing resources");
    Logger::outputDebugString("");

    pMeterBallistics = nullptr;
    pAverageLevelFiltered = nullptr;
    pTruePeakMeter = nullptr;
}


void KmeterAudioProcessor::processBlock(AudioBuffer<float> &buffer, MidiBuffer &midiMessages)
{
    UNUSED(midiMessages);

    // This is the place where you'd normally do the guts of your
    // plug-in's audio processing...

    int nNumSamples = buffer.getNumSamples();

    if (!bSampleRateIsValid)
    {
        for (int nChannel = 0; nChannel < getMainBusNumOutputChannels(); ++nChannel)
        {
            buffer.clear(nChannel, 0, nNumSamples);
        }

        return;
    }

    if (getMainBusNumInputChannels() < 1)
    {
        Logger::outputDebugString("[K-Meter] no input channels!");
        return;
    }

    // In case we have more outputs than inputs, we'll clear any
    // output channels that didn't contain input data, because these
    // aren't guaranteed to be empty -- they may contain garbage.

    for (int nChannel = getMainBusNumInputChannels(); nChannel < getMainBusNumOutputChannels(); ++nChannel)
    {
        buffer.clear(nChannel, 0, nNumSamples);
    }

    if (audioFilePlayer)
    {
        audioFilePlayer->fillBufferChunk(&buffer);
    }

    // process two channels only
    if (isStereo)
    {
        float *inputLeft = buffer.getWritePointer(0);
        float *inputRight = buffer.getWritePointer(1);

        // "Mono" button has been pressed
        if (getBoolean(KmeterPluginParameters::selMono))
        {
            for (int i = 0; i < nNumSamples; ++i)
            {
                inputLeft[i] = 0.5f * (inputLeft[i] + inputRight[i]);
                inputRight[i] = inputLeft[i];
            }
        }
        // "Flip" button has been pressed
        else if (getBoolean(KmeterPluginParameters::selFlip))
        {
            for (int i = 0; i < nNumSamples; ++i)
            {
                float oldInputLeft = inputLeft[i];

                inputLeft[i] = inputRight[i];
                inputRight[i] = oldInputLeft;
            }
        }
    }

    pRingBufferInput->addSamples(buffer, 0, nNumSamples);

    nSamplesInBuffer += nNumSamples;
    nSamplesInBuffer %= nTrakmeterBufferSize;

    pRingBufferOutput->copyToBuffer(buffer, 0, nNumSamples, nTrakmeterBufferSize - nSamplesInBuffer);

    // fade output attenuation from old to new value (JUCE takes care
    // of any optimizations)
    float oldAttenuationLevel = attenuationLevel_;

    if (getBoolean(KmeterPluginParameters::selMute))
    {
        attenuationLevel_ = 0.0f;
    }
    else if (getBoolean(KmeterPluginParameters::selDim))
    {
        attenuationLevel_ = 0.1f;
    }
    else
    {
        attenuationLevel_ = 1.0f;
    }

    buffer.applyGainRamp(0, buffer.getNumSamples(),
                         oldAttenuationLevel, attenuationLevel_);
}


void KmeterAudioProcessor::processBufferChunk(AudioBuffer<float> &buffer, const unsigned int uChunkSize, const unsigned int uBufferPosition, const unsigned int uProcessedSamples)
{
    UNUSED(uBufferPosition);
    UNUSED(uProcessedSamples);

    // silence input if validation window is open
    if (isSilent)
    {
        buffer.clear();
        pRingBufferInput->clear();
    }

    unsigned int uPreDelay = uChunkSize / 2;
    bool bMono = getBoolean(KmeterPluginParameters::selMono);

    // length of buffer chunk in fractional seconds
    // (1024 samples / 44100 samples/s = 23.2 ms)
    fProcessedSeconds = (float) uChunkSize / (float) getSampleRate();

    // copy ring buffer to determine average level (FIR filter already
    // adds delay of (uChunkSize / 2) samples)
    pAverageLevelFiltered->copyFromBuffer(*pRingBufferInput, 0, (int) getSampleRate());

    // copy ring buffer to determine true peak level (use pre-delay)
    pTruePeakMeter->copyFromBuffer(*pRingBufferInput, uPreDelay);

    for (int nChannel = 0; nChannel < getMainBusNumInputChannels(); ++nChannel)
    {
        if (bMono && (nChannel == 1))
        {
            arrPeakLevels.set(nChannel, arrPeakLevels[0]);
            arrRmsLevels.set(nChannel, arrRmsLevels[0]);
            arrAverageLevelsFiltered.set(nChannel, arrAverageLevelsFiltered[0]);
            arrTruePeakLevels.set(nChannel, arrTruePeakLevels[0]);

            arrOverflows.set(nChannel, arrOverflows[0]);
        }
        else
        {
            // determine peak level for uChunkSize samples (use pre-delay)
            arrPeakLevels.set(nChannel, pRingBufferInput->getMagnitude(nChannel, uChunkSize, uPreDelay));

            // determine peak level for uChunkSize samples (use pre-delay)
            arrRmsLevels.set(nChannel, pRingBufferInput->getRMSLevel(nChannel, uChunkSize, uPreDelay));

            // determine filtered average level for uChunkSize samples
            // (please note that this level has already been converted
            // to decibels!)
            arrAverageLevelsFiltered.set(nChannel, pAverageLevelFiltered->getLevel(nChannel));

            // determine true peak level for uChunkSize samples (uses
            // pre-delay)
            arrTruePeakLevels.set(nChannel, pTruePeakMeter->getLevel(nChannel));

            // determine overflows for uChunkSize samples (use pre-delay)
            arrOverflows.set(nChannel, countOverflows(pRingBufferInput, nChannel, uChunkSize, uPreDelay));
        }

        // apply meter ballistics and store values so that the editor
        // can access them
        pMeterBallistics->updateChannel(nChannel, fProcessedSeconds, arrPeakLevels[nChannel], arrTruePeakLevels[nChannel], arrRmsLevels[nChannel], arrAverageLevelsFiltered[nChannel], arrOverflows[nChannel]);
    }

    // phase correlation is only defined for stereo signals
    if (isStereo)
    {
        float fPhaseCorrelation = 1.0f;

        // check whether the stereo signal has been mixed down to mono
        if (bMono)
        {
            fPhaseCorrelation = 1.0f;
        }
        // otherwise, process only levels at or above -80 dB
        else if ((arrRmsLevels[0] >= 0.0001f) || (arrRmsLevels[1] >= 0.0001f))
        {
            float sum_of_product = 0.0f;
            float sum_of_squares_left = 0.0f;
            float sum_of_squares_right = 0.0f;

            // determine correlation for uChunkSize samples (use pre-delay)
            for (unsigned int uSample = 0; uSample < uChunkSize; ++uSample)
            {
                float ringbuffer_left = pRingBufferInput->getSample(0, uSample, uPreDelay);
                float ringbuffer_right = pRingBufferInput->getSample(1, uSample, uPreDelay);

                sum_of_product += ringbuffer_left * ringbuffer_right;
                sum_of_squares_left += ringbuffer_left * ringbuffer_left;
                sum_of_squares_right += ringbuffer_right * ringbuffer_right;
            }

            float fSumsOfSquares = sum_of_squares_left * sum_of_squares_right;

            // prevent division by zero and taking the square root of
            // a negative number
            if (fSumsOfSquares > 0.0f)
            {
                fPhaseCorrelation = sum_of_product / sqrt(fSumsOfSquares);
            }
            else
            {
                // this is mathematically incorrect, but "musically"
                // correct (i.e. signal is mono-compatible)
                fPhaseCorrelation = 1.0f;
            }
        }

        pMeterBallistics->setPhaseCorrelation(fProcessedSeconds, fPhaseCorrelation);

        float fStereoMeterValue = 0.0f;

        // do not process levels below -80 dB
        if ((arrRmsLevels[0] < 0.0001f) && (arrRmsLevels[1] < 0.0001f))
        {
            fStereoMeterValue = 0.0f;
        }
        else if (arrRmsLevels[1] >= arrRmsLevels[0])
        {
            fStereoMeterValue = 1.0f - arrRmsLevels[0] / arrRmsLevels[1];
        }
        else
        {
            fStereoMeterValue = arrRmsLevels[1] / arrRmsLevels[0] - 1.0f;
        }

        pMeterBallistics->setStereoMeterValue(fProcessedSeconds, fStereoMeterValue);
    }

    // "UM" --> update meters
    sendActionMessage("UM");

    // To hear the audio source after average filtering, simply set
    // DEBUG_FILTER to "true".  Please remember to revert this
    // variable to "false" before committing your changes.
    if (DEBUG_FILTER)
    {
        pAverageLevelFiltered->copyToBuffer(*pRingBufferOutput, 0, uChunkSize);
    }
    else
    {
        AudioBuffer<float> TempAudioBuffer(getMainBusNumInputChannels(), uChunkSize);
        pRingBufferInput->copyToBuffer(TempAudioBuffer, 0, uChunkSize, 0);
        pRingBufferOutput->addSamples(TempAudioBuffer, 0, uChunkSize);
    }
}


void KmeterAudioProcessor::silenceInput(bool isSilentNew)
{
    isSilent = isSilentNew;
}


void KmeterAudioProcessor::startValidation(File fileAudio, int nSelectedChannel, bool bReportCSV, bool bAverageMeterLevel, bool bPeakMeterLevel, bool bMaximumPeakLevel, bool bTruePeakMeterLevel, bool bMaximumTruePeakLevel, bool bStereoMeterValue, bool bPhaseCorrelation)
{
    // reset all meters before we start the validation
    pMeterBallistics->reset();

    isSilent = false;

    int nCrestFactor = getRealInteger(KmeterPluginParameters::selCrestFactor);
    audioFilePlayer = new AudioFilePlayer(fileAudio, (int) getSampleRate(), pMeterBallistics, nCrestFactor);

    if (audioFilePlayer->matchingSampleRates())
    {
        audioFilePlayer->setReporters(nSelectedChannel, bReportCSV, bAverageMeterLevel, bPeakMeterLevel, bMaximumPeakLevel, bTruePeakMeterLevel, bMaximumTruePeakLevel, bStereoMeterValue, bPhaseCorrelation);

        // refresh editor; "V+" --> validation started
        sendActionMessage("V+");
    }
    else
    {
        stopValidation();

        AlertWindow::showMessageBoxAsync(
            AlertWindow::WarningIcon,
            "Validation error",
            "Sample rates of host and validation file do not match.");
    }
}


void KmeterAudioProcessor::stopValidation()
{
    isSilent = false;
    audioFilePlayer = nullptr;

    // reset all meters after the validation
    pMeterBallistics->reset();

    // refresh editor; "V-" --> validation stopped
    sendActionMessage("V-");
}


bool KmeterAudioProcessor::isValidating()
{
    if (audioFilePlayer == nullptr)
    {
        return false;
    }
    else
    {
        if (audioFilePlayer->isPlaying())
        {
            return true;
        }
        else
        {
            stopValidation();
            return false;
        }
    }
}


int KmeterAudioProcessor::countOverflows(frut::audio::RingBuffer *ring_buffer, const unsigned int channel, const unsigned int length, const unsigned int pre_delay)
{
    // initialise number of overflows in this buffer
    int nOverflows = 0;

    // loop through samples of buffer
    for (unsigned int uSample = 0; uSample < length; ++uSample)
    {
        // get current sample value
        float fSampleValue = ring_buffer->getSample(channel, uSample, pre_delay);

        // in the 16-bit domain, full scale corresponds to an absolute
        // integer value of 32'767 or 32'768, so we'll treat absolute
        // levels of 32'767 and above as overflows; this corresponds
        // to a floating-point level of 32'767 / 32'768 = 0.9999694
        // (approx. -0.001 dBFS).
        if ((fSampleValue < -0.9999f) || (fSampleValue > 0.9999f))
        {
            ++nOverflows;
        }
    }

    // return number of overflows in this buffer
    return nOverflows;
}


MeterBallistics *KmeterAudioProcessor::getLevels()
{
    return pMeterBallistics;
}


int KmeterAudioProcessor::getAverageAlgorithm()
{
    return nAverageAlgorithm;
}


void KmeterAudioProcessor::setAverageAlgorithm(const int average_algorithm)
{
    if (average_algorithm != nAverageAlgorithm)
    {
        if (pAverageLevelFiltered != nullptr)
        {
            pAverageLevelFiltered->setAlgorithm(average_algorithm);
        }
        else
        {
            nAverageAlgorithm = average_algorithm;
        }
    }
}


void KmeterAudioProcessor::setAverageAlgorithmFinal(const int average_algorithm)
{
    nAverageAlgorithm = average_algorithm;
    pMeterBallistics->setAverageAlgorithm(nAverageAlgorithm);

    //  the level averaging alghorithm has been changed, so update the
    // "RMS" and "ITU-R" buttons to make sure that the correct button
    // is lit
    //
    // "AC" --> algorithm changed
    sendActionMessage("AC");
}

//==============================================================================

AudioProcessorEditor *KmeterAudioProcessor::createEditor()
{
    return new KmeterAudioProcessorEditor(this, getMainBusNumInputChannels());
}


bool KmeterAudioProcessor::hasEditor() const
{
    return true;
}


//==============================================================================

void KmeterAudioProcessor::getStateInformation(MemoryBlock &destData)
{
    copyXmlToBinary(pluginParameters.storeAsXml(), destData);
}


void KmeterAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    ScopedPointer<XmlElement> xmlDocument(getXmlFromBinary(data, sizeInBytes));
    pluginParameters.loadFromXml(xmlDocument);

    updateParameters(true);
}

//==============================================================================

// This creates new instances of the plug-in.
AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new KmeterAudioProcessor();
}


// Local Variables:
// ispell-local-dictionary: "british"
// End:
