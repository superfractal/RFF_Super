//
// Modified by GPT-5 on 2026-08-18, 2026-08-24, 2026-08-26, 2026-08-31
// Modified by Opus 5 on 2026-08-19, 2026-08-20, 2026-08-23, 2026-08-25, 2026-08-26, 2026-08-31, 2026-09-01
// Modified by GPT-6 on 2026-09-14, 2026-09-15, 2026-09-18, 2026-09-19, 2026-09-20, 2026-09-21, 2026-09-22, 2026-09-23
//

#pragma once

#include "workspace/ShortsGuide.hpp"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <stop_token>
#include <thread>
#include <utility>
#include <vector>
#include <windows.h>
#include <opencv2/core/mat.hpp>

#include "../../vulkan_helper/impl/Engine.hpp"
#include "../attr/Attribute.h"
#include "../video/TimelineSchedule.hpp"
#include "workspace/HistoryOrder.hpp"
#include "workspace/TimelineDockLayout.hpp"
#include "workspace/PaneSplitter.hpp"
#include "workspace/AccessibleItems.hpp"
#include "workspace/TimelineItems.hpp"
#include "workspace/WorkspaceTheme.hpp"

namespace merutilm::rff2 {
    namespace workspace {
        class FormWorkspace;
    }
    class RenderScene;
    class ZoomOverlay;
    class SettingsWindow;
    struct SettingsMenu;
    class VideoFrameSource;
    class VideoRenderScene;

    class TimelineWindow final {
        struct TrackLayout {
            uint16_t targetId = 0;
            RECT row = {};
            // The name cell left of the axis, which selects the row when it is clicked.
            RECT label = {};
            bool editable = false;
            float minValue = 0.0f;
            float maxValue = 1.0f;
            // Where the row stands among the rows a drag can reorder, below zero when it is pinned.
            int order = -1;
        };

        // Which transport readout a sideways drag is currently moving the playhead from.
        enum class FieldDrag { NONE, DEPTH, TIME };

        enum class FieldEdit { NONE, DISTANCE, KEYFRAME, TIME };

        struct KeyHit {
            uint16_t targetId = 0;
            int keyIndex = -1;

            [[nodiscard]] bool valid() const {
                return keyIndex >= 0;
            }
        };

