#pragma once

#include "Audio/AudioTypes.h"
#include "Core/Object/FunctionLibrary.h"
#include "Core/Object/ObjectMacros.h"
#include "Core/Math/Vector/VectorTypes.h"
#include "AudioLibrary.generated.h"

namespace Lumina
{
    class CSoundBase;

    /** Playback and mix control. There is one audio device per process, so none of this takes a world. */
    REFLECT()
    class RUNTIME_API CAudioLibrary : public CFunctionLibrary
    {
        GENERATED_BODY()

    public:

        //~ Playback.

        /** A non-spatialized sound such as music or a UI cue. An invalid handle means it did not start. */
        FUNCTION()
        static SAudioHandle PlaySound2D(CSoundBase* Sound, float Volume = 1.0f, float Pitch = 1.0f,
            bool bLoop = false);

        /** Attenuated between MinDistance and MaxDistance around Location. */
        FUNCTION()
        static SAudioHandle PlaySoundAtLocation(CSoundBase* Sound, FVector3 Location, float Volume = 1.0f,
            float Pitch = 1.0f, float MinDistance = 1.0f, float MaxDistance = 50.0f, bool bLoop = false);

        /** Full control over bus, attenuation, priority, fades and occlusion. */
        FUNCTION()
        static SAudioHandle PlaySoundEx(CSoundBase* Sound, SAudioPlayParams Params);

        /** The engine defaults, so script starts from full volume on the SFX bus rather than from zeros. */
        FUNCTION()
        static SAudioPlayParams DefaultPlayParams();

        FUNCTION()
        static void Stop(SAudioHandle Handle, bool bAllowFadeOut = false, float FadeSeconds = 0.5f);

        FUNCTION()
        static void StopAll(bool bAllowFadeOut = false, float FadeSeconds = 0.5f);

        //~ One voice.

        FUNCTION()
        static void SetVolume(SAudioHandle Handle, float Volume);

        FUNCTION()
        static void SetPitch(SAudioHandle Handle, float Pitch);

        FUNCTION()
        static void SetLooping(SAudioHandle Handle, bool bLoop);

        FUNCTION()
        static void SetPaused(SAudioHandle Handle, bool bPaused);

        FUNCTION()
        static void SetPan(SAudioHandle Handle, float Pan);

        FUNCTION()
        static void SetBus(SAudioHandle Handle, EAudioBus Bus);

        FUNCTION()
        static void FadeTo(SAudioHandle Handle, float Volume, float Seconds);

        FUNCTION()
        static void SeekToFrame(SAudioHandle Handle, uint64 Frame);

        FUNCTION()
        static EAudioVoiceState GetVoiceState(SAudioHandle Handle);

        FUNCTION()
        static uint64 GetPlaybackFrame(SAudioHandle Handle);

        //~ Spatialization.

        FUNCTION()
        static void SetPosition(SAudioHandle Handle, FVector3 Position);

        FUNCTION()
        static void SetVelocity(SAudioHandle Handle, FVector3 Velocity);

        FUNCTION()
        static void SetDirection(SAudioHandle Handle, FVector3 Direction);

        FUNCTION()
        static void SetMinMaxDistance(SAudioHandle Handle, float MinDistance, float MaxDistance);

        FUNCTION()
        static void SetAttenuation(SAudioHandle Handle, SAudioAttenuation Attenuation);

        FUNCTION()
        static void SetOcclusion(SAudioHandle Handle, float Amount, float LowPassFrequency = 700.0f,
            float VolumeAttenuation = 0.5f);

        FUNCTION()
        static void SetLowPassCutoff(SAudioHandle Handle, float CutoffHz);

        //~ Graph parameters, so a one shot fired without a component can still be driven.

        FUNCTION()
        static bool SetGraphFloat(SAudioHandle Handle, const FName& Name, float Value);

        FUNCTION()
        static bool SetGraphInt(SAudioHandle Handle, const FName& Name, int32 Value);

        FUNCTION()
        static bool SetGraphBool(SAudioHandle Handle, const FName& Name, bool Value);

        FUNCTION()
        static bool TriggerGraph(SAudioHandle Handle, const FName& Name);

        FUNCTION()
        static float GetGraphFloatOutput(SAudioHandle Handle, const FName& Name);

        FUNCTION()
        static uint32 GetGraphTriggerCount(SAudioHandle Handle, const FName& Name);

        //~ The mix.

        FUNCTION()
        static int32 GetActiveVoiceCount();

        FUNCTION()
        static void SetBusVolume(EAudioBus Bus, float Volume);

        /** Zero when there is no audio device, which is the contract script has always had here. */
        FUNCTION()
        static float GetBusVolume(EAudioBus Bus);

        FUNCTION()
        static void SetBusMuted(EAudioBus Bus, bool bMuted);

        FUNCTION()
        static bool IsBusMuted(EAudioBus Bus);

        FUNCTION()
        static void SetBusReverbSend(EAudioBus Bus, float SendLevel);

        FUNCTION()
        static void SetReverbParams(float RoomSize, float Damping, float Width, float WetLevel);

        FUNCTION()
        static void SetDopplerScale(float Scale);

        FUNCTION()
        static void SetSuspended(bool bSuspended);

        /** Writes the live bus volumes back into CAudioSettings and persists them, the options-menu save. */
        FUNCTION()
        static void SaveMixSettings();
    };
}
