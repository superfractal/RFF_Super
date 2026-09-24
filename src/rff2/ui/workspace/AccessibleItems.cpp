//
// Modified by GPT-6 on 2026-09-14, 2026-09-15, 2026-09-21
//

#include "../UiLanguage.hpp"
#include "AccessibleItems.hpp"
#include <algorithm>
#include <unordered_map>
#include <oleidl.h>

namespace merutilm::rff2::workspace {
    struct AccessibleItems::State {
        HWND window;
        std::function<std::wstring()> name;
        std::function<std::vector<AccessibleItem>()> items;
        std::function<bool(long, bool)> operate;
        std::function<bool(long, std::wstring_view)> write;
        bool alive() const {
            return window && IsWindow(window);
        }
        std::vector<AccessibleItem> read() const {
            return alive() ? items() : std::vector<AccessibleItem>{};
        }
    };

    struct AccessibleItems::Provider final : IAccessible, IEnumVARIANT, IOleWindow {
        LONG referenceCount = 1;
        std::shared_ptr<State> state;
        size_t enumerationCursor = 0;
        explicit Provider(std::shared_ptr<State> value) : state(std::move(value)) {}
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **result) override {
            if (!result) {
                return E_POINTER;
            }
            *result = nullptr;
            if (iid == IID_IUnknown || iid == IID_IDispatch || iid == IID_IAccessible) {
                *result = static_cast<IAccessible *>(this);
            } else if (iid == IID_IEnumVARIANT) {
                *result = static_cast<IEnumVARIANT *>(this);
            } else if (iid == IID_IOleWindow) {
                *result = static_cast<IOleWindow *>(this);
            } else {
                return E_NOINTERFACE;
            }
            AddRef();
            return S_OK;
        }
        ULONG STDMETHODCALLTYPE AddRef() override {
            return InterlockedIncrement(&referenceCount);
        }
        ULONG STDMETHODCALLTYPE Release() override {
            const auto result = InterlockedDecrement(&referenceCount);
            if (!result) {
                delete this;
            }
            return result;
        }
        HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *count) override {
            if (!count) {
                return E_POINTER;
            }
            *count = 0;
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT, LCID, ITypeInfo **info) override {
            if (!info) {
                return E_POINTER;
            }
            *info = nullptr;
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID, LPOLESTR *, UINT count, LCID, DISPID *ids) override {
            if (!ids) {
                return E_POINTER;
            }
            std::fill_n(ids, count, DISPID_UNKNOWN);
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE Invoke(DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *result,
                                         EXCEPINFO *error, UINT *argument) override {
            if (result) {
                VariantInit(result);
            }
            if (error) {
                *error = {};
            }
            if (argument) {
                *argument = 0;
            }
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE GetWindow(HWND *window) override {
            if (!window) {
                return E_POINTER;
            }
            *window = state->window;
            return state->alive() ? S_OK : CO_E_OBJNOTCONNECTED;
        }
        HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL) override {
            return E_NOTIMPL;
        }
        static HRESULT string(BSTR *out, const std::wstring &value) {
            if (!out) {
                return E_POINTER;
            }
            *out = SysAllocStringLen(value.data(), UINT(value.size()));
            return *out ? S_OK : E_OUTOFMEMORY;
        }
        HRESULT item(VARIANT child, AccessibleItem &result) const {
            if (!state->alive()) {
                return CO_E_OBJNOTCONNECTED;
            }
            if (child.vt != VT_I4 || child.lVal < 0) {
                return E_INVALIDARG;
            }
            if (child.lVal == CHILDID_SELF) {
                result.name = state->name();
                result.role = ROLE_SYSTEM_PANE;
                result.state = 0;
                GetClientRect(state->window, &result.bounds);
                if (!IsWindowVisible(state->window)) {
                    result.state |= STATE_SYSTEM_INVISIBLE;
                }
                return S_OK;
            }
            const auto list = state->read();
            const auto found = std::find_if(list.begin(), list.end(),
                                            [&](const auto &value) { return value.id == child.lVal; });
            if (found == list.end()) {
                return E_INVALIDARG;
            }
            result = *found;
            return S_OK;
        }
        static HRESULT variant(VARIANT *out, const AccessibleItem &item) {
            if (!out) {
                return E_POINTER;
            }
            VariantInit(out);
            if (item.native) {
                IAccessible *native = nullptr;
                const auto hr = AccessibleObjectFromWindow(item.native, OBJID_CLIENT, IID_IAccessible,
                                                           reinterpret_cast<void **>(&native));
                if (FAILED(hr)) {
                    return hr;
                }
                out->vt = VT_DISPATCH;
                out->pdispVal = native;
            } else {
                out->vt = VT_I4;
                out->lVal = item.id;
            }
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_accParent(IDispatch **parent) override {
            if (!parent) {
                return E_POINTER;
            }
            *parent = nullptr;
            if (!state->alive()) {
                return CO_E_OBJNOTCONNECTED;
            }
            return AccessibleObjectFromWindow(state->window, OBJID_WINDOW, IID_IDispatch,
                                              reinterpret_cast<void **>(parent));
        }
        HRESULT STDMETHODCALLTYPE get_accChildCount(long *count) override {
            if (!count) {
                return E_POINTER;
            }
            *count = long(state->read().size());
            return state->alive() ? S_OK : CO_E_OBJNOTCONNECTED;
        }
        HRESULT STDMETHODCALLTYPE get_accChild(VARIANT child, IDispatch **out) override {
            if (!out) {
                return E_POINTER;
            }
            *out = nullptr;
            AccessibleItem value;
            const auto hr = item(child, value);
            if (FAILED(hr)) {
                return hr;
            }
            return value.native ? AccessibleObjectFromWindow(value.native, OBJID_CLIENT, IID_IDispatch,
                                                             reinterpret_cast<void **>(out))
                                : S_FALSE;
        }
        HRESULT STDMETHODCALLTYPE get_accName(VARIANT child, BSTR *out) override {
            AccessibleItem value;
            const auto hr = item(child, value);
            return FAILED(hr) ? hr : string(out, UiLanguage::text(value.name));
        }
        HRESULT STDMETHODCALLTYPE get_accDescription(VARIANT child, BSTR *out) override {
            AccessibleItem value;
            const auto hr = item(child, value);
            return FAILED(hr) ? hr : string(out, UiLanguage::text(value.help));
        }
        HRESULT STDMETHODCALLTYPE get_accHelp(VARIANT child, BSTR *out) override {
            return get_accDescription(child, out);
        }
        HRESULT STDMETHODCALLTYPE get_accValue(VARIANT child, BSTR *out) override {
            if (!out) {
                return E_POINTER;
            }
            *out = nullptr;
            AccessibleItem value;
            const auto hr = item(child, value);
            if (FAILED(hr)) {
                return hr;
            }
            if (value.native) {
                IAccessible *native = nullptr;
                const auto nativeHr = AccessibleObjectFromWindow(value.native, OBJID_CLIENT, IID_IAccessible,
                                                                 reinterpret_cast<void **>(&native));
                if (FAILED(nativeHr)) {
                    return nativeHr;
                }
                VARIANT self{};
                self.vt = VT_I4;
                const auto result = native->get_accValue(self, out);
                native->Release();
                return result;
            }
            return value.value.empty() ? S_FALSE : string(out, value.value);
        }
        HRESULT STDMETHODCALLTYPE get_accRole(VARIANT child, VARIANT *out) override {
            if (!out) {
                return E_POINTER;
            }
            VariantInit(out);
            AccessibleItem value;
            const auto hr = item(child, value);
            if (FAILED(hr)) {
                return hr;
            }
            out->vt = VT_I4;
            out->lVal = value.role;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_accState(VARIANT child, VARIANT *out) override {
            if (!out) {
                return E_POINTER;
            }
            VariantInit(out);
            AccessibleItem value;
            const auto hr = item(child, value);
            if (FAILED(hr)) {
                return hr;
            }
            out->vt = VT_I4;
            out->lVal = value.state;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_accHelpTopic(BSTR *file, VARIANT, long *topic) override {
            if (!file || !topic) {
                return E_POINTER;
            }
            *file = nullptr;
            *topic = 0;
            return S_FALSE;
        }
        HRESULT STDMETHODCALLTYPE get_accKeyboardShortcut(VARIANT, BSTR *out) override {
            if (!out) {
                return E_POINTER;
            }
            *out = nullptr;
            return S_FALSE;
        }
        HRESULT STDMETHODCALLTYPE get_accFocus(VARIANT *out) override {
            if (!out) {
                return E_POINTER;
            }
            VariantInit(out);
            if (!state->alive()) {
                return CO_E_OBJNOTCONNECTED;
            }
            for (const auto &value : state->read()) {
                if (value.state & STATE_SYSTEM_FOCUSED) {
                    return variant(out, value);
                }
            }
            return S_FALSE;
        }
        HRESULT STDMETHODCALLTYPE get_accSelection(VARIANT *out) override {
            if (!out) {
                return E_POINTER;
            }
            VariantInit(out);
            if (!state->alive()) {
                return CO_E_OBJNOTCONNECTED;
            }
            for (const auto &value : state->read()) {
                if (value.state & STATE_SYSTEM_SELECTED) {
                    return variant(out, value);
                }
            }
            return S_FALSE;
        }
        HRESULT STDMETHODCALLTYPE get_accDefaultAction(VARIANT child, BSTR *out) override {
            AccessibleItem value;
            const auto hr = item(child, value);
            return FAILED(hr) ? hr : string(out, UiLanguage::text(value.action));
        }
        HRESULT STDMETHODCALLTYPE accSelect(long flags, VARIANT child) override {
            if (flags != SELFLAG_TAKEFOCUS) {
                return E_INVALIDARG;
            }
            AccessibleItem value;
            const auto hr = item(child, value);
            if (FAILED(hr)) {
                return hr;
            }
            return value.id && !(value.state & STATE_SYSTEM_UNAVAILABLE) && state->operate(value.id, false)
                       ? S_OK
                       : S_FALSE;
        }
        HRESULT STDMETHODCALLTYPE accDoDefaultAction(VARIANT child) override {
            AccessibleItem value;
            const auto hr = item(child, value);
            if (FAILED(hr)) {
                return hr;
            }
            return value.id && !(value.state & STATE_SYSTEM_UNAVAILABLE) && state->operate(value.id, true)
                       ? S_OK
                       : S_FALSE;
        }
        HRESULT STDMETHODCALLTYPE accLocation(long *x, long *y, long *width, long *height,
                                              VARIANT child) override {
            if (!x || !y || !width || !height) {
                return E_POINTER;
            }
            *x = *y = *width = *height = 0;
            AccessibleItem value;
            const auto hr = item(child, value);
            if (FAILED(hr)) {
                return hr;
            }
            MapWindowPoints(state->window, nullptr, reinterpret_cast<POINT *>(&value.bounds), 2);
            *x = value.bounds.left;
            *y = value.bounds.top;
            *width = value.bounds.right - value.bounds.left;
            *height = value.bounds.bottom - value.bounds.top;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE accNavigate(long direction, VARIANT start, VARIANT *out) override {
            if (!out) {
                return E_POINTER;
            }
            VariantInit(out);
            AccessibleItem source;
            const auto hr = item(start, source);
            if (FAILED(hr)) {
                return hr;
            }
            const auto list = state->read();
            if (list.empty()) {
                return S_FALSE;
            }
            if (!source.id && (direction == NAVDIR_FIRSTCHILD || direction == NAVDIR_LASTCHILD)) {
                return variant(out, direction == NAVDIR_FIRSTCHILD ? list.front() : list.back());
            }
            const auto found = std::find_if(list.begin(), list.end(),
                                            [&](const auto &value) { return value.id == source.id; });
            if (found == list.end()) {
                return S_FALSE;
            }
            int step;
            switch (direction) {
            case NAVDIR_NEXT:
            case NAVDIR_DOWN:
            case NAVDIR_RIGHT:
                step = 1;
                break;
            case NAVDIR_PREVIOUS:
            case NAVDIR_UP:
            case NAVDIR_LEFT:
                step = -1;
                break;
            default:
                return E_INVALIDARG;
            }
            const auto next = int(found - list.begin()) + step;
            return next >= 0 && next < int(list.size()) ? variant(out, list[next]) : S_FALSE;
        }
        HRESULT STDMETHODCALLTYPE accHitTest(long x, long y, VARIANT *out) override {
            if (!out) {
                return E_POINTER;
            }
            VariantInit(out);
            if (!state->alive()) {
                return CO_E_OBJNOTCONNECTED;
            }
            POINT point{x, y};
            ScreenToClient(state->window, &point);
            RECT client;
            GetClientRect(state->window, &client);
            if (!PtInRect(&client, point)) {
                return S_FALSE;
            }
            for (const auto &value : state->read()) {
                if (!(value.state & (STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_OFFSCREEN)) &&
                    PtInRect(&value.bounds, point)) {
                    return variant(out, value);
                }
            }
            out->vt = VT_I4;
            out->lVal = CHILDID_SELF;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE put_accName(VARIANT, BSTR) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE put_accValue(VARIANT child, BSTR text) override {
            AccessibleItem value;
            const auto hr = item(child, value);
            if (FAILED(hr)) {
                return hr;
            }
            if (!value.writable || !state->write) {
                return E_NOTIMPL;
            }
            if (value.state & (STATE_SYSTEM_UNAVAILABLE | STATE_SYSTEM_READONLY)) {
                return E_ACCESSDENIED;
            }
            return state->write(value.id,
                                text ? std::wstring_view(text, SysStringLen(text)) : std::wstring_view{})
                       ? S_OK
                       : E_INVALIDARG;
        }
        HRESULT STDMETHODCALLTYPE Next(ULONG count, VARIANT *out, ULONG *fetched) override {
            if (!out || (!fetched && count != 1)) {
                return E_POINTER;
            }
            if (fetched) {
                *fetched = 0;
            }
            if (!state->alive()) {
                return CO_E_OBJNOTCONNECTED;
            }
            const auto list = state->read();
            ULONG filled = 0;
            while (filled < count && enumerationCursor < list.size()) {
                const auto hr = variant(out + filled, list[enumerationCursor++]);
                if (FAILED(hr)) {
                    for (ULONG i = 0; i < filled; ++i) {
                        VariantClear(out + i);
                    }
                    return hr;
                }
                ++filled;
            }
            if (fetched) {
                *fetched = filled;
            }
            return filled == count ? S_OK : S_FALSE;
        }
        HRESULT STDMETHODCALLTYPE Skip(ULONG count) override {
            const auto size = state->read().size();
            const auto remaining = size - std::min(enumerationCursor, size);
            enumerationCursor += std::min<size_t>(count, remaining);
            return count <= remaining ? S_OK : S_FALSE;
        }
        HRESULT STDMETHODCALLTYPE Reset() override {
            enumerationCursor = 0;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE Clone(IEnumVARIANT **out) override {
            if (!out) {
                return E_POINTER;
            }
            auto *copy = new Provider(state);
            copy->enumerationCursor = enumerationCursor;
            *out = static_cast<IEnumVARIANT *>(copy);
            return S_OK;
        }
    };

    AccessibleItems::AccessibleItems(HWND window, std::function<std::wstring()> name,
                                     std::function<std::vector<AccessibleItem>()> items,
                                     std::function<bool(long, bool)> operate,
                                     std::function<bool(long, std::wstring_view)> write)
        : state(std::make_shared<State>(
              State{window, std::move(name), std::move(items), std::move(operate), std::move(write)})),
          provider(new Provider(state)) {}
    AccessibleItems::~AccessibleItems() {
        state->window = nullptr;
        state->name = {};
        state->items = {};
        state->operate = {};
        state->write = {};
        provider->Release();
    }
    LRESULT AccessibleItems::object(WPARAM flags) const {
        return LresultFromObject(IID_IAccessible, flags, static_cast<IAccessible *>(provider));
    }
    void AccessibleItems::changed() {
        if (!state->alive()) {
            return;
        }
        const auto name = state->name();
        if (name != previousName) {
            NotifyWinEvent(EVENT_OBJECT_NAMECHANGE, state->window, OBJID_CLIENT, CHILDID_SELF);
            previousName = name;
        }
        auto current = state->read();
        const bool reordered = current.size() != previous.size() ||
                               !std::equal(current.begin(), current.end(), previous.begin(), previous.end(),
                                           [](const auto &a, const auto &b) { return a.id == b.id; });
        if (reordered) {
            NotifyWinEvent(EVENT_OBJECT_REORDER, state->window, OBJID_CLIENT, CHILDID_SELF);
        }
        std::unordered_map<long, const AccessibleItem *> oldItems;
        oldItems.reserve(previous.size());
        for (const auto &item : previous) {
            oldItems.emplace(item.id, &item);
        }
        for (const auto &item : current) {
            const auto found = oldItems.find(item.id);
            const auto *old = found == oldItems.end() ? nullptr : found->second;
            if (!old || old->state != item.state) {
                NotifyWinEvent(EVENT_OBJECT_STATECHANGE, state->window, OBJID_CLIENT, item.id);
                if (item.state & STATE_SYSTEM_FOCUSED) {
                    NotifyWinEvent(EVENT_OBJECT_FOCUS, state->window, OBJID_CLIENT, item.id);
                }
            }
            if (!old) {
                continue;
            }
            if (old->name != item.name) {
                NotifyWinEvent(EVENT_OBJECT_NAMECHANGE, state->window, OBJID_CLIENT, item.id);
            }
            if (old->help != item.help) {
                NotifyWinEvent(EVENT_OBJECT_DESCRIPTIONCHANGE, state->window, OBJID_CLIENT, item.id);
            }
            if (old->value != item.value) {
                NotifyWinEvent(EVENT_OBJECT_VALUECHANGE, state->window, OBJID_CLIENT, item.id);
            }
            if (!EqualRect(&old->bounds, &item.bounds)) {
                NotifyWinEvent(EVENT_OBJECT_LOCATIONCHANGE, state->window, OBJID_CLIENT, item.id);
            }
        }
        previous = std::move(current);
    }
} // namespace merutilm::rff2::workspace
