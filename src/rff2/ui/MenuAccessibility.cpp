//
// Modified by GPT-6 on 2026-09-17, 2026-09-22
//

#include "MenuAccessibility.hpp"
#include <uiautomation.h>
#include <atomic>
#include <mutex>
#include <algorithm>

namespace merutilm::rff2 {
    struct MenuAccessibility::State {
        std::mutex mutex;
        HWND window = nullptr;
        ULONG generation = 0;
        std::vector<MenuAccessibleNode> nodes;
    };

    class MenuAccessibility::Provider final : public IRawElementProviderSimple,
                                              public IRawElementProviderFragment,
                                              public IRawElementProviderFragmentRoot,
                                              public IInvokeProvider,
                                              public IToggleProvider,
                                              public IExpandCollapseProvider {
        std::atomic<ULONG> refs{1};
        std::shared_ptr<State> state;
        int id;
        bool read(MenuAccessibleNode &value) const {
            std::lock_guard lock(state->mutex);
            if (!state->window) {
                return false;
            }
            const auto it = std::find_if(state->nodes.begin(), state->nodes.end(),
                                         [&](const auto &n) { return n.id == id; });
            if (it == state->nodes.end()) {
                return false;
            }
            value = *it;
            return true;
        }
        HRESULT action(Action action) {
            std::lock_guard lock(state->mutex);
            const auto it = std::find_if(state->nodes.begin(), state->nodes.end(),
                                         [&](const auto &n) { return n.id == id; });
            if (!state->window || it == state->nodes.end()) {
                return UIA_E_ELEMENTNOTAVAILABLE;
            }
            if (!it->enabled) {
                return UIA_E_ELEMENTNOTENABLED;
            }
            return PostMessageW(state->window, ActionMessage, MAKEWPARAM(id, action), state->generation)
                       ? S_OK
                       : UIA_E_ELEMENTNOTAVAILABLE;
        }
        Provider *make(int node) {
            return new Provider(state, node);
        }

      public:
        Provider(std::shared_ptr<State> state, int id) : state(std::move(state)), id(id) {}
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **out) override {
            if (!out) {
                return E_POINTER;
            }
            *out = nullptr;
            if (iid == IID_IUnknown || iid == __uuidof(IRawElementProviderSimple)) {
                *out = static_cast<IRawElementProviderSimple *>(this);
            } else if (iid == __uuidof(IRawElementProviderFragment)) {
                *out = static_cast<IRawElementProviderFragment *>(this);
            } else if (iid == __uuidof(IRawElementProviderFragmentRoot) && id == 0) {
                *out = static_cast<IRawElementProviderFragmentRoot *>(this);
            } else if (iid == __uuidof(IInvokeProvider)) {
                *out = static_cast<IInvokeProvider *>(this);
            } else if (iid == __uuidof(IToggleProvider)) {
                *out = static_cast<IToggleProvider *>(this);
            } else if (iid == __uuidof(IExpandCollapseProvider)) {
                *out = static_cast<IExpandCollapseProvider *>(this);
            } else {
                return E_NOINTERFACE;
            }
            AddRef();
            return S_OK;
        }
        ULONG STDMETHODCALLTYPE AddRef() override {
            return ++refs;
        }
        ULONG STDMETHODCALLTYPE Release() override {
            const ULONG value = --refs;
            if (!value) {
                delete this;
            }
            return value;
        }
        HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions *out) override {
            if (!out) {
                return E_POINTER;
            }
            *out = ProviderOptions_ServerSideProvider;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID pattern, IUnknown **out) override {
            if (!out) {
                return E_POINTER;
            }
            *out = nullptr;
            MenuAccessibleNode n;
            if (!read(n)) {
                return UIA_E_ELEMENTNOTAVAILABLE;
            }
            if (pattern == UIA_ExpandCollapsePatternId && n.expandable) {
                *out = static_cast<IExpandCollapseProvider *>(this);
            } else if (pattern == UIA_TogglePatternId && n.checkable) {
                *out = static_cast<IToggleProvider *>(this);
            } else if (pattern == UIA_InvokePatternId && !n.expandable && !n.checkable && !n.menu && !n.bar) {
                *out = static_cast<IInvokeProvider *>(this);
            }
            if (*out) {
                AddRef();
            }
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID property, VARIANT *out) override {
            if (!out) {
                return E_POINTER;
            }
            VariantInit(out);
            MenuAccessibleNode n;
            if (!read(n)) {
                return UIA_E_ELEMENTNOTAVAILABLE;
            }
            const auto boolean = [&](bool value) {
                out->vt = VT_BOOL;
                out->boolVal = value ? VARIANT_TRUE : VARIANT_FALSE;
            };
            const auto text = [&](const std::wstring &value) {
                out->vt = VT_BSTR;
                out->bstrVal = SysAllocString(value.c_str());
            };
            switch (property) {
            case UIA_ControlTypePropertyId:
                out->vt = VT_I4;
                out->lVal = n.bar    ? UIA_MenuBarControlTypeId
                            : n.menu ? UIA_MenuControlTypeId
                                     : UIA_MenuItemControlTypeId;
                break;
            case UIA_NamePropertyId:
                text(n.name);
                break;
            case UIA_AutomationIdPropertyId:
                text(L"RFF.Menu." + std::to_wstring(id));
                break;
            case UIA_AccessKeyPropertyId:
                text(n.accessKey);
                break;
            case UIA_AcceleratorKeyPropertyId:
                text(n.shortcut);
                break;
            case UIA_FrameworkIdPropertyId:
                text(L"Win32");
                break;
            case UIA_IsEnabledPropertyId:
                boolean(n.enabled);
                break;
            case UIA_HasKeyboardFocusPropertyId:
                boolean(n.focused);
                break;
            case UIA_IsKeyboardFocusablePropertyId:
                boolean(!n.menu && !n.bar);
                break;
            case UIA_IsControlElementPropertyId:
            case UIA_IsContentElementPropertyId:
                boolean(true);
                break;
            case UIA_IsOffscreenPropertyId:
                boolean(IsRectEmpty(&n.bounds));
                break;
            }
            return out->vt == VT_BSTR && !out->bstrVal ? E_OUTOFMEMORY : S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple **out) override {
            if (!out) {
                return E_POINTER;
            }
            *out = nullptr;
            std::lock_guard lock(state->mutex);
            if (!state->window) {
                return UIA_E_ELEMENTNOTAVAILABLE;
            }
            return id == 0 ? UiaHostProviderFromHwnd(state->window, out) : S_OK;
        }
        HRESULT STDMETHODCALLTYPE Navigate(NavigateDirection direction,
                                           IRawElementProviderFragment **out) override {
            if (!out) {
                return E_POINTER;
            }
            *out = nullptr;
            std::lock_guard lock(state->mutex);
            if (!state->window) {
                return UIA_E_ELEMENTNOTAVAILABLE;
            }
            const auto find = [&](int key) {
                return std::find_if(state->nodes.begin(), state->nodes.end(),
                                    [&](const auto &n) { return n.id == key; });
            };
            const auto it = find(id);
            if (it == state->nodes.end()) {
                return UIA_E_ELEMENTNOTAVAILABLE;
            }
            int target = -1;
            if (direction == NavigateDirection_Parent) {
                target = it->parent;
            } else if (direction == NavigateDirection_FirstChild && !it->children.empty()) {
                target = it->children.front();
            } else if (direction == NavigateDirection_LastChild && !it->children.empty()) {
                target = it->children.back();
            } else if (direction == NavigateDirection_NextSibling ||
                       direction == NavigateDirection_PreviousSibling) {
                const auto parent = find(it->parent);
                if (parent != state->nodes.end()) {
                    const auto pos = std::find(parent->children.begin(), parent->children.end(), id);
                    const int index = int(pos - parent->children.begin()) +
                                      (direction == NavigateDirection_NextSibling ? 1 : -1);
                    if (index >= 0 && index < int(parent->children.size())) {
                        target = parent->children[index];
                    }
                }
            }
            if (target >= 0) {
                *out = static_cast<IRawElementProviderFragment *>(make(target));
            }
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE GetRuntimeId(SAFEARRAY **out) override {
            if (!out) {
                return E_POINTER;
            }
            *out = SafeArrayCreateVector(VT_I4, 0, 2);
            if (!*out) {
                return E_OUTOFMEMORY;
            }
            LONG index = 0, value = UiaAppendRuntimeId;
            SafeArrayPutElement(*out, &index, &value);
            index = 1;
            value = id;
            SafeArrayPutElement(*out, &index, &value);
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_BoundingRectangle(UiaRect *out) override {
            if (!out) {
                return E_POINTER;
            }
            MenuAccessibleNode n;
            if (!read(n)) {
                return UIA_E_ELEMENTNOTAVAILABLE;
            }
            *out = {double(n.bounds.left), double(n.bounds.top), double(n.bounds.right - n.bounds.left),
                    double(n.bounds.bottom - n.bounds.top)};
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(SAFEARRAY **out) override {
            if (!out) {
                return E_POINTER;
            }
            *out = nullptr;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE SetFocus() override {
            return action(Focus);
        }
        HRESULT STDMETHODCALLTYPE get_FragmentRoot(IRawElementProviderFragmentRoot **out) override {
            if (!out) {
                return E_POINTER;
            }
            *out = static_cast<IRawElementProviderFragmentRoot *>(make(0));
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(double x, double y,
                                                           IRawElementProviderFragment **out) override {
            if (!out) {
                return E_POINTER;
            }
            *out = nullptr;
            std::lock_guard lock(state->mutex);
            if (!state->window) {
                return UIA_E_ELEMENTNOTAVAILABLE;
            }
            const POINT point{LONG(x), LONG(y)};
            for (auto it = state->nodes.rbegin(); it != state->nodes.rend(); ++it) {
                if (PtInRect(&it->bounds, point)) {
                    *out = static_cast<IRawElementProviderFragment *>(make(it->id));
                    break;
                }
            }
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE GetFocus(IRawElementProviderFragment **out) override {
            if (!out) {
                return E_POINTER;
            }
            *out = nullptr;
            std::lock_guard lock(state->mutex);
            if (!state->window) {
                return UIA_E_ELEMENTNOTAVAILABLE;
            }
            for (const auto &n : state->nodes) {
                if (n.focused) {
                    *out = static_cast<IRawElementProviderFragment *>(make(n.id));
                    break;
                }
            }
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE Invoke() override {
            return action(MenuAccessibility::Invoke);
        }
        HRESULT STDMETHODCALLTYPE Toggle() override {
            return action(MenuAccessibility::Invoke);
        }
        HRESULT STDMETHODCALLTYPE get_ToggleState(ToggleState *out) override {
            if (!out) {
                return E_POINTER;
            }
            MenuAccessibleNode n;
            if (!read(n)) {
                return UIA_E_ELEMENTNOTAVAILABLE;
            }
            *out = n.checked ? ToggleState_On : ToggleState_Off;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE Expand() override {
            return action(MenuAccessibility::Expand);
        }
        HRESULT STDMETHODCALLTYPE Collapse() override {
            return action(MenuAccessibility::Collapse);
        }
        HRESULT STDMETHODCALLTYPE get_ExpandCollapseState(ExpandCollapseState *out) override {
            if (!out) {
                return E_POINTER;
            }
            MenuAccessibleNode n;
            if (!read(n)) {
                return UIA_E_ELEMENTNOTAVAILABLE;
            }
            *out = !n.expandable ? ExpandCollapseState_LeafNode
                   : n.expanded  ? ExpandCollapseState_Expanded
                                 : ExpandCollapseState_Collapsed;
            return S_OK;
        }
    };

    MenuAccessibility::MenuAccessibility(HWND window) : state(std::make_shared<State>()) {
        state->window = window;
    }
    MenuAccessibility::~MenuAccessibility() {
        std::lock_guard lock(state->mutex);
        state->window = nullptr;
        state->nodes.clear();
    }
    LRESULT MenuAccessibility::object(WPARAM flags, LPARAM objectId) {
        if (objectId != UiaRootObjectId) {
            return 0;
        }
        HWND window;
        {
            std::lock_guard lock(state->mutex);
            window = state->window;
        }
        auto *provider = new Provider(state, 0);
        const auto result = UiaReturnRawElementProvider(window, flags, objectId,
                                                        static_cast<IRawElementProviderSimple *>(provider));
        provider->Release();
        return result;
    }
    void MenuAccessibility::update(std::vector<MenuAccessibleNode> nodes, ULONG generation) {
        std::vector<MenuAccessibleNode> previous;
        {
            std::lock_guard lock(state->mutex);
            previous = state->nodes;
        }
        for (const auto &old : previous) {
            if (old.menu &&
                std::none_of(nodes.begin(), nodes.end(), [&](const auto &n) { return n.id == old.id; })) {
                auto *p = new Provider(state, old.id);
                UiaRaiseAutomationEvent(p, UIA_MenuClosedEventId);
                p->Release();
            }
        }
        {
            std::lock_guard lock(state->mutex);
            state->nodes = nodes;
            state->generation = generation;
        }
        for (const auto &node : nodes) {
            const auto old = std::find_if(previous.begin(), previous.end(),
                                          [&](const auto &n) { return n.id == node.id; });
            auto *p = new Provider(state, node.id);
            if (node.menu && old == previous.end()) {
                UiaRaiseAutomationEvent(p, UIA_MenuOpenedEventId);
            }
            if (node.focused && (old == previous.end() || !old->focused)) {
                UiaRaiseAutomationEvent(p, UIA_AutomationFocusChangedEventId);
            }
            if (old != previous.end()) {
                const auto changed = [&](PROPERTYID property, int a, int b, bool boolean) {
                    if (a == b) {
                        return;
                    }
                    VARIANT before{}, after{};
                    before.vt = after.vt = boolean ? VT_BOOL : VT_I4;
                    if (boolean) {
                        before.boolVal = a ? VARIANT_TRUE : VARIANT_FALSE;
                        after.boolVal = b ? VARIANT_TRUE : VARIANT_FALSE;
                    } else {
                        before.lVal = a;
                        after.lVal = b;
                    }
                    UiaRaiseAutomationPropertyChangedEvent(p, property, before, after);
                };
                changed(UIA_IsEnabledPropertyId, old->enabled, node.enabled, true);
                changed(UIA_ExpandCollapseExpandCollapseStatePropertyId, old->expanded, node.expanded, false);
                changed(UIA_ToggleToggleStatePropertyId, old->checked, node.checked, false);
                changed(UIA_IsOffscreenPropertyId, IsRectEmpty(&old->bounds), IsRectEmpty(&node.bounds),
                        true);
                if (!EqualRect(&old->bounds, &node.bounds)) {
                    const auto rectangle = [](RECT rect) {
                        VARIANT value{};
                        value.vt = VT_ARRAY | VT_R8;
                        value.parray = SafeArrayCreateVector(VT_R8, 0, 4);
                        if (value.parray) {
                            double values[] = {double(rect.left), double(rect.top),
                                               double(rect.right - rect.left),
                                               double(rect.bottom - rect.top)};
                            for (LONG i = 0; i < 4; ++i) {
                                SafeArrayPutElement(value.parray, &i, &values[i]);
                            }
                        }
                        return value;
                    };
                    auto before = rectangle(old->bounds), after = rectangle(node.bounds);
                    if (before.parray && after.parray) {
                        UiaRaiseAutomationPropertyChangedEvent(p, UIA_BoundingRectanglePropertyId, before,
                                                               after);
                    }
                    VariantClear(&before);
                    VariantClear(&after);
                }
                if (old->children != node.children) {
                    UiaRaiseStructureChangedEvent(p, StructureChangeType_ChildrenInvalidated, nullptr, 0);
                }
            }
            p->Release();
        }
    }
} // namespace merutilm::rff2