        vkh::EngineRef engine;
        Attribute *sourceAttribute = nullptr;
        Attribute attribute;
        TimelineSchedule schedule;
        std::shared_ptr<const TimelineSchedule> previewScheduleSnapshot;
        HWND window = nullptr;
        UINT uiDpi = 96;
        bool fontsEmbedded = false;
        void updateDpi(UINT dpi);
        bool mainPreviewPauseClaimed = false;
        bool embedded = false;
        bool floatingWorkspace = false;
        workspace::PanelBackBuffer paintBuffer;
        workspace::PanelBackBuffer dockToggleBuffer;
        workspace::PanelBackBuffer inspectorToggleBuffer;
        workspace::TimelineDockState dockState, dockResizeStart;
        workspace::TimelineDockLayout dockLayout;
        std::filesystem::path dockPreferences;
        std::unique_ptr<workspace::PaneSplitter> dockSplitter;
        HWND dockToggle = nullptr;
        std::unique_ptr<workspace::FormWorkspace> inspector;
        std::unique_ptr<workspace::PaneSplitter> inspectorSplitter;
        workspace::WorkspaceTheme inspectorTheme = workspace::WorkspaceTheme::current();
        HWND inspectorToggle = nullptr;
        int inspectorResizeStart = 340, inspectorReservedWidth = 0;
        uint16_t inspectorTarget = 0;
        int inspectorKey = -2;
        std::string inspectorDocumentKey;
        int inspectorSectionRequest = -1;
        static constexpr UINT inspectorToggleId = 0x7830;
        static constexpr UINT_PTR inspectorTimerId = 0x7831;
        void initializeInspector();
        void refreshInspector(bool force = false);
        int layoutInspector(int width, int height, int restoreScroll = -1);
        void showInspectorSection(int section);
        void showParameterCatalog(std::wstring_view prefix);
        void addParameterTrack(uint16_t target);
        void addInspectorKey();
        void applyLayoutPreset(int previewPercent, bool preview = true, bool tracks = true);
        int dockResizeHeight = 0;
        bool dockPreferencesFailed = false;
        std::unique_ptr<workspace::AccessibleItems> accessibility;
        workspace::TimelineItems itemIds;
        long keyboardFocus = workspace::TimelineItems::play;
        bool accessibilityDirty = true;
        ULONGLONG accessibilityTick = 0;
        std::vector<workspace::AccessibleItem> accessibleItems();
        bool focusItem(long id);
        bool activateItem(long id);
        bool writeItem(long id, std::wstring_view value);
        void tabItem(int direction);
        bool keyItem(WPARAM key);
        void paintItemFocus(HDC dc);
        void initializeWorkspaceDock();
        void layoutWorkspaceDock();
        void saveWorkspaceDock();
        void toggleWorkspaceDock();
        static LRESULT CALLBACK dockToggleProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
        std::string workspaceSource;
        std::unique_ptr<ZoomOverlay> overlayRenderer;
        RECT overlayButton{}, overlayImageRect{};
        workspace::ShortsGuide shortsGuide;
        bool overlayPositionMode = false, draggingOverlay = false;
        POINT overlayDragStart{};
        VidZoomOverlayAttribute overlayDragBefore;
        float publishedPreviewZoom = 0;
        void openOverlaySettings();
        void chooseOverlayFont();
        void fitOverlayInsideFrame();
        void resetOverlayAppearance();
        void resetOverlayPosition();
        void refreshOverlaySettings();
        void commitOverlay();
        HBITMAP previewBitmap = nullptr;
        SIZE previewSize = {};
        HFONT titleFont = nullptr;
        HFONT bodyFont = nullptr;
        HFONT smallFont = nullptr;
        HFONT captionFont = nullptr;
        HFONT valueFont = nullptr;
        RECT timelineAxis = {};
        RECT timelinePanel = {};
        RECT scrollTrack = {};
        RECT scrollThumb = {};
        RECT trackScrollTrack = {};
        RECT trackScrollThumb = {};
        float viewStartDepth = 0.0f;
        float viewEndDepth = 0.0f;
        bool draggingScrollThumb = false;
        bool hoverScrollThumb = false;
        int scrollGrabOffset = 0;
        // How far the track rows are scrolled down, in pixels, and how far they can be.
        int trackScrollOffset = 0;
        int trackScrollRange = 0;
        bool draggingTrackScrollThumb = false;
        bool hoverTrackScrollThumb = false;
        int trackScrollGrabOffset = 0;
        std::vector<TrackLayout> trackLayouts;
        RECT framesButton = {};
        RECT loadButton = {};
        RECT saveButton = {};
        RECT exportButton = {};
        RECT aiButton = {};
        RECT themeButton = {};
        RECT fullscreenButton = {};
        RECT playButton = {};
        RECT pauseButton = {};
        RECT stopButton = {};
        RECT loopButton = {};
        RECT zoomPresetButton = {};
        RECT controlsButton = {};
        bool hoverControls = false;
        std::unique_ptr<SettingsWindow> controlsGuide;
        // The transport readouts. Dragging the distance, keyframe or time box moves the playhead.
        RECT distanceField = {};
        RECT keyframeField = {};
        RECT zoomField = {};
        RECT timeField = {};
        FieldDrag fieldDrag = FieldDrag::NONE;
        FieldEdit pendingFieldEdit = FieldEdit::NONE;
        FieldEdit activeFieldEdit = FieldEdit::NONE;
        FieldEdit hoveredFieldEdit = FieldEdit::NONE;
        HWND fieldEdit = nullptr;
        HWND fieldTooltip = nullptr;
        HBRUSH fieldEditBrush = nullptr;
        bool fieldDragMoved = false;
        int fieldDragOriginX = 0;
        float fieldDragDepth = 0.0f;
        // The two ruler rows above the axis, which drag the view sideways.
        RECT rulerStrip = {};
        uint16_t selectedTrackTarget = 0;
        // The rows picked out around it, which a drag carries as one block.
        std::vector<uint16_t> selectedTrackTargets;
        int selectedTrackKey = -1;
        KeyHit hoveredTrackKey = {};
        bool draggingTrackKey = false;
        float dragValueMin = 0.0f;
        float dragValueMax = 1.0f;
        // The row carried up or down the stack by its name cell, and where it would be dropped.
        bool draggingTrackRow = false;
        bool trackRowDragMoved = false;
        std::vector<uint16_t> carriedRows;
        int trackRowDragOriginY = 0;
        int trackRowDropIndex = -1;
        // The rows a drag can reorder, stacked in the order the last paint drew them.
        std::vector<uint16_t> reorderRowTargets;
        bool scrubbingTimeline = false;
        bool hoverFrames = false;
        bool hoverLoad = false;
        bool hoverSave = false;
        bool hoverExport = false;
        bool hoverAi = false;
        bool hoverTheme = false;
        bool hoverFullscreen = false;
        bool hoverPlay = false;
        bool hoverPause = false;
        bool hoverStop = false;
        bool hoverLoop = false;
        bool hoverZoomPreset = false;
        // Playback walks the schedule in real time and the preview follows wherever it lands.
        bool playing = false;
        bool loopPlayback = false;
        float playSeconds = 0.0f;
        ULONGLONG playTick = 0;
        bool draggingRuler = false;
        float rulerGrabDepth = 0.0f;
        // Set while the window covers a whole monitor with its frame taken off.
        bool fullscreen = false;
        bool exporting = false;
        std::stop_source exportStopSource;
        WINDOWPLACEMENT windowedPlacement = {};
        LONG_PTR windowedStyle = 0;
        // The three Cycle Length channels are edited as one row until this is turned off.
        bool linkColorCycle = true;
        bool lightMode = false;
        // The menu and the scene the Shader settings panels are built against, so the editor opens
        // those panels themselves rather than a second set of rows standing for them.
        SettingsMenu *settingsMenu = nullptr;
        RenderScene *renderScene = nullptr;
        // The Shader panels opened from this editor. An edit is recorded as a key only while one of
        // them is open, so the Shader menu's own panels go on editing the settings alone.
        std::vector<HWND> recordingPanels;
        // Every panel opened from this editor, which is drawn in this editor's colors rather than the app's.
        std::vector<HWND> themedPanels;
        // The shader as it stood when a panel last reported an edit, which the next one is read
        // against to find what a row has changed.
        ShaderAttribute recordBaseline = {};
        // The timeline as each committed change left it, and the states before and after the ones
        // taken back. Only the timeline is held: a shader setting a panel wrote is that panel's to
        // undo, and a track this takes back leaves the setting reading whatever the row shows.
        struct TimelineEdit {
            VidTimelineAttribute before, after;
            uint64_t serial;
            bool beforeStatic, afterStatic;
        };
        std::vector<TimelineEdit> undoSteps;
        std::vector<TimelineEdit> redoSteps;
        workspace::HistoryOrder historyOrder;
        bool dragHasUndoStep = false;
        std::function<void(bool)> historyRequest;
        // Set while a step is being put back, so restoring one is not itself recorded as a change.
        VidTimelineAttribute undoBaseline;
        bool undoBaselineStatic = false;
        ULONGLONG lastUndoStep = 0;
        HWND previewRenderWindow = nullptr;
        std::atomic<bool> previewContextAttached{false};
        std::unique_ptr<VideoFrameSource> frameSource;
        std::unique_ptr<VideoRenderScene> previewScene;
        float previewDepth = 0.0f;
        // The zoom each keyframe records in its own header, read once the playhead first reaches it.
        std::vector<std::optional<float>> keyframeLogZooms;
        std::wstring previewMessage = L"Select a keyframe folder to enable scrubbing";
        // A preview is outstanding, and whether it has lasted long enough to be worth saying so.
        bool previewPending = false;
        bool previewBusy = false;
        float previewBusyDepth = 0.0f;
        std::jthread previewWorker;
        std::atomic<bool> previewWorkerFailed{false};
        std::atomic<bool> cachePreloading{false};
        std::stop_source cacheStop;
        std::wstring cacheMessage;
        std::mutex previewRequestMutex;
        std::condition_variable previewRequestCondition;
        std::mutex previewBitmapMutex;
        uint64_t previewRequestGeneration = 0;
        uint64_t publishedPreviewGeneration = 0;
        float requestedPreviewDepth = 0.0f;
        float requestedPreviewSec = 0.0f;
        VidTimelineAttribute requestedPreviewTimeline = {};
        ShaderAttribute requestedPreviewShader = {};
        std::shared_ptr<const TimelineSchedule> requestedPreviewSchedule;

