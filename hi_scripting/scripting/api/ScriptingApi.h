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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef HI_SCRIPTING_API_H_INCLUDED
#define HI_SCRIPTING_API_H_INCLUDED



class ApiHelpers
{
public:


	static Rectangle<float> getRectangleFromVar(const var &data, Result *r = nullptr);

	static Rectangle<int> getIntRectangleFromVar(const var &data, Result* r = nullptr);

	static String getFileNameFromErrorMessage(const String &errorMessage);

#if USE_BACKEND

	static AttributedString createAttributedStringFromApi(const ValueTree &method, const String &className, bool multiLine, Colour textColour);

	static String createCodeToInsert(const ValueTree &method, const String &className);

	static void getColourAndCharForType(int type, char &c, Colour &colour);

	static String getValueType(const var &v);

	



	struct Api
	{
		Api();

		ValueTree apiTree;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Api)
	};

#endif
};



/** This class wraps all available functions for the scripting engine provided by a ScriptProcessor.
*	@ingroup scripting
*/
class ScriptingApi
{
public:

	/** All scripting methods related to the midi message that triggered the callback.
	*	@ingroup scriptingApi
	*
	*	Every method must be called on the message like this:
	*
	*		message.delayEvent(200);
	*/
	class Message: public ScriptingObject,
				   public ApiClass
	{
	public:

		// ============================================================================================================

		Message(ProcessorWithScriptingContent *p);;
		~Message();

		Identifier getName() const override { RETURN_STATIC_IDENTIFIER("Message"); }
		static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("Message"); }

		// ============================================================================================================ API Methods

		/** Return the note number. This can be called only on midi event callbacks. */
		int getNoteNumber() const;

		/** Delays the event by the sampleAmount. */
		void delayEvent(int samplesToDelay);

		/** returns the controller number or 'undefined', if the message is neither controller nor pitch wheel nor aftertouch.
		*
		*	You can also check for pitch wheel values and aftertouch messages.
		*	Pitchwheel has number 128, Aftertouch has number 129.
		*/
		var getControllerNumber() const;
		
		/** Returns the value of the controller. */
		var getControllerValue() const;
		
		/** Returns the MIDI Channel from 1 to 16. */
		int getChannel() const;

		/** Changes the MIDI channel from 1 to 16. */
		void setChannel(int newChannel);

		/** Changes the note number. */
		void setNoteNumber(int newNoteNumber);

		/** Changes the velocity (range 1 - 127). */
		void setVelocity(int newVelocity);

		/** Changes the ControllerNumber. */
		void setControllerNumber(int newControllerNumber);
		
		/** Changes the controller value (range 0 - 127). */
		void setControllerValue(int newControllerValue);

		/** Returns the Velocity. */
		int getVelocity() const;

		/** Ignores the event. */
		void ignoreEvent(bool shouldBeIgnored=true) { ignored = shouldBeIgnored; };

		/** Returns the event id of the current message. */
		int getEventId() const;

		// ============================================================================================================

		// sets the reference to the midi message.
		void setMidiMessage(MidiMessage *m);

		void setMidiMessage(const MidiMessage *m)
		{
			constMessageHolder = m;
		};

		struct Wrapper;

	private:

		// ============================================================================================================

		struct MidiMessageWithEventId
		{
			MidiMessageWithEventId() :m(MidiMessage::noteOn(1, 0, 1.0f)), eventId (-1) { };

			MidiMessageWithEventId(MidiMessage &m_, int eventId_):	m(m_),	eventId(eventId_) { };

			inline int getNoteNumber() {return m.getNoteNumber(); };
			bool isVoid() const { return (eventId == -1); }
			void setVoid() {eventId = -1;};

			MidiMessage m;
			int eventId;
			static MidiMessageWithEventId empty;
		};

		// ============================================================================================================

		friend class JavascriptMidiProcessor;
		friend class HardcodedScriptProcessor;

		MidiMessage const* constMessageHolder;
		MidiMessage *messageHolder;

