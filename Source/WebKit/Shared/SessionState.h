/*
 * Copyright (C) 2014-2018 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "ViewSnapshotStore.h"
#include <WebCore/BackForwardItemIdentifier.h>
#include <WebCore/FloatRect.h>
#include <WebCore/FrameLoaderTypes.h>
#include <WebCore/IntRect.h>
#include <WebCore/SerializedScriptValue.h>
#include <wtf/EnumTraits.h>
#include <wtf/RunLoop.h>
#include <wtf/URL.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace IPC {
class Decoder;
class Encoder;
}

namespace WebKit {

struct HTTPBody {
    struct Element {
        void encode(IPC::Encoder&) const;
        static std::optional<Element> decode(IPC::Decoder&);

        enum class Type {
            Data,
            File,
            Blob,
        };

        // FIXME: This should be a std::variant. It's also unclear why we don't just use FormDataElement here.
        Type type = Type::Data;

        // Data.
        Vector<uint8_t> data;

        // File.
        String filePath;
        int64_t fileStart;
        std::optional<int64_t> fileLength;
        std::optional<WallTime> expectedFileModificationTime;

        // Blob.
        String blobURLString;
    };

    void encode(IPC::Encoder&) const;
    static WARN_UNUSED_RETURN bool decode(IPC::Decoder&, HTTPBody&);

    String contentType;
    Vector<Element> elements;
};

class FrameState {
public:
    void encode(IPC::Encoder&) const;
    static std::optional<FrameState> decode(IPC::Decoder&);

    // These are used to help debug <rdar://problem/48634553>.
    FrameState() { RELEASE_ASSERT(RunLoop::isMain()); }
    ~FrameState() { RELEASE_ASSERT(RunLoop::isMain()); }
    FrameState(const FrameState&) = default;
    FrameState(FrameState&&) = default;
    FrameState& operator=(const FrameState&) = default;
    FrameState& operator=(FrameState&&) = default;

    const Vector<AtomString>& documentState() const { return m_documentState; }
    enum class ShouldValidate : bool { No, Yes };
    void setDocumentState(const Vector<AtomString>&, ShouldValidate = ShouldValidate::No);
    void validateDocumentState() const;

    String urlString;
    String originalURLString;
    String referrer;
    String target;

    std::optional<Vector<uint8_t>> stateObjectData;

    int64_t documentSequenceNumber { 0 };
    int64_t itemSequenceNumber { 0 };

    WebCore::IntPoint scrollPosition;
    bool shouldRestoreScrollPosition { true };
    float pageScaleFactor { 0 };

    std::optional<HTTPBody> httpBody;

    // FIXME: These should not be per frame.
#if PLATFORM(IOS_FAMILY)
    WebCore::FloatRect exposedContentRect;
    WebCore::IntRect unobscuredContentRect;
    WebCore::FloatSize minimumLayoutSizeInScrollViewCoordinates;
    WebCore::IntSize contentSize;
    bool scaleIsInitial { false };
    WebCore::FloatBoxExtent obscuredInsets;
#endif

    Vector<FrameState> children;

private:
    Vector<AtomString> m_documentState;
};

struct PageState {
    void encode(IPC::Encoder&) const;
    static WARN_UNUSED_RETURN bool decode(IPC::Decoder&, PageState&);

    String title;
    FrameState mainFrameState;
    WebCore::ShouldOpenExternalURLsPolicy shouldOpenExternalURLsPolicy { WebCore::ShouldOpenExternalURLsPolicy::ShouldNotAllow };
    RefPtr<WebCore::SerializedScriptValue> sessionStateObject;
};

struct BackForwardListItemState {
    void encode(IPC::Encoder&) const;
    static std::optional<BackForwardListItemState> decode(IPC::Decoder&);

    WebCore::BackForwardItemIdentifier identifier;

    PageState pageState;
#if PLATFORM(COCOA) || PLATFORM(GTK)
    RefPtr<ViewSnapshot> snapshot;
#endif
    bool hasCachedPage { false };
};

struct BackForwardListState {
    void encode(IPC::Encoder&) const;
    static std::optional<BackForwardListState> decode(IPC::Decoder&);

    Vector<BackForwardListItemState> items;
    std::optional<uint32_t> currentIndex;
};

struct SessionState {
    BackForwardListState backForwardListState;
    uint64_t renderTreeSize;
    URL provisionalURL;
    bool isAppInitiated { true };
};

} // namespace WebKit

namespace WTF {

template<> struct EnumTraits<WebKit::HTTPBody::Element::Type> {
    using values = EnumValues<
        WebKit::HTTPBody::Element::Type,
        WebKit::HTTPBody::Element::Type::Data,
        WebKit::HTTPBody::Element::Type::File,
        WebKit::HTTPBody::Element::Type::Blob
    >;
};

} // namespace WTF