        TimelineWindow(SettingsMenu &menu, RenderScene &scene);

        ~TimelineWindow();

        TimelineWindow(const TimelineWindow &) = delete;

        TimelineWindow &operator=(const TimelineWindow &) = delete;

        [[nodiscard]] bool create(HWND owner);
        void rememberWorkspaceSource();

        void paint(HDC target, const RECT &client);
        void presentPaint(HDC target, const RECT &activeEditRect);

        [[nodiscard]] VidTimelineTrack *track(uint16_t targetId);

        [[nodiscard]] const VidTimelineTrack *track(uint16_t targetId) const;

        VidTimelineTrack &ensureScalarTrack(uint16_t targetId);

        void ensureEditableTracks();

        void removeTrack(uint16_t targetId);

        [[nodiscard]] float baseValue(uint16_t targetId) const;

        [[nodiscard]] std::pair<float, float> valueRange(uint16_t targetId) const;

        [[nodiscard]] const TrackLayout *layout(uint16_t targetId) const;

        // Brings the tracks onto an axis whose upper Depth has just changed, so no key is left above
        // the start with nowhere on the axis to be drawn or grabbed.
        void retargetTrackDepths(float previousStartDepth);

        void rebuildSchedule();

        void commitTimeline();

        void recordUndoStep();

