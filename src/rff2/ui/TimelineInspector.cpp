//
// Modified by GPT-6 on 2026-09-19, 2026-09-20, 2026-09-21, 2026-09-23, 2026-09-25, 2026-09-26
//

#include "TimelineWindow.hpp"
#include "IOUtilities.h"
#include "NativeDialogs.hpp"
#include "../video/AudioSourceInfo.hpp"
#include "workspace/FormWorkspace.hpp"
#include "workspace/WorkspaceButton.hpp"
#include "workspace/TimelineOverlayForm.hpp"
#include "../io/TimelineIO.h"
#include "../io/AudioTimelineIO.hpp"
#include "../video/TimelineParams.hpp"
#include "../video/TimelineEvaluator.hpp"
#include "../attr/VidTimelineTarget.h"
#include <sstream>

namespace merutilm::rff2 {
    namespace {
        LRESULT CALLBACK inspectorButtonProc(HWND hwnd, UINT message, WPARAM w, LPARAM l, UINT_PTR id,
                                             DWORD_PTR) {
            if (message == WM_ERASEBKGND) {
                return 1;
            }
            if (message == WM_NCDESTROY) {
                RemoveWindowSubclass(hwnd, inspectorButtonProc, id);
            }
            return DefSubclassProc(hwnd, message, w, l);
        }
        std::string documentKey(const VidTimelineAttribute &value) {
            std::ostringstream bytes(std::ios::out | std::ios::binary);
            TimelineIO::writeTimeline(bytes, value);
            AudioTimelineIO::write(bytes, value.audio);
            TimelineIO::writeOverlayPrecision(bytes, value.zoomOverlay);
            return bytes.str();
        }
        bool offered(const TimelineParamDesc &parameter, bool staticSource) {
            if (parameter.id == uint16_t(VidTimelineTarget::PALETTE_INTERVAL_A)) {
                return false;
            }
            if (staticSource && !TimelineParams::movesOverStaticImage(parameter.id)) {
                return false;
            }
            if (parameter.kind == TimelineParamKind::COLOR) {
                return parameter.getColor && parameter.setColor;
            }
            return parameter.getValue && parameter.setValue;
        }
        bool editableTrack(uint16_t target, bool staticSource) {
            if (target == uint16_t(VidTimelineTarget::SPEED)) {
                return true;
            }
            const auto *parameter = TimelineParams::find(target);
            return parameter && offered(*parameter, staticSource);
        }
        std::vector<std::wstring> inspectorGroups(bool staticSource) {
            std::vector<std::wstring> groups{L"Selection", L"Audio", L"Zoom Overlay", L"Panel Layout"};
            for (const auto &parameter : TimelineParams::all()) {
                if (!offered(parameter, staticSource)) {
                    continue;
                }
                const auto label = std::wstring(L"Add Parameter: ") + parameter.group;
                if (std::ranges::find(groups, label) == groups.end()) {
                    groups.push_back(label);
                }
            }
            return groups;
        }
        template <class T> std::vector<workspace::FormChoice> choices() {
            std::vector<workspace::FormChoice> result;
            for (auto value : Selectable::values<T>()) {
                result.push_back({std::to_wstring(int(value)), Selectable::toString(value)});
            }
            return result;
        }
        std::vector<workspace::FormChoice> parameterChoices(const TimelineParamDesc &p) {
            if (p.kind == TimelineParamKind::BOOL) {
                return {{L"0", L"Off"}, {L"1", L"On"}};
            }
            if (p.kind != TimelineParamKind::ENUM) {
                return {};
            }
            const std::wstring_view group = p.group, name = p.name;
            if (name == L"UV Mode") {
                return choices<ShdTextureUVMode>();
            }
            if (name == L"Blend Mode") {
                return choices<ShdTextureBlendMode>();
            }
            if (name == L"Ink Mode") {
                return choices<ShdPatternInkMode>();
            }
            if (name == L"Type" && group.starts_with(L"Pattern")) {
                return choices<ShdPatternType>();
            }
            if (name == L"Type" && group == L"Stripe") {
                return choices<ShdStripeType>();
            }
            if (name == L"Source" && group == L"Warp") {
                return choices<ShdWarpSource>();
            }
            if (p.id == uint16_t(VidTimelineTarget::PALETTE_CYCLE_CURVE)) {
                return choices<ShdPaletteCycleCurve>();
            }
            if (p.id == uint16_t(VidTimelineTarget::PALETTE_ITERATION_COLORING)) {
                return choices<ShdPalIterationColoringMode>();
            }
            if (p.id == uint16_t(VidTimelineTarget::CAMERA_PROJECTION)) {
                return choices<decltype(ShaderAttribute{}.camera.projection)>();
            }
            if (p.id == uint16_t(VidTimelineTarget::CAMERA_LAYOUT)) {
                return choices<decltype(ShaderAttribute{}.camera.layout)>();
            }
            std::vector<workspace::FormChoice> result;
            for (int i = int(p.minValue); i <= int(p.maxValue); ++i) {
                result.push_back({std::to_wstring(i), std::to_wstring(i)});
            }
            return result;
        }
    }

