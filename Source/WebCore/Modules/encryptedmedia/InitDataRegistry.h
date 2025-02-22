/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#if ENABLE(ENCRYPTED_MEDIA)

#include <wtf/Function.h>
#include <wtf/Ref.h>
#include <wtf/RefPtr.h>
#include <wtf/RobinHoodHashMap.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomString.h>
#include <wtf/text/AtomStringHash.h>

namespace WebCore {

class ISOProtectionSystemSpecificHeaderBox;
class FragmentedSharedBuffer;

class InitDataRegistry {
public:
    WEBCORE_EXPORT static InitDataRegistry& shared();
    friend class NeverDestroyed<InitDataRegistry>;

    RefPtr<FragmentedSharedBuffer> sanitizeInitData(const AtomString& initDataType, const FragmentedSharedBuffer&);
    WEBCORE_EXPORT std::optional<Vector<Ref<FragmentedSharedBuffer>>> extractKeyIDs(const AtomString& initDataType, const FragmentedSharedBuffer&);

    struct InitDataTypeCallbacks {
        using SanitizeInitDataCallback = Function<RefPtr<FragmentedSharedBuffer>(const FragmentedSharedBuffer&)>;
        using ExtractKeyIDsCallback = Function<std::optional<Vector<Ref<FragmentedSharedBuffer>>>(const FragmentedSharedBuffer&)>;

        SanitizeInitDataCallback sanitizeInitData;
        ExtractKeyIDsCallback extractKeyIDs;
    };
    void registerInitDataType(const AtomString& initDataType, InitDataTypeCallbacks&&);

    static const AtomString& cencName();
    static const AtomString& keyidsName();
    static const AtomString& webmName();

    static std::optional<Vector<std::unique_ptr<ISOProtectionSystemSpecificHeaderBox>>> extractPsshBoxesFromCenc(const FragmentedSharedBuffer&);
    static std::optional<Vector<Ref<FragmentedSharedBuffer>>> extractKeyIDsCenc(const FragmentedSharedBuffer&);
    static RefPtr<FragmentedSharedBuffer> sanitizeCenc(const FragmentedSharedBuffer&);

private:
    InitDataRegistry();
    ~InitDataRegistry();

    MemoryCompactRobinHoodHashMap<AtomString, InitDataTypeCallbacks> m_types;
};

}

#endif // ENABLE(ENCRYPTED_MEDIA)
