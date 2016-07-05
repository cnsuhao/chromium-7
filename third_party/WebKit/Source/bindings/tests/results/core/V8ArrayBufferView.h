// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated by code_generator_v8.py. DO NOT MODIFY!

#ifndef V8ArrayBufferView_h
#define V8ArrayBufferView_h

#include "bindings/core/v8/ScriptWrappable.h"
#include "bindings/core/v8/ToV8.h"
#include "bindings/core/v8/V8Binding.h"
#include "bindings/core/v8/V8DOMWrapper.h"
#include "bindings/core/v8/WrapperTypeInfo.h"
#include "bindings/tests/idls/core/TestArrayBufferView.h"
#include "core/CoreExport.h"
#include "platform/heap/Handle.h"

namespace blink {

class V8ArrayBufferView {
    STATIC_ONLY(V8ArrayBufferView);
public:
    CORE_EXPORT static TestArrayBufferView* toImpl(v8::Local<v8::Object> object);
    CORE_EXPORT static TestArrayBufferView* toImplWithTypeCheck(v8::Isolate*, v8::Local<v8::Value>);
    CORE_EXPORT static const WrapperTypeInfo wrapperTypeInfo;
    static HeapObjectHeader* getHeapObjectHeader(ScriptWrappable* scriptWrappable)
    {
        return HeapObjectHeader::fromPayload(scriptWrappable->toImpl<TestArrayBufferView>());
    }
    template<typename VisitorDispatcher>
    static void trace(VisitorDispatcher visitor, ScriptWrappable* scriptWrappable)
    {
        visitor->trace(scriptWrappable->toImpl<TestArrayBufferView>());
    }
    static const int internalFieldCount = v8DefaultWrapperInternalFieldCount + 0;
    CORE_EXPORT static void preparePrototypeAndInterfaceObject(v8::Local<v8::Context>, const DOMWrapperWorld&, v8::Local<v8::Object> prototypeObject, v8::Local<v8::Function> interfaceObject, v8::Local<v8::FunctionTemplate> interfaceTemplate) { }
};

template <>
struct V8TypeOf<TestArrayBufferView> {
    typedef V8ArrayBufferView Type;
};

} // namespace blink

#endif // V8ArrayBufferView_h