		bool wrongNoteOff;
		bool ignored;
		int currentEventId;
		int eventIdCounter;
		MidiMessageWithEventId noteOnMessages[1024];

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Message);
	};

	/** All scripting methods related to the main engine can be accessed here.
	*	@ingroup scriptingApi
	*/
	class Engine: public ScriptingObject,
				  public ApiClass
	{
	public:

		// ============================================================================================================

		Engine(ProcessorWithScriptingContent *p);
		~Engine() {};

		Identifier getName() const override  { RETURN_STATIC_IDENTIFIER("Engine"); };

		// ============================================================================================================ API Methods

		/** Loads a font file. */
		void loadFont(const String &fileName);

		/** Returns the current sample rate. */
		double getSampleRate() const;

		/** Converts milli seconds to samples */
		double getSamplesForMilliSeconds(double milliSeconds) const;;
		
		/** Converts samples to milli seconds. */
		double getMilliSecondsForSamples(double samples) const { return samples / getSampleRate() * 1000.0; };
		
		/** Converts decibel (-100.0 ... 0.0) to gain factor (0.0 ... 1.0). */
		double getGainFactorForDecibels(double decibels) const { return Decibels::decibelsToGain<double>(decibels); };
		
		/** Converts gain factor (0.0 .. 1.0) to decibel (-100.0 ... 0). */
		double getDecibelsForGainFactor(double gainFactor) const { return Decibels::gainToDecibels<double>(gainFactor); };

		/** Converts midi note number 0 ... 127 to Frequency 20 ... 20.000. */
		double getFrequencyForMidiNoteNumber(int midiNumber) const { return MidiMessage::getMidiNoteInHertz(midiNumber); };

		/** Converts a semitone value to a pitch ratio (-12 ... 12) -> (0.5 ... 2.0) */
		double getPitchRatioFromSemitones(double semiTones) const { return pow(2.0, semiTones / 12.0); }

		/** Converts a pitch ratio to semitones (0.5 ... 2.0) -> (-12 ... 12) */
		double getSemitonesFromPitchRatio(double pitchRatio) const { return 1200.0 * log2(pitchRatio); }

		/** Converts MIDI note number to Midi note name ("C3" for middle C). */
		String getMidiNoteName(int midiNumber) const { return MidiMessage::getMidiNoteName(midiNumber, true, true, 3); };

		/** Converts MIDI note name to MIDI number ("C3" for middle C). */
		int getMidiNoteFromName(String midiNoteName) const;

		/** Sends an allNotesOff message at the next buffer. */
		void allNotesOff();

		/** Returns the uptime of the engine in seconds. */
		double getUptime() const;
		
		/** Sets a key of the global keyboard to the specified colour (using the form 0x00FF00 for eg. of the key to the specified colour. */
		void setKeyColour(int keyNumber, int colourAsHex);
		
		/** Changes the lowest visible key on the on screen keyboard. */
		void setLowestKeyToDisplay(int keyNumber);

		/** Returns the millisecond value for the supplied tempo (HINT: Use "TempoSync" mode from Slider!) */
		double getMilliSecondsForTempo(int tempoIndex) const;;

		/** Returns the Bpm of the host. */
		double getHostBpm() const;
		
		/** Returns the name for the given macro index. */
		String getMacroName(int index);
		
		/** Returns the current operating system ("OSX" or ("WIN"). */
		String getOS();

		/** Allows access to the data of the host (playing status, timeline, etc...). */
		DynamicObject *getPlayHead();

		/** Creates a MIDI List object. */
        ScriptingObjects::MidiList *createMidiList(); 

		/** Creates a new timer object. */
		ScriptingObjects::TimerObject* createTimerObject();

		/** Exports an object as JSON. */
		void dumpAsJSON(var object, String fileName);

		/** Imports a JSON file as object. */
		var loadFromJSON(String fileName);

		/** Displays the progress (0.0 to 1.0) in the progress bar of the editor. */
		void setCompileProgress(var progress);

		/** Matches the string against the regex token. */
		bool matchesRegex(String stringToMatch, String regex);

        /** Returns an array with all matches. */
        var getRegexMatches(String stringToMatch, String regex);
        
        /** Returns a string of the value with the supplied number of digits. */
        String doubleToString(double value, int digits);
        
		// ============================================================================================================

		struct Wrapper;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Engine);
	};

	/** All scripting functions for sampler specific functionality. */
	class Sampler : public ConstScriptingObject
	{
	public:

		// ============================================================================================================

		Sampler(ProcessorWithScriptingContent *p, ModulatorSampler *sampler);
		~Sampler() {};

		Identifier getObjectName() const override { return "Sampler"; }
		bool objectDeleted() const override { return sampler.get() == nullptr; }
		bool objectExists() const override { return sampler.get() != nullptr; }

		// ============================================================================================================ API Methods

		/** Enables / Disables the automatic round robin group start logic (works only on samplers). */
		void enableRoundRobin(bool shouldUseRoundRobin);

		/** Enables the group with the given index (one-based). Works only with samplers and `enableRoundRobin(false)`. */
		void setActiveGroup(int activeGroupIndex);

		/** Returns the amount of actual RR groups for the notenumber and velocity*/
		int getRRGroupsForMessage(int noteNumber, int velocity);

		/** Recalculates the RR Map. Call this at compile time if you want to use 'getRRGroupForMessage()'. */
		void refreshRRMap();

		/** Selects samples using the regex string as wildcard and the selectMode ("SELECT", "ADD", "SUBTRACT")*/
		void selectSounds(String regex);

		/** Returns the amount of selected samples. */
		int getNumSelectedSounds();

		/** Sets the property of the sampler sound for the selection. */
		void setSoundPropertyForSelection(int propertyIndex, var newValue);

		/** Returns the property of the sound with the specified index. */
		var getSoundProperty(int propertyIndex, int soundIndex);

		/** Sets the property for the index within the selection. */
		void setSoundProperty(int soundIndex, int propertyIndex, var newValue);

		/** Purges all samples of the given mic (Multimic samples only). */
		void purgeMicPosition(String micName, bool shouldBePurged);

		/** Returns the name of the channel with the given index (Multimic samples only. */
		String getMicPositionName(int channelIndex);

		/** Refreshes the interface. Call this after you changed the properties. */
		void refreshInterface();

		/** Loads a new samplemap into this sampler. */
		void loadSampleMap(const String &fileName);

        /** Gets the attribute with the given index (use the constants for clearer code). */
        var getAttribute(int index) const;
        
        /** Sets a attribute to the given value. */
        void setAttribute(int index, var newValue);
        
		// ============================================================================================================

		struct Wrapper;

	private:

		WeakReference<Processor> sampler;
		SelectedItemSet<WeakReference<ModulatorSamplerSound>> soundSelection;
	};
	

	/** Provides access to the synth where the script processor resides.
	*	@ingroup scriptingApi
	*
	*	There are special methods for SynthGroups which only work with SynthGroups
	*/
	class Synth: public ScriptingObject,
				 public ApiClass
	{
	public:

		// ============================================================================================================

		Synth(ProcessorWithScriptingContent *p, ModulatorSynth *ownerSynth);
		~Synth() { artificialNoteOns.clear(); }

		Identifier getName() const override { RETURN_STATIC_IDENTIFIER("Synth"); };

		typedef ScriptingObjects::ScriptingModulator ScriptModulator;
		typedef ScriptingObjects::ScriptingEffect ScriptEffect;
		typedef ScriptingObjects::ScriptingMidiProcessor ScriptMidiProcessor;
		typedef ScriptingObjects::ScriptingSynth ScriptSynth;
		typedef ScriptingObjects::ScriptingAudioSampleProcessor ScriptAudioSampleProcessor;
		typedef ScriptingObjects::ScriptingTableProcessor ScriptTableProcessor;

		// ============================================================================================================ API Methods

		/** Adds the interface to the Container's body (or the frontend interface if compiled) */
		void addToFront(bool addToFront);

		/** Defers all callbacks to the message thread (midi callbacks become read-only). */
		void deferCallbacks(bool makeAsynchronous);

		/** Sends a note off message. The envelopes will tail off. */
		void noteOff(int noteNumber);
		
		/** Plays a note. Be careful or you get stuck notes! */
		void playNote(int noteNumber, int velocity);
		
		/** Starts the timer of the synth. */
		void startTimer(double milliseconds);
		
		/** Sets an attribute of the parent synth. */
		void setAttribute(int attributeIndex, float newAttribute);

		/** Applies a gain factor to a specified voice. */
		void setVoiceGainValue(int voiceIndex, float gainValue);

		/** Applies a pitch factor (0.5 ... 2.0) to a specified voice. */
		void setVoicePitchValue(int voiceIndex, double pitchValue);

		/** Returns the attribute of the parent synth. */
		float getAttribute(int attributeIndex) const;

		/** Adds a note on to the buffer. */
		void addNoteOn(int channel, int noteNumber, int velocity, int timeStampSamples);

		/** Adds a note off to the buffer. */
		void addNoteOff(int channel, int noteNumber, int timeStampSamples);

		/** Adds a controller to the buffer. */
		void addController(int channel, int number, int value, int timeStampSamples);

		/** Sets the internal clock speed. */
		void setClockSpeed(int clockSpeed);

		/** Stops the timer of the synth. You can call this also in the timer callback. */
		void stopTimer();

		/** Sets one of the eight macro controllers to the newValue.
		*
		*	@param macroIndex the index of the macro from 1 - 8
		*	@param newValue The range for the newValue is 0.0 - 127.0. 
		*/
		void setMacroControl(int macroIndex, float newValue);
		

		/** Sends a controller event to the synth.
		*
		*	The message will be only sent to the internal ModulatorChains (the MidiProcessorChain will be bypassed)
		*/
		void sendController(int controllerNumber, int controllerValue);

		/** Sends a controller event to all Child synths. Works only if the script sits in a ModulatorSynthChain. */
		void sendControllerToChildSynths(int controllerNumber, int controllerValue);

		/** Returns the number of child synths. Works with SynthGroups and SynthChains. */
		int getNumChildSynths() const;

		/** Sets a ModulatorAttribute.
		*
		*	@param chainId the chain where the Modulator is. GainModulation = 1, PitchModulation = 0
		*	@param modulatorIndex the index of the Modulator starting with 0.
		*	@param attributeIndex the index of the Modulator starting with 0. Intensity is '-12', Bypassed is '-13'
		*	@param newValue the value. The range for Gain is 0.0 - 1.0, the Range for Pitch is -12.0 ... 12.0
		*
		*/
		void setModulatorAttribute(int chainId, int modulatorIndex, int attributeIndex, float newValue);

		/** Returns the number of pressed keys (!= the number of playing voices!). */
		int getNumPressedKeys() const {return numPressedKeys.get(); };

		/** Checks if any key is pressed. */
		bool isLegatoInterval() const { return numPressedKeys.get() != 1; };

		/** Adds a Modulator to the synth's chain. If it already exists, it returns the index. */
		int addModulator(int chainId, const String &type, const String &id) const;

		/** Returns the Modulator with the supplied name. Can be only called in onInit. It looks also in all child processors. */
		ScriptModulator *getModulator(const String &name);

		/** Returns the Effect with the supplied name. Can only be called in onInit(). It looks also in all child processors. */
		ScriptEffect *getEffect(const String &name);

		/** Returns the MidiProcessor with the supplied name. Can not be the own name! */
		ScriptMidiProcessor * getMidiProcessor(const String &name);

		/** Returns the child synth with the supplied name. */
		ScriptSynth * getChildSynth(const String &name);

		/** Returns the child synth with the supplied name. */
		ScriptAudioSampleProcessor * getAudioSampleProcessor(const String &name);

		/** Returns the table processor with the given name. */
		ScriptTableProcessor *getTableProcessor(const String &name);

		/** Returns the sampler with the supplied name. */
		Sampler *getSampler(const String &name);

		/** Returns the index of the Modulator in the chain with the supplied chainId */
		int getModulatorIndex(int chainId, const String &id) const;

		/** Returns true if the sustain pedal is pressed. */
		bool isSustainPedalDown() const { return sustainState; }

		// ============================================================================================================

		void increaseNoteCounter() noexcept { ++numPressedKeys; }
		void decreaseNoteCounter() { --numPressedKeys; if (numPressedKeys.get() < 0) numPressedKeys.set(0); }
		void setSustainPedal(bool shouldBeDown) { sustainState = shouldBeDown; };

		struct Wrapper;

	private:

		OwnedArray<Message> artificialNoteOns;
		ModulatorSynth * const owner;
		Atomic<int> numPressedKeys;

		SelectedItemSet<WeakReference<ModulatorSamplerSound>> soundSelection;

		bool sustainState;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Synth);
	};

	/** A set of handy function to debug the script. 
	*	@ingroup scriptingApi
	*	
	*
	*/
	class Console: public ApiClass,
				   public ScriptingObject
	{
	public:

		// ============================================================================================================

		Console(ProcessorWithScriptingContent *p);;

		Identifier getName() const override { RETURN_STATIC_IDENTIFIER("Console"); }
		static Identifier getClassName()   { RETURN_STATIC_IDENTIFIER("Console"); };

		// ============================================================================================================ API Methods

		/** Prints a message to the console. */
		void print(var debug);

		/** Starts the benchmark. You can give it a name that will be displayed with the result if desired. */
		void start() { startTime = Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks()); };

		/** Stops the benchmark and prints the result. */
		void stop();
		
		/** Clears the console. */
		void clear();

		struct Wrapper;

	private:

		double startTime;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Console)
	};

	class Content;

	class Colours: public ApiClass
	{
	public:

		// ============================================================================================================

		Colours();
		~Colours() {};

		Identifier getName() const override { RETURN_STATIC_IDENTIFIER("Colours"); }

		// ============================================================================================================ API Methods

		/** Returns a colour value with the specified alpha value. */
		int withAlpha(int colour, float alpha);

		// ============================================================================================================

		struct Wrapper;

	private:

		// ============================================================================================================

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Colours);
	};

	class ModulatorApi : public ApiClass
	{
	public:

		ModulatorApi(Modulator* mod_);

		Identifier getName() const override { RETURN_STATIC_IDENTIFIER("Modulator") }

		/** Sets the intensity of the modulator (raw value) */
		void setIntensity(var newValue)
		{
			m->setIntensity((float)newValue);
			BACKEND_ONLY(mod->sendChangeMessage());
		}

		/** Bypasses the modulator. */
		void setBypassed(var newValue)
		{
			mod->setBypassed((bool)newValue);
			BACKEND_ONLY(mod->sendChangeMessage());
		}

	private:

		struct Wrapper
		{
			API_VOID_METHOD_WRAPPER_1(ModulatorApi, setIntensity);
			API_VOID_METHOD_WRAPPER_1(ModulatorApi, setBypassed);
		};

		Modulator* mod;
		Modulation* m;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorApi)
	};
};


#endif