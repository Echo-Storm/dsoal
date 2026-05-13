#include "dsfx.h"

#include <algorithm>
#include <cmath>

#include "buffer.h"
#include "AL/alext.h"
#include "AL/efx.h"
#include "dsoal.h"

namespace {
    using voidp = void*;

    inline float DbToLin(float db)
    {
        return std::pow(10.0f, db / 20.0f);
    }

    template<typename T>
    inline T Clamp(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

} /* namespace */

/* =========================================================================
 * BufferFXDistortion
 *
 * DirectSound param ranges -> OpenAL EFX param ranges:
 *  fGain               [-60, 0] dB      -> AL_DISTORTION_GAIN          [0.01, 1.0]
 *  fEdge               [0, 100] %       -> AL_DISTORTION_EDGE          [0.0, 1.0]
 *  fPostEQCenterFreq   [100, 8000] Hz   -> AL_DISTORTION_EQCENTER      same range
 *  fPostEQBandwidth    [100, 8000] Hz   -> AL_DISTORTION_EQBANDWIDTH   same range
 *  fPreLowpassCutoff   [100, 8000] Hz   -> AL_DISTORTION_LOWPASS_CUTOFF same range
 * ========================================================================= */

HRESULT STDMETHODCALLTYPE BufferFXDistortion::QueryInterface(REFIID riid, void** ppvObject) noexcept
{
    if (!ppvObject) return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_IDirectSoundFXDistortion8)
    {
        AddRef();
        *ppvObject = static_cast<IDirectSoundFXDistortion8*>(this);
        return S_OK;
    }
    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE BufferFXDistortion::AddRef() noexcept
{
    return ++mRef;
}

ULONG STDMETHODCALLTYPE BufferFXDistortion::Release() noexcept
{
    return --mRef;
}

HRESULT STDMETHODCALLTYPE BufferFXDistortion::SetAllParameters(const DSFXDistortion* p) noexcept
{
    if (!p) return E_POINTER;
    if (p->fGain < DSFXDISTORTION_GAIN_MIN || p->fGain > DSFXDISTORTION_GAIN_MAX ||
        p->fEdge < DSFXDISTORTION_EDGE_MIN || p->fEdge > DSFXDISTORTION_EDGE_MAX ||
        p->fPostEQCenterFrequency < DSFXDISTORTION_POSTEQCENTERFREQUENCY_MIN ||
        p->fPostEQCenterFrequency > DSFXDISTORTION_POSTEQCENTERFREQUENCY_MAX ||
        p->fPostEQBandwidth < DSFXDISTORTION_POSTEQBANDWIDTH_MIN ||
        p->fPostEQBandwidth > DSFXDISTORTION_POSTEQBANDWIDTH_MAX ||
        p->fPreLowpassCutoff < DSFXDISTORTION_PRELOWPASSCUTOFF_MIN ||
        p->fPreLowpassCutoff > DSFXDISTORTION_PRELOWPASSCUTOFF_MAX)
        return DSERR_INVALIDPARAM;

    mParams = *p;
    if (mEffect == 0) return S_OK;

    const float alGain = Clamp(DbToLin(p->fGain), 0.01f, 1.0f);
    const float alEdge = Clamp(p->fEdge / 100.0f, 0.0f, 1.0f);
    const float alEqCtr = Clamp(p->fPostEQCenterFrequency, 80.0f, 24000.0f);
    const float alEqBw = Clamp(p->fPostEQBandwidth, 80.0f, 24000.0f);
    const float alLpCut = Clamp(p->fPreLowpassCutoff, 80.0f, 24000.0f);

    ALCcontext* ctx = mParent->getContext();
    alEffectfDirect(ctx, mEffect, AL_DISTORTION_GAIN, alGain);
    alEffectfDirect(ctx, mEffect, AL_DISTORTION_EDGE, alEdge);
    alEffectfDirect(ctx, mEffect, AL_DISTORTION_EQCENTER, alEqCtr);
    alEffectfDirect(ctx, mEffect, AL_DISTORTION_EQBANDWIDTH, alEqBw);
    alEffectfDirect(ctx, mEffect, AL_DISTORTION_LOWPASS_CUTOFF, alLpCut);
    alGetErrorDirect(ctx);

    alAuxiliaryEffectSlotiDirect(ctx, mSlot, AL_EFFECTSLOT_EFFECT,
        static_cast<ALint>(mEffect));
    alGetErrorDirect(ctx);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BufferFXDistortion::GetAllParameters(DSFXDistortion* p) noexcept
{
    if (!p) return E_POINTER;
    *p = mParams;
    return S_OK;
}


/* =========================================================================
 * BufferFXEcho
 *
 *  fWetDryMix   [0, 100] %   -> no direct AL param
 *  fFeedback    [0, 100] %   -> AL_ECHO_FEEDBACK  [0.0, 1.0]
 *  fLeftDelay   [0, 2000] ms -> AL_ECHO_DELAY     [0.0, 0.207] sec
 *  fRightDelay  [0, 2000] ms -> AL_ECHO_LRDELAY   [0.0, 0.404] sec
 *  lPanDelay    [0, 1] bool  -> AL_ECHO_SPREAD    -1.0 or 1.0
 * ========================================================================= */

HRESULT STDMETHODCALLTYPE BufferFXEcho::QueryInterface(REFIID riid, void** ppvObject) noexcept
{
    if (!ppvObject) return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_IDirectSoundFXEcho8)
    {
        AddRef();
        *ppvObject = static_cast<IDirectSoundFXEcho8*>(this);
        return S_OK;
    }
    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE BufferFXEcho::AddRef() noexcept
{
    return ++mRef;
}

ULONG STDMETHODCALLTYPE BufferFXEcho::Release() noexcept
{
    return --mRef;
}

HRESULT STDMETHODCALLTYPE BufferFXEcho::SetAllParameters(const DSFXEcho* p) noexcept
{
    if (!p) return E_POINTER;

    mParams = *p;
    if (mEffect == 0) return S_OK;

    const float alFeedback = Clamp(p->fFeedback / 100.0f, 0.0f, 1.0f);
    const float alDelay = Clamp(p->fLeftDelay / 1000.0f, 0.0f, 0.207f);
    const float alLRDelay = Clamp(p->fRightDelay / 1000.0f, 0.0f, 0.404f);
    const float alSpread = (p->lPanDelay != 0) ? -1.0f : 1.0f;
    const float alDamping = 0.5f;

    ALCcontext* ctx = mParent->getContext();
    alEffectfDirect(ctx, mEffect, AL_ECHO_FEEDBACK, alFeedback);
    alEffectfDirect(ctx, mEffect, AL_ECHO_DELAY, alDelay);
    alEffectfDirect(ctx, mEffect, AL_ECHO_LRDELAY, alLRDelay);
    alEffectfDirect(ctx, mEffect, AL_ECHO_SPREAD, alSpread);
    alEffectfDirect(ctx, mEffect, AL_ECHO_DAMPING, alDamping);
    alGetErrorDirect(ctx);

    alAuxiliaryEffectSlotiDirect(ctx, mSlot, AL_EFFECTSLOT_EFFECT,
        static_cast<ALint>(mEffect));
    alGetErrorDirect(ctx);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BufferFXEcho::GetAllParameters(DSFXEcho* p) noexcept
{
    if (!p) return E_POINTER;
    *p = mParams;
    return S_OK;
}


/* =========================================================================
 * BufferFXParamEq
 *
 *  fCenter     [80, 16000] Hz   -> AL_EQUALIZER_MID1_CENTER  same range
 *  fBandwidth  [0.01, 5.0] oct -> AL_EQUALIZER_MID1_WIDTH   [0.01, 1.0]
 *  fGain       [-15, 15] dB    -> AL_EQUALIZER_MID1_GAIN    linear [0.126, 7.94]
 *
 * AL_EQUALIZER is a 4-band EQ. ParamEQ maps to the MID1 band.
 * Other bands left at defaults (flat response).
 * ========================================================================= */

HRESULT STDMETHODCALLTYPE BufferFXParamEq::QueryInterface(REFIID riid, void** ppvObject) noexcept
{
    if (!ppvObject) return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_IDirectSoundFXParamEq8)
    {
        AddRef();
        *ppvObject = static_cast<IDirectSoundFXParamEq8*>(this);
        return S_OK;
    }
    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE BufferFXParamEq::AddRef() noexcept
{
    return ++mRef;
}

ULONG STDMETHODCALLTYPE BufferFXParamEq::Release() noexcept
{
    return --mRef;
}

HRESULT STDMETHODCALLTYPE BufferFXParamEq::SetAllParameters(const DSFXParamEq* p) noexcept
{
    if (!p) return E_POINTER;

    mParams = *p;
    if (mEffect == 0) return S_OK;

    const float alCenter = Clamp(p->fCenter, 80.0f, 16000.0f);
    const float alWidth = Clamp(p->fBandwidth, 0.01f, 1.0f);
    const float alGain = Clamp(DbToLin(p->fGain), 0.126f, 7.94f);

    ALCcontext* ctx = mParent->getContext();
    alEffectfDirect(ctx, mEffect, AL_EQUALIZER_MID1_CENTER, alCenter);
    alEffectfDirect(ctx, mEffect, AL_EQUALIZER_MID1_WIDTH, alWidth);
    alEffectfDirect(ctx, mEffect, AL_EQUALIZER_MID1_GAIN, alGain);
    alGetErrorDirect(ctx);

    alAuxiliaryEffectSlotiDirect(ctx, mSlot, AL_EFFECTSLOT_EFFECT,
        static_cast<ALint>(mEffect));
    alGetErrorDirect(ctx);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE BufferFXParamEq::GetAllParameters(DSFXParamEq* p) noexcept
{
    if (!p) return E_POINTER;
    *p = mParams;
    return S_OK;
}
