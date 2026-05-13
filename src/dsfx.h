#ifndef DSFX_H
#define DSFX_H

/* dsfx.h
 * Implements IDirectSoundFXDistortion8, IDirectSoundFXEcho8, and
 * IDirectSoundFXParamEq8 via OpenAL EFX. These are the three DS8 FX
 * interfaces used by Fallout New Vegas for voice modulation filters.
 *
 * Each DSFXxxx COM class is an inner class of Buffer, following the same
 * pattern as Buffer::Buffer3D and Buffer::Notify. The Buffer class owns
 * a std::vector<FXEntry> (mFXList) populated by SetFX. GetObjectInPath
 * returns a pointer to the appropriate inner class instance by matching
 * the requested GUID against mFXList entries.
 */

#include <atomic>
#include <dsound.h>

#include "AL/al.h"
#include "AL/alext.h"
#include "AL/efx.h"


/* Stored per effect created by SetFX. */
struct FXEntry {
    GUID    mGuid{};        /* which DSFX type */
    ALuint  mEffect{0};     /* OpenAL effect object */
    ALuint  mSlot{0};       /* OpenAL aux effect slot */
};


/* Forward declarations of the three inner COM classes. Defined in dsfx.cpp
 * and declared here so buffer.h can embed them as members. Each class holds
 * a raw index into Buffer::mFXList rather than a pointer so it survives
 * vector reallocations. The parent Buffer* is recovered via CONTAINING_RECORD.
 */

class Buffer;

class BufferFXDistortion final : public IDirectSoundFXDistortion8 {
    Buffer *mParent{};
    std::atomic<ULONG> mRef{0u};

public:
    explicit BufferFXDistortion(Buffer *parent) noexcept : mParent{parent} {}

    /* IUnknown */
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) noexcept override;
    ULONG   STDMETHODCALLTYPE AddRef()  noexcept override;
    ULONG   STDMETHODCALLTYPE Release() noexcept override;

    /* IDirectSoundFXDistortion8 */
    HRESULT STDMETHODCALLTYPE SetAllParameters(const DSFXDistortion *distortion) noexcept override;
    HRESULT STDMETHODCALLTYPE GetAllParameters(DSFXDistortion *distortion) noexcept override;

    /* Cached params for GetAllParameters */
    DSFXDistortion mParams{};
    ALuint mEffect{0};      /* owned by FXEntry - do NOT delete here */
    ALuint mSlot{0};
};


class BufferFXEcho final : public IDirectSoundFXEcho8 {
    Buffer *mParent{};
    std::atomic<ULONG> mRef{0u};

public:
    explicit BufferFXEcho(Buffer *parent) noexcept : mParent{parent} {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) noexcept override;
    ULONG   STDMETHODCALLTYPE AddRef()  noexcept override;
    ULONG   STDMETHODCALLTYPE Release() noexcept override;

    HRESULT STDMETHODCALLTYPE SetAllParameters(const DSFXEcho *echo) noexcept override;
    HRESULT STDMETHODCALLTYPE GetAllParameters(DSFXEcho *echo) noexcept override;

    DSFXEcho mParams{};
    ALuint mEffect{0};
    ALuint mSlot{0};
};


class BufferFXParamEq final : public IDirectSoundFXParamEq8 {
    Buffer *mParent{};
    std::atomic<ULONG> mRef{0u};

public:
    explicit BufferFXParamEq(Buffer *parent) noexcept : mParent{parent} {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) noexcept override;
    ULONG   STDMETHODCALLTYPE AddRef()  noexcept override;
    ULONG   STDMETHODCALLTYPE Release() noexcept override;

    HRESULT STDMETHODCALLTYPE SetAllParameters(const DSFXParamEq *parameq) noexcept override;
    HRESULT STDMETHODCALLTYPE GetAllParameters(DSFXParamEq *parameq) noexcept override;

    DSFXParamEq mParams{};
    ALuint mEffect{0};
    ALuint mSlot{0};
};

#endif /* DSFX_H */
