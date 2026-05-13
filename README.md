## Work in Progress — DS8 FX Implementation

This fork is working toward implementing IDirectSoundFXDistortion8,
IDirectSoundFXEcho8, and IDirectSoundFXParamEq8 via OpenAL EFX, restoring
voice modulation filters (power armor, masks, etc.) in games like Fallout
New Vegas that use the DS8 FX pipeline. See dsfx.h / dsfx.cpp.

What needs to be implemented in DSOAL
The two stub functions
Buffer::SetFX (buffer.cpp line 1343) — currently logs "Unsupported effect" and returns DSERR_FXUNAVAILABLE. Needs to create OpenAL EFX effect objects and store them on the buffer.
Buffer::GetObjectInPath (buffer.cpp line 1442) — currently returns E_NOTIMPL. Needs to return a pointer to the effect interface object so the game can call SetAllParameters on it.

The effects FNV uses
From the DSOAL log lines in those GitHub issues, FNV calls SetFX with GUID_DSFX_STANDARD_DISTORTION. The GUIDs are already known to DSOAL via guidprinter.h. The three effects FNV uses and their OpenAL EFX equivalents:
DirectSound FXGUIDOpenAL EFX typeAL_EFFECT_TYPEGUID_DSFX_STANDARD_DISTORTION{120ced89...}AL_EFFECT_DISTORTION0x0003GUID_DSFX_STANDARD_ECHOalready in guidprinter.hAL_EFFECT_ECHO0x0004GUID_DSFX_STANDARD_PARAMEQalready in guidprinter.hAL_EFFECT_EQUALIZER0x000C

Parameter mappings
Distortion (DSFXDistortion → OpenAL EFX):
cpp// DSFXDistortion struct fields -> OpenAL EFX params
fGain         -> AL_DISTORTION_GAIN          // -60 to 0 dB -> 0.01 to 1.0
fEdge         -> AL_DISTORTION_EDGE          // 0 to 100 -> 0.0 to 1.0
fPostEQCenterFrequency -> AL_DISTORTION_EQCENTER    // 100-8000 Hz -> same
fPostEQBandwidth       -> AL_DISTORTION_EQBANDWIDTH // 100-8000 Hz -> same
fPreLowpassCutoff      -> AL_DISTORTION_LOWPASS_CUTOFF // 100-8000 Hz -> same
Echo (DSFXEcho → OpenAL EFX):
cppfWetDryMix    -> AL_ECHO_FEEDBACK    // approximate
fFeedback     -> AL_ECHO_FEEDBACK
fLeftDelay    -> AL_ECHO_DELAY       // ms -> seconds (/1000)
fRightDelay   -> AL_ECHO_LRDELAY    // ms -> seconds (/1000)
lPanDelay     -> AL_ECHO_SPREAD
ParamEQ (DSFXParamEq → OpenAL EFX):
cppfCenter       -> AL_EQUALIZER_MID1_CENTER
fBandwidth    -> AL_EQUALIZER_MID1_WIDTH
fGain         -> AL_EQUALIZER_MID1_GAIN   // dB -> linear

What needs to be added to buffer.h
The Buffer class needs new members:
cpp// Per-buffer FX state
struct FXEntry {
    ALuint mEffect{0};      // OpenAL effect object
    ALuint mSlot{0};        // OpenAL aux effect slot
    GUID   mGuid{};         // which DSFX type this is
};
std::vector<FXEntry> mFXList;
And a new inner COM class for each effect type — same pattern as Buffer3D and Notify — implementing IDirectSoundFXDistortion8, IDirectSoundFXEcho8, IDirectSoundFXParamEq8.

The implementation flow
SetFX needs to:

Clear any existing effects (alDeleteEffects, alDeleteAuxiliaryEffectSlots)
For each effect in fxdescs: alGenEffects(1, &effect), alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_DISTORTION) etc.
alGenAuxiliaryEffectSlots(1, &slot), alAuxiliaryEffectSloti(slot, AL_EFFECTSLOT_EFFECT, effect)
Bind to the source: alSource3i(mSource, AL_AUXILIARY_SEND_FILTER, slot, 0, AL_FILTER_NULL)
Store in mFXList, set result codes to DSFXR_LOCATED

GetObjectInPath needs to:

Match objectId GUID against mFXList entries
Return a pointer to the appropriate inner COM class (IDirectSoundFXDistortion8 etc.)
That COM class's SetAllParameters calls alEffectf(mEffect, AL_DISTORTION_GAIN, ...) etc. then alAuxiliaryEffectSloti(mSlot, AL_EFFECTSLOT_EFFECT, mEffect) to apply


# DSOAL

This project is for a DirectSound DLL replacement. It implements the
DirectSound interfaces by translating the calls to OpenAL, and fools
applications into thinking there is a hardware accelerated sound device. EAX is
also implemented (up to version 4) by using OpenAL's EAX extension, allowing
for environmental reverb with sound obstruction and occlusion effects.

Effectively, this allows DirectSound applications to enable their DirectSound3D
acceleration path, and turn on EAX. The actual processing is being done by
OpenAL with no hardware acceleration requirement, allowing it to work on
systems where audio acceleration is not otherwise available.

Or more succinctly: it enables DirectSound3D surround sound and EAX for systems
without the requisite hardware.


## Source Code

To build the source, you will need [CMake](https://cmake.org/) 2.6 or newer.
You can either use the CMake GUI, specifying the directories for the source and
where the build files should go, or using one of the command-line programs, for
example by first making sure to be in an empty directory where the build files
will go (such as the provided build/ sub-directory) and running cmake with the
path to the source.

Once successfully built, it should have created dsound.dll.


## Usage

Once built, copy dsound.dll to the same location as the desired application's
executable. You must also provide an OpenAL DLL in the same location, named as
dsoal-aldrv.dll, or else the DLL will fail to work. Some applications may need
to be configured to use DirectSound3D acceleration and EAX, but it otherwise
goes to work the next time the application is run.

Source releases and Windows binaries for OpenAL Soft are
available at its [homepage](https://openal-soft.org/).
Instructions are also provided there.

### Environment Variables
The following environment variables can be set:
- `DSOAL_LOGLEVEL`:
  - Values: Integer, range `0-3`
  - Description: Set the verbosity of DSOAL logging. Level 3 enables logging of API traces. By default, a log level of `1` is used to report errors only.
- `DSOAL_LOGFILE`:
  - Values: String
  - Description: Path to a file that will be created/overwritten by DSOAL on each execution. All logging will be redirected to that file. If unset, logging it written to the process's `stderr` output.