    void TimelineWindow::initializeInspector() {
        if (inspectorToggle) {
            return;
        }
        inspectorToggle =
            CreateWindowExW(0, L"BUTTON", UiLanguage::label(L"Hide Settings"),
                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 1, 1, window,
                            reinterpret_cast<HMENU>(inspectorToggleId), GetModuleHandleW(nullptr), nullptr);
        workspace::WorkspaceButton::attach(inspectorToggle);
        SetWindowSubclass(inspectorToggle, inspectorButtonProc, 5, 0);
        workspace::AccessibleControl::describe(inspectorToggle, L"Timeline Settings",
                                               L"Show or hide the right-hand settings inspector.",
                                               L"timeline.inspector");
        inspectorSplitter = std::make_unique<workspace::PaneSplitter>(
            window, false, L"Resize timeline settings",
            [this] {
                inspectorResizeStart = dockState.inspectorWidth;
                return !exporting;
            },
            [this](int delta) {
                dockState.inspectorWidth =
                    std::clamp(inspectorResizeStart +
                                   (dockState.inspectorLeft ? 1 : -1) * int(std::lround(delta * 96. / uiDpi)),
                               300, 640);
                layoutWorkspaceDock();
            },
            [this](bool cancel) {
                if (cancel) {
                    dockState.inspectorWidth = inspectorResizeStart;
                    layoutWorkspaceDock();
                } else {
                    saveWorkspaceDock();
                }
            },
            [this] {
                dockState.inspectorWidth = 340;
                layoutWorkspaceDock();
                saveWorkspaceDock();
            },
            [this](int direction) {
                if (direction > 0 && inspector) {
                    inspector->focus();
                } else {
                    SetFocus(window);
                }
            });
        refreshInspector(true);
        SetTimer(window, inspectorTimerId, 200, nullptr);
    }

    int TimelineWindow::layoutInspector(int width, int height, int restoreScroll) {
        if (!inspectorToggle) {
            return width;
        }
        const auto px = [this](int value) {
            return UiDpi::pixels(value, uiDpi);
        };
        const int rail = std::min(width, px(40));
        const int panel =
            dockState.inspectorOpen ? std::min(px(dockState.inspectorWidth), std::max(0, width - rail)) : 0;
        const int divider = panel ? std::min(px(8), std::max(0, width - panel)) : 0;
        inspectorReservedWidth = panel ? panel + divider : rail;
        const int left = std::max(0, width - inspectorReservedWidth);
        const int panelLeft = dockState.inspectorLeft ? 0 : left + divider;
        const int dividerLeft = dockState.inspectorLeft ? panel : left;
        const int header = px(44);
        SendMessageW(inspectorToggle, WM_SETFONT, reinterpret_cast<WPARAM>(smallFont), FALSE);
        const wchar_t *captionKey = L"Hide Settings";
        if (!panel) {
            captionKey = dockState.inspectorLeft ? L">" : L"<";
        }
        const auto caption = UiLanguage::label(captionKey);
        wchar_t current[128]{};
        GetWindowTextW(inspectorToggle, current, 128);
        if (wcscmp(current, caption) != 0) {
            SetWindowTextW(inspectorToggle, caption);
        }
        SetWindowPos(inspectorToggle, HWND_TOP, panelLeft + px(4), px(8),
                     std::max(1, (panel ? panel : rail) - px(8)), px(28), SWP_NOACTIVATE);
        inspectorSplitter->layout(panel ? RECT{dividerLeft, 0, dividerLeft + divider, height} : RECT{},
                                  uiDpi / 96.f);
        if (inspector) {
            if (panel) {
                inspector->layoutAt(panelLeft, header, panel, std::max(1, height - header), true);
                if (restoreScroll >= 0) {
                    inspector->restoreScrollPosition(restoreScroll);
                }
            }
            inspector->show(panel > 0);
        }
        return left;
    }

    void TimelineWindow::showInspectorSection(int section) {
        if (inspector && !inspector->applyPending()) {
            inspector->focus();
            return;
        }
        dockState.inspectorOpen = true;
        refreshInspector();
        if (inspector) {
            inspector->selectGroup(section);
        }
        layoutWorkspaceDock();
        saveWorkspaceDock();
        if (inspector) {
            inspector->focus();
        }
    }

