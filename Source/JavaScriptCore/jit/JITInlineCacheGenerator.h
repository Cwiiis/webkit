/*
 * Copyright (C) 2013-2018 Apple Inc. All rights reserved.
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

#pragma once

#if ENABLE(JIT)

#include "AssemblyHelpers.h"
#include "CCallHelpers.h"
#include "CodeOrigin.h"
#include "JITOperationValidation.h"
#include "JITOperations.h"
#include "JSCJSValue.h"
#include "PutKind.h"
#include "RegisterSet.h"
#include <wtf/Bag.h>

namespace JSC {

class CacheableIdentifier;
class CallSiteIndex;
class CodeBlock;
class JIT;
class StructureStubInfo;
struct UnlinkedStructureStubInfo;

enum class AccessType : int8_t;
enum class JITType : uint8_t;

class JITInlineCacheGenerator {
protected:
    JITInlineCacheGenerator() { }
    JITInlineCacheGenerator(CodeBlock*, Bag<StructureStubInfo>*, JITType, CodeOrigin, CallSiteIndex, AccessType, const RegisterSet& usedRegisters);
    
public:
    StructureStubInfo* stubInfo() const { return m_stubInfo; }

    void reportSlowPathCall(CCallHelpers::Label slowPathBegin, CCallHelpers::Call call)
    {
        m_slowPathBegin = slowPathBegin;
        m_slowPathCall = call;
    }
    
    CCallHelpers::Label slowPathBegin() const { return m_slowPathBegin; }

    void finalize(
        LinkBuffer& fastPathLinkBuffer, LinkBuffer& slowPathLinkBuffer,
        CodeLocationLabel<JITStubRoutinePtrTag> start);

    void generateBaselineDataICFastPath(JIT&, unsigned stubInfoConstant, GPRReg stubInfoGPR);

    UnlinkedStructureStubInfo* m_unlinkedStubInfo { nullptr };
    unsigned m_unlinkedStubInfoConstantIndex { std::numeric_limits<unsigned>::max() };

protected:
    JITType m_jitType;
    StructureStubInfo* m_stubInfo { nullptr };

public:
    CCallHelpers::Label m_start;
    CCallHelpers::Label m_done;
    CCallHelpers::Label m_slowPathBegin;
    CCallHelpers::Call m_slowPathCall;
};

class JITByIdGenerator : public JITInlineCacheGenerator {
protected:
    JITByIdGenerator() { }

    JITByIdGenerator(
        CodeBlock*, Bag<StructureStubInfo>*, JITType, CodeOrigin, CallSiteIndex, AccessType, const RegisterSet& usedRegisters,
        JSValueRegs base, JSValueRegs value, GPRReg stubInfoGPR);

public:
    CCallHelpers::Jump slowPathJump() const
    {
        ASSERT(m_slowPathJump.isSet());
        return m_slowPathJump;
    }

    void finalize(
        LinkBuffer& fastPathLinkBuffer, LinkBuffer& slowPathLinkBuffer);
    
protected:
    
    void generateFastCommon(CCallHelpers&, size_t size);
    
    JSValueRegs m_base;
    JSValueRegs m_value;

public:
    CCallHelpers::Jump m_slowPathJump;
};

class JITGetByIdGenerator final : public JITByIdGenerator {
public:
    JITGetByIdGenerator() { }

    JITGetByIdGenerator(
        CodeBlock*, Bag<StructureStubInfo>*, JITType, CodeOrigin, CallSiteIndex, const RegisterSet& usedRegisters, CacheableIdentifier,
        JSValueRegs base, JSValueRegs value, GPRReg stubInfoGPR, AccessType);
    
    void generateFastPath(CCallHelpers&, GPRReg scratchGPR);
    void generateBaselineDataICFastPath(JIT&, unsigned stubInfoConstant, GPRReg stubInfoGPR);

private:
    bool m_isLengthAccess;
};

class JITGetByIdWithThisGenerator final : public JITByIdGenerator {
public:
    JITGetByIdWithThisGenerator() { }

    JITGetByIdWithThisGenerator(
        CodeBlock*, Bag<StructureStubInfo>*, JITType, CodeOrigin, CallSiteIndex, const RegisterSet& usedRegisters, CacheableIdentifier,
        JSValueRegs value, JSValueRegs base, JSValueRegs thisRegs, GPRReg stubInfoGPR);

    void generateFastPath(CCallHelpers&, GPRReg scratchGPR);
    void generateBaselineDataICFastPath(JIT&, unsigned stubInfoConstant, GPRReg stubInfoGPR);
};

class JITPutByIdGenerator final : public JITByIdGenerator {
public:
    JITPutByIdGenerator() = default;

    JITPutByIdGenerator(
        CodeBlock*, Bag<StructureStubInfo>*, JITType, CodeOrigin, CallSiteIndex, const RegisterSet& usedRegisters, CacheableIdentifier,
        JSValueRegs base, JSValueRegs value, GPRReg stubInfoGPR, GPRReg scratch, ECMAMode, PutKind);
    
    void generateFastPath(CCallHelpers&, GPRReg scratchGPR, GPRReg scratch2GPR);
    void generateBaselineDataICFastPath(JIT&, unsigned stubInfoConstant, GPRReg stubInfoGPR);
    
    V_JITOperation_GSsiJJC slowPathFunction();

private:
    ECMAMode m_ecmaMode { ECMAMode::strict() };
    PutKind m_putKind;
};

class JITPutByValGenerator final : public JITInlineCacheGenerator {
    using Base = JITInlineCacheGenerator;
public:
    JITPutByValGenerator() = default;

    JITPutByValGenerator(
        CodeBlock*, Bag<StructureStubInfo>*, JITType, CodeOrigin, CallSiteIndex, AccessType, const RegisterSet& usedRegisters,
        JSValueRegs base, JSValueRegs property, JSValueRegs result, GPRReg arrayProfileGPR, GPRReg stubInfoGPR);

    CCallHelpers::Jump slowPathJump() const
    {
        ASSERT(m_slowPathJump.m_jump.isSet());
        return m_slowPathJump.m_jump;
    }

    void finalize(LinkBuffer& fastPathLinkBuffer, LinkBuffer& slowPathLinkBuffer);

    void generateFastPath(CCallHelpers&);

    JSValueRegs m_base;
    JSValueRegs m_value;

    CCallHelpers::PatchableJump m_slowPathJump;
};

class JITDelByValGenerator final : public JITInlineCacheGenerator {
    using Base = JITInlineCacheGenerator;
public:
    JITDelByValGenerator() { }

    JITDelByValGenerator(
        CodeBlock*, Bag<StructureStubInfo>*, JITType, CodeOrigin, CallSiteIndex, const RegisterSet& usedRegisters,
        JSValueRegs base, JSValueRegs property, JSValueRegs result, GPRReg stubInfoGPR, GPRReg scratch);

    CCallHelpers::Jump slowPathJump() const
    {
        ASSERT(m_slowPathJump.m_jump.isSet());
        return m_slowPathJump.m_jump;
    }

    void finalize(
        LinkBuffer& fastPathLinkBuffer, LinkBuffer& slowPathLinkBuffer);

    void generateFastPath(CCallHelpers&);

    CCallHelpers::PatchableJump m_slowPathJump;
};

class JITDelByIdGenerator final : public JITInlineCacheGenerator {
    using Base = JITInlineCacheGenerator;
public:
    JITDelByIdGenerator() { }

    JITDelByIdGenerator(
        CodeBlock*, Bag<StructureStubInfo>*, JITType, CodeOrigin, CallSiteIndex, const RegisterSet& usedRegisters, CacheableIdentifier,
        JSValueRegs base, JSValueRegs result, GPRReg stubInfoGPR, GPRReg scratch);

    CCallHelpers::Jump slowPathJump() const
    {
        ASSERT(m_slowPathJump.m_jump.isSet());
        return m_slowPathJump.m_jump;
    }

    void finalize(
        LinkBuffer& fastPathLinkBuffer, LinkBuffer& slowPathLinkBuffer);

    void generateFastPath(CCallHelpers&);

    CCallHelpers::PatchableJump m_slowPathJump;
};

class JITInByValGenerator : public JITInlineCacheGenerator {
    using Base = JITInlineCacheGenerator;
public:
    JITInByValGenerator() { }

    JITInByValGenerator(
        CodeBlock*, Bag<StructureStubInfo>*, JITType, CodeOrigin, CallSiteIndex, AccessType, const RegisterSet& usedRegisters,
        JSValueRegs base, JSValueRegs property, JSValueRegs result, GPRReg stubInfoGPR);

    CCallHelpers::Jump slowPathJump() const
    {
        ASSERT(m_slowPathJump.m_jump.isSet());
        return m_slowPathJump.m_jump;
    }

    void finalize(
        LinkBuffer& fastPathLinkBuffer, LinkBuffer& slowPathLinkBuffer);

    void generateFastPath(CCallHelpers&);

    CCallHelpers::PatchableJump m_slowPathJump;
};

class JITInByIdGenerator final : public JITByIdGenerator {
public:
    JITInByIdGenerator() { }

    JITInByIdGenerator(
        CodeBlock*, Bag<StructureStubInfo>*, JITType, CodeOrigin, CallSiteIndex, const RegisterSet& usedRegisters, CacheableIdentifier,
        JSValueRegs base, JSValueRegs value, GPRReg stubInfoGPR);

    void generateFastPath(CCallHelpers&, GPRReg scratchGPR);
    void generateBaselineDataICFastPath(JIT&, unsigned stubInfoConstant, GPRReg stubInfoGPR);
};

class JITInstanceOfGenerator final : public JITInlineCacheGenerator {
public:
    using Base = JITInlineCacheGenerator;
    JITInstanceOfGenerator() { }
    
    JITInstanceOfGenerator(
        CodeBlock*, Bag<StructureStubInfo>*, JITType, CodeOrigin, CallSiteIndex, const RegisterSet& usedRegisters, GPRReg result,
        GPRReg value, GPRReg prototype, GPRReg stubInfoGPR, GPRReg scratch1, GPRReg scratch2,
        bool prototypeIsKnownObject = false);
    
    void generateFastPath(CCallHelpers&);

    CCallHelpers::Jump slowPathJump() const
    {
        ASSERT(m_slowPathJump.m_jump.isSet());
        return m_slowPathJump.m_jump;
    }

    void finalize(LinkBuffer& fastPathLinkBuffer, LinkBuffer& slowPathLinkBuffer);

    CCallHelpers::PatchableJump m_slowPathJump;
};

class JITGetByValGenerator final : public JITInlineCacheGenerator {
    using Base = JITInlineCacheGenerator;
public:
    JITGetByValGenerator() { }

    JITGetByValGenerator(
        CodeBlock*, Bag<StructureStubInfo>*, JITType, CodeOrigin, CallSiteIndex, AccessType, const RegisterSet& usedRegisters,
        JSValueRegs base, JSValueRegs property, JSValueRegs result, GPRReg stubInfoGPR);

    CCallHelpers::Jump slowPathJump() const
    {
        ASSERT(m_slowPathJump.m_jump.isSet());
        return m_slowPathJump.m_jump;
    }

    void finalize(
        LinkBuffer& fastPathLinkBuffer, LinkBuffer& slowPathLinkBuffer);
    
    void generateFastPath(CCallHelpers&);

    JSValueRegs m_base;
    JSValueRegs m_result;

    CCallHelpers::PatchableJump m_slowPathJump;
};

class JITPrivateBrandAccessGenerator final : public JITInlineCacheGenerator {
    using Base = JITInlineCacheGenerator;
public:
    JITPrivateBrandAccessGenerator() { }

    JITPrivateBrandAccessGenerator(
        CodeBlock*, Bag<StructureStubInfo>*, JITType, CodeOrigin, CallSiteIndex, AccessType, const RegisterSet& usedRegisters,
        JSValueRegs base, JSValueRegs brand, GPRReg stubInfoGPR);

    CCallHelpers::Jump slowPathJump() const
    {
        ASSERT(m_slowPathJump.m_jump.isSet());
        return m_slowPathJump.m_jump;
    }

    void finalize(
        LinkBuffer& fastPathLinkBuffer, LinkBuffer& slowPathLinkBuffer);
    
    void generateFastPath(CCallHelpers&);

    CCallHelpers::PatchableJump m_slowPathJump;
};

template<typename VectorType>
void finalizeInlineCaches(VectorType& vector, LinkBuffer& fastPath, LinkBuffer& slowPath)
{
    for (auto& entry : vector)
        entry.finalize(fastPath, slowPath);
}

template<typename VectorType>
void finalizeInlineCaches(VectorType& vector, LinkBuffer& linkBuffer)
{
    finalizeInlineCaches(vector, linkBuffer, linkBuffer);
}

} // namespace JSC

#endif // ENABLE(JIT)