        bool undoTimeline();

        bool redoTimeline();

        void applyRestoredTimeline(VidTimelineAttribute &&restored);

        [[nodiscard]] KeyHit hitTrackKey(POINT point) const;

        [[nodiscard]] uint16_t hitTrackRow(POINT point, bool editableOnly = true) const;

        [[nodiscard]] uint16_t hitTrackLabel(POINT point) const;

        // Where in the reorderable part of the stack the row being carried would be dropped.
        [[nodiscard]] int trackRowDropTarget(POINT point) const;

        // Puts the rows at that place in the stack, as one block in the order they stood in.
        void moveTrackRows(const std::vector<uint16_t> &targets, int dropIndex);

        // Picks the row: on its own, added to or dropped from the ones picked, or the run up to it.
        void selectTrackRow(uint16_t targetId, bool extend, bool range);

        [[nodiscard]] bool trackRowSelected(uint16_t targetId) const;

        void updateFieldDrag(POINT point);

        void beginFieldEdit(FieldEdit field);

        [[nodiscard]] bool commitFieldEdit();

        void closeFieldEdit();

        // The axis is labelled by how far the video has travelled, so it reads from 0 while the
        // keyframe files keep the depth they are named after.
        [[nodiscard]] float displayDistance(float depth) const;

        [[nodiscard]] float depthFromDistance(float distance) const;

        void updateTrackKey(POINT point);

        void addTrackKey(uint16_t targetId, POINT point);

        void deleteTrackKey();

        void setTrackInterpolation(VidKeyInterpolation interpolation);

        void openTrackKeyEditor();

        void loadTimeline();

        void saveTimeline() const;

        void openAiExchangeMenu();
        void importTimelineJson(const std::string &text, const std::filesystem::path &document = {});
        void requestAiImages();
        struct AiBundleRequest {
            std::filesystem::path parent;
            std::string prompt;
            VidTimelineAttribute timeline;
            ShaderAttribute shader;
            std::shared_ptr<const TimelineSchedule> mapping;
        };
        void saveAiBundle();
        cv::Mat renderAiImagePage(int side, uint32_t page, const VidTimelineAttribute &timeline,
                                 const ShaderAttribute &shader, const TimelineSchedule &mapping,
                                 uint64_t generation, const std::function<bool()> &cancelled);
        void renderAiImages(int side, uint32_t page, const VidTimelineAttribute &timeline,
                            const ShaderAttribute &shader, const TimelineSchedule &mapping,
                            uint64_t generation, std::stop_token stop,
                            const std::shared_ptr<const AiBundleRequest> &bundle);
        void finishAiImages(uint64_t generation);
        int aiImageSide = 2;
        uint32_t aiImagePage = 0;
        int requestedAiImageSide = 0;
        uint32_t requestedAiImagePage = 0;
        std::atomic<bool> aiImagesBusy{false};
        cv::Mat aiImageSheet;
        std::string aiImageError;
        uint64_t aiImageGeneration = 0;
        std::shared_ptr<const AiBundleRequest> requestedAiBundle;
        std::stop_source aiImagesCancel;
        std::filesystem::path aiSavedDirectory;
        bool aiSaveCancelled = false;
        std::atomic<uint32_t> aiSavedPages{0}, aiTotalPages{0};

        void openExportMenu();

        void openExportSettings();

