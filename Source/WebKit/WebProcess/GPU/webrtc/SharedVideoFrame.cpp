/*
 * Copyright (C) 2022 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SharedVideoFrame.h"

#if ENABLE(GPU_PROCESS) && PLATFORM(COCOA) && ENABLE(VIDEO)

#include "Logging.h"
#include "RemoteVideoFrameObjectHeap.h"
#include "RemoteVideoFrameProxy.h"
#include <WebCore/CVUtilities.h>
#include <WebCore/IOSurface.h>
#include <WebCore/SharedVideoFrameInfo.h>
#include <WebCore/VideoFrameCV.h>
#include <WebCore/VideoFrameLibWebRTC.h>
#include <wtf/Scope.h>

#if USE(LIBWEBRTC)
#include <webrtc/sdk/WebKit/WebKitUtilities.h>
#endif

#include <pal/cf/CoreMediaSoftLink.h>
#include <WebCore/CoreVideoSoftLink.h>

namespace WebKit {
using namespace WebCore;

SharedVideoFrameWriter::SharedVideoFrameWriter()
    : m_semaphore(makeUniqueRef<IPC::Semaphore>())
{
}

bool SharedVideoFrameWriter::wait(const Function<void(IPC::Semaphore&)>& newSemaphoreCallback)
{
    if (!m_isSemaphoreInUse) {
        m_isSemaphoreInUse = true;
        newSemaphoreCallback(m_semaphore.get());
        return true;
    }
    return !m_isDisabled && m_semaphore->wait();
}

bool SharedVideoFrameWriter::allocateStorage(size_t size, const Function<void(const SharedMemory::IPCHandle&)>& newMemoryCallback)
{
    m_storage = SharedMemory::allocate(size);
    if (!m_storage)
        return false;

    SharedMemory::Handle handle;
    m_storage->createHandle(handle, SharedMemory::Protection::ReadOnly);

    auto dataSize = handle.size();
    newMemoryCallback(SharedMemory::IPCHandle { WTFMove(handle), dataSize });
    return true;
}

bool SharedVideoFrameWriter::prepareWriting(const SharedVideoFrameInfo& info, const Function<void(IPC::Semaphore&)>& newSemaphoreCallback, const Function<void(const SharedMemory::IPCHandle&)>& newMemoryCallback)
{
    if (!info.isReadWriteSupported())
        return false;

    if (!wait(newSemaphoreCallback))
        return false;

    size_t size = info.storageSize();
    if (!m_storage || m_storage->size() < size) {
        if (!allocateStorage(size, newMemoryCallback))
            return false;
    }
    return true;
}

std::optional<SharedVideoFrame> SharedVideoFrameWriter::write(VideoFrame& frame, const Function<void(IPC::Semaphore&)>& newSemaphoreCallback, const Function<void(const SharedMemory::IPCHandle&)>& newMemoryCallback)
{
    auto buffer = writeBuffer(frame, newSemaphoreCallback, newMemoryCallback);
    if (!buffer)
        return { };
    return SharedVideoFrame { frame.presentationTime(), frame.isMirrored(), frame.rotation(), WTFMove(*buffer) };
}

std::optional<SharedVideoFrame::Buffer> SharedVideoFrameWriter::writeBuffer(VideoFrame& frame, const Function<void(IPC::Semaphore&)>& newSemaphoreCallback, const Function<void(const SharedMemory::IPCHandle&)>& newMemoryCallback)
{
    if (is<RemoteVideoFrameProxy>(frame))
        return downcast<RemoteVideoFrameProxy>(frame).newReadReference();

#if USE(LIBWEBRTC)
    if (is<VideoFrameLibWebRTC>(frame))
        return writeBuffer(*downcast<VideoFrameLibWebRTC>(frame).buffer(), newSemaphoreCallback, newMemoryCallback);
#endif

    return writeBuffer(frame.pixelBuffer(), newSemaphoreCallback, newMemoryCallback);
}

std::optional<SharedVideoFrame::Buffer> SharedVideoFrameWriter::writeBuffer(CVPixelBufferRef pixelBuffer, const Function<void(IPC::Semaphore&)>& newSemaphoreCallback, const Function<void(const SharedMemory::IPCHandle&)>& newMemoryCallback, bool canUseIOSurface)
{
    if (!pixelBuffer)
        return { };

    if (canUseIOSurface) {
        if (auto surface = CVPixelBufferGetIOSurface(pixelBuffer))
            return MachSendRight::adopt(IOSurfaceCreateMachPort(surface));
    }

    auto info = SharedVideoFrameInfo::fromCVPixelBuffer(pixelBuffer);
    if (!prepareWriting(info, newSemaphoreCallback, newMemoryCallback))
        return { };

    if (!info.writePixelBuffer(pixelBuffer, static_cast<uint8_t*>(m_storage->data())))
        return { };
    return nullptr;
}

#if USE(LIBWEBRTC)
std::optional<SharedVideoFrame::Buffer> SharedVideoFrameWriter::writeBuffer(const webrtc::VideoFrame& frame, const Function<void(IPC::Semaphore&)>& newSemaphoreCallback, const Function<void(const SharedMemory::IPCHandle&)>& newMemoryCallback)
{
    if (auto* provider = webrtc::videoFrameBufferProvider(frame))
        return writeBuffer(*static_cast<VideoFrame*>(provider), newSemaphoreCallback, newMemoryCallback);

    if (auto pixelBuffer = adoptCF(webrtc::pixelBufferFromFrame(frame)))
        return writeBuffer(pixelBuffer.get(), newSemaphoreCallback, newMemoryCallback);

    return writeBuffer(*frame.video_frame_buffer(), newSemaphoreCallback, newMemoryCallback);
}

std::optional<SharedVideoFrame::Buffer> SharedVideoFrameWriter::writeBuffer(webrtc::VideoFrameBuffer& frameBuffer, const Function<void(IPC::Semaphore&)>& newSemaphoreCallback, const Function<void(const SharedMemory::IPCHandle&)>& newMemoryCallback)
{
    auto info = SharedVideoFrameInfo::fromVideoFrameBuffer(frameBuffer);
    if (!prepareWriting(info, newSemaphoreCallback, newMemoryCallback))
        return { };

    if (!info.writeVideoFrameBuffer(frameBuffer, static_cast<uint8_t*>(m_storage->data())))
        return { };
    return nullptr;
}
#endif

void SharedVideoFrameWriter::disable()
{
    m_isDisabled = true;
    m_semaphore->signal();
}

SharedVideoFrameReader::SharedVideoFrameReader(RefPtr<RemoteVideoFrameObjectHeap>&& objectHeap, const ProcessIdentity& resourceOwner, UseIOSurfaceBufferPool useIOSurfaceBufferPool)
    : m_objectHeap(WTFMove(objectHeap))
    , m_resourceOwner(resourceOwner)
    , m_useIOSurfaceBufferPool(useIOSurfaceBufferPool)
{
}

SharedVideoFrameReader::SharedVideoFrameReader() = default;

RetainPtr<CVPixelBufferRef> SharedVideoFrameReader::readBufferFromSharedMemory()
{
    if (!m_storage)
        return { };

    auto scope = makeScopeExit([&] {
        m_semaphore.signal();
    });

    auto* data = static_cast<const uint8_t*>(m_storage->data());
    auto info = SharedVideoFrameInfo::decode({ data, m_storage->size() });
    if (!info)
        return { };

    if (!info->isReadWriteSupported())
        return { };

    if (m_storage->size() < info->storageSize())
        return { };

    auto result = info->createPixelBufferFromMemory(data + SharedVideoFrameInfoEncodingLength, pixelBufferPool(*info));
    if (result && m_resourceOwner && m_useIOSurfaceBufferPool == UseIOSurfaceBufferPool::Yes)
        setOwnershipIdentityForCVPixelBuffer(result.get(), m_resourceOwner);
    return result;
}

RetainPtr<CVPixelBufferRef> SharedVideoFrameReader::readBuffer(SharedVideoFrame::Buffer&& buffer)
{
    return switchOn(WTFMove(buffer),
    [this](RemoteVideoFrameReadReference&& reference) -> RetainPtr<CVPixelBufferRef> {
        ASSERT(m_objectHeap);
        if (!m_objectHeap)
            return nullptr;

        auto sample = m_objectHeap->get(WTFMove(reference));
        if (!sample)
            return nullptr;
        ASSERT(sample->pixelBuffer());
        return sample->pixelBuffer();
    } , [](MachSendRight&& sendRight) -> RetainPtr<CVPixelBufferRef> {
        auto surface = WebCore::IOSurface::createFromSendRight(WTFMove(sendRight), DestinationColorSpace::SRGB());
        if (!surface)
            return nullptr;
        return WebCore::createCVPixelBuffer(surface->surface()).value_or(nullptr);
    }, [this](std::nullptr_t representation) -> RetainPtr<CVPixelBufferRef> {
        return readBufferFromSharedMemory();
    }, [this](IntSize size) -> RetainPtr<CVPixelBufferRef> {
        if (m_blackFrameSize != size) {
            m_blackFrameSize = size;
            m_blackFrame = WebCore::createBlackPixelBuffer(m_blackFrameSize.width(), m_blackFrameSize.height(), m_useIOSurfaceBufferPool == UseIOSurfaceBufferPool::Yes);
            if (m_resourceOwner && m_useIOSurfaceBufferPool == UseIOSurfaceBufferPool::Yes)
                setOwnershipIdentityForCVPixelBuffer(m_blackFrame.get(), m_resourceOwner);
        }
        return m_blackFrame.get();
    });
}

RefPtr<VideoFrame> SharedVideoFrameReader::read(SharedVideoFrame&& sharedVideoFrame)
{
    auto pixelBuffer = readBuffer(WTFMove(sharedVideoFrame.buffer));
    if (!pixelBuffer)
        return nullptr;

    return VideoFrameCV::create(sharedVideoFrame.time, sharedVideoFrame.mirrored, sharedVideoFrame.rotation, WTFMove(pixelBuffer));
}

CVPixelBufferPoolRef SharedVideoFrameReader::pixelBufferPool(const SharedVideoFrameInfo& info)
{
    if (m_useIOSurfaceBufferPool == UseIOSurfaceBufferPool::No)
        return nullptr;

    if (!m_bufferPool || m_bufferPoolType != info.bufferType() || m_bufferPoolWidth != info.width() || m_bufferPoolHeight != info.height()) {
        m_bufferPoolType = info.bufferType();
        m_bufferPoolWidth = info.width();
        m_bufferPoolHeight = info.height();
        m_bufferPool = info.createCompatibleBufferPool();
    }

    return m_bufferPool.get();
}

bool SharedVideoFrameReader::setSharedMemory(const SharedMemory::IPCHandle& ipcHandle)
{
    m_storage = SharedMemory::map(ipcHandle.handle, SharedMemory::Protection::ReadOnly);
    return !!m_storage;
}

}

#endif
