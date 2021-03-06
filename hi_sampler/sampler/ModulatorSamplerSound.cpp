/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

ModulatorSamplerSound::ModulatorSamplerSound(MainController* mc, StreamingSamplerSound *sound, int index_):
	ControlledObject(mc),
index(index_),
firstSound(sound),
gain(1.0f),
isMultiMicSound(false),
centPitch(0),
pitchFactor(1.0),
maxRRGroup(1),
rrGroup(1),
normalizedPeak(-1.0f),
isNormalized(false),
upperVeloXFadeValue(0),
lowerVeloXFadeValue(0),
pan(0),
purged(false),
purgeChannels(0)
{
	soundArray.add(firstSound.get());

	setProperty(Pan, 0, dontSendNotification);
}


ModulatorSamplerSound::ModulatorSamplerSound(MainController* mc, StreamingSamplerSoundArray &soundArray_, int index_):
	ControlledObject(mc),
index(index_),
soundArray(soundArray_),
firstSound(soundArray_.getFirst()),
isMultiMicSound(true),
gain(1.0f),
centPitch(0),
pitchFactor(1.0),
maxRRGroup(1),
rrGroup(1),
normalizedPeak(-1.0f),
isNormalized(false),
upperVeloXFadeValue(0),
lowerVeloXFadeValue(0),
pan(0),
purged(false),
purgeChannels(0)
{
	
	
	setProperty(Pan, 0, dontSendNotification);
}

ModulatorSamplerSound::~ModulatorSamplerSound()
{
	getMainController()->getSampleManager().getModulatorSamplerSoundPool()->clearUnreferencedSamples();

	firstSound = nullptr;
	soundArray.clear();
	
	removeAllChangeListeners();
}

String ModulatorSamplerSound::getPropertyName(Property p)
{
	switch (p)
	{
	case ID:			return "ID";
	case FileName:		return "FileName";
	case RootNote:		return "Root";
	case KeyHigh:		return "HiKey";
	case KeyLow:		return "LoKey";
	case VeloLow:		return "LoVel";
	case VeloHigh:		return "HiVel";
	case RRGroup:		return "RRGroup";
	case Volume:		return "Volume";
	case Pan:			return "Pan";
	case Normalized:	return "Normalized";
	case Pitch:			return "Pitch";
	case SampleStart:	return "SampleStart";
	case SampleEnd:		return "SampleEnd";
	case SampleStartMod:return "SampleStartMod";
	case LoopEnabled:	return "LoopEnabled";
	case LoopStart:		return "LoopStart";
	case LoopEnd:		return "LoopEnd";
	case LoopXFade:		return "LoopXFade";
	case UpperVelocityXFade:	return "UpperVelocityXFade";
	case LowerVelocityXFade:	return "LowerVelocityXFade";
	case SampleState:	return "SampleState";
	default:			jassertfalse; return String();
	}
}

bool ModulatorSamplerSound::isAsyncProperty(Property p)
{
	return p >= SampleStart;
}

Range<int> ModulatorSamplerSound::getPropertyRange(Property p) const
{
	auto s = soundArray.getFirst();

	if (s == nullptr)
		return {};

	switch (p)
	{
	case ID:			return Range<int>(0, INT_MAX);
	case FileName:		jassertfalse; return Range<int>();
	case RootNote:		return Range<int>(0, 127);
	case KeyHigh:		return Range<int>((int)getProperty(KeyLow), 127);
	case KeyLow:		return Range<int>(0, (int)getProperty(KeyHigh));
	case VeloLow:		return Range<int>(0, (int)getProperty(VeloHigh) - 1);
	case VeloHigh:		return Range<int>((int)getProperty(VeloLow) + 1, 127);
	case Volume:		return Range<int>(-100, 18);
	case Pan:			return Range<int>(-100, 100);
	case Normalized:	return Range<int>(0, 1);
	case RRGroup:		return Range<int>(1, maxRRGroup);
	case Pitch:			return Range<int>(-100, 100);
	case SampleStart:	return firstSound->isLoopEnabled() ? Range<int>(0, jmin<int>((int)firstSound->getLoopStart() - (int)firstSound->getLoopCrossfade(), (int)(firstSound->getSampleEnd() - (int)firstSound->getSampleStartModulation()))) :
		Range<int>(0, (int)firstSound->getSampleEnd() - (int)firstSound->getSampleStartModulation());
	case SampleEnd:		{
		const int sampleStartMinimum = (int)(firstSound->getSampleStart() + firstSound->getSampleStartModulation());
		const int upperLimit = (int)firstSound->getLengthInSamples();

		if (firstSound->isLoopEnabled())
		{
			const int lowerLimit = jmax<int>(sampleStartMinimum, (int)firstSound->getLoopEnd());
			return Range<int>(lowerLimit, upperLimit);

		}
		else
		{
			const int lowerLimit = sampleStartMinimum;
			return Range<int>(lowerLimit, upperLimit);

		}
	}
	case SampleStartMod:return Range<int>(0, (int)firstSound->getSampleLength());
	case LoopEnabled:	return Range<int>(0, 1);
	case LoopStart:		return Range<int>((int)firstSound->getSampleStart() + (int)firstSound->getLoopCrossfade(), (int)firstSound->getLoopEnd() - (int)firstSound->getLoopCrossfade());
	case LoopEnd:		return Range<int>((int)firstSound->getLoopStart() + (int)firstSound->getLoopCrossfade(), (int)firstSound->getSampleEnd());
	case LoopXFade:		return Range<int>(0, jmin<int>((int)(firstSound->getLoopStart() - firstSound->getSampleStart()), (int)firstSound->getLoopLength()));
	case UpperVelocityXFade: return Range < int >(0, (int)getProperty(VeloHigh) - ((int)getProperty(VeloLow) + lowerVeloXFadeValue));
	case LowerVelocityXFade: return Range < int >(0, (int)getProperty(VeloHigh) - upperVeloXFadeValue - (int)getProperty(VeloLow));
	case SampleState:	return Range<int>(0, (int)StreamingSamplerSound::numSampleStates - 1);
	default:			jassertfalse; return Range<int>();
	}
}

