#include "ui.h"

#include <algorithm>
#include <exception>
#include <functional>
#include <optional>
#include <utility>

#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyle>
#include <QStyleFactory>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "sample.h"
#include "widgets/CameraInspector.h"

#if defined(_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <Windows.h>
    #include <dwmapi.h>
#endif

namespace raum::sample {
namespace {

constexpr int kInspectorWidth = 320;
constexpr int kUiRefreshIntervalMs = 100;

class MainWindow final : public QMainWindow {
public:
    using CloseHandler = std::function<void()>;

    void setCloseHandler(CloseHandler handler) {
        _closeHandler = std::move(handler);
    }

protected:
    void closeEvent(QCloseEvent* event) override {
        if (_closeHandler) {
            _closeHandler();
        }
        QMainWindow::closeEvent(event);
    }

private:
    CloseHandler _closeHandler;
};

class LoadingSpinner final : public QWidget {
public:
    explicit LoadingSpinner(QWidget* parent = nullptr) : QWidget(parent) {
        setFixedSize(42, 42);
        _timer.setInterval(24);
        QObject::connect(&_timer, &QTimer::timeout, this, [this] {
            _angle = (_angle + 8) % 360;
            update();
        });
        _timer.start();
    }

    void setRunning(bool running) {
        if (running) {
            _timer.start();
        } else {
            _timer.stop();
        }
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QRectF bounds = rect().adjusted(4, 4, -4, -4);

        QPen trackPen(QColor(QStringLiteral("#252a33")), 3.0, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(trackPen);
        painter.drawArc(bounds, 0, 360 * 16);

        QPen progressPen(QColor(QStringLiteral("#69a0ff")), 3.0, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(progressPen);
        painter.drawArc(bounds, (90 - _angle) * 16, -105 * 16);
    }

private:
    QTimer _timer;
    int _angle{0};
};

QLabel* makeLabel(const QString& text, const char* objectName, QWidget* parent = nullptr) {
    auto* label = new QLabel(text, parent);
    label->setObjectName(objectName);
    return label;
}

QFrame* makeVerticalDivider(const char* objectName, QWidget* parent) {
    auto* divider = new QFrame(parent);
    divider->setObjectName(objectName);
    divider->setFrameShape(QFrame::VLine);
    divider->setFixedSize(1, 16);
    return divider;
}

void refreshStyle(QWidget* widget) {
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

void enableDarkTitleBar(QWidget* window) {
#if defined(_WIN32)
    const BOOL enabled = TRUE;
    constexpr DWORD kImmersiveDarkMode = 20;
    constexpr DWORD kImmersiveDarkModeBefore20H1 = 19;
    const auto handle = reinterpret_cast<HWND>(window->winId());
    if (FAILED(DwmSetWindowAttribute(handle, kImmersiveDarkMode, &enabled, sizeof(enabled)))) {
        DwmSetWindowAttribute(handle, kImmersiveDarkModeBefore20H1, &enabled, sizeof(enabled));
    }
#else
    Q_UNUSED(window);
#endif
}

} // namespace

struct UI::Impl {
    explicit Impl(int argc, char** argv) {
        QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
        loadStyleSheet();

        _sample = std::make_unique<::raum::Sample>(argc, argv);
        _engineWindow = _sample->window();

        _mainWindow = std::make_unique<MainWindow>();
        _mainWindow->setCloseHandler([this] {
            shutdown();
        });
        _mainWindow->setObjectName("raumMainWindow");
        _mainWindow->setWindowTitle(QStringLiteral("Raum Renderer Lab"));
        _mainWindow->setMinimumSize(1024, 680);
        _mainWindow->resize(1480, 900);

        auto* root = new QWidget(_mainWindow.get());
        root->setObjectName("workspaceRoot");
        auto* rootLayout = new QVBoxLayout(root);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(0);
        rootLayout->addWidget(buildWorkspace(root), 1);
        rootLayout->addWidget(buildStatusBar(root));
        _mainWindow->setCentralWidget(root);

        connectUi();
        updateSampleLabels();
        updateSampleNavigation();
        _cameraInspector->setEnabled(false);
        _resetCameraButton->setEnabled(false);
        updateTelemetry();

        _refreshTimer = new QTimer(_mainWindow.get());
        _refreshTimer->setInterval(kUiRefreshIntervalMs);
        QObject::connect(_refreshTimer, &QTimer::timeout, _mainWindow.get(), [this] {
            updateLoadingUi();
            updateTelemetry();
        });
        _refreshTimer->start();
    }

    ~Impl() {
        shutdown();
    }

    void shutdown() {
        if (_shuttingDown) {
            return;
        }
        _shuttingDown = true;
        if (_refreshTimer) {
            _refreshTimer->stop();
        }
        if (_cameraInspector) {
            _cameraInspector->setChangeHandler({});
            _cameraInspector->setLightingChangeHandler({});
        }
        if (_sample) {
            _sample->shutdown();
        }
    }

    void show() {
        _mainWindow->showMaximized();
        enableDarkTitleBar(_mainWindow.get());
        _uiShown = true;
    }

private:
    void loadStyleSheet() {
        QFile styleFile(QStringLiteral(":/raum/ui/style.qss"));
        if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qApp->setStyleSheet(QString::fromUtf8(styleFile.readAll()));
        }
    }

    QWidget* buildWorkspace(QWidget* parent) {
        auto* splitter = new QSplitter(Qt::Horizontal, parent);
        splitter->setObjectName("workspaceSplitter");
        splitter->setChildrenCollapsible(false);
        splitter->setHandleWidth(3);
        splitter->addWidget(buildViewport(splitter));
        splitter->addWidget(buildInspector(splitter));
        splitter->setStretchFactor(0, 1);
        splitter->setStretchFactor(1, 0);
        splitter->setSizes({1120, kInspectorWidth});
        return splitter;
    }

    QWidget* buildViewport(QWidget* parent) {
        auto* viewport = new QWidget(parent);
        viewport->setObjectName("viewportPanel");
        auto* layout = new QVBoxLayout(viewport);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        auto* header = new QWidget(viewport);
        header->setObjectName("viewportHeader");
        header->setFixedHeight(38);
        auto* headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(12, 0, 12, 0);
        headerLayout->setSpacing(8);
        auto* viewportDot = makeLabel({}, "viewportDot", header);
        viewportDot->setFixedSize(7, 7);
        headerLayout->addWidget(viewportDot);
        headerLayout->addWidget(makeLabel(QStringLiteral("Viewport"), "panelTitle", header));
        headerLayout->addWidget(makeVerticalDivider("panelDivider", header));

        _previousSampleButton = new QPushButton(header);
        _previousSampleButton->setObjectName("sampleNavigationButton");
        _previousSampleButton->setIcon(header->style()->standardIcon(QStyle::SP_ArrowBack));
        _previousSampleButton->setToolTip(QStringLiteral("Previous sample"));
        _previousSampleButton->setAccessibleName(QStringLiteral("Previous sample"));
        _previousSampleButton->setFixedSize(26, 26);
        headerLayout->addWidget(_previousSampleButton);

        _viewportTitle = makeLabel({}, "viewportSample", header);
        headerLayout->addWidget(_viewportTitle);
        if (_sample->samples().size() > 1) {
            _viewportTitle->hide();
            _sampleSelector = new QComboBox(header);
            _sampleSelector->setObjectName("sampleSelector");
            _sampleSelector->setMinimumWidth(160);
            for (const auto& sample : _sample->samples()) {
                _sampleSelector->addItem(QString::fromStdString(sample->name()));
            }
            _sampleSelector->setCurrentIndex(static_cast<int>(_sample->currentSampleIndex()));
            headerLayout->addWidget(_sampleSelector);
        }

        _nextSampleButton = new QPushButton(header);
        _nextSampleButton->setObjectName("sampleNavigationButton");
        _nextSampleButton->setIcon(header->style()->standardIcon(QStyle::SP_ArrowForward));
        _nextSampleButton->setToolTip(QStringLiteral("Next sample"));
        _nextSampleButton->setAccessibleName(QStringLiteral("Next sample"));
        _nextSampleButton->setFixedSize(26, 26);
        headerLayout->addWidget(_nextSampleButton);

        headerLayout->addStretch(1);
        headerLayout->addWidget(makeLabel(QStringLiteral("Perspective"), "viewMode", header));
        _viewportResolution = makeLabel(QStringLiteral("-- x --"), "resolutionLabel", header);
        headerLayout->addWidget(_viewportResolution);
        layout->addWidget(header);

        _viewportStack = new QStackedWidget(viewport);
        _viewportStack->setObjectName("viewportStack");
        _loadingPage = buildLoadingPage(_viewportStack);
        _viewportStack->addWidget(_loadingPage);
        _viewportStack->setCurrentWidget(_loadingPage);
        layout->addWidget(_viewportStack, 1);
        return viewport;
    }

    QWidget* buildLoadingPage(QWidget* parent) {
        auto* page = new QWidget(parent);
        page->setObjectName("loadingPage");
        auto* pageLayout = new QVBoxLayout(page);
        pageLayout->setContentsMargins(32, 32, 32, 32);
        pageLayout->addStretch(1);

        auto* card = new QFrame(page);
        card->setObjectName("loadingCard");
        card->setFixedWidth(380);
        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(30, 28, 30, 26);
        cardLayout->setSpacing(0);

        _loadingSpinner = new LoadingSpinner(card);
        cardLayout->addWidget(_loadingSpinner, 0, Qt::AlignHCenter);
        cardLayout->addSpacing(18);
        _loadingTitle = makeLabel(QStringLiteral("Loading scene"), "loadingTitle", card);
        _loadingTitle->setAlignment(Qt::AlignCenter);
        cardLayout->addWidget(_loadingTitle);
        cardLayout->addSpacing(7);
        _loadingMessage = makeLabel(QStringLiteral("Preparing scene resources"), "loadingMessage", card);
        _loadingMessage->setAlignment(Qt::AlignCenter);
        _loadingMessage->setWordWrap(true);
        cardLayout->addWidget(_loadingMessage);
        cardLayout->addSpacing(20);

        auto* progressRow = new QWidget(card);
        progressRow->setObjectName("loadingProgressRow");
        auto* progressLayout = new QHBoxLayout(progressRow);
        progressLayout->setContentsMargins(0, 0, 0, 0);
        progressLayout->setSpacing(12);
        _loadingProgress = new QProgressBar(progressRow);
        _loadingProgress->setObjectName("loadingProgress");
        _loadingProgress->setRange(0, 100);
        _loadingProgress->setTextVisible(false);
        progressLayout->addWidget(_loadingProgress, 1);
        _loadingPercent = makeLabel(QStringLiteral("0%"), "loadingPercent", progressRow);
        _loadingPercent->setFixedWidth(38);
        _loadingPercent->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        progressLayout->addWidget(_loadingPercent);
        cardLayout->addWidget(progressRow);

        pageLayout->addWidget(card, 0, Qt::AlignHCenter);
        pageLayout->addStretch(1);
        return page;
    }

    void prepareRenderSurface() {
        if (_renderSurface) {
            return;
        }
        _renderSurface = static_cast<QWidget*>(_engineWindow->container());
        _renderSurface->setObjectName("renderSurface");
        _renderSurface->setMinimumSize(480, 270);
        _renderSurface->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        _viewportStack->addWidget(_renderSurface);
    }

    QWidget* buildInspector(QWidget* parent) {
        auto* dock = new QWidget(parent);
        dock->setObjectName("inspectorDock");
        dock->setMinimumWidth(kInspectorWidth);
        dock->setMaximumWidth(380);
        auto* dockLayout = new QVBoxLayout(dock);
        dockLayout->setContentsMargins(0, 0, 0, 0);
        dockLayout->setSpacing(0);

        auto* header = new QWidget(dock);
        header->setObjectName("inspectorHeader");
        header->setFixedHeight(58);
        auto* headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(16, 0, 14, 0);
        headerLayout->setSpacing(8);

        auto* heading = new QWidget(header);
        auto* headingLayout = new QVBoxLayout(heading);
        headingLayout->setContentsMargins(0, 0, 0, 0);
        headingLayout->setSpacing(1);
        headingLayout->addWidget(makeLabel(QStringLiteral("Scene"), "inspectorTitle", heading));
        headingLayout->addWidget(makeLabel(QStringLiteral("Camera and directional light"), "inspectorSubtitle", heading));
        headerLayout->addWidget(heading);
        headerLayout->addStretch(1);

        _cameraStatusDot = makeLabel({}, "cameraStatusDot", header);
        _cameraStatusDot->setFixedSize(6, 6);
        headerLayout->addWidget(_cameraStatusDot);
        _cameraStatusText = makeLabel(QStringLiteral("Live"), "cameraStatusText", header);
        headerLayout->addWidget(_cameraStatusText);
        _resetCameraButton = new QPushButton(QStringLiteral("Reset"), header);
        _resetCameraButton->setObjectName("resetCameraButton");
        _resetCameraButton->setFixedHeight(26);
        headerLayout->addWidget(_resetCameraButton);
        dockLayout->addWidget(header);

        auto* scrollArea = new QScrollArea(dock);
        scrollArea->setObjectName("inspectorScrollArea");
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setWidgetResizable(true);

        auto* panel = new QWidget(scrollArea);
        panel->setObjectName("inspectorPanel");
        auto* panelLayout = new QVBoxLayout(panel);
        panelLayout->setContentsMargins(0, 0, 0, 0);
        panelLayout->setSpacing(0);
        _cameraInspector = new CameraInspector(panel);
        panelLayout->addWidget(_cameraInspector);
        panelLayout->addStretch(1);
        scrollArea->setWidget(panel);
        dockLayout->addWidget(scrollArea, 1);
        dockLayout->addWidget(buildShortcutPanel(dock));
        return dock;
    }

    QWidget* buildShortcutPanel(QWidget* parent) {
        auto* panel = new QWidget(parent);
        panel->setObjectName("shortcutPanel");
        auto* layout = new QVBoxLayout(panel);
        layout->setContentsMargins(16, 12, 16, 13);
        layout->setSpacing(7);
        layout->addWidget(makeLabel(QStringLiteral("Viewport controls"), "shortcutTitle", panel));

        const auto addShortcut = [layout, panel](const QString& key, const QString& action) {
            auto* row = new QWidget(panel);
            auto* rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            rowLayout->setSpacing(8);
            auto* keyLabel = makeLabel(key, "keycap", row);
            keyLabel->setAlignment(Qt::AlignCenter);
            keyLabel->setMinimumWidth(58);
            keyLabel->setFixedHeight(21);
            rowLayout->addWidget(keyLabel);
            rowLayout->addWidget(makeLabel(action, "shortcutAction", row));
            rowLayout->addStretch(1);
            layout->addWidget(row);
        };
        addShortcut(QStringLiteral("W A S D"), QStringLiteral("Move camera"));
        addShortcut(QStringLiteral("Drag"), QStringLiteral("Look around"));
        return panel;
    }

    QWidget* buildStatusBar(QWidget* parent) {
        auto* statusBar = new QWidget(parent);
        statusBar->setObjectName("statusBar");
        statusBar->setFixedHeight(28);
        auto* layout = new QHBoxLayout(statusBar);
        layout->setContentsMargins(12, 0, 12, 0);
        layout->setSpacing(8);

        auto* runningDot = makeLabel({}, "runningDot", statusBar);
        runningDot->setFixedSize(6, 6);
        layout->addWidget(runningDot);
        _statusPrimary = makeLabel(QStringLiteral("Loading scene"), "statusPrimary", statusBar);
        layout->addWidget(_statusPrimary);
        layout->addWidget(makeVerticalDivider("statusDivider", statusBar));
        layout->addWidget(makeLabel(QStringLiteral("Vulkan"), "statusSecondary", statusBar));
        layout->addStretch(1);

        _fpsValue = makeLabel(QStringLiteral("-- FPS"), "metricValue", statusBar);
        layout->addWidget(_fpsValue);
        layout->addWidget(makeVerticalDivider("statusDivider", statusBar));
        _frameTimeValue = makeLabel(QStringLiteral("-- ms"), "metricValue", statusBar);
        layout->addWidget(_frameTimeValue);
        layout->addWidget(makeVerticalDivider("statusDivider", statusBar));
        _statusResolution = makeLabel(QStringLiteral("-- x --"), "statusResolution", statusBar);
        layout->addWidget(_statusResolution);
        return statusBar;
    }

    void connectUi() {
        if (_sampleSelector) {
            QObject::connect(
                _sampleSelector,
                qOverload<int>(&QComboBox::currentIndexChanged),
                _mainWindow.get(),
                [this](int index) {
                    if (index < 0) {
                        return;
                    }
                    selectSample(static_cast<uint32_t>(index));
                },
                Qt::QueuedConnection);
        }

        QObject::connect(
            _previousSampleButton,
            &QPushButton::clicked,
            _mainWindow.get(),
            [this] {
                const auto index = _sample->currentSampleIndex();
                if (index > 0) {
                    selectSample(index - 1);
                }
            },
            Qt::QueuedConnection);
        QObject::connect(
            _nextSampleButton,
            &QPushButton::clicked,
            _mainWindow.get(),
            [this] {
                const auto index = _sample->currentSampleIndex();
                if (index + 1 < _sample->samples().size()) {
                    selectSample(index + 1);
                }
            },
            Qt::QueuedConnection);

        QObject::connect(_resetCameraButton, &QPushButton::clicked, _mainWindow.get(), [this] {
            auto* current = _sample->currentSample();
            if (!current || !_resetCameraState) {
                return;
            }
            current->applyCameraControlState(*_resetCameraState);
            _cameraInspector->setState(*_resetCameraState);
            if (_resetLightingState) {
                current->applyLightingControlState(*_resetLightingState);
                _cameraInspector->setLightingState(*_resetLightingState);
            }
        });

        _cameraInspector->setChangeHandler([this](const CameraControlState& state) {
            if (auto* current = _sample->currentSample()) {
                current->applyCameraControlState(state);
            }
        });
        _cameraInspector->setLightingChangeHandler([this](const LightingControlState& state) {
            if (auto* current = _sample->currentSample()) {
                current->applyLightingControlState(state);
            }
        });
    }

    void updateSampleLabels() {
        const auto* current = _sample->currentSample();
        _viewportTitle->setText(current ? QString::fromStdString(current->name()) : QStringLiteral("No sample"));
    }

    void selectSample(uint32_t index) {
        if (!_sample->ready() || index >= _sample->samples().size()) {
            updateSampleNavigation();
            return;
        }
        if (index == _sample->currentSampleIndex()) {
            updateSampleNavigation();
            return;
        }

        try {
            if (!_sample->changeSample(index)) {
                updateSampleNavigation();
                return;
            }
        } catch (const std::exception& error) {
            _statusPrimary->setText(
                QStringLiteral("Sample switch failed · %1").arg(QString::fromUtf8(error.what())));
            updateSampleNavigation();
            return;
        }

        _pendingSampleIndex = index;
        _resetCameraState.reset();
        _resetLightingState.reset();
        _cameraInspector->setEnabled(false);
        _resetCameraButton->setEnabled(false);
        _statusPrimary->setText(
            QStringLiteral("Switching to %1").arg(
                QString::fromStdString(_sample->samples()[index]->name())));
        updateSampleNavigation();
    }

    void finishPendingSampleChange() {
        if (!_pendingSampleIndex || _sample->sampleSwitchPending()) {
            return;
        }

        const auto targetIndex = *_pendingSampleIndex;
        _pendingSampleIndex.reset();
        if (_sample->currentSampleIndex() != targetIndex) {
            const auto error = _sample->sampleSwitchError();
            _statusPrimary->setText(
                error.empty()
                    ? QStringLiteral("Sample switch failed")
                    : QStringLiteral("Sample switch failed · %1").arg(QString::fromStdString(error)));
            updateSampleNavigation();
            syncCameraState(false);
            return;
        }

        updateSampleLabels();
        updateSampleNavigation();
        syncCameraState(true);
        _statusPrimary->setText(QStringLiteral("Renderer ready"));
    }

    void updateSampleNavigation() {
        const auto count = _sample->samples().size();
        const auto index = _pendingSampleIndex.value_or(_sample->currentSampleIndex());
        const bool ready = _sample->ready() && !_pendingSampleIndex;
        _previousSampleButton->setEnabled(ready && index > 0);
        _nextSampleButton->setEnabled(ready && index + 1 < count);
        if (_sampleSelector) {
            const QSignalBlocker blocker(_sampleSelector);
            _sampleSelector->setCurrentIndex(static_cast<int>(index));
            _sampleSelector->setEnabled(ready && count > 1);
        }
    }

    void updatePendingSampleChange() {
        finishPendingSampleChange();
    }

    void syncCameraState(bool captureResetState) {
        if (!_sample->ready()) {
            _cameraInspector->setEnabled(false);
            _resetCameraButton->setEnabled(false);
            return;
        }
        auto* current = _sample->currentSample();
        const auto state = current ? current->cameraControlState() : std::nullopt;
        const auto lightingState = current ? current->lightingControlState() : std::nullopt;
        const bool available = state.has_value();
        _cameraInspector->setEnabled(available);
        _resetCameraButton->setEnabled(available);
        _cameraStatusDot->setProperty("active", available);
        _cameraStatusText->setProperty("active", available);
        _cameraStatusText->setText(available ? QStringLiteral("Live") : QStringLiteral("Unavailable"));
        refreshStyle(_cameraStatusDot);
        refreshStyle(_cameraStatusText);
        if (!available) {
            _resetCameraState.reset();
            _resetLightingState.reset();
            return;
        }

        if (captureResetState || !_resetCameraState) {
            _resetCameraState = state;
        }
        if (lightingState && (captureResetState || !_resetLightingState)) {
            _resetLightingState = lightingState;
        }
        if (!_cameraInspector->isEditing()) {
            _cameraInspector->setState(*state);
            if (lightingState) {
                _cameraInspector->setLightingState(*lightingState);
            }
        }
    }

    void updateLoadingUi() {
        _sample->pollLoading();
        const auto status = _sample->loadingStatus();

        if (status.state == ::raum::Sample::LoadingState::Failed) {
            _loadingSpinner->setRunning(false);
            _loadingTitle->setText(QStringLiteral("Scene loading failed"));
            _loadingMessage->setText(QString::fromStdString(status.message));
            _loadingProgress->setValue(0);
            _loadingProgress->setProperty("failed", true);
            refreshStyle(_loadingProgress);
            _loadingPercent->setText(QStringLiteral("Error"));
            updateSampleNavigation();
            _cameraInspector->setEnabled(false);
            _resetCameraButton->setEnabled(false);
            _statusPrimary->setText(QStringLiteral("Scene loading failed"));
            return;
        }

        if (status.state != ::raum::Sample::LoadingState::Ready || !_sample->readyForDisplay()) {
            _viewportStack->setCurrentWidget(_loadingPage);
            _loadingSpinner->setRunning(true);
            _loadingProgress->setProperty("failed", false);
            refreshStyle(_loadingProgress);
            const int percent = std::clamp(static_cast<int>(status.progress * 100.0f), 0, 100);
            _loadingProgress->setValue(percent);
            _loadingPercent->setText(QStringLiteral("%1%").arg(percent));
            _loadingTitle->setText(
                status.state == ::raum::Sample::LoadingState::Ready
                    ? QStringLiteral("Preparing viewport")
                    : QStringLiteral("Loading Sponza scene"));
            _loadingMessage->setText(QString::fromStdString(status.message));
            updateSampleNavigation();
            _cameraInspector->setEnabled(false);
            _resetCameraButton->setEnabled(false);
            _statusPrimary->setText(QStringLiteral("Loading scene · %1%").arg(percent));

            if (_uiShown && status.state == ::raum::Sample::LoadingState::Ready && !_engineStarted) {
                _engineStarted = true;
                prepareRenderSurface();
                _sample->showWindow();
            }
            return;
        }

        updatePendingSampleChange();

        if (!_readyUiInitialized) {
            _readyUiInitialized = true;
            _loadingSpinner->setRunning(false);
            _viewportStack->setCurrentWidget(_renderSurface);
            updateSampleNavigation();
            _statusPrimary->setText(QStringLiteral("Renderer ready"));
            syncCameraState(true);
        } else {
            syncCameraState(false);
        }
    }

    void updateTelemetry() {
        const float fps = _sample->getFps();
        _fpsValue->setText(fps > 0.0f ? QStringLiteral("%1 FPS").arg(fps, 0, 'f', 0) : QStringLiteral("-- FPS"));
        _frameTimeValue->setText(
            fps > 0.0f ? QStringLiteral("%1 ms").arg(1000.0f / fps, 0, 'f', 2) : QStringLiteral("-- ms"));

        const auto size = _engineWindow->size();
        const auto resolution = QStringLiteral("%1 x %2").arg(size.width).arg(size.height);
        _viewportResolution->setText(resolution);
        _statusResolution->setText(resolution);
    }

    // Keep the host window alive longer than Sample: Sample owns renderer state used
    // by the embedded render surface during its teardown.
    std::unique_ptr<MainWindow> _mainWindow;
    std::unique_ptr<::raum::Sample> _sample;
    platform::WindowPtr _engineWindow;
    QComboBox* _sampleSelector{nullptr};
    QPushButton* _previousSampleButton{nullptr};
    QPushButton* _nextSampleButton{nullptr};
    QPushButton* _resetCameraButton{nullptr};
    CameraInspector* _cameraInspector{nullptr};
    QStackedWidget* _viewportStack{nullptr};
    QWidget* _renderSurface{nullptr};
    QWidget* _loadingPage{nullptr};
    LoadingSpinner* _loadingSpinner{nullptr};
    QLabel* _loadingTitle{nullptr};
    QLabel* _loadingMessage{nullptr};
    QProgressBar* _loadingProgress{nullptr};
    QLabel* _loadingPercent{nullptr};
    QLabel* _viewportTitle{nullptr};
    QLabel* _viewportResolution{nullptr};
    QLabel* _cameraStatusDot{nullptr};
    QLabel* _cameraStatusText{nullptr};
    QLabel* _fpsValue{nullptr};
    QLabel* _frameTimeValue{nullptr};
    QLabel* _statusResolution{nullptr};
    QLabel* _statusPrimary{nullptr};
    QTimer* _refreshTimer{nullptr};
    std::optional<CameraControlState> _resetCameraState;
    std::optional<LightingControlState> _resetLightingState;
    std::optional<uint32_t> _pendingSampleIndex;
    bool _shuttingDown{false};
    bool _engineStarted{false};
    bool _readyUiInitialized{false};
    bool _uiShown{false};
};

UI::UI(int argc, char** argv) : _impl(std::make_unique<Impl>(argc, argv)) {}

UI::~UI() = default;

void UI::show() {
    _impl->show();
}

} // namespace raum::sample
