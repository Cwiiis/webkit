/*
 * Copyright (C) 2011-2021 Apple Inc. All rights reserved.
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

#include "config.h"
#include "HandleSet.h"

#include "HandleBlock.h"
#include "HandleBlockInlines.h"
#include "JSCJSValueInlines.h"

namespace JSC {

HandleSet::HandleSet(VM& vm)
    : m_vm(vm)
{
    grow();
}

HandleSet::~HandleSet()
{
    while (!m_blockList.isEmpty())
        HandleBlock::destroy(m_blockList.removeHead());
}

void HandleSet::grow()
{
    HandleBlock* newBlock = HandleBlock::create(this);
    m_blockList.append(newBlock);

    for (int i = newBlock->nodeCapacity() - 1; i >= 0; --i) {
        Node* node = newBlock->nodeAtIndex(i);
        new (NotNull, node) Node;
        m_freeList.push(node);
    }
}

template<typename Visitor>
void HandleSet::visitStrongHandles(Visitor& visitor)
{
    for (Node& node : m_strongList) {
#if ENABLE(GC_VALIDATION)
        RELEASE_ASSERT(isLiveNode(&node));
#endif
        visitor.appendUnbarriered(*node.slot());
    }
}

template void HandleSet::visitStrongHandles(AbstractSlotVisitor&);
template void HandleSet::visitStrongHandles(SlotVisitor&);

void HandleSet::writeBarrier(HandleSlot slot, const JSValue& value)
{
    if (!value == !*slot && slot->isCell() == value.isCell())
        return;

    Node* node = toNode(slot);
#if ENABLE(GC_VALIDATION)
    RELEASE_ASSERT(isLiveNode(node));
#endif
    SentinelLinkedList<Node>::remove(node);
    if (!value || !value.isCell()) {
        m_immediateList.push(node);
        return;
    }

    m_strongList.push(node);
#if ENABLE(GC_VALIDATION)
    RELEASE_ASSERT(isLiveNode(node));
#endif
}

unsigned HandleSet::protectedGlobalObjectCount()
{
    unsigned count = 0;
    for (Node& node : m_strongList) {
        JSValue value = *node.slot();
        if (value.isObject() && asObject(value.asCell())->isGlobalObject())
            count++;
    }
    return count;
}

#if ENABLE(GC_VALIDATION) || ASSERT_ENABLED
bool HandleSet::isLiveNode(Node* node)
{
    if (node->prev()->next() != node)
        return false;
    if (node->next()->prev() != node)
        return false;
        
    return true;
}
#endif // ENABLE(GC_VALIDATION) || ASSERT_ENABLED

} // namespace JSC
