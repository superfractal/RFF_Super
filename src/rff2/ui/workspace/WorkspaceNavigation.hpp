//
// Modified by GPT-6 on 2026-09-14, 2026-09-23
//

#pragma once
#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>

namespace merutilm::rff2::workspace {
    class WorkspaceNavigation {
    public:
        struct Module {
            int id;
            int tab;
            std::wstring title;
        };

    private:
        std::vector<Module> entries{
            {0, 0, L"Location & Calculation"},
            {1, 1, L"Surface Effects"},
            {2, 2, L"Animation"},
            {3, 3, L"Export"},
        };
        std::array<int, 4> recent{0, 1, 2, 3};

    public:
        int tab(int id) const {
            const auto found = std::find_if(entries.begin(), entries.end(),
                                            [id](const Module &module) { return module.id == id; });
            return found == entries.end() ? 1 : found->tab;
        }

        void add(int id, int tab, std::wstring title) {
            if (tab < 0 || tab >= 4) {
                return;
            }
            const bool alreadyPresent = std::any_of(entries.begin(), entries.end(),
                                                    [id](const Module &module) { return module.id == id; });
            if (!alreadyPresent) {
                entries.push_back({id, tab, std::move(title)});
            }
        }

        std::vector<Module> modules(int tab) const {
            std::vector<Module> result;
            for (const auto &module : entries) {
                if (module.tab == tab) {
                    result.push_back(module);
                }
            }
            return result;
        }

        void selected(int id) {
            recent[tab(id)] = id;
        }

        int last(int tab) const {
            return recent.at(tab);
        }
    };
}