    void TimelineWindow::refreshInspector(bool force) {
        if (!inspectorToggle || draggingTrackKey || draggingOverlay || draggingTrackRow || draggingAudio) {
            return;
        }
        if (inspector && inspector->hasPending()) {
            return;
        }
        const auto signature = documentKey(attribute.video.timeline);
        if (!force && inspectorSectionRequest < 0 && inspector && inspectorTarget == selectedTrackTarget &&
            inspectorKey == selectedTrackKey && signature == inspectorDocumentKey) {
            return;
        }
        int group = 0;
        if (inspectorSectionRequest >= 0) {
            group = std::exchange(inspectorSectionRequest, -1);
        } else if (inspector) {
            group = inspector->selectedGroup();
        }
        const int savedScroll =
            inspector && inspector->selectedGroup() == group ? inspector->scrollPosition() : 0;
        const bool hadFocus = inspector && IsChild(inspector->handle(), GetFocus());
        inspectorTarget = selectedTrackTarget;
        inspectorKey = selectedTrackKey;
        inspectorDocumentKey = signature;
        auto snapshot = std::make_shared<Attribute>(attribute);
        auto baseline = std::make_shared<std::string>(signature);
        auto keyChanged = std::make_shared<bool>(false);
        auto overlayOnly = std::make_shared<bool>(false);
        const uint16_t target = inspectorTarget;
        const int index = inspectorKey;
        const auto found =
            std::ranges::find(snapshot->video.timeline.tracks, target, &VidTimelineTrack::targetId);
        const size_t trackIndex = size_t(found - snapshot->video.timeline.tracks.begin());
        const bool supported = editableTrack(target, attribute.video.data.isStatic);
        const bool hasKey = supported && found != snapshot->video.timeline.tracks.end() && index >= 0 &&
                            index < int(found->keys.size());
        const auto key = [trackIndex, index](auto &value) -> auto & {
            return value.video.timeline.tracks.at(trackIndex).keys.at(size_t(index));
        };
        auto model = std::make_shared<workspace::AttributeFormModel>(
            [snapshot]() -> Attribute & {
                return *snapshot;
            },
            [this, snapshot, baseline, hasKey, key, target, keyChanged, overlayOnly] {
                if (*overlayOnly) {
                    attribute.video.timeline.zoomOverlay = snapshot->video.timeline.zoomOverlay;
                    lastUndoStep = 0;
                    commitOverlay();
                    *baseline = documentKey(attribute.video.timeline);
                    return;
                }
                float depth = hasKey ? key(*snapshot).depth : previewDepth;
                attribute.video.timeline = snapshot->video.timeline;
                if (hasKey && *keyChanged) {
                    auto *current = track(target);
                    std::ranges::stable_sort(current->keys, [](const auto &a, const auto &b) {
                        return a.depth > b.depth;
                    });
                    selectedTrackTarget = target;
                    selectedTrackKey = int(std::ranges::find(current->keys, depth, &VidTimelineKey::depth) -
                                           current->keys.begin());
                    previewDepth = depth;
                    (void)syncLinkedColorCycle(target);
                }
                attribute.video.animation.showText = attribute.video.timeline.zoomOverlay.visible;
                if (sourceAttribute) {
                    sourceAttribute->video.animation.showText = attribute.video.animation.showText;
                }
                lastUndoStep = 0;
                commitTimeline();
                *baseline = documentKey(attribute.video.timeline);
            });
        using Model = workspace::AttributeFormModel;
        std::wstring selection =
            supported ? L"Select a timeline key to edit its exact values."
                      : L"This parameter is unavailable for the current source. Its saved keys are retained.";
        if (hasKey) {
            const auto *parameter = TimelineParams::find(target);
            const bool speed = target == uint16_t(VidTimelineTarget::SPEED);
            if (speed) {
                selection = L"Zoom Speed";
            } else if (parameter) {
                selection = parameter->name;
            } else {
                selection = L"Timeline Key";
            }
            model->numeric(
                "key.depth", 0, L"Keyframe Depth",
                L"The animation key stays at this depth when the speed changes.",
                [key](auto &a) -> auto & {
                    return key(a).depth;
                },
                schedule.getEndDepth(), schedule.getStartDepth());
            if (parameter && parameter->kind == TimelineParamKind::COLOR) {
                model->color("key.color", 0, L"Color", [key](auto &a) -> auto & {
                    return key(a).color;
                });
            } else {
                float minimum = -std::numeric_limits<float>::max();
                float maximum = std::numeric_limits<float>::max();
                if (speed) {
                    minimum = VidTimelineAttribute::MIN_SPEED;
                } else if (parameter) {
                    minimum = parameter->minValue;
                    maximum = parameter->maxValue;
                }
                model->numeric(
                    "key.value", 0, speed ? L"Zoom Speed" : L"Value",
                    L"Exact value of the selected animation key.",
                    [key](auto &a) -> auto & {
                        return key(a).value;
                    },
                    minimum, maximum);
            }
            model->choice("key.interpolation", 0, L"Interpolation",
                          L"How this key reaches the next key on its track.", [key](auto &a) -> auto & {
                              return key(a).out;
                          });
        }
        model->setValidator(
            [hasKey, trackIndex, index, target, keyChanged](const Attribute &candidate) -> std::wstring {
                if (!candidate.video.timeline.audio.valid()) {
                    return L"Audio: Source In must be before Source Out and within the source length. Fades must fit the trimmed clip, and clips cannot overlap.";
                }
                if (hasKey && *keyChanged) {
                    const auto &keys = candidate.video.timeline.tracks[trackIndex].keys;
                    const auto *parameter = TimelineParams::find(target);
                    if (parameter &&
                        (parameter->kind == TimelineParamKind::BOOL ||
                         parameter->kind == TimelineParamKind::ENUM) &&
                        (std::round(keys[index].value) != keys[index].value ||
                         keys[index].out != VidKeyInterpolation::STEP)) {
                        return L"Choose a listed value and Step interpolation for this parameter.";
                    }
                    for (size_t i = 0; i < keys.size(); ++i) {
                        if (i != size_t(index) && std::abs(keys[i].depth - keys[index].depth) < 1.f) {
                            return L"Keep animation keys at least one keyframe apart.";
                        }
                    }
                }
                return {};
            });
        model->choice("audio.enabled", 1, L"Export Audio", L"Include music in the exported video.",
                      [](auto &a) -> auto & {
                          return a.video.timeline.audio.exportEnabled;
                      });
        model->numeric(
            "audio.gain", 1, L"Master Volume", L"0 is silent, 1 is original volume, 4 is the maximum gain.",
            [](auto &a) -> auto & {
                return a.video.timeline.audio.gain;
            },
            0.f, 4.f);
        auto &clips = snapshot->video.timeline.audio.clips;
        if (std::ranges::find(clips, selectedAudioClip, &VidAudioClip::id) == clips.end()) {
            selectedAudioClip = clips.empty() ? 0 : clips.front().id;
        }
        const uint64_t audioId = selectedAudioClip;
        if (audioId) {
            const auto clip = [audioId](auto &a) -> auto & {
                auto &values = a.video.timeline.audio.clips;
                return *std::ranges::find(values, audioId, &VidAudioClip::id);
            };
            auto probedPath = std::make_shared<std::wstring>();
            auto probedDuration = std::make_shared<std::optional<int64_t>>();
            model->text("audio.path", 1, L"Audio File", L"Choose the source for this clip. Source times refer to this file.",
                [clip](const Attribute &a) {
                    const auto &path = clip(a).path;
                    return std::filesystem::path(std::u8string(path.begin(), path.end())).wstring();
                },
                [clip, probedPath, probedDuration](Attribute &a, const std::wstring &value) {
                    std::error_code error;
                    const std::filesystem::path source(value);
                    if (!std::filesystem::is_regular_file(source, error)) return false;
                    if (*probedPath != value) {
                        *probedPath = value;
                        *probedDuration = AudioSourceInfo::duration(source);
                    }
                    const auto duration = *probedDuration;
                    if (!duration) return false;
                    const auto path = std::filesystem::absolute(source).u8string();
                    auto &c = clip(a);
                    c.path.assign(path.begin(), path.end());
                    c.sourceDuration = *duration;
                    return true;
                }, workspace::FormField::Editor::FILE);
            const auto timeField = [&](std::string id, std::wstring label, std::wstring hint, int64_t VidAudioClip::*member) {
                model->text(std::move(id), 1, std::move(label), std::move(hint),
                    [clip, member](const Attribute &a) { return Model::number(double(clip(a).*member) / 1000000); },
                    [clip, member](Attribute &a, const std::wstring &text) {
                        double seconds = 0;
                        if (!Model::parse(text, seconds) || !(seconds >= 0 && seconds <= 604800)) return false;
                        clip(a).*member = static_cast<int64_t>(std::llround(seconds * 1000000));
                        return true;
                    });
            };
            timeField("audio.start", L"Start in Video (s)", L"Place the clip at this time in the video. Clips cannot overlap.", &VidAudioClip::start);
            timeField("audio.in", L"Source In (s)", L"Start reading here in the source audio file.", &VidAudioClip::in);
            timeField("audio.out", L"Source Out (s)", L"Stop reading here in the source audio file.", &VidAudioClip::out);
            timeField("audio.fadeIn", L"Fade In (s)", L"Fade from silence at the beginning of the trimmed clip.", &VidAudioClip::fadeIn);
            timeField("audio.fadeOut", L"Fade Out (s)", L"Fade to silence at the end of the trimmed clip.", &VidAudioClip::fadeOut);
            model->numeric("audio.clipGain", 1, L"Clip Volume", L"Multiplied by Master Volume. 1 preserves the original level.",
                [clip](auto &a) -> auto & { return clip(a).gain; }, 0.f, 4.f);
            model->choice("audio.muted", 1, L"Mute Clip", L"Keep the clip but omit its sound from export.",
                [clip](auto &a) -> auto & { return clip(a).muted; });
        }
        workspace::addTimelineOverlayFields(*model);
        model->setNormalizer([audioId](const Attribute &before, Attribute &after, const workspace::FormDraft &draft) {
            workspace::normalizeTimelineOverlay(after, draft);
            if (audioId && draft.contains("audio.path")) {
                const auto &oldClips = before.video.timeline.audio.clips;
                auto &newClips = after.video.timeline.audio.clips;
                const auto old = std::ranges::find(oldClips, audioId, &VidAudioClip::id);
                auto current = std::ranges::find(newClips, audioId, &VidAudioClip::id);
                if (old->path != current->path) {
                    if (!draft.contains("audio.in")) current->in = 0;
                    if (!draft.contains("audio.out")) current->out = current->sourceDuration;
                    if (!draft.contains("audio.fadeIn")) current->fadeIn = 0;
                    if (!draft.contains("audio.fadeOut")) current->fadeOut = 0;
                }
            }
        });
        auto form = model->form(L"Timeline Settings", inspectorGroups(attribute.video.data.isStatic));
        if (audioId) {
            workspace::FormField selected{"audio.selection", 1, L"Audio Clip", L"Select the clip to edit.",
                [audioId] { return std::to_wstring(audioId); }};
            selected.persisted = false;
            for (const auto &c : clips) {
                const auto path = std::filesystem::path(std::u8string(c.path.begin(), c.path.end()));
                selected.choices.push_back({std::to_wstring(c.id), std::to_wstring(c.id) + L": " + path.filename().wstring()});
            }
            const auto pos = std::ranges::find(form.fields, std::string("audio.path"), &workspace::FormField::id);
            form.fields.insert(pos, std::move(selected));
        }
        form.actions.push_back({1, L"Add Audio File", [this] {
            if (!inspector || inspector->applyPending()) addAudioClip();
        }, true});
        if (audioId) form.actions.push_back({1, L"Remove Audio Clip", [this] { removeAudioClip(); }});
        for (auto &field : form.fields) {
            if (field.id == "overlay.anchor") {
                const wchar_t *labels[]{L"Top Left",    L"Top Center",    L"Top Right",
                                        L"Middle Left", L"Center",        L"Middle Right",
                                        L"Bottom Left", L"Bottom Center", L"Bottom Right"};
                for (int i = 0; i < 9; ++i) {
                    field.choices.push_back({std::to_wstring(i), labels[i]});
                }
            }
            if (field.id == "overlay.style") {
                field.choices = {
                    {L"0", L"Regular"}, {L"1", L"Bold"}, {L"2", L"Italic"}, {L"3", L"Bold Italic"}};
            }
        }
        workspace::FormField positionField{
            "overlay.positionMode",
            2,
            L"Edit Overlay Position",
            L"Drag in the preview or use arrow keys. Shift moves ten pixels. Escape cancels a drag.",
            [this] {
                return overlayPositionMode ? L"1" : L"0";
            },
            {{L"0", L"Off"}, {L"1", L"On"}}};
        positionField.persisted = false;
        positionField.validate = [](const std::wstring &value) {
            return value == L"0" || value == L"1" ? std::wstring{} : L"Choose On or Off.";
        };
        form.fields.insert(form.fields.begin(), std::move(positionField));
        workspace::FormField guideField{"guide.visible",
                                        2,
                                        L"YouTube Shorts Guide",
                                        L"Preview only; never exported. Shaded margins mark areas to avoid: "
                                        L"top UI, bottom title and description, right controls, and left "
                                        L"edge. Keep text inside the dashed frame. Device layouts vary.",
                                        [this] {
                                            return shortsGuide.visible ? L"1" : L"0";
                                        },
                                        {{L"0", L"Off"}, {L"1", L"On"}}};
        guideField.persisted = false;
        guideField.validate = [](const std::wstring &value) {
            return value == L"0" || value == L"1" ? std::wstring{} : L"Choose On or Off.";
        };
        form.fields.push_back(std::move(guideField));
        workspace::FormField guideDescriptions{"guide.descriptions",
                                               2,
                                               L"Show Guide Descriptions",
                                               L"Show explanatory labels around the preview guide. Off keeps "
                                               L"only the shaded margins and dashed frame. Preview only.",
                                               [this] {
                                                   return shortsGuide.showDescriptions ? L"1" : L"0";
                                               },
                                               {{L"0", L"Off"}, {L"1", L"On"}}};
        guideDescriptions.persisted = false;
        guideDescriptions.validate = [](const std::wstring &value) {
            return value == L"0" || value == L"1" ? std::wstring{} : L"Choose On or Off.";
        };
        form.fields.push_back(std::move(guideDescriptions));
        const auto guideMargin = [&](const char *id, const wchar_t *label,
                                     int workspace::ShortsGuide::*member) {
            workspace::FormField field{
                id, 2, label,
                L"Approximate margin as a percentage of the video frame. 0 to 40; preview only.",
                [this, member] {
                    return std::to_wstring(shortsGuide.*member);
                }};
            field.persisted = false;
            field.validate = workspace::AttributeFormModel::rangeValidation<int>(0, 40);
            form.fields.push_back(std::move(field));
        };
        guideMargin("guide.top", L"Guide Top (%)", &workspace::ShortsGuide::top);
        guideMargin("guide.bottom", L"Guide Bottom (%)", &workspace::ShortsGuide::bottom);
        guideMargin("guide.left", L"Guide Left (%)", &workspace::ShortsGuide::left);
        guideMargin("guide.right", L"Guide Right (%)", &workspace::ShortsGuide::right);
        if (hasKey) {
            if (const auto *p = TimelineParams::find(target);
                p && (p->kind == TimelineParamKind::ENUM || p->kind == TimelineParamKind::BOOL)) {
                for (auto &field : form.fields) {
                    if (field.id == "key.value") {
                        field.choices = parameterChoices(*p);
                    }
                    if (field.id == "key.interpolation") {
                        field.choices = {{L"0", L"Step"}};
                    }
                }
            }
        }
        for (const auto &parameter : TimelineParams::all()) {
            if (!offered(parameter, attribute.video.data.isStatic)) {
                continue;
            }
            const auto label = std::wstring(L"Add Parameter: ") + parameter.group;
            const int page = int(std::ranges::find(form.groups, label) - form.groups.begin());
            form.actions.push_back(
                {page, std::wstring(track(parameter.id) ? L"Select: " : L"Add: ") + parameter.name,
                 [this, id = parameter.id] {
                     addParameterTrack(id);
                 }});
        }
        form.actions.push_back({0, L"Add Parameter", [this] {
                                    inspectorSectionRequest = 4;
                                }});
        if (found != snapshot->video.timeline.tracks.end()) {
            if (supported) {
                form.actions.push_back({0, L"Add Key at Playhead", [this] {
                                            addInspectorKey();
                                        }});
            }
            form.actions.push_back({0, L"Select First Key", [this] {
                                        selectedTrackKey = 0;
                                        inspectorSectionRequest = 0;
                                    }});
            if (hasKey) {
                form.actions.push_back({0, L"Delete Selected Key", [this] {
                                            lastUndoStep = 0;
                                            deleteTrackKey();
                                        }});
            }
        }
        const auto apply = form.apply;
        form.apply = [this, baseline, apply, keyChanged,
                      overlayOnly](const workspace::FormDraft &draft) -> std::wstring {
            if (exporting) {
                return L"Wait for the export to finish before editing.";
            }
            if (*baseline != documentKey(attribute.video.timeline)) {
                return L"The timeline changed. Discard this draft and edit the current selection.";
            }
            auto documentDraft = draft;
            uint64_t nextAudio = selectedAudioClip;
            if (const auto selected = documentDraft.find("audio.selection"); selected != documentDraft.end()) {
                if (!workspace::AttributeFormModel::parse(selected->second, nextAudio) ||
                    std::ranges::find(attribute.video.timeline.audio.clips, nextAudio, &VidAudioClip::id) == attribute.video.timeline.audio.clips.end()) {
                    return L"Select an existing audio clip.";
                }
                documentDraft.erase(selected);
            }
            auto guide = shortsGuide;
            for (auto it = documentDraft.begin(); it != documentDraft.end();) {
                if (!it->first.starts_with("guide.")) {
                    ++it;
                    continue;
                }
                int value = 0;
                if (!workspace::AttributeFormModel::parse(it->second, value) || value < 0 || value > 40) {
                    return L"Enter a whole number from 0 to 40.";
                }
                if (it->first == "guide.visible") {
                    if (value > 1) {
                        return L"Choose On or Off.";
                    }
                    guide.visible = value != 0;
                } else if (it->first == "guide.descriptions") {
                    if (value > 1) {
                        return L"Choose On or Off.";
                    }
                    guide.showDescriptions = value != 0;
                } else if (it->first == "guide.top") {
                    guide.top = value;
                } else if (it->first == "guide.bottom") {
                    guide.bottom = value;
                } else if (it->first == "guide.left") {
                    guide.left = value;
                } else if (it->first == "guide.right") {
                    guide.right = value;
                }
                it = documentDraft.erase(it);
            }
            const auto position = documentDraft.find("overlay.positionMode");
            bool positionMode = overlayPositionMode;
            if (position != documentDraft.end()) {
                if (position->second != L"0" && position->second != L"1") {
                    return L"Choose On or Off.";
                }
                positionMode = position->second == L"1";
                documentDraft.erase(position);
            }
            *keyChanged = std::ranges::any_of(documentDraft, [](const auto &field) {
                return field.first.starts_with("key.");
            });
            *overlayOnly =
                !documentDraft.empty() && std::ranges::all_of(documentDraft, [](const auto &field) {
                    return field.first.starts_with("overlay.");
                });
            const auto error = apply(documentDraft);
            if (!error.empty()) {
                return error;
            }
            if (selectedAudioClip != nextAudio) {
                selectedAudioClip = nextAudio;
                inspectorSectionRequest = 1;
            }
            overlayPositionMode = positionMode;
            shortsGuide = guide;
            InvalidateRect(window, nullptr, FALSE);
            return {};
        };
        form.canEdit = [this] {
            return !exporting && !draggingTrackKey && !draggingOverlay && !draggingAudio;
        };
        form.status = [this, selection] {
            if (inspector && inspector->selectedGroup() == 1) {
                const auto &clips = attribute.video.timeline.audio.clips;
                const auto found = std::ranges::find(clips, selectedAudioClip, &VidAudioClip::id);
                if (found == clips.end()) return std::wstring(L"Choose Add Audio File to include music. Times are entered in seconds.");
                return std::format(L"Source length: {:.3f} s. Clip length: {:.3f} s. Apply edits before selecting another clip. Playback previews the audio with clip volume and fades.",
                    double(found->sourceDuration) / 1000000, double(found->duration()) / 1000000);
            }
            if (inspector && inspector->selectedGroup() == 2) {
                return std::wstring(L"Settings apply to the entire video. Timeline Save includes the "
                                    L"overlay. Font files are not embedded.");
            }
            if (inspector && inspector->selectedGroup() >= 4) {
                return std::wstring(attribute.video.data.isStatic
                                        ? L"Only parameters supported by PNG sources are listed. "
                                        : L"Add a parameter, then edit its keys in Selection. ") +
                       L"Texture images and other non-animated base settings must be configured separately.";
            }
            return selection;
        };
        form.canUndo = [this] {
            return !undoSteps.empty();
        };
        form.canRedo = [this] {
            return !redoSteps.empty() && historyOrder.validRedo();
        };
        form.undo = [this] {
            return undoTimeline();
        };
        form.redo = [this] {
            return redoTimeline();
        };
        form.undoOrder = [this] {
            return undoSteps.empty() ? 0 : undoSteps.back().serial;
        };
        form.redoOrder = [this] {
            return redoSteps.empty() ? 0 : redoSteps.back().serial;
        };
        form.requestHistory = [this](bool redo) {
            if (historyRequest) {
                historyRequest(redo);
            } else if (redo) {
                redoTimeline();
            } else {
                undoTimeline();
            }
        };
        form.actions.push_back({2, L"Choose Font", [this] {
                                    chooseOverlayFont();
                                }});
        form.actions.push_back({2, L"Reset Position", [this] {
                                    resetOverlayPosition();
                                }});
        form.actions.push_back({2, L"Fit Inside Frame", [this] {
                                    fitOverlayInsideFrame();
                                }});
        form.actions.push_back({2, L"Reset Appearance", [this] {
                                    resetOverlayAppearance();
                                }});
        form.actions.push_back({3, L"70% Preview", [this] {
                                    applyLayoutPreset(70);
                                }});
        form.actions.push_back({3, L"50% / 50%", [this] {
                                    applyLayoutPreset(50);
                                }});
        form.actions.push_back({3, L"70% Tracks", [this] {
                                    applyLayoutPreset(30);
                                }});
        form.actions.push_back({3, L"Reset Settings Width", [this] {
                                    dockState.inspectorWidth = 340;
                                    layoutWorkspaceDock();
                                    saveWorkspaceDock();
                                }});
        form.actions.push_back({3, L"Move Settings Left", [this] {
                                    dockState.inspectorLeft = true;
                                    layoutWorkspaceDock();
                                    saveWorkspaceDock();
                                }});
        form.actions.push_back({3, L"Move Settings Right", [this] {
                                    dockState.inspectorLeft = false;
                                    layoutWorkspaceDock();
                                    saveWorkspaceDock();
                                }});
        inspector = std::make_unique<workspace::FormWorkspace>(
            window, std::move(form), bodyFont, inspectorTheme, uiDpi / 96.f,
            [] {
            },
            [this](int) {
                SetFocus(window);
            });
        inspector->selectGroup(group);
        RECT client{};
        GetClientRect(window, &client);
        layoutInspector(client.right, client.bottom, savedScroll);
        if (hadFocus) {
            inspector->focus();
        }
    }