String ModulatorSamplerSound::getPropertyAsString(Property p) const
{
	auto s = soundArray.getFirst();

	if (s == nullptr)
		return {};

	switch (p)
	{
	case ID:			return String(index);
	case FileName:		return firstSound->getFileName(false);
	case RootNote:		return MidiMessage::getMidiNoteName(rootNote, true, true, 3);
	case KeyHigh:		return MidiMessage::getMidiNoteName(midiNotes.getHighestBit(), true, true, 3);
	case KeyLow:		return MidiMessage::getMidiNoteName(midiNotes.findNextSetBit(0), true, true, 3);
	case VeloHigh:		return String(velocityRange.getHighestBit());
	case VeloLow:		return String(velocityRange.findNextSetBit(0));
	case RRGroup:		return String(rrGroup);
	case Volume:		return String(Decibels::gainToDecibels(gain.get()), 1) + " dB";
	case Pan:			return BalanceCalculator::getBalanceAsString(pan);
	case Normalized:	return isNormalized ? "Enabled" : "Disabled";
	case Pitch:			return String(centPitch, 0) + " ct";
	case SampleStart:	return String(firstSound->getSampleStart());
	case SampleEnd:		return String(firstSound->getSampleEnd());
	case SampleStartMod:return String(firstSound->getSampleStartModulation());
	case LoopEnabled:	return firstSound->isLoopEnabled() ? "Enabled" : "Disabled";
	case LoopStart:		return String(firstSound->getLoopStart());
	case LoopEnd:		return String(firstSound->getLoopEnd());
	case LoopXFade:		return String(firstSound->getLoopCrossfade());
	case UpperVelocityXFade: return String(upperVeloXFadeValue);
	case LowerVelocityXFade: return String(lowerVeloXFadeValue);
	case SampleState:	return firstSound->getSampleStateAsString();
	default:			jassertfalse; return String();
	}
}

var ModulatorSamplerSound::getProperty(Property p) const
{
	auto s = soundArray.getFirst();

	if (s == nullptr)
		return {};

	switch (p)
	{
	case ID:			return var(index);
	case FileName:		return var(firstSound->getFileName(true));
	case RootNote:		return var(rootNote);
	case KeyHigh:		return var(midiNotes.getHighestBit());
	case KeyLow:		return var(midiNotes.findNextSetBit(0));
	case VeloHigh:		return var(velocityRange.getHighestBit());
	case VeloLow:		return var(velocityRange.findNextSetBit(0));
	case RRGroup:		return var(rrGroup);
	case Volume:		return var(Decibels::gainToDecibels(gain.get()));
	case Pan:			return var(pan);
	case Normalized:	return var(isNormalized);
	case Pitch:			return var(centPitch);
	case SampleStart:	return var(firstSound->getSampleStart());
	case SampleEnd:		return var(firstSound->getSampleEnd());
	case SampleStartMod:return var(firstSound->getSampleStartModulation());
	case LoopEnabled:	return var(firstSound->isLoopEnabled());
	case LoopStart:		return var(firstSound->getLoopStart());
	case LoopEnd:		return var(firstSound->getLoopEnd());
	case LoopXFade:		return var(firstSound->getLoopCrossfade());
	case UpperVelocityXFade: return var(upperVeloXFadeValue);
	case LowerVelocityXFade: return var(lowerVeloXFadeValue);
	case SampleState:	return var(isPurged());
	default:			jassertfalse; return var::undefined();
	}
}

