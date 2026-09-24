//
// Modified by GPT-6 on 2026-09-14, 2026-09-23
//

#include "PaletteWorkspace.hpp"

namespace merutilm::rff2::workspace {
    WorkspaceForm paletteForm(AttributeGetter attribute, std::function<void()> changed) {
        auto paletteWorkspace = std::make_shared<PaletteWorkspace>(std::move(attribute), std::move(changed));
        return paletteWorkspace->form();
    }
}
