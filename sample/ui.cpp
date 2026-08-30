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

constexpr int kInspectorWidth = 336;
constexpr int kUiRefreshIntervalMs = 100;

QLabel* makeLabel(const QString& text, const char* objectName, QWidget* parent = nullptr) {
    auto* label = new QLabel(text, parent);
    label->setObjectName(objectName);
    return label;
}

QFrame* makeSeparator(QWidget* parent = nullptr) {
    auto* separator = new QFrame(parent);
    separator->setObjectName("statusSeparator");
    separator->setFrameShape(QFrame::VLine);
    separator->setFixedHeight(16);
    return separator;
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
        _mainWindow->setWindowTitle(QStringLiteral("RAUM - Realtime Renderer"));
        _mainWindow->setMinimumSize(960, 640);
        _mainWindow->resize(1440, 900);

        auto* root = new QWidget(_mainWindow.get());
        root->setObjectName("workspaceRoot");
        auto* rootLayout = new QVBoxLayout(root);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(0);

        rootLayout->addWidget(buildTopBar(root));
        rootLayout->addWidget(buildWorkspace(root), 1);
        rootLayout->addWidget(buildStatusBar(root));
        _mainWindow->setCentralWidget(root);

        connectUi();
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
        _mainWindow->show();
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

    QWidget* buildTopBar(QWidget* parent) {
        auto* topBar = new QWidget(parent);
        topBar->setObjectName("topBar");
        topBar->setFixedHeight(64);
        auto* layout = new QHBoxLayout(topBar);
        layout->setContentsMargins(18, 0, 18, 0);
        layout->setSpacing(12);

        auto* brandMark = makeLabel(QStringLiteral("R"), "brandMark", topBar);
        brandMark->setAlignment(Qt::AlignCenter);
        brandMark->setFixedSize(34, 34);
        layout->addWidget(brandMark);

        auto* titleBlock = new QWidget(topBar);
        auto* titleLayout = new QVBoxLayout(titleBlock);
        titleLayout->setContentsMargins(0, 0, 0, 0);
        titleLayout->setSpacing(1);
        titleLayout->addWidget(makeLabel(QStringLiteral("RAUM"), "productTitle", titleBlock));
        titleLayout->addWidget(makeLabel(QStringLiteral("REALTIME RENDERER"), "productSubtitle", titleBlock));
        layout->addWidget(titleBlock);
        layout->addStretch(1);

        layout->addWidget(makeLabel(QStringLiteral("SCENE"), "toolbarLabel", topBar));
        _sampleSelector = new QComboBox(topBar);
        _sampleSelector->setObjectName("sampleSelector");
        _sampleSelector->setMinimumWidth(190);
        for (const auto& sample : _sample->samples()) {
            _sampleSelector->addItem(QString::fromStdString(sample->name()));
        }
        _sampleSelector->setCurrentIndex(static_cast<int>(_sample->currentSampleIndex()));
        layout->addWidget(_sampleSelector);

        auto* apiBadge = new QFrame(topBar);
        apiBadge->setObjectName("apiBadge");
        auto* apiLayout = new QHBoxLayout(apiBadge);
        apiLayout->setContentsMargins(10, 5, 10, 5);
        apiLayout->setSpacing(7);
        auto* apiDot = makeLabel(QStringLiteral(""), "apiDot", apiBadge);
        apiDot->setFixedSize(7, 7);
        apiLayout->addWidget(apiDot);
        apiLayout->addWidget(makeLabel(QStringLiteral("VULKAN"), "apiLabel", apiBadge));
        layout->addWidget(apiBadge);

        _resetCameraButton = new QPushButton(QStringLiteral("Reset camera"), topBar);
        _resetCameraButton->setObjectName("resetCameraButton");
        _resetCameraButton->setMinimumHeight(34);
        layout->addWidget(_resetCameraButton);
        return topBar;
    }

    QWidget* buildWorkspace(QWidget* parent) {
        auto* splitter = new QSplitter(Qt::Horizontal, parent);
        splitter->setObjectName("workspaceSplitter");
        splitter->setChildrenCollapsible(false);
        splitter->setHandleWidth(1);
        splitter->addWidget(buildViewport(splitter));
        splitter->addWidget(buildInspector(splitter));
        splitter->setStretchFactor(0, 1);
        splitter->setStretchFactor(1, 0);
        splitter->setSizes({1080, kInspectorWidth});
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
        header->setFixedHeight(42);
        auto* headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(16, 0, 14, 0);
        headerLayout->setSpacing(8);
        headerLayout->addWidget(makeLabel(QStringLiteral("VIEWPORT"), "panelTitle", header));
        headerLayout->addStretch(1);
        _viewportResolution = makeLabel(QStringLiteral("-- x --"), "resolutionBadge", header);
        headerLayout->addWidget(_viewportResolution);
        layout->addWidget(header);

        auto* canvas = new QWidget(viewport);
        canvas->setObjectName("viewportCanvas");
        auto* canvasLayout = new QVBoxLayout(canvas);
        canvasLayout->setContentsMargins(10, 10, 10, 10);
        canvasLayout->setSpacing(0);

        auto* engineWidget = static_cast<QWidget*>(_engineWindow->container());
        engineWidget->setObjectName("renderSurface");
        engineWidget->setMinimumSize(480, 270);
        engineWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        canvasLayout->addWidget(engineWidget);
        layout->addWidget(canvas, 1);
        return viewport;
    }

    QWidget* buildInspector(QWidget* parent) {
        auto* scrollArea = new QScrollArea(parent);
        scrollArea->setObjectName("inspectorScrollArea");
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setWidgetResizable(true);
        scrollArea->setMinimumWidth(kInspectorWidth);
        scrollArea->setMaximumWidth(420);

        auto* panel = new QWidget(scrollArea);
        panel->setObjectName("inspectorPanel");
        auto* layout = new QVBoxLayout(panel);
        layout->setContentsMargins(16, 16, 16, 18);
        layout->setSpacing(12);

        auto* headingRow = new QWidget(panel);
        auto* headingLayout = new QHBoxLayout(headingRow);
        headingLayout->setContentsMargins(0, 0, 0, 2);
        headingLayout->setSpacing(8);
        auto* headingText = new QWidget(headingRow);
        auto* headingTextLayout = new QVBoxLayout(headingText);
        headingTextLayout->setContentsMargins(0, 0, 0, 0);
        headingTextLayout->setSpacing(2);
        headingTextLayout->addWidget(makeLabel(QStringLiteral("CAMERA"), "inspectorTitle", headingText));
        headingTextLayout->addWidget(makeLabel(QStringLiteral("Transform and projection"), "inspectorSubtitle", headingText));
        headingLayout->addWidget(headingText);
        headingLayout->addStretch(1);
        auto* liveBadge = makeLabel(QStringLiteral("LIVE"), "liveBadge", headingRow);
        liveBadge->setAlignment(Qt::AlignCenter);
        headingLayout->addWidget(liveBadge);
        layout->addWidget(headingRow);

        _cameraInspector = new CameraInspector(panel);
        layout->addWidget(_cameraInspector);
        layout->addStretch(1);

        auto* help = new QFrame(panel);
        help->setObjectName("controlsHint");
        auto* helpLayout = new QVBoxLayout(help);
        helpLayout->setContentsMargins(12, 10, 12, 10);
        helpLayout->setSpacing(4);
        helpLayout->addWidget(makeLabel(QStringLiteral("VIEWPORT CONTROLS"), "hintTitle", help));
        helpLayout->addWidget(makeLabel(QStringLiteral("WASD to move  /  Drag to look"), "hintText", help));
        layout->addWidget(help);

        scrollArea->setWidget(panel);
        return scrollArea;
    }

    QWidget* buildStatusBar(QWidget* parent) {
        auto* statusBar = new QWidget(parent);
        statusBar->setObjectName("statusBar");
        statusBar->setFixedHeight(32);
        auto* layout = new QHBoxLayout(statusBar);
        layout->setContentsMargins(14, 0, 14, 0);
        layout->setSpacing(9);

        auto* runningDot = makeLabel(QStringLiteral(""), "runningDot", statusBar);
        runningDot->setFixedSize(7, 7);
        layout->addWidget(runningDot);
        layout->addWidget(makeLabel(QStringLiteral("RUNNING"), "runningText", statusBar));
        layout->addWidget(makeSeparator(statusBar));
        layout->addWidget(makeLabel(QStringLiteral("FPS"), "metricLabel", statusBar));
        _fpsValue = makeLabel(QStringLiteral("--"), "metricValue", statusBar);
        layout->addWidget(_fpsValue);
        layout->addWidget(makeSeparator(statusBar));
        layout->addWidget(makeLabel(QStringLiteral("FRAME"), "metricLabel", statusBar));
        _frameTimeValue = makeLabel(QStringLiteral("-- ms"), "metricValue", statusBar);
        layout->addWidget(_frameTimeValue);
        layout->addStretch(1);
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

    void syncCameraState(bool captureResetState) {
        auto* current = _sample->currentSample();
        const auto state = current ? current->cameraControlState() : std::nullopt;
        const bool available = state.has_value();
        _cameraInspector->setEnabled(available);
        _resetCameraButton->setEnabled(available);
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
        _fpsValue->setText(fps > 0.0f ? QString::number(fps, 'f', 0) : QStringLiteral("--"));
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
    QLabel* _viewportResolution{nullptr};
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