void ModulatorSamplerSound::setProperty(Property p, int newValue, NotificationType notifyEditor/*=sendNotification*/)
{
	//ScopedLock sl(getLock());
	
	if (enableAsyncPropertyChange && isAsyncProperty(p))
	{
		ModulatorSamplerSound::Ptr refPtr = this;
		auto f = [refPtr, p, newValue, notifyEditor](Processor*)
		{
			if (refPtr != nullptr)
			{
				static_cast<ModulatorSamplerSound*>(refPtr.get())->setPreloadPropertyInternal(p, newValue);
                
                if(notifyEditor)
                    static_cast<ModulatorSamplerSound*>(refPtr.get())->sendChangeMessage();
			}

			return true;
		};

		getMainController()->getKillStateHandler().killVoicesAndCall(getMainController()->getMainSynthChain(), f, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
	}
	else
	{
		setPropertyInternal(p, newValue);
        
        if(notifyEditor)
            sendChangeMessage();
	}

	
}

void ModulatorSamplerSound::toggleBoolProperty(ModulatorSamplerSound::Property p, NotificationType notifyEditor/*=sendNotification*/)
{
	

	switch (p)
	{
	case Normalized:
	{
		isNormalized = !isNormalized; 

		if (isNormalized) calculateNormalizedPeak();

		break;
	}
	case LoopEnabled:
	{
		const bool wasEnabled = firstSound->isLoopEnabled();
		FOR_EVERY_SOUND(setLoopEnabled(!wasEnabled)); break;
	}
		
	default:			jassertfalse; break;
	}

	if(notifyEditor == sendNotification) sendChangeMessage();
}

ValueTree ModulatorSamplerSound::exportAsValueTree() const
{
	const ScopedLock sl(getLock());
	ValueTree v("sample");

	for (int i = ID; i < numProperties; i++)
	{
		Property p = (Property)i;

		v.setProperty(getPropertyName(p), getProperty(p), nullptr);
	}

	if (isMultiMicSound)
	{
		v.removeProperty(getPropertyName(FileName), nullptr);

		for (auto s: soundArray)
		{
			ValueTree fileChild("file");
			fileChild.setProperty("FileName", s->getFileName(true), nullptr);
			v.addChild(fileChild, -1, nullptr);
		}
	}

	v.setProperty("NormalizedPeak", normalizedPeak, nullptr);

    if(firstSound.get()->isMonolithic())
    {
        const int64 offset = firstSound->getMonolithOffset();
        const int64 length = firstSound->getMonolithLength();
        const double sampleRate = firstSound->getMonolithSampleRate();
        
        v.setProperty("MonolithOffset", offset, nullptr);
        v.setProperty("MonolithLength", length, nullptr);
        v.setProperty("SampleRate", sampleRate, nullptr);
    }
    
    
	v.setProperty("Duplicate", firstSound->getReferenceCount() >= 3, nullptr);

	return v;
}

void ModulatorSamplerSound::restoreFromValueTree(const ValueTree &v)
{
    const ScopedLock sl(getLock());
 
	ScopedValueSetter<bool> svs(enableAsyncPropertyChange, false);

    normalizedPeak = v.getProperty("NormalizedPeak", -1.0f);
    
	for (int i = RootNote; i < numProperties; i++) // ID and filename must be passed to the constructor!
	{
		Property p = (Property)i;

		var x = v.getProperty(getPropertyName(p), var::undefined());

		if (!x.isUndefined()) setProperty(p, x, dontSendNotification);
	}

}

void ModulatorSamplerSound::startPropertyChange(Property p, int newValue)
{
	String x;
	x << getPropertyName(p) << ": " << getPropertyAsString(p) << " -> " << String(newValue);
	if (undoManager != nullptr) undoManager->beginNewTransaction(x);
}

void ModulatorSamplerSound::endPropertyChange(Property p, int startValue, int endValue)
{
	String x;
	x << getPropertyName(p) << ": " << String(startValue) << " -> " << String(endValue);
	if (undoManager != nullptr) undoManager->setCurrentTransactionName(x);
}

void ModulatorSamplerSound::endPropertyChange(const String &actionName)
{
	if (undoManager != nullptr) undoManager->setCurrentTransactionName(actionName);
}

void ModulatorSamplerSound::setPropertyWithUndo(Property p, var newValue)
{
	if (undoManager != nullptr)
	{
		undoManager->perform(new PropertyChange(this, p, newValue));
	}
	else					   setProperty(p, (int)newValue);
}

void ModulatorSamplerSound::openFileHandle()
{
	FOR_EVERY_SOUND(openFileHandle());
}

void ModulatorSamplerSound::closeFileHandle()
{
	FOR_EVERY_SOUND(closeFileHandle());
}

Range<int> ModulatorSamplerSound::getNoteRange() const			{ return Range<int>(midiNotes.findNextSetBit(0), midiNotes.getHighestBit() + 1); }
Range<int> ModulatorSamplerSound::getVelocityRange() const		{ return Range<int>(velocityRange.findNextSetBit(0), velocityRange.getHighestBit() + 1); }
float ModulatorSamplerSound::getPropertyVolume() const noexcept { return gain.get(); }
double ModulatorSamplerSound::getPropertyPitch() const noexcept { return pitchFactor.load(); }

void ModulatorSamplerSound::setMaxRRGroupIndex(int newGroupLimit)
{
	maxRRGroup = newGroupLimit;

	// Not sure why this is here, remove it when nobody complains...
	// rrGroup = jmin(rrGroup, newGroupLimit);
}

void ModulatorSamplerSound::setMappingData(MappingData newData)
{
	rootNote = newData.rootNote;
	velocityRange.clear();
	velocityRange.setRange(newData.loVel, newData.hiVel - newData.loVel + 1, true);
	midiNotes.clear();
	midiNotes.setRange(newData.loKey, newData.hiKey - newData.loKey + 1, true);
	rrGroup = newData.rrGroup;

	setProperty(SampleStart, newData.sampleStart, dontSendNotification);
	setProperty(SampleEnd, newData.sampleEnd, dontSendNotification);
	setProperty(SampleStartMod, newData.sampleStartMod, dontSendNotification);
	setProperty(LoopEnabled, newData.loopEnabled, dontSendNotification);
	setProperty(LoopStart, newData.loopStart, dontSendNotification);
	setProperty(LoopEnd, newData.loopEnd, dontSendNotification);
	setProperty(LoopXFade, newData.loopXFade, dontSendNotification);
	setProperty(Volume, newData.volume, dontSendNotification);
	setProperty(Pan, newData.pan, dontSendNotification);
	setProperty(Pitch, newData.pitch, dontSendNotification);
}

void ModulatorSamplerSound::calculateNormalizedPeak(bool forceScan /*= false*/)
{
	if (forceScan || normalizedPeak < 0.0f)
	{
		float highestPeak = 0.0f;

		for (auto s: soundArray)
		{
			highestPeak = jmax<float>(highestPeak, s->calculatePeakValue());
		}

		if (highestPeak != 0.0f)
		{
			normalizedPeak = 1.0f / highestPeak;
		}

		
	}
}

float ModulatorSamplerSound::getNormalizedPeak() const
{
	return (isNormalized && normalizedPeak != -1.0f) ? normalizedPeak : 1.0f;
}

float ModulatorSamplerSound::getBalance(bool getRightChannelGain) const
{
	return getRightChannelGain ? rightBalanceGain : leftBalanceGain;
}

void ModulatorSamplerSound::setVelocityXFade(int crossfadeLength, bool isUpperSound)
{
	if (isUpperSound) lowerVeloXFadeValue = crossfadeLength;
	else upperVeloXFadeValue = crossfadeLength;
}

void ModulatorSamplerSound::setPurged(bool shouldBePurged) 
{
	purged = shouldBePurged;
	FOR_EVERY_SOUND(setPurged(shouldBePurged));
}

void ModulatorSamplerSound::checkFileReference()
{
	allFilesExist = true;

	FOR_EVERY_SOUND(checkFileReference())

	for (auto s: soundArray)
	{
		if (s->isMissing())
		{
			allFilesExist = false;
			break;
		}
	}
}

float ModulatorSamplerSound::getGainValueForVelocityXFade(int velocity)
{
	if (upperVeloXFadeValue == 0 && lowerVeloXFadeValue == 0) return 1.0f;

	Range<int> upperRange = Range<int>(velocityRange.getHighestBit() - upperVeloXFadeValue, velocityRange.getHighestBit());
	Range<int> lowerRange = Range<int>(velocityRange.findNextSetBit(0), velocityRange.findNextSetBit(0) + lowerVeloXFadeValue);

	float delta = 1.0f;

	if (upperRange.contains(velocity))
	{
		delta = (float)(velocity - upperRange.getStart()) / (upperRange.getLength());

		return Interpolator::interpolateLinear(1.0f, 0.0f, delta);
	}
	else if (lowerRange.contains(velocity))
	{
		delta = (float)(velocity - lowerRange.getStart()) / (lowerRange.getLength());

		return Interpolator::interpolateLinear(0.0f, 1.0f, delta);
	}
	else
	{
		return 1.0f;
	}
}

int ModulatorSamplerSound::getNumMultiMicSamples() const noexcept { return soundArray.size(); }

bool ModulatorSamplerSound::isChannelPurged(int channelIndex) const { return purgeChannels[channelIndex]; }

void ModulatorSamplerSound::setChannelPurged(int channelIndex, bool shouldBePurged)
{
	if (purged) return;

	purgeChannels.setBit(channelIndex, shouldBePurged);

	if (auto s = soundArray[channelIndex])
	{
		s->setPurged(shouldBePurged);
	}
}

bool ModulatorSamplerSound::preloadBufferIsNonZero() const noexcept
{
	for (auto s: soundArray)
	{
		if (!s->isPurged() && s->getPreloadBuffer().getNumSamples() != 0)
		{
			return true;
		}
	}

	return false;
}

int ModulatorSamplerSound::getRRGroup() const {	return rrGroup; }

void ModulatorSamplerSound::selectSoundsBasedOnRegex(const String &regexWildcard, ModulatorSampler *sampler, SelectedItemSet<ModulatorSamplerSound::Ptr> &set)
{
	bool subtractMode = false;

	bool addMode = false;

	String wildcard = regexWildcard;

	if (wildcard.startsWith("sub:"))
	{
		subtractMode = true;
		wildcard = wildcard.fromFirstOccurrenceOf("sub:", false, true);
	}
	else if (wildcard.startsWith("add:"))
	{
		addMode = true;
		wildcard = wildcard.fromFirstOccurrenceOf("add:", false, true);
	}
	else
	{
		set.deselectAll();
	}


    try
    {
		std::regex reg(wildcard.toStdString());

		ModulatorSampler::SoundIterator iter(sampler, false);

		while (auto sound = iter.getNextSound())
		{
			const String name = sound->getPropertyAsString(Property::FileName);

			if (std::regex_search(name.toStdString(), reg))
			{
				if (subtractMode)
				{
					set.deselect(sound.get());
				}
				else
				{
					set.addToSelection(sound.get());
				}
			}
		}
	}
	catch (std::regex_error e)
	{
		debugError(sampler, e.what());
	}
}


void ModulatorSamplerSound::setPropertyInternal(Property p, int newValue)
{
	switch (p)
	{
	case ID:			jassertfalse; break;
	case FileName:		jassertfalse; break;
	case RootNote:		rootNote = newValue; break;
	case VeloHigh: {	int low = jmin(velocityRange.findNextSetBit(0), newValue, 127);
		velocityRange.clear();
		velocityRange.setRange(low, newValue - low + 1, true); break; }
	case VeloLow: {	int high = jmax(velocityRange.getHighestBit(), newValue, 0);
		velocityRange.clear();
		velocityRange.setRange(newValue, high - newValue + 1, true); break; }
	case KeyHigh: {	int low = jmin(midiNotes.findNextSetBit(0), newValue, 127);
		midiNotes.clear();
		midiNotes.setRange(low, newValue - low + 1, true); break; }
	case KeyLow: {	int high = jmax(midiNotes.getHighestBit(), newValue, 0);
		midiNotes.clear();
		midiNotes.setRange(newValue, high - newValue + 1, true); break; }
	case RRGroup:		rrGroup = newValue; break;
	case Normalized:	isNormalized = newValue == 1;
		if (isNormalized && normalizedPeak < 0.0f) calculateNormalizedPeak();
		break;
	case Volume: {	gain.set(Decibels::decibelsToGain((float)newValue));
		break;
	}
	case Pan: {
		pan = (int)newValue;
		leftBalanceGain = BalanceCalculator::getGainFactorForBalance((float)newValue, true);
		rightBalanceGain = BalanceCalculator::getGainFactorForBalance((float)newValue, false);
		break;
	}
	case Pitch: {	centPitch = newValue;
		pitchFactor.store(powf(2.0f, (float)centPitch / 1200.f));
		break;
	};
	case SampleStart:	FOR_EVERY_SOUND(setSampleStart(newValue)); break;
	case SampleEnd:		FOR_EVERY_SOUND(setSampleEnd(newValue)); break;
	case SampleStartMod: FOR_EVERY_SOUND(setSampleStartModulation(newValue)); break;

	case LoopEnabled:	FOR_EVERY_SOUND(setLoopEnabled(newValue == 1.0f)); break;
	case LoopStart:		FOR_EVERY_SOUND(setLoopStart(newValue)); break;
	case LoopEnd:		FOR_EVERY_SOUND(setLoopEnd(newValue)); break;
	case LoopXFade:		FOR_EVERY_SOUND(setLoopCrossfade(newValue)); break;
	case LowerVelocityXFade: lowerVeloXFadeValue = newValue; break;
	case UpperVelocityXFade: upperVeloXFadeValue = newValue; break;
	case SampleState:	setPurged(newValue == 1.0f); break;
	default:			jassertfalse; break;
	}

}

void ModulatorSamplerSound::setPreloadPropertyInternal(Property p, int newValue)
{
	auto firstSampler = ProcessorHelpers::getFirstProcessorWithType<ModulatorSampler>(getMainController()->getMainSynthChain());

	auto f = [this, p, newValue](Processor*)->bool {
		setPropertyInternal(p, newValue);
		return true;
	};

	firstSampler->killAllVoicesAndCall(f);
}

// ====================================================================================================================



ModulatorSamplerSound::PropertyChange::PropertyChange(ModulatorSamplerSound *soundToChange, Property p, var newValue) :
changedProperty(p),
currentValue(newValue),
sound(soundToChange),
lastValue(soundToChange->getProperty(p))
{

}


bool ModulatorSamplerSound::PropertyChange::perform()
{
	if (sound != nullptr)
	{
		auto s = dynamic_cast<ModulatorSamplerSound*>(sound.get());

		s->setProperty(changedProperty, currentValue);
		return true;
	}
	else return false;
}

bool ModulatorSamplerSound::PropertyChange::undo()
{
	if (sound != nullptr)
	{
		auto s = dynamic_cast<ModulatorSamplerSound*>(sound.get());

		s->setProperty(changedProperty, lastValue);
		return true;
	}
	else return false;
}

// ====================================================================================================================

ModulatorSamplerSoundPool::ModulatorSamplerSoundPool(MainController *mc_) :
mc(mc_),
debugProcessor(nullptr),
mainAudioProcessor(nullptr),
updatePool(true),
searchPool(true),
forcePoolSearch(false),
isCurrentlyLoading(false),
asyncCleaner(*this)
{
	
}

void ModulatorSamplerSoundPool::setDebugProcessor(Processor *p)
{
	debugProcessor = p;
}

ModulatorSamplerSound * ModulatorSamplerSoundPool::addSound(const ValueTree &soundDescription, int index, bool forceReuse /*= false*/)
{
	if (soundDescription.getNumChildren() > 1)
	{
		return addSoundWithMultiMic(soundDescription, index, forceReuse);
	}
	else
	{
		return addSoundWithSingleMic(soundDescription, index, forceReuse);
	}
}



bool ModulatorSamplerSoundPool::loadMonolithicData(const ValueTree &sampleMap, const Array<File>& monolithicFiles, OwnedArray<ModulatorSamplerSound> &sounds)
{
	jassert(!mc->getMainSynthChain()->areVoicesActive());

	clearUnreferencedMonoliths();
	
	loadedMonoliths.add(new MonolithInfoToUse(monolithicFiles));

	MonolithInfoToUse* hmaf = loadedMonoliths.getLast();

	try
	{
		hmaf->fillMetadataInfo(sampleMap);
	}
	catch (StreamingSamplerSound::LoadingError l)
	{
		String x;
		x << "Error at loading sample " << l.fileName << ": " << l.errorDescription;
		mc->getDebugLogger().logMessage(x);

#if USE_FRONTEND
		mc->sendOverlayMessage(DeactiveOverlay::State::CustomErrorMessage, x);
#else
		debugError(mc->getMainSynthChain(), x);
#endif

		return false;
	}

	for (int i = 0; i < sampleMap.getNumChildren(); i++)
	{
		ValueTree sample = sampleMap.getChild(i);

		if (sample.getNumChildren() == 0)
		{
			String fileName = sample.getProperty("FileName").toString().fromFirstOccurrenceOf("{PROJECT_FOLDER}", false, false);
			StreamingSamplerSound* sound = new StreamingSamplerSound(hmaf, 0, i);
			pool.add(sound);
			sounds.add(new ModulatorSamplerSound(mc, sound, i));
		}
		else
		{
			StreamingSamplerSoundArray multiMicArray;

			for (int j = 0; j < sample.getNumChildren(); j++)
			{
				StreamingSamplerSound* sound = new StreamingSamplerSound(hmaf, j, i);
				pool.add(sound);
				multiMicArray.add(sound);
			}

			sounds.add(new ModulatorSamplerSound(mc, multiMicArray, i));
		}
	}

	sendChangeMessage();

	return true;
}


void ModulatorSamplerSoundPool::clearUnreferencedSamples()
{
	asyncCleaner.triggerAsyncUpdate();
}

void ModulatorSamplerSoundPool::clearUnreferencedSamplesInternal()
{
	WeakStreamingSamplerSoundArray currentList;
	currentList.ensureStorageAllocated(pool.size());

	for (int i = 0; i < pool.size(); i++)
	{
		if (pool[i].get() != nullptr)
		{
			currentList.add(pool[i]);
		}
	}

	pool.swapWith(currentList);
	if (updatePool) sendChangeMessage();
}



int ModulatorSamplerSoundPool::getNumSoundsInPool() const noexcept
{
	return pool.size();
}

void ModulatorSamplerSoundPool::getMissingSamples(StreamingSamplerSoundArray &missingSounds) const
{
	for (auto s: pool)
	{
		if (s != nullptr && s->isMissing())
		{
			missingSounds.add(s);
		}
	}
}


class SampleResolver : public DialogWindowWithBackgroundThread
{
public:

    class HorizontalSpacer: public Component
    {
    public:
        
        HorizontalSpacer()
        {
            setSize(900, 2);
        }
    };
    
	SampleResolver(ModulatorSamplerSoundPool *pool_, Processor *synthChain_):
		DialogWindowWithBackgroundThread("Sample Resolver"),
		pool(pool_),
		mainSynthChain(synthChain_)
	{
		pool->getMissingSamples(missingSounds);

		if (missingSounds.size() == 0)
		{	
			addBasicComponents(false);
		}
		else
		{
			numMissingSounds = missingSounds.size();

			remainingSounds = numMissingSounds;

			String textToShow = "Remaining missing sounds: " + String(remainingSounds) + " / " + String(numMissingSounds) + " missing sounds.";

            addCustomComponent(spacer = new HorizontalSpacer());

			String fileNames = missingSounds[0]->getFileName(true);
            
            String path;

            if(ProjectHandler::isAbsolutePathCrossPlatform(fileNames))
            {
                path = File(fileNames).getParentDirectory().getFullPathName();

            }
            else path = fileNames;
            
			addTextEditor("fileNames", fileNames, "Filenames:");
           
            addTextEditor("search", path, "Search for:");
			addTextEditor("replace", path, "Replace with:");
            
            addButton("Search in Finder", 5);
            
            addBasicComponents(true);
            
            
            
            showStatusMessage(textToShow);
		}
        

	};


	void run() override
	{
		const String search = getTextEditorContents("search");
		const String replace = getTextEditorContents("replace");

		pool->setUpdatePool(false);

		int foundThisTime = 0;

		showStatusMessage("Replacing references");

		try
		{
            const double numMissingSoundsDouble = (double)missingSounds.size();
            
			for (int i = 0; i < missingSounds.size(); i++)
			{
				if (threadShouldExit()) return;

                setProgress(double(i) / numMissingSoundsDouble);

				auto sound = missingSounds[i];

				String newFileName = sound->getFileName(true).replace(search, replace, true);

                String newFileNameSanitized = newFileName.replace("\\", "/");
                
				if (File(newFileNameSanitized).existsAsFile())
				{
					sound->replaceFileReference(newFileNameSanitized);

					foundThisTime++;
					missingSounds.remove(i--);
				}
			}
		}
		catch (StreamingSamplerSound::LoadingError e)
		{
			String x;

			x << "Error at loading sample " << e.fileName << ": " << e.errorDescription;

			mainSynthChain->getMainController()->getDebugLogger().logMessage(x);

			errorMessage = "There was an error at preloading.";
			return;
		}

		remainingSounds -= foundThisTime;
		
		showStatusMessage("Replacing references");

		Processor::Iterator<ModulatorSampler> iter(mainSynthChain);

		int numSamplers = iter.getNumProcessors();
		int index = 0;

		while (ModulatorSampler *s = iter.getNextProcessor())
		{
			setProgress((double)index / (double)numSamplers);

			ModulatorSampler::SoundIterator sIter(s);

			while (auto sound = sIter.getNextSound())
			{
				sound->checkFileReference();
			}

			s->sendChangeMessage();

			index++;
		}
	}

	void threadFinished()
	{
		if (errorMessage.isEmpty())
		{
			PresetHandler::showMessageWindow("Missing Samples resolved", String(numMissingSounds - remainingSounds) + " out of " + String(numMissingSounds) + " were resolved.", PresetHandler::IconType::Info);
		}
		else
		{
			PresetHandler::showMessageWindow("Error", errorMessage, PresetHandler::IconType::Error);
		}

		pool->setUpdatePool(true);
		pool->sendChangeMessage();
	}
    
    void resultButtonClicked(const String &name) override
    {
        if(name == "Search in Finder")
        {
            String file = getTextEditor("fileNames")->getText();
            
            file.replace("\\", "/");
            
            String fileName = file.fromLastOccurrenceOf("/", false, false);
            String pathName = file.upToLastOccurrenceOf("/", true, false);
            
#if JUCE_WINDOWS
            String dialogName = "Explorer";
#else
            String dialogName = "Finder";
#endif
            
            PresetHandler::showMessageWindow("Search file", "Search for the sample:\n\n"+fileName +"\n\nPress OK to open the " + dialogName, PresetHandler::IconType::Info);
            
            FileChooser fc("Search sample location " + fileName);
            
            if(fc.browseForFileToOpen())
            {
                File f = fc.getResult();

				
                
                String newPath = f.getFullPathName().replaceCharacter('\\', '/').upToLastOccurrenceOf("/", true, false);;
                
                getTextEditor("search")->setText(pathName);
                getTextEditor("replace")->setText(newPath);
            }
        }
    };

private:

	StreamingSamplerSoundArray missingSounds;

    ScopedPointer<HorizontalSpacer> spacer;
    
	int remainingSounds;
	int numMissingSounds;

	String errorMessage;

	ModulatorSamplerSoundPool *pool;
	WeakReference<Processor> mainSynthChain;


};


void ModulatorSamplerSoundPool::resolveMissingSamples(Component *childComponentOfMainEditor)
{
#if USE_BACKEND
	auto editor = dynamic_cast<BackendRootWindow*>(childComponentOfMainEditor);
	
	if (editor == nullptr) editor = GET_BACKEND_ROOT_WINDOW(childComponentOfMainEditor);

	SampleResolver *r = new SampleResolver(this, editor->getMainSynthChain());
    
	r->setModalBaseWindowComponent(childComponentOfMainEditor);

#else 

	ignoreUnused(childComponentOfMainEditor);

#endif
}

StringArray ModulatorSamplerSoundPool::getFileNameList() const
{
	StringArray sa;

	for (int i = 0; i < pool.size(); i++)
	{
		sa.add(pool[i]->getFileName(true));
	}

	return sa;
}

size_t ModulatorSamplerSoundPool::getMemoryUsageForAllSamples() const noexcept
{
	if (mc->getSampleManager().isPreloading()) return 0;

	ScopedLock sl(mc->getSampleManager().getSamplerSoundLock());

	size_t memoryUsage = 0;

	for (auto s : pool)
	{
		if (s == nullptr)
			continue;

		memoryUsage += s->getActualPreloadSize();
	}

	return memoryUsage;
}



String ModulatorSamplerSoundPool::getTextForPoolTable(int columnId, int indexInPool)
{
#if USE_BACKEND

	if (auto s = pool[indexInPool])
	{
		switch (columnId)
		{
		case SamplePoolTable::FileName:	return s->getFileName();
		case SamplePoolTable::Memory:	return String((int)(s->getActualPreloadSize() / 1024)) + " kB";
		case SamplePoolTable::State:	return String(s->getSampleStateAsString());
		case SamplePoolTable::References:	return String(s->getReferenceCount());
		default:						jassertfalse; return "";
		}
	}
	else
	{
		return "Invalid Index";

	}
#else
	ignoreUnused(columnId, indexInPool);
	return "";

#endif
}

void ModulatorSamplerSoundPool::increaseNumOpenFileHandles()
{
	StreamingSamplerSoundPool::increaseNumOpenFileHandles();

	if(updatePool) sendChangeMessage();
}

void ModulatorSamplerSoundPool::decreaseNumOpenFileHandles()
{
	StreamingSamplerSoundPool::decreaseNumOpenFileHandles();

	if(updatePool) sendChangeMessage();
}

bool ModulatorSamplerSoundPool::isFileBeingUsed(int poolIndex)
{
	if (auto s = pool[poolIndex])
	{
		return s->isOpened();
	}

	return false;
}

int ModulatorSamplerSoundPool::getSoundIndexFromPool(int64 hashCode, int64 otherPossibleHashCode)
{
	if (!searchPool) return -1;

	for (int i = 0; i < pool.size(); i++)
	{
		StreamingSamplerSound::Ptr s = pool[i].get();
		
		if (s == nullptr)
			return -1;

		if (s->getHashCode() == hashCode) return i;

		if (otherPossibleHashCode != -1 && s->getHashCode() == otherPossibleHashCode)
			return i;
	}

	return -1;
}

ModulatorSamplerSound * ModulatorSamplerSoundPool::addSoundWithSingleMic(const ValueTree &soundDescription, int index, bool forceReuse /*= false*/)
{
	String fileNameWildcard = soundDescription.getProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::FileName));
	String fileName = GET_PROJECT_HANDLER(mc->getMainSynthChain()).getFilePath(fileNameWildcard, ProjectHandler::SubDirectories::Samples);
	

	if (forceReuse)
	{
        int64 hash = fileName.hashCode64();
		int64 hashWildcard = fileNameWildcard.hashCode64();
		

		int i = getSoundIndexFromPool(hash, hashWildcard);

		if (i != -1)
		{
			if(updatePool) sendChangeMessage();
			return new ModulatorSamplerSound(mc, pool[i], index);
		}
		else
		{
			jassertfalse;
			return nullptr;
		}
	}
	else
	{
        static Identifier duplicate("Duplicate");
        
        const bool isDuplicate = soundDescription.getProperty(duplicate, true);
        
        const bool searchThisSampleInPool = forcePoolSearch || isDuplicate;
        
        if(searchThisSampleInPool)
        {
            int64 hash = fileName.hashCode64();
			int64 hashWildcard = fileNameWildcard.hashCode64();

            int i = getSoundIndexFromPool(hash, hashWildcard);
            
            if (i != -1)
            {
                ModulatorSamplerSound *sound = new ModulatorSamplerSound(mc, pool[i], index);
                if(updatePool) sendChangeMessage();
                return sound;
            }
        }
        
		StreamingSamplerSound::Ptr s = new StreamingSamplerSound(fileName, this);

		pool.add(s.get());

		if(updatePool) sendChangeMessage();

		return new ModulatorSamplerSound(mc, s, index);
	}
}