    void TimelineWindow::addAudioClip() {
        if (exporting) return;
        auto &audio = attribute.video.timeline.audio;
        if (audio.clips.size() >= VidAudioAttribute::maximumClips) {
            NativeDialogs::message(window, L"The audio clip limit has been reached.", L"Audio", MB_OK | MB_ICONERROR);
            return;
        }
        const auto source = IOUtilities::ioFileDialogMulti(L"Add Audio File", IOUtilities::OPEN_FILE,
            {{L"WAV Audio", L"wav"}, {L"MP3 Audio", L"mp3"}, {L"FLAC Audio", L"flac"},
             {L"AAC Audio", L"m4a"}, {L"Ogg Audio", L"ogg"}, {L"All Files", L"*"}});
        if (!source) return;
        const auto duration = AudioSourceInfo::duration(*source);
        if (!duration) {
            NativeDialogs::message(window, L"Cannot read this audio file's length. Choose a readable audio file and ensure ffprobe.exe is beside RFF_Super.exe or available on PATH.", L"Audio", MB_OK | MB_ICONERROR);
            return;
        }
        VidAudioClip clip;
        clip.id = 1;
        while (std::ranges::find(audio.clips, clip.id, &VidAudioClip::id) != audio.clips.end()) ++clip.id;
        const auto path = std::filesystem::absolute(*source).u8string();
        clip.path.assign(path.begin(), path.end());
        clip.sourceDuration = clip.out = *duration;
        for (const auto &existing : audio.clips) clip.start = std::max(clip.start, existing.start + existing.duration());
        if (clip.start > VidAudioAttribute::maximumTime - clip.duration()) {
            NativeDialogs::message(window, L"This clip would exceed the seven-day audio timeline limit.", L"Audio", MB_OK | MB_ICONERROR);
            return;
        }
        selectedAudioClip = clip.id;
        audio.clips.push_back(std::move(clip));
        lastUndoStep = 0;
        commitTimeline();
        inspectorSectionRequest = 1;
    }