        void exportVideo();

        void loadKeyframeDirectory();
        void loadKeyframeDirectory(const std::filesystem::path &directory);

        [[nodiscard]] bool initializeFramePreview();

        void destroyFramePreview();

        [[nodiscard]] bool createFramePreview(const Attribute &initialAttribute);

        [[nodiscard]] bool renderFramePreview(float depth, float sec, const VidTimelineAttribute &timeline,
                                              const ShaderAttribute &shader, const TimelineSchedule &timelineSchedule,
                                              uint64_t generation, cv::Mat *capture = nullptr);

        void startFramePreviewWorker();

        void processFramePreviewRequest(uint64_t &processedGeneration, std::stop_token stopToken, bool wait);

        void stopFramePreviewWorker();

        void requestFramePreview(float seconds = -1.0f);

        [[nodiscard]] float previewSeconds() const;

        void updateScrubDepth(POINT point);

        // Pulls the view after the playhead while it is dragged at or past an end of the axis.
        bool scrubEdgeScroll(POINT point);

        // Scrolls the stack while a row is carried to an end of it.
        bool rowEdgeScroll(POINT point);

        void syncPlaybackClock();

        [[nodiscard]] float keyframeLogZoom(uint32_t id);

        [[nodiscard]] float zoomExponentAt(float depth);

        [[nodiscard]] float viewSpan() const;

        [[nodiscard]] float viewDepthAt(int x) const;

        void resetView();

        void clampView();

        void zoomView(float pivotDepth, float scale);

        void panView(float depthDelta);

        void updateScrollThumb(POINT point);

        void pageScrollView(POINT point);

        void scrollTracks(int delta);

        void updateTrackScrollThumb(POINT point);

        void pageScrollTracks(POINT point);

        bool syncLinkedColorCycle(uint16_t sourceTarget);

        void toggleTheme();

        // Hands this editor's Light/Dark choice to the parameter panels it has opened.
        void applyPanelTheme() const;
        void refreshTheme();

        // Takes a freshly opened panel under this editor's colors, and keeps it for later switches.
        void adoptPanel(SettingsWindow &panel);

        void toggleFullscreen();

        void setPlaying(bool play);

        void stopPlayback();

        void advancePlayback();

        void setViewZoom(float factor);

        void openZoomMenu();
        void openControlsGuide();

        // The right-click menu of the track stack: keys, and the settings group whose panel to open.
        void openTrackMenu(POINT point);

        // Opens one of the Shader menu's own settings panels, with the rows this editor cannot
        // carry as a track left disabled.
        void openShaderPanel(size_t index);

        [[nodiscard]] bool recording() const;

        // Reads what a panel has just changed and puts it on the timeline as a key.
        void recordShaderEdits();

        // The value a parameter holds where the playhead stands, off its track or off the settings.
        [[nodiscard]] float playheadValue(uint16_t targetId) const;

        // Puts a key of this value on the parameter's track at the playhead, giving it a track first
        // when it has none.
        void setParameterValue(uint16_t targetId, float value);

        void setParameterColor(uint16_t targetId, const glm::vec4 &color);

      public:
        static void open(SettingsMenu &menu, RenderScene &scene, HWND owner);

        [[nodiscard]] static bool isOpen();
        static HWND createWorkspace(SettingsMenu &menu, RenderScene &scene, HWND parent,
                                    bool floating = false);
        static void applyWorkspaceDpi(HWND handle, UINT dpi);
        static void applyWorkspaceTheme(HWND handle);
        static void showWorkspace(HWND handle, const RECT &rect, bool visible);
        static void syncWorkspace(HWND handle);
        static bool workspaceHistory(HWND handle, bool redo, bool execute = false);
        static uint64_t workspaceHistoryOrder(HWND handle, bool redo);
        static void bindWorkspaceHistory(HWND handle, std::shared_ptr<workspace::HistoryDomain> domain,
                                         std::function<void(bool)> request);
        static void workspaceAction(HWND handle, int action);
        static bool loadWorkspaceKeyframes(HWND handle, const std::filesystem::path &directory);
        static bool workspacePreviewReady(HWND handle);
        static std::wstring workspaceStatus(HWND handle);

        static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

        static LRESULT CALLBACK fieldEditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam,
                                              UINT_PTR subclassId, DWORD_PTR referenceData);

      private:
        static LRESULT handleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    };
} // namespace merutilm::rff2
