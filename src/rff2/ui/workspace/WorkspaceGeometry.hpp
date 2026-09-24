//
// Modified by GPT-6 on 2026-09-14, 2026-09-17, 2026-09-18, 2026-09-22
//

#pragma once
#include <algorithm>
#include <windows.h>

namespace merutilm::rff2::workspace {
    struct PaneWidths {
        int navigation = 220;
        int inspector = 340;
        static constexpr int minimumNavigation = 180, maximumNavigation = 360;
        static constexpr int minimumInspector = 300, maximumInspector = 640;
        void normalize() {
            navigation = std::clamp(navigation, minimumNavigation, maximumNavigation);
            inspector = std::clamp(inspector, minimumInspector, maximumInspector);
        }
        bool operator==(const PaneWidths &) const = default;
    };

    struct WorkspaceGeometry {
        RECT navigation{}, inspector{}, canvas{}, navigationDivider{}, inspectorDivider{};
        bool compact = false;

        static WorkspaceGeometry arrange(int width, int height, int top, float scale, PaneWidths preferred,
                                         bool inspectorOpen, bool navigationOpen,
                                         bool navigationDockedOpen = true, bool mainLeft = false,
                                         bool navigationRight = false, bool navigationOuter = true) {
            if (navigationRight) {
                auto result = arrange(width, height, top, scale, preferred, inspectorOpen, navigationOpen,
                                      navigationDockedOpen, !mainLeft, false, navigationOuter);
                const auto mirror = [width](RECT &rect) {
                    if (rect.right <= rect.left) {
                        return;
                    }
                    const LONG originalLeft = rect.left;
                    rect.left = std::max(1, width) - rect.right;
                    rect.right = std::max(1, width) - originalLeft;
                };
                mirror(result.navigation);
                mirror(result.inspector);
                mirror(result.canvas);
                mirror(result.navigationDivider);
                mirror(result.inspectorDivider);
                return result;
            }
            const auto px = [scale](int value) { return int(value * scale + .5f); };
            preferred.normalize();
            width = std::max(1, width);
            height = std::max(top + 1, height);
            WorkspaceGeometry result;
            result.compact = width < px(1180);
            const int navigationWidth =
                result.compact || !navigationDockedOpen ? 0 : px(preferred.navigation);
            const int navigationDividerWidth = navigationWidth > 0 ? px(8) : 0;
            const int inspectorDividerWidth = inspectorOpen ? std::min(px(8), width - 1) : 0;
            const int maximumInspectorWidth =
                std::max(px(PaneWidths::minimumInspector),
                         width - navigationWidth - navigationDividerWidth - inspectorDividerWidth - px(320));
            const int inspectorWidth =
                std::min({px(preferred.inspector), maximumInspectorWidth, width - 1 - inspectorDividerWidth});
            result.canvas = {navigationWidth + navigationDividerWidth, top,
                             std::max(navigationWidth + navigationDividerWidth + 1,
                                      width - (inspectorOpen ? inspectorWidth + inspectorDividerWidth : 0)),
                             height};
            result.navigation = {0, top,
                                 result.compact
                                     ? (navigationOpen ? std::min(width, px(preferred.navigation)) : 0)
                                     : navigationWidth,
                                 height};
            result.inspector = {width - inspectorWidth, top, width, height};
            if (result.navigation.right > 0) {
                result.navigationDivider = {result.navigation.right, top,
                                            std::min<LONG>(width, result.navigation.right + px(8)), height};
            }
            if (inspectorOpen && inspectorWidth > 0) {
                result.inspectorDivider = {width - inspectorWidth - inspectorDividerWidth, top,
                                           width - inspectorWidth, height};
            }
            if (mainLeft) {
                result.inspector = {navigationWidth + navigationDividerWidth, top,
                                    navigationWidth + navigationDividerWidth + inspectorWidth, height};
                result.canvas = {navigationWidth + navigationDividerWidth +
                                     (inspectorOpen ? inspectorWidth + inspectorDividerWidth : 0),
                                 top, width, height};
                if (inspectorOpen && inspectorWidth > 0) {
                    result.inspectorDivider = {result.inspector.right, top,
                                               result.inspector.right + inspectorDividerWidth, height};
                }
                if (!navigationOuter && navigationWidth > 0 && inspectorOpen) {
                    result.inspector = {0, top, inspectorWidth, height};
                    result.inspectorDivider = {inspectorWidth, top, inspectorWidth + inspectorDividerWidth,
                                               height};
                    result.navigation = {inspectorWidth + inspectorDividerWidth, top,
                                         inspectorWidth + inspectorDividerWidth + navigationWidth, height};
                    result.navigationDivider = {result.navigation.right, top,
                                                result.navigation.right + navigationDividerWidth, height};
                }
            }
            if (result.compact && navigationOpen && result.inspector.left < result.navigation.right) {
                result.inspectorDivider = {};
            }
            return result;
        }
    };
} // namespace merutilm::rff2::workspace