    void TimelineWindow::removeAudioClip() {
        if (exporting) return;
        auto &clips = attribute.video.timeline.audio.clips;
        if (!std::erase_if(clips, [this](const VidAudioClip &clip) { return clip.id == selectedAudioClip; })) return;
        selectedAudioClip = clips.empty() ? 0 : clips.front().id;
        lastUndoStep = 0;
        commitTimeline();
        inspectorSectionRequest = 1;
    }

    void TimelineWindow::showParameterCatalog(std::wstring_view prefix) {
        const auto groups = inspectorGroups(attribute.video.data.isStatic);
        for (size_t i = 4; i < groups.size(); ++i) {
            if (std::wstring_view(groups[i])
                    .substr(std::wstring_view(L"Add Parameter: ").size())
                    .starts_with(prefix)) {
                showInspectorSection(int(i));
                return;
            }
        }
    }

    void TimelineWindow::addParameterTrack(uint16_t target) {
        const auto *parameter = TimelineParams::find(target);
        if (exporting || !parameter || !offered(*parameter, attribute.video.data.isStatic)) {
            return;
        }
        const bool created = !track(target);
        auto &current = ensureScalarTrack(target);
        if (created) {
            current.enabled = true;
            if (parameter->kind == TimelineParamKind::COLOR) {
                for (auto &key : current.keys) {
                    key.color = parameter->getColor(attribute.shader);
                }
            }
            lastUndoStep = 0;
            commitTimeline();
        }
        selectedTrackTarget = target;
        selectedTrackKey = current.keys.empty() ? -1 : 0;
        selectedTrackTargets = {target};
        trackScrollOffset = std::numeric_limits<int>::max() / 2;
        inspectorSectionRequest = 0;
        InvalidateRect(window, nullptr, FALSE);
    }