ModulatorSamplerSound * ModulatorSamplerSoundPool::addSoundWithMultiMic(const ValueTree &soundDescription, int index, bool forceReuse /*= false*/)
{
	StreamingSamplerSoundArray multiMicArray;

	for (int i = 0; i < soundDescription.getNumChildren(); i++)
	{
		String fileNameWildcard = soundDescription.getChild(i).getProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::FileName));
		String fileName = GET_PROJECT_HANDLER(mc->getMainSynthChain()).getFilePath(fileNameWildcard, ProjectHandler::SubDirectories::Samples);


		if (forceReuse)
		{
            int64 hash = fileName.hashCode64();
			int64 hashWildcard = fileNameWildcard.hashCode64();
			int j = getSoundIndexFromPool(hash, hashWildcard);

			jassert(j != -1);

			multiMicArray.add(pool[j]);
			if(updatePool) sendChangeMessage();
		}
		else
		{
            static Identifier duplicate("Duplicate");
            
			const bool isDuplicate = soundDescription.getProperty(duplicate, true);

			const bool searchThisSampleInPool = forcePoolSearch || isDuplicate;

			if (searchThisSampleInPool)
            {
                int64 hash = fileName.hashCode64();
				int64 hashWildcard = fileNameWildcard.hashCode64();
                int j = getSoundIndexFromPool(hash, hashWildcard);
                
                if (j != -1)
                {
                    multiMicArray.add(pool[j]);
                    continue;
                }
				else
				{
					StreamingSamplerSound::Ptr s = new StreamingSamplerSound(fileName, this);

					multiMicArray.add(s);
					pool.add(s.get());
					continue;
				}
            }
			else
			{
				StreamingSamplerSound::Ptr s = new StreamingSamplerSound(fileName, this);

				multiMicArray.add(s);
				pool.add(s.get());
			}
		}
	}

	if(updatePool) sendChangeMessage();

	return new ModulatorSamplerSound(mc, multiMicArray, index);
}

