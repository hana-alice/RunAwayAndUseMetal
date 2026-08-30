#include "ui.h"

#include <optional>

#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSplitter>
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

        _mainWindow = std::make_unique<QMainWindow>();
        _mainWindow->setObjectName("raumMainWindow");
        _mainWindow->setWindowTitle(QStringLiteral("Raum Renderer Lab"));
        _mainWindow->setMinimumSize(1024, 680);
        _mainWindow->resize(1480, 900);

        auto* root = new QWidget(_mainWindow.get());
        root->setObjectName("workspaceRoot");
        auto* rootLayout = new QVBoxLayout(root);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(0);
        rootLayout->addWidget(buildCommandBar(root));
        rootLayout->addWidget(buildWorkspace(root), 1);
        rootLayout->addWidget(buildStatusBar(root));
        _mainWindow->setCentralWidget(root);

        connectUi();
        updateSampleLabels();
        syncCameraState(true);
        updateTelemetry();

        _refreshTimer = new QTimer(_mainWindow.get());
        _refreshTimer->setInterval(kUiRefreshIntervalMs);
        QObject::connect(_refreshTimer, &QTimer::timeout, _mainWindow.get(), [this] {
            syncCameraState(false);
            updateTelemetry();
        });
        _refreshTimer->start();
    }

    ~Impl() {
        if (_refreshTimer) {
            _refreshTimer->stop();
        }
    }

    void show() {
        _mainWindow->showMaximized();
        enableDarkTitleBar(_mainWindow.get());
        _sample->showWindow();
    }

