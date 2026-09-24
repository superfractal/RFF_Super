//
// Modified by GPT-6 on 2026-09-14, 2026-09-15, 2026-09-23
//

#include "../UiLanguage.hpp"
#include "AccessibleControl.hpp"
#include <initguid.h>
#include <oleacc.h>
#include <uiautomation.h>
#include <commctrl.h>
#include <memory>
#include <string>

namespace merutilm::rff2::workspace {
    namespace {
        class AnnotationService {
            HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
            IAccPropServices* service = nullptr;

        public:
            AnnotationService() {
                if (SUCCEEDED(initialized) || initialized == RPC_E_CHANGED_MODE) {
                    CoCreateInstance(CLSID_AccPropServices, nullptr, CLSCTX_INPROC_SERVER,
                                     IID_IAccPropServices, reinterpret_cast<void**>(&service));
                }
            }
            ~AnnotationService() {
                if (service) service->Release();
                if (SUCCEEDED(initialized)) CoUninitialize();
            }
            bool text(HWND window, const GUID& property, const std::wstring& value) const {
                return service && SUCCEEDED(service->SetHwndPropStr(
                    window, OBJID_CLIENT, CHILDID_SELF, property, value.c_str()));
            }
            bool valid(HWND window, bool value) const {
                VARIANT state{};
                state.vt = VT_BOOL;
                state.boolVal = value ? VARIANT_TRUE : VARIANT_FALSE;
                return service && SUCCEEDED(service->SetHwndProp(
                    window, OBJID_CLIENT, CHILDID_SELF, IsDataValidForForm_Property_GUID, state));
            }
            void clear(HWND window) const {
                const MSAAPROPID properties[] = {
                    PROPID_ACC_NAME, PROPID_ACC_DESCRIPTION, PROPID_ACC_HELP,
                    Name_Property_GUID, HelpText_Property_GUID, FullDescription_Property_GUID,
                    AutomationId_Property_GUID, ItemStatus_Property_GUID,
                    IsDataValidForForm_Property_GUID
                };
                if (service) {
                    service->ClearHwndProps(window, OBJID_CLIENT, CHILDID_SELF,
                                            properties, int(std::size(properties)));
                }
            }
        };
        AnnotationService& annotations() {
            static thread_local AnnotationService service;
            return service;
        }

        struct Properties {
            std::wstring name, help, automationId, error;
            bool installed = false;
        };
        constexpr auto propertyName = L"RFF.Workspace.Accessibility";
        LRESULT CALLBACK procedure(HWND window, UINT message, WPARAM w, LPARAM l,
                                   UINT_PTR id, DWORD_PTR data) {
            auto* properties = reinterpret_cast<Properties*>(data);
            if (message == WM_DESTROY) {
                annotations().clear(window);
                properties->installed = false;
            }
            if (message == WM_NCDESTROY) {
                RemoveWindowSubclass(window, procedure, id);
                RemovePropW(window, propertyName);
                delete properties;
            }
            return DefSubclassProc(window, message, w, l);
        }
        Properties* properties(HWND window) {
            return static_cast<Properties*>(GetPropW(window, propertyName));
        }
        bool describeValidation(HWND window, const Properties& properties) {
            auto& service = annotations();
            const auto description = properties.error.empty()
                ? properties.help : UiLanguage::text(L"Error: ") + properties.error;
            bool result = service.text(window, PROPID_ACC_DESCRIPTION, description);
            result = service.text(window, PROPID_ACC_HELP, description) && result;
            result = service.text(window, HelpText_Property_GUID, description) && result;
            result = service.text(window, FullDescription_Property_GUID, description) && result;
            result = service.text(window, ItemStatus_Property_GUID,
                                  properties.error.empty() ? L"" : UiLanguage::label(L"Invalid value")) && result;
            return service.valid(window, properties.error.empty()) && result;
        }
    }

    bool AccessibleControl::describe(HWND window, std::wstring_view name, std::wstring_view help,
                                     std::wstring_view automationId) {
        if (!IsWindow(window)) {
            return false;
        }
        auto* current = properties(window);
        if (!current) {
            auto created = std::make_unique<Properties>();
            if (!SetPropW(window, propertyName, created.get())) {
                return false;
            }
            if (!SetWindowSubclass(window, procedure, 1,
                                   reinterpret_cast<DWORD_PTR>(created.get()))) {
                RemovePropW(window, propertyName);
                return false;
            }
            current = created.release();
        }
        const auto localizedName = UiLanguage::text(name);
        const auto localizedHelp = UiLanguage::text(help);
        const bool nameChanged = current->name != localizedName;
        const bool helpChanged = current->help != localizedHelp;
        const bool idChanged = current->automationId != automationId;
        if (current->installed && !nameChanged && !helpChanged && !idChanged) {
            return true;
        }

        const bool previouslyInstalled = current->installed;
        current->name = localizedName;
        current->help = localizedHelp;
        current->automationId = automationId;
        auto& service = annotations();
        bool result = service.text(window, PROPID_ACC_NAME, current->name);
        result = service.text(window, Name_Property_GUID, current->name) && result;
        result = service.text(window, AutomationId_Property_GUID, current->automationId) && result;
        result = describeValidation(window, *current) && result;
        current->installed = result;
        if (previouslyInstalled && IsWindowVisible(window)) {
            if (nameChanged) {
                NotifyWinEvent(EVENT_OBJECT_NAMECHANGE, window, OBJID_CLIENT, CHILDID_SELF);
            }
            if (helpChanged) {
                NotifyWinEvent(EVENT_OBJECT_DESCRIPTIONCHANGE, window, OBJID_CLIENT, CHILDID_SELF);
            }
        }
        return result;
    }

    bool AccessibleControl::validation(HWND window, std::wstring_view error) {
        auto* current = properties(window);
        if (!current) {
            return false;
        }
        const auto localizedError = UiLanguage::text(error);
        if (current->installed && current->error == localizedError) {
            return true;
        }

        current->error = localizedError;
        const bool result = describeValidation(window, *current);
        if (IsWindowVisible(window)) {
            NotifyWinEvent(EVENT_OBJECT_DESCRIPTIONCHANGE, window, OBJID_CLIENT, CHILDID_SELF);
            NotifyWinEvent(EVENT_OBJECT_STATECHANGE, window, OBJID_CLIENT, CHILDID_SELF);
        }
        return result;
    }
}