    void TimelineWindow::addInspectorKey() {
        if (exporting || !editableTrack(selectedTrackTarget, attribute.video.data.isStatic)) {
            return;
        }
        auto *current = track(selectedTrackTarget);
        if (!current || current->keys.size() >= VidTimelineAttribute::MAX_KEYS_PER_TRACK) {
            return;
        }
        const auto *p = TimelineParams::find(selectedTrackTarget);
        const float depth =
            std::clamp(std::round(previewDepth), schedule.getEndDepth(), schedule.getStartDepth());
        for (size_t i = 0; i < current->keys.size(); ++i) {
            if (std::abs(current->keys[i].depth - depth) < 1.f) {
                selectedTrackKey = int(i);
                inspectorSectionRequest = 0;
                return;
            }
        }
        ShaderAttribute evaluated;
        TimelineEvaluator(attribute.video.timeline)
            .evaluate(depth, previewSeconds(), attribute.shader, evaluated);
        auto mode = VidKeyInterpolation::SMOOTH;
        float value = 0;
        auto color = glm::vec4(1);
        if (p) {
            if (p->kind == TimelineParamKind::BOOL || p->kind == TimelineParamKind::ENUM) {
                mode = VidKeyInterpolation::STEP;
            }
        }
        if (p && p->kind != TimelineParamKind::COLOR) {
            value = p->getValue(evaluated);
        } else if (selectedTrackTarget == uint16_t(VidTimelineTarget::SPEED)) {
            value = schedule.speedAt(depth);
        }
        if (p && p->kind == TimelineParamKind::COLOR) {
            color = p->getColor(evaluated);
        }
        current->keys.push_back({depth, value, color, mode});
        current->enabled = true;
        std::ranges::stable_sort(current->keys, [](const auto &a, const auto &b) {
            return a.depth > b.depth;
        });
        selectedTrackKey =
            int(std::ranges::find(current->keys, depth, &VidTimelineKey::depth) - current->keys.begin());
        (void)syncLinkedColorCycle(selectedTrackTarget);
        lastUndoStep = 0;
        commitTimeline();
        inspectorSectionRequest = 0;
    }
}