private:
    void loadStyleSheet() {
        QFile styleFile(QStringLiteral(":/raum/ui/style.qss"));
        if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qApp->setStyleSheet(QString::fromUtf8(styleFile.readAll()));
        }
    }

    QWidget* buildCommandBar(QWidget* parent) {
        auto* commandBar = new QWidget(parent);
        commandBar->setObjectName("commandBar");
        commandBar->setFixedHeight(50);
        auto* layout = new QHBoxLayout(commandBar);
        layout->setContentsMargins(14, 0, 14, 0);
        layout->setSpacing(10);

        auto* brandMark = makeLabel(QStringLiteral("R"), "brandMark", commandBar);
        brandMark->setAlignment(Qt::AlignCenter);
        brandMark->setFixedSize(26, 26);
        layout->addWidget(brandMark);
        layout->addWidget(makeLabel(QStringLiteral("Raum"), "productTitle", commandBar));
        layout->addWidget(makeLabel(QStringLiteral("Renderer Lab"), "workspaceName", commandBar));
        layout->addSpacing(3);
        layout->addWidget(makeVerticalDivider("commandDivider", commandBar));
        layout->addSpacing(3);

        layout->addWidget(makeLabel(QStringLiteral("Sample"), "commandLabel", commandBar));
        _sampleSelector = new QComboBox(commandBar);
        _sampleSelector->setObjectName("sampleSelector");
        _sampleSelector->setMinimumWidth(184);
        for (const auto& sample : _sample->samples()) {
            _sampleSelector->addItem(QString::fromStdString(sample->name()));
        }
        _sampleSelector->setCurrentIndex(static_cast<int>(_sample->currentSampleIndex()));
        layout->addWidget(_sampleSelector);
        layout->addStretch(1);

        auto* backend = new QWidget(commandBar);
        backend->setObjectName("backendStatus");
        auto* backendLayout = new QHBoxLayout(backend);
        backendLayout->setContentsMargins(0, 0, 0, 0);
        backendLayout->setSpacing(7);
        auto* backendDot = makeLabel({}, "backendDot", backend);
        backendDot->setFixedSize(6, 6);
        backendLayout->addWidget(backendDot);
        backendLayout->addWidget(makeLabel(QStringLiteral("Vulkan"), "backendText", backend));
        layout->addWidget(backend);
        layout->addWidget(makeVerticalDivider("commandDivider", commandBar));

        _resetCameraButton = new QPushButton(QStringLiteral("Reset camera"), commandBar);
        _resetCameraButton->setObjectName("resetCameraButton");
        _resetCameraButton->setFixedHeight(30);
        layout->addWidget(_resetCameraButton);
        return commandBar;
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
        _viewportTitle = makeLabel({}, "viewportSample", header);
        headerLayout->addWidget(_viewportTitle);
        headerLayout->addStretch(1);
        headerLayout->addWidget(makeLabel(QStringLiteral("Perspective"), "viewMode", header));
        _viewportResolution = makeLabel(QStringLiteral("-- x --"), "resolutionLabel", header);
        headerLayout->addWidget(_viewportResolution);
        layout->addWidget(header);

        auto* engineWidget = static_cast<QWidget*>(_engineWindow->container());
        engineWidget->setObjectName("renderSurface");
        engineWidget->setMinimumSize(480, 270);
        engineWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        layout->addWidget(engineWidget, 1);
        return viewport;
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
        headingLayout->addWidget(makeLabel(QStringLiteral("Camera"), "inspectorTitle", heading));
        headingLayout->addWidget(makeLabel(QStringLiteral("Active view properties"), "inspectorSubtitle", heading));
        headerLayout->addWidget(heading);
        headerLayout->addStretch(1);

        _cameraStatusDot = makeLabel({}, "cameraStatusDot", header);
        _cameraStatusDot->setFixedSize(6, 6);
        headerLayout->addWidget(_cameraStatusDot);
        _cameraStatusText = makeLabel(QStringLiteral("Live"), "cameraStatusText", header);
        headerLayout->addWidget(_cameraStatusText);
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
        layout->addWidget(makeLabel(QStringLiteral("Renderer ready"), "statusPrimary", statusBar));
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
        QObject::connect(
            _sampleSelector,
            qOverload<int>(&QComboBox::currentIndexChanged),
            _mainWindow.get(),
            [this](int index) {
                if (index < 0 || !_sample->changeSample(static_cast<uint32_t>(index))) {
                    return;
                }
                _resetCameraState.reset();
                updateSampleLabels();
                syncCameraState(true);
            });

        QObject::connect(_resetCameraButton, &QPushButton::clicked, _mainWindow.get(), [this] {
            auto* current = _sample->currentSample();
            if (!current || !_resetCameraState) {
                return;
            }
            current->applyCameraControlState(*_resetCameraState);
            _cameraInspector->setState(*_resetCameraState);
        });

        _cameraInspector->setChangeHandler([this](const CameraControlState& state) {
            if (auto* current = _sample->currentSample()) {
                current->applyCameraControlState(state);
            }
        });
    }

    void updateSampleLabels() {
        const auto* current = _sample->currentSample();
        _viewportTitle->setText(current ? QString::fromStdString(current->name()) : QStringLiteral("No sample"));
    }

    void syncCameraState(bool captureResetState) {
        auto* current = _sample->currentSample();
        const auto state = current ? current->cameraControlState() : std::nullopt;
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
            return;
        }

        if (captureResetState || !_resetCameraState) {
            _resetCameraState = state;
        }
        if (!_cameraInspector->isEditing()) {
            _cameraInspector->setState(*state);
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
    std::unique_ptr<QMainWindow> _mainWindow;
    std::unique_ptr<::raum::Sample> _sample;
    platform::WindowPtr _engineWindow;
    QComboBox* _sampleSelector{nullptr};
    QPushButton* _resetCameraButton{nullptr};
    CameraInspector* _cameraInspector{nullptr};
    QLabel* _viewportTitle{nullptr};
    QLabel* _viewportResolution{nullptr};
    QLabel* _cameraStatusDot{nullptr};
    QLabel* _cameraStatusText{nullptr};
    QLabel* _fpsValue{nullptr};
    QLabel* _frameTimeValue{nullptr};
    QLabel* _statusResolution{nullptr};
    QTimer* _refreshTimer{nullptr};
    std::optional<CameraControlState> _resetCameraState;
};

UI::UI(int argc, char** argv) : _impl(std::make_unique<Impl>(argc, argv)) {}

UI::~UI() = default;

void UI::show() {
    _impl->show();
}

} // namespace raum::sample
