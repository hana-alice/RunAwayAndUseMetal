#include "CameraInspector.h"

#include <cmath>
#include <utility>

#include <QAbstractSpinBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QSlider>
#include <QVBoxLayout>

namespace raum::sample {
namespace {

QLabel* makeLabel(const QString& text, const char* objectName, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setObjectName(objectName);
    return label;
}

void configureSpinBox(
    QDoubleSpinBox* spinBox,
    double minimum,
    double maximum,
    double step,
    int decimals,
    const QString& suffix = {}) {
    spinBox->setRange(minimum, maximum);
    spinBox->setSingleStep(step);
    spinBox->setDecimals(decimals);
    spinBox->setSuffix(suffix);
    spinBox->setKeyboardTracking(false);
    spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    spinBox->setAlignment(Qt::AlignRight);
    spinBox->setMinimumHeight(34);
}

} // namespace

CameraInspector::CameraInspector(QWidget* parent) : QWidget(parent) {
    setObjectName("cameraInspector");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    layout->addWidget(buildPositionCard());
    layout->addWidget(buildOrientationCard());
    layout->addWidget(buildLensCard());

    for (auto* editor : _position) {
        connect(editor, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this] {
            notifyChanged();
        });
    }
    connect(_yaw, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this] {
        notifyChanged();
    });
    connect(_pitch, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this] {
        notifyChanged();
    });
    connect(_fov, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        const QSignalBlocker blocker(_fovSlider);
        _fovSlider->setValue(static_cast<int>(std::lround(value)));
        notifyChanged();
    });
    connect(_fovSlider, &QSlider::valueChanged, this, [this](int value) {
        const QSignalBlocker blocker(_fov);
        _fov->setValue(static_cast<double>(value));
        notifyChanged();
    });
}

void CameraInspector::setChangeHandler(ChangeHandler handler) {
    _changeHandler = std::move(handler);
}

void CameraInspector::setState(const CameraControlState& state) {
    _syncing = true;
    const QSignalBlocker positionXBlocker(_position[0]);
    const QSignalBlocker positionYBlocker(_position[1]);
    const QSignalBlocker positionZBlocker(_position[2]);
    const QSignalBlocker yawBlocker(_yaw);
    const QSignalBlocker pitchBlocker(_pitch);
    const QSignalBlocker fovBlocker(_fov);
    const QSignalBlocker sliderBlocker(_fovSlider);

    _position[0]->setValue(state.position.x);
    _position[1]->setValue(state.position.y);
    _position[2]->setValue(state.position.z);
    _yaw->setValue(state.yawDegrees);
    _pitch->setValue(state.pitchDegrees);
    _fov->setValue(state.verticalFovDegrees);
    _fovSlider->setValue(static_cast<int>(std::lround(state.verticalFovDegrees)));
    _syncing = false;
}

CameraControlState CameraInspector::state() const {
    return CameraControlState{
        .position = Vec3f(
            static_cast<float>(_position[0]->value()),
            static_cast<float>(_position[1]->value()),
            static_cast<float>(_position[2]->value())),
        .yawDegrees = static_cast<float>(_yaw->value()),
        .pitchDegrees = static_cast<float>(_pitch->value()),
        .verticalFovDegrees = static_cast<float>(_fov->value()),
    };
}

bool CameraInspector::isEditing() const {
    for (const auto* editor : _position) {
        if (editor->hasFocus()) {
            return true;
        }
    }
    return _yaw->hasFocus() || _pitch->hasFocus() || _fov->hasFocus() || _fovSlider->hasFocus();
}

QFrame* CameraInspector::makeCard(const QString& title, const QString& description) {
    auto* card = new QFrame(this);
    card->setObjectName("inspectorCard");

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(14, 13, 14, 14);
    cardLayout->setSpacing(10);
    cardLayout->addWidget(makeLabel(title, "sectionTitle", card));
    cardLayout->addWidget(makeLabel(description, "sectionDescription", card));
    return card;
}

QWidget* CameraInspector::makeAxisEditor(
    const QString& axis,
    const QString& color,
    QDoubleSpinBox*& editor,
    QWidget* parent) {
    auto* column = new QWidget(parent);
    auto* layout = new QVBoxLayout(column);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    auto* axisLabel = makeLabel(axis, "axisLabel", column);
    axisLabel->setProperty("axisColor", color);
    editor = new QDoubleSpinBox(column);
    configureSpinBox(editor, -10000.0, 10000.0, 0.1, 2);

    layout->addWidget(axisLabel);
    layout->addWidget(editor);
    return column;
}

QWidget* CameraInspector::makeValueEditor(
    const QString& name,
    QDoubleSpinBox*& editor,
    double minimum,
    double maximum,
    double step,
    QWidget* parent) {
    auto* row = new QWidget(parent);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    layout->addWidget(makeLabel(name, "fieldLabel", row));
    layout->addStretch(1);
    editor = new QDoubleSpinBox(row);
    editor->setMinimumWidth(132);
    configureSpinBox(editor, minimum, maximum, step, 1, QStringLiteral(" deg"));
    layout->addWidget(editor);
    return row;
}

QFrame* CameraInspector::buildPositionCard() {
    auto* card = makeCard(QStringLiteral("POSITION"), QStringLiteral("World-space camera origin"));
    auto* cardLayout = qobject_cast<QVBoxLayout*>(card->layout());
    auto* editorRow = new QWidget(card);
    auto* rowLayout = new QHBoxLayout(editorRow);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(8);
    rowLayout->addWidget(makeAxisEditor(QStringLiteral("X"), QStringLiteral("x"), _position[0], editorRow));
    rowLayout->addWidget(makeAxisEditor(QStringLiteral("Y"), QStringLiteral("y"), _position[1], editorRow));
    rowLayout->addWidget(makeAxisEditor(QStringLiteral("Z"), QStringLiteral("z"), _position[2], editorRow));
    cardLayout->addWidget(editorRow);
    return card;
}

QFrame* CameraInspector::buildOrientationCard() {
    auto* card = makeCard(QStringLiteral("ORIENTATION"), QStringLiteral("Euler angles in degrees"));
    auto* cardLayout = qobject_cast<QVBoxLayout*>(card->layout());
    cardLayout->addWidget(makeValueEditor(QStringLiteral("Yaw"), _yaw, -360.0, 360.0, 1.0, card));
    cardLayout->addWidget(makeValueEditor(QStringLiteral("Pitch"), _pitch, -89.9, 89.9, 1.0, card));
    return card;
}

QFrame* CameraInspector::buildLensCard() {
    auto* card = makeCard(QStringLiteral("LENS"), QStringLiteral("Perspective projection"));
    auto* cardLayout = qobject_cast<QVBoxLayout*>(card->layout());
    cardLayout->addWidget(makeValueEditor(QStringLiteral("Field of view"), _fov, 30.0, 120.0, 1.0, card));

    _fovSlider = new QSlider(Qt::Horizontal, card);
    _fovSlider->setObjectName("fovSlider");
    _fovSlider->setRange(30, 120);
    _fovSlider->setSingleStep(1);
    _fovSlider->setPageStep(5);
    cardLayout->addWidget(_fovSlider);
    return card;
}

void CameraInspector::notifyChanged() {
    if (!_syncing && _changeHandler) {
        _changeHandler(state());
    }
}

} // namespace raum::sample
