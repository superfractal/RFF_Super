//
// Modified by GPT-6 on 2026-09-13, 2026-09-14, 2026-09-15, 2026-09-19, 2026-09-23
//

#pragma once
#include "../UiLanguage.hpp"

namespace merutilm::rff2::workspace {
    enum class TextKey {
        Explore, Appearance, Animation, Export, Undo, Redo, Save, Effects, BaseStyle, EditPalette,
        SearchResults, NoResults, SearchPlaceholder, Color, Reflection, Film, Contour, Emission,
        Flame, Print, Noise, Relief, Mix, SearchShortcut, ClearSearch
    };
    inline const wchar_t* uiText(TextKey key) {
        switch(key) {
            case TextKey::Explore: return UiLanguage::label(L"Explore");
            case TextKey::Appearance: return UiLanguage::label(L"Appearance");
            case TextKey::Animation: return UiLanguage::label(L"Animation");
            case TextKey::Export: return UiLanguage::label(L"Export");
            case TextKey::Undo: return UiLanguage::label(L"Undo");
            case TextKey::Redo: return UiLanguage::label(L"Redo");
            case TextKey::Save: return UiLanguage::label(L"Save");
            case TextKey::Effects: return UiLanguage::label(L"Effects");
            case TextKey::BaseStyle: return UiLanguage::label(L"Base Style");
            case TextKey::EditPalette: return UiLanguage::label(L"Edit Palette →");
            case TextKey::SearchResults: return UiLanguage::label(L"Search Results");
            case TextKey::NoResults: return UiLanguage::label(L"No matching settings");
            case TextKey::SearchPlaceholder: return UiLanguage::label(L"Search settings…");
            case TextKey::SearchShortcut: return UiLanguage::label(L"Ctrl+F");
            case TextKey::ClearSearch: return UiLanguage::label(L"Clear search");
            case TextKey::Color: return UiLanguage::label(L"Color & Palette");
            case TextKey::Reflection: return UiLanguage::label(L"Material & Reflection");
            case TextKey::Film: return UiLanguage::label(L"Thin Film & Glints");
            case TextKey::Contour: return UiLanguage::label(L"Band Line");
            case TextKey::Emission: return UiLanguage::label(L"Emission & Detail");
            case TextKey::Flame: return UiLanguage::label(L"Flames & Frost");
            case TextKey::Print: return UiLanguage::label(L"Ink & Quantization");
            case TextKey::Noise: return UiLanguage::label(L"Noise & VHS");
            case TextKey::Relief: return UiLanguage::label(L"Relief & Lighting");
            case TextKey::Mix: return UiLanguage::label(L"Effect Amounts");
        }
        return UiLanguage::label(L"");
    }
}