bool ModulatorSamplerSoundPool::isPoolSearchForced() const
{
	return forcePoolSearch;
}

void ModulatorSamplerSoundPool::clearUnreferencedMonoliths()
{
	for (int i = 0; i < loadedMonoliths.size(); i++)
	{
		if (loadedMonoliths[i]->getReferenceCount() == 2)
		{
			loadedMonoliths.remove(i--);
		}
	}

	if(updatePool) sendChangeMessage();
}

#define SET_IF_NOT_ZERO(field, x) if ((int)sound->getProperty(x) != 0) field = (int)sound->getProperty(x);

void MappingData::fillOtherProperties(ModulatorSamplerSound* sound)
{
	SET_IF_NOT_ZERO(volume, ModulatorSamplerSound::Volume);

	SET_IF_NOT_ZERO(pan, ModulatorSamplerSound::Pan);
	SET_IF_NOT_ZERO(pitch, ModulatorSamplerSound::Pitch);
	SET_IF_NOT_ZERO(sampleStart, ModulatorSamplerSound::SampleStart);
	SET_IF_NOT_ZERO(sampleEnd, ModulatorSamplerSound::SampleEnd);
	SET_IF_NOT_ZERO(sampleStartMod, ModulatorSamplerSound::SampleStartMod);

	// Skip the rest if the loop isn't enabled.
	if (!sound->getProperty(ModulatorSamplerSound::LoopEnabled))
		return;

	SET_IF_NOT_ZERO(loopEnabled, ModulatorSamplerSound::LoopEnabled);
	SET_IF_NOT_ZERO(loopStart, ModulatorSamplerSound::LoopStart);
	SET_IF_NOT_ZERO(loopEnd, ModulatorSamplerSound::LoopEnd);
	SET_IF_NOT_ZERO(loopXFade, ModulatorSamplerSound::LoopXFade);


}

#undef SET_IF_NOT_ZERO

} // namespace hise
