/*
This file (API.hpp) is licensed under the MIT license and is separate from the rest of the UEVR codebase.

Copyright (c) 2023 praydog

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

extern "C" {
    #include "API.h"
}

#include <mutex>
#include <array>
#include <vector>
#include <cassert>
#include <string_view>
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace uevr {
class API {
private:
    static inline std::unique_ptr<API> s_instance{};

public:
    // ALWAYS call initialize first in uevr_plugin_initialize
    static auto& initialize(const UEVR_PluginInitializeParam* param) {
        if (param == nullptr) {
            throw std::runtime_error("param is null");
        }

        if (s_instance != nullptr) {
            throw std::runtime_error("API already initialized");
        }

        s_instance = std::make_unique<API>(param);
        return s_instance;
    }

    // only call this AFTER calling initialize
    static auto& get() {
        if (s_instance == nullptr) {
            throw std::runtime_error("API not initialized");
        }

        return s_instance;
    }

public:
    API(const UEVR_PluginInitializeParam* param) 
        : m_param{param},
        m_sdk{param->sdk}
    {
    }

    virtual ~API() {

    }

    inline const auto param() const {
        return m_param;
    }

    inline const auto sdk() const {
        return m_sdk;
    }

    template <typename... Args> void log_error(const char* format, Args... args) { m_param->functions->log_error(format, args...); }
    template <typename... Args> void log_warn(const char* format, Args... args) { m_param->functions->log_warn(format, args...); }
    template <typename... Args> void log_info(const char* format, Args... args) { m_param->functions->log_info(format, args...); }

public:
    // C++ wrapper structs for the C structs
    struct UObject;
    struct UEngine;
    struct UGameEngine;
    struct UWorld;
    struct UStruct;
    struct UClass;
    struct UFunction;
    struct FField;
    struct FProperty;
    struct FFieldClass;
    struct FUObjectArray;
    struct FName;

    template<typename T = UObject>
    T* find_uobject(std::wstring_view name) {
        static const auto fn = sdk()->uobject_array->find_uobject;
        return (T*)fn(name.data());
    }

    UEngine* get_engine() {
        static const auto fn = sdk()->functions->get_uengine;
        return (UEngine*)fn();
    }

    UObject* get_player_controller(int32_t index) {
        static const auto fn = sdk()->functions->get_player_controller;
        return (UObject*)fn(index);
    }

    UObject* get_local_pawn(int32_t index) {
        static const auto fn = sdk()->functions->get_local_pawn;
        return (UObject*)fn(index);
    }

    UObject* spawn_object(UClass* klass, UObject* outer) {
        static const auto fn = sdk()->functions->spawn_object;
        return (UObject*)fn(klass->to_handle(), outer->to_handle());
    }

    void execute_command(std::wstring_view command) {
        static const auto fn = sdk()->functions->execute_command;
        fn(command.data());
    }

    void execute_command_ex(UWorld* world, std::wstring_view command, void* output_device) {
        static const auto fn = sdk()->functions->execute_command_ex;
        fn((UEVR_UObjectHandle)world, command.data(), output_device);
    }

    FUObjectArray* get_uobject_array() {
        static const auto fn = sdk()->functions->get_uobject_array;
        return (FUObjectArray*)fn();
    }

    struct FName {
        inline UEVR_FNameHandle to_handle() { return (UEVR_FNameHandle)this; }
        inline UEVR_FNameHandle to_handle() const { return (UEVR_FNameHandle)this; }

        std::wstring to_string() const {
            static const auto fn = initialize()->to_string;
            const auto size = fn(to_handle(), nullptr, 0);
            if (size == 0) {
                return L"";
            }

            std::wstring result(size, L'\0');
            fn(to_handle(), result.data(), size + 1);
            return result;
        }

    private:
        static inline const UEVR_FNameFunctions* s_functions{nullptr};
        static inline const UEVR_FNameFunctions* initialize() {
            if (s_functions == nullptr) {
                s_functions = API::get()->sdk()->fname;
            }

            return s_functions;
        }
    };

    struct UObject {
        inline UEVR_UObjectHandle to_handle() { return (UEVR_UObjectHandle)this; }
        inline UEVR_UObjectHandle to_handle() const { return (UEVR_UObjectHandle)this; }

        static UClass* static_class() {
            static auto result = API::get()->find_uobject<UClass>(L"Class /Script/CoreUObject.Object");
            return result;
        }

        inline UClass* get_class() const {
            static const auto fn = initialize()->get_class;
            return (UClass*)fn(to_handle());
        }

        inline UObject* get_outer() const {
            static const auto fn = initialize()->get_outer;
            return (UObject*)fn(to_handle());
        }

        inline bool is_a(UClass* cmp) const {
            static const auto fn = initialize()->is_a;
            return fn(to_handle(), cmp->to_handle());
        }

        template<typename T>
        inline bool is_a() const {
            return is_a(T::static_class());
        }

        void process_event(UFunction* function, void* params) {
            static const auto fn = initialize()->process_event;
            fn(to_handle(), function->to_handle(), params);
        }

        void call_function(std::wstring_view name, void* params) {
            static const auto fn = initialize()->call_function;
            fn(to_handle(), name.data(), params);
        }

        // Pointer that points to the address of the data within the object, not the data itself
        template<typename T>
        T* get_property_data(std::wstring_view name) const {
            static const auto fn = initialize()->get_property_data;
            return (T*)fn(to_handle(), name.data());
        }

        // Pointer that points to the address of the data within the object, not the data itself
        void* get_property_data(std::wstring_view name) const {
            return get_property_data<void*>(name);
        }

        // use this if you know for sure that the property exists
        // or you will cause an access violation
        template<typename T>
        T& get_property(std::wstring_view name) const {
            return *get_property_data<T>(name);
        }

        FName* get_fname() const {
            static const auto fn = initialize()->get_fname;
            return (FName*)fn(to_handle());
        }

        std::wstring get_full_name() const {
            const auto c = get_class();

            if (c == nullptr) {
                return L"";
            }

            auto obj_name = get_fname()->to_string();

            for (auto outer = this->get_outer(); outer != nullptr && outer != this; outer = outer->get_outer()) {
                obj_name = outer->get_fname()->to_string() + L'.' + obj_name;
            }

            return c->get_fname()->to_string() + L' ' + obj_name;
        }

    private:
        static inline const UEVR_UObjectFunctions* s_functions{nullptr};
        inline static const UEVR_UObjectFunctions* initialize() {
            if (s_functions == nullptr) {
                s_functions = API::get()->sdk()->uobject;
            }

            return s_functions;
        }
    };

    struct UStruct : public UObject {
        inline UEVR_UStructHandle to_handle() { return (UEVR_UStructHandle)this; }
        inline UEVR_UStructHandle to_handle() const { return (UEVR_UStructHandle)this; }

        static UClass* static_class() {
            static auto result = API::get()->find_uobject<UClass>(L"Class /Script/CoreUObject.Struct");
            return result;
        }

        UStruct* get_super_struct() const {
            static const auto fn = initialize()->get_super_struct;
            return (UStruct*)fn(to_handle());
        }

        UStruct* get_super() const {
            return get_super_struct();
        }
        
        UFunction* find_function(std::wstring_view name) const {
            static const auto fn = initialize()->find_function;
            return (UFunction*)fn(to_handle(), name.data());
        }

        // Not an array, it's a linked list. Meant to call ->get_next() until nullptr
        FField* get_child_properties() const {
            static const auto fn = initialize()->get_child_properties;
            return (FField*)fn(to_handle());
        }

    private:
        static inline const UEVR_UStructFunctions* s_functions{nullptr};
        inline static const UEVR_UStructFunctions* initialize() {
            if (s_functions == nullptr) {
                s_functions = API::get()->sdk()->ustruct;
            }

            return s_functions;
        }
    };

    struct UClass : public UStruct {
        inline UEVR_UClassHandle to_handle() { return (UEVR_UClassHandle)this; }
        inline UEVR_UClassHandle to_handle() const { return (UEVR_UClassHandle)this; }

        static UClass* static_class() {
            static auto result = API::get()->find_uobject<UClass>(L"Class /Script/CoreUObject.Class");
            return result;
        }

        UObject* get_class_default_object() const {
            static const auto fn = initialize()->get_class_default_object;
            return (UObject*)fn(to_handle());
        }

        std::vector<UObject*> get_objects_matching(bool allow_default = false) const {
            static auto activate_fn = API::get()->sdk()->uobject_hook->activate;
            static auto fn = API::get()->sdk()->uobject_hook->get_objects_by_class;
            activate_fn();
            std::vector<UObject*> result{};
            const auto size = fn(to_handle(), nullptr, 0, allow_default);
            if (size == 0) {
                return result;
            }

            result.resize(size);

            fn(to_handle(), (UEVR_UObjectHandle*)result.data(), size, allow_default);
            return result;
        }

        UObject* get_first_object_matching(bool allow_default = false) const {
            static auto activate_fn = API::get()->sdk()->uobject_hook->activate;
            static auto fn = API::get()->sdk()->uobject_hook->get_first_object_by_class;
            activate_fn();
            return (UObject*)fn(to_handle(), allow_default);
        }

        template<typename T>
        std::vector<T*> get_objects_matching(bool allow_default = false) const {
            std::vector<UObject*> objects = get_objects_matching(allow_default);

            return *reinterpret_cast<std::vector<T*>*>(&objects);
        }

        template<typename T>
        T* get_first_object_matching(bool allow_default = false) const {
            return (T*)get_first_object_matching(allow_default);
        }

    private:
        static inline const UEVR_UClassFunctions* s_functions{nullptr};
        inline static const UEVR_UClassFunctions* initialize() {
            if (s_functions == nullptr) {
                s_functions = API::get()->sdk()->uclass;
            }

            return s_functions;
        }
    };

    struct UFunction : public UStruct {
        inline UEVR_UFunctionHandle to_handle() { return (UEVR_UFunctionHandle)this; }
        inline UEVR_UFunctionHandle to_handle() const { return (UEVR_UFunctionHandle)this; }

        static UClass* static_class() {
            static auto result = API::get()->find_uobject<UClass>(L"Class /Script/CoreUObject.Function");
            return result;
        }

        void call(UObject* obj, void* params) {
            if (obj == nullptr) {
                return;
            }

            obj->process_event(this, params);
        }

        void* get_native_function() const {
            static const auto fn = initialize()->get_native_function;
            return fn(to_handle());
        }

    private:
        static inline const UEVR_UFunctionFunctions* s_functions{nullptr};
        inline static const UEVR_UFunctionFunctions* initialize() {
            if (s_functions == nullptr) {
                s_functions = API::get()->sdk()->ufunction;
            }

            return s_functions;
        }
    };

    // Wrapper class for UField AND FField
    struct FField {
        inline UEVR_FFieldHandle to_handle() { return (UEVR_FFieldHandle)this; }
        inline UEVR_FFieldHandle to_handle() const { return (UEVR_FFieldHandle)this; }

        inline FField* get_next() const {
            static const auto fn = initialize()->get_next;
            return (FField*)fn(to_handle());
        }
        
        FName* get_fname() const {
            static const auto fn = initialize()->get_fname;
            return (FName*)fn(to_handle());
        }

        FFieldClass* get_class() const {
            static const auto fn = initialize()->get_class;
            return (FFieldClass*)fn(to_handle());
        }

    private:
        static inline const UEVR_FFieldFunctions* s_functions{nullptr};
        static inline const UEVR_FFieldFunctions* initialize() {
            if (s_functions == nullptr) {
                s_functions = API::get()->sdk()->ffield;
            }

            return s_functions;
        }
    };

    // Wrapper class for FProperty AND UProperty
    struct FProperty : public FField {
        inline UEVR_FPropertyHandle to_handle() { return (UEVR_FPropertyHandle)this; }
        inline UEVR_FPropertyHandle to_handle() const { return (UEVR_FPropertyHandle)this; }

        int32_t get_offset() const {
            static const auto fn = initialize()->get_offset;
            return fn(to_handle());
        }

    private:
        static inline const UEVR_FPropertyFunctions* s_functions{nullptr};
        static inline const UEVR_FPropertyFunctions* initialize() {
            if (s_functions == nullptr) {
                s_functions = API::get()->sdk()->fproperty;
            }

            return s_functions;
        }
    };

    struct FFieldClass {
        inline UEVR_FFieldClassHandle to_handle() { return (UEVR_FFieldClassHandle)this; }
        inline UEVR_FFieldClassHandle to_handle() const { return (UEVR_FFieldClassHandle)this; }

        FName* get_fname() const {
            static const auto fn = initialize()->get_fname;
            return (FName*)fn(to_handle());
        }

        std::wstring get_name() const {
            return get_fname()->to_string();
        }

    private:
        static inline const UEVR_FFieldClassFunctions* s_functions{nullptr};
        static inline const UEVR_FFieldClassFunctions* initialize() {
            if (s_functions == nullptr) {
                s_functions = API::get()->sdk()->ffield_class;
            }

            return s_functions;
        }
    };

    // TODO
    struct UEngine : public UObject {

    };

    struct UGameEngine : public UEngine {

    };

    struct UWorld : public UObject {

    };

    template <typename T>
    struct TArray {
        T* data;
        int32_t count;
        int32_t capacity;
    };

private:
    const UEVR_PluginInitializeParam* m_param;
    const UEVR_SDKData* m_sdk;
};
}