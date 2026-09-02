#include "CameraInspector.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <QAbstractSpinBox>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
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
    const QString& suffix) {
    spinBox->setRange(minimum, maximum);
    spinBox->setSingleStep(step);
    spinBox->setDecimals(decimals);
    spinBox->setSuffix(suffix);
    spinBox->setKeyboardTracking(false);
    spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    spinBox->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    spinBox->setFixedHeight(28);
    spinBox->setMinimumWidth(116);
}

QFrame* makeDivider(QWidget* parent) {
    auto* divider = new QFrame(parent);
    divider->setObjectName("propertyDivider");
    divider->setFrameShape(QFrame::HLine);
    divider->setFixedHeight(1);
    return divider;
}

} // namespace

CameraInspector::CameraInspector(QWidget* parent) : QWidget(parent) {
    setObjectName("cameraInspector");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(buildTransformSection());
    layout->addWidget(buildNavigationSection());
    layout->addWidget(buildProjectionSection());
    layout->addWidget(buildLightingSection());

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
    connect(_moveSpeed, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this] {
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
    for (auto* editor : _lightDirection) {
        connect(editor, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this] {
            notifyLightingChanged();
        });
    }
    for (auto* editor : _lightColor) {
        connect(editor, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this] {
            updateColorPreview();
            notifyLightingChanged();
        });
    }
    connect(_colorPicker, &QPushButton::clicked, this, [this] {
        const auto light = lightingState();
        const QColor initial = QColor::fromRgbF(
            std::clamp(light.color.r, 0.0f, 1.0f),
            std::clamp(light.color.g, 0.0f, 1.0f),
            std::clamp(light.color.b, 0.0f, 1.0f));
        const QColor selected = QColorDialog::getColor(
            initial,
            this,
            QStringLiteral("Directional light color"));
        if (!selected.isValid()) {
            return;
        }
        const QSignalBlocker redBlocker(_lightColor[0]);
        const QSignalBlocker greenBlocker(_lightColor[1]);
        const QSignalBlocker blueBlocker(_lightColor[2]);
        _lightColor[0]->setValue(selected.redF());
        _lightColor[1]->setValue(selected.greenF());
        _lightColor[2]->setValue(selected.blueF());
        updateColorPreview();
        notifyLightingChanged();
    });
}

void CameraInspector::setChangeHandler(ChangeHandler handler) {
    _changeHandler = std::move(handler);
}

void CameraInspector::setLightingChangeHandler(LightingChangeHandler handler) {
    _lightingChangeHandler = std::move(handler);
}

void CameraInspector::setState(const CameraControlState& state) {
    _syncing = true;
    const QSignalBlocker positionXBlocker(_position[0]);
    const QSignalBlocker positionYBlocker(_position[1]);
    const QSignalBlocker positionZBlocker(_position[2]);
    const QSignalBlocker yawBlocker(_yaw);
    const QSignalBlocker pitchBlocker(_pitch);
    const QSignalBlocker fovBlocker(_fov);
    const QSignalBlocker moveSpeedBlocker(_moveSpeed);
    const QSignalBlocker sliderBlocker(_fovSlider);

    _position[0]->setValue(state.position.x);
    _position[1]->setValue(state.position.y);
    _position[2]->setValue(state.position.z);
    _yaw->setValue(state.yawDegrees);
    _pitch->setValue(state.pitchDegrees);
    _fov->setValue(state.verticalFovDegrees);
    _moveSpeed->setValue(state.moveSpeed);
    _fovSlider->setValue(static_cast<int>(std::lround(state.verticalFovDegrees)));
    _syncing = false;
}

void CameraInspector::setLightingState(const LightingControlState& state) {
    _syncing = true;
    const QSignalBlocker directionXBlocker(_lightDirection[0]);
    const QSignalBlocker directionYBlocker(_lightDirection[1]);
    const QSignalBlocker directionZBlocker(_lightDirection[2]);
    const QSignalBlocker colorRBlocker(_lightColor[0]);
    const QSignalBlocker colorGBlocker(_lightColor[1]);
    const QSignalBlocker colorBBlocker(_lightColor[2]);

    _lightDirection[0]->setValue(state.direction.x);
    _lightDirection[1]->setValue(state.direction.y);
    _lightDirection[2]->setValue(state.direction.z);
    _lightColor[0]->setValue(state.color.r);
    _lightColor[1]->setValue(state.color.g);
    _lightColor[2]->setValue(state.color.b);
    updateColorPreview();
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
        .moveSpeed = static_cast<float>(_moveSpeed->value()),
    };
}

LightingControlState CameraInspector::lightingState() const {
    return LightingControlState{
        .direction = Vec3f(
            static_cast<float>(_lightDirection[0]->value()),
            static_cast<float>(_lightDirection[1]->value()),
            static_cast<float>(_lightDirection[2]->value())),
        .color = Vec3f(
            static_cast<float>(_lightColor[0]->value()),
            static_cast<float>(_lightColor[1]->value()),
            static_cast<float>(_lightColor[2]->value())),
    };
}

bool CameraInspector::isEditing() const {
    for (const auto* editor : _position) {
        if (editor->hasFocus()) {
            return true;
        }
    }
    for (const auto* editor : _lightDirection) {
        if (editor->hasFocus()) {
            return true;
        }
    }
    for (const auto* editor : _lightColor) {
        if (editor->hasFocus()) {
            return true;
        }
    }
    return _yaw->hasFocus() || _pitch->hasFocus() || _fov->hasFocus() ||
           _moveSpeed->hasFocus() || _fovSlider->hasFocus() || _colorPicker->hasFocus();
}

QFrame* CameraInspector::makeSection(const QString& title, const QString& description) {
    auto* section = new QFrame(this);
    section->setObjectName("propertySection");

    auto* sectionLayout = new QVBoxLayout(section);
    sectionLayout->setContentsMargins(16, 15, 16, 16);
    sectionLayout->setSpacing(5);
    sectionLayout->addWidget(makeLabel(title, "sectionTitle", section));
    sectionLayout->addWidget(makeLabel(description, "sectionDescription", section));
    sectionLayout->addSpacing(8);
    return section;
}

QWidget* CameraInspector::makeValueEditor(
    const QString& name,
    const QString& axis,
    const QString& axisColor,
    QDoubleSpinBox*& editor,
    double minimum,
    double maximum,
    double step,
    int decimals,
    const QString& suffix,
    QWidget* parent) {
    auto* row = new QWidget(parent);
    row->setObjectName("propertyRow");
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 3, 0, 3);
    layout->setSpacing(9);

    auto* axisLabel = makeLabel(axis, "axisToken", row);
    axisLabel->setProperty("axisColor", axisColor);
    axisLabel->setAlignment(Qt::AlignCenter);
    axisLabel->setFixedSize(20, 20);
    layout->addWidget(axisLabel);
    layout->addWidget(makeLabel(name, "fieldLabel", row));
    layout->addStretch(1);

    editor = new QDoubleSpinBox(row);
    configureSpinBox(editor, minimum, maximum, step, decimals, suffix);
    layout->addWidget(editor);
    return row;
}

QFrame* CameraInspector::buildTransformSection() {
    auto* section = makeSection(QStringLiteral("Transform"), QStringLiteral("World-space camera pose"));
    auto* layout = qobject_cast<QVBoxLayout*>(section->layout());

    layout->addWidget(makeValueEditor(
        QStringLiteral("Position X"), QStringLiteral("X"), QStringLiteral("x"), _position[0], -10000.0, 10000.0, 0.1, 2, {}, section));
    layout->addWidget(makeValueEditor(
        QStringLiteral("Position Y"), QStringLiteral("Y"), QStringLiteral("y"), _position[1], -10000.0, 10000.0, 0.1, 2, {}, section));
    layout->addWidget(makeValueEditor(
        QStringLiteral("Position Z"), QStringLiteral("Z"), QStringLiteral("z"), _position[2], -10000.0, 10000.0, 0.1, 2, {}, section));
    layout->addSpacing(7);
    layout->addWidget(makeDivider(section));
    layout->addSpacing(7);
    layout->addWidget(makeValueEditor(
        QStringLiteral("Yaw"), QStringLiteral("Y"), QStringLiteral("y"), _yaw, -360.0, 360.0, 1.0, 1, QStringLiteral(" deg"), section));
    layout->addWidget(makeValueEditor(
        QStringLiteral("Pitch"), QStringLiteral("P"), QStringLiteral("pitch"), _pitch, -89.9, 89.9, 1.0, 1, QStringLiteral(" deg"), section));
    return section;
}

QFrame* CameraInspector::buildProjectionSection() {
    auto* section = makeSection(QStringLiteral("Projection"), QStringLiteral("Perspective camera settings"));
    auto* layout = qobject_cast<QVBoxLayout*>(section->layout());
    layout->addWidget(makeValueEditor(
        QStringLiteral("Field of view"), QStringLiteral("F"), QStringLiteral("fov"), _fov, 30.0, 120.0, 1.0, 1, QStringLiteral(" deg"), section));

    _fovSlider = new QSlider(Qt::Horizontal, section);
    _fovSlider->setObjectName("fovSlider");
    _fovSlider->setRange(30, 120);
    _fovSlider->setSingleStep(1);
    _fovSlider->setPageStep(5);
    layout->addSpacing(3);
    layout->addWidget(_fovSlider);

    auto* rangeRow = new QWidget(section);
    auto* rangeLayout = new QHBoxLayout(rangeRow);
    rangeLayout->setContentsMargins(1, 0, 1, 0);
    rangeLayout->addWidget(makeLabel(QStringLiteral("30 deg"), "rangeLabel", rangeRow));
    rangeLayout->addStretch(1);
    rangeLayout->addWidget(makeLabel(QStringLiteral("120 deg"), "rangeLabel", rangeRow));
    layout->addWidget(rangeRow);
    return section;
}

QFrame* CameraInspector::buildNavigationSection() {
    auto* section = makeSection(QStringLiteral("Navigation"), QStringLiteral("Fly-camera response"));
    auto* layout = qobject_cast<QVBoxLayout*>(section->layout());
    layout->addWidget(makeValueEditor(
        QStringLiteral("Move speed"),
        QStringLiteral("S"),
        QStringLiteral("speed"),
        _moveSpeed,
        0.1,
        50.0,
        0.25,
        2,
        QStringLiteral(" u/s"),
        section));
    return section;
}

QFrame* CameraInspector::buildLightingSection() {
    auto* section = makeSection(
        QStringLiteral("Directional light"),
        QStringLiteral("Direction of travel and linear HDR color"));
    auto* layout = qobject_cast<QVBoxLayout*>(section->layout());

    layout->addWidget(makeValueEditor(
        QStringLiteral("Direction X"), QStringLiteral("X"), QStringLiteral("x"), _lightDirection[0], -1.0, 1.0, 0.05, 2, {}, section));
    layout->addWidget(makeValueEditor(
        QStringLiteral("Direction Y"), QStringLiteral("Y"), QStringLiteral("y"), _lightDirection[1], -1.0, 1.0, 0.05, 2, {}, section));
    layout->addWidget(makeValueEditor(
        QStringLiteral("Direction Z"), QStringLiteral("Z"), QStringLiteral("z"), _lightDirection[2], -1.0, 1.0, 0.05, 2, {}, section));
    layout->addSpacing(7);
    layout->addWidget(makeDivider(section));
    layout->addSpacing(7);
    layout->addWidget(makeValueEditor(
        QStringLiteral("Color R"), QStringLiteral("R"), QStringLiteral("r"), _lightColor[0], 0.0, 8.0, 0.05, 2, {}, section));
    layout->addWidget(makeValueEditor(
        QStringLiteral("Color G"), QStringLiteral("G"), QStringLiteral("g"), _lightColor[1], 0.0, 8.0, 0.05, 2, {}, section));
    layout->addWidget(makeValueEditor(
        QStringLiteral("Color B"), QStringLiteral("B"), QStringLiteral("b"), _lightColor[2], 0.0, 8.0, 0.05, 2, {}, section));

    auto* pickerRow = new QWidget(section);
    pickerRow->setObjectName("propertyRow");
    auto* pickerLayout = new QHBoxLayout(pickerRow);
    pickerLayout->setContentsMargins(29, 5, 0, 1);
    pickerLayout->addWidget(makeLabel(QStringLiteral("Color picker"), "fieldLabel", pickerRow));
    pickerLayout->addStretch(1);
    _colorPicker = new QPushButton(pickerRow);
    _colorPicker->setObjectName("colorPickerButton");
    _colorPicker->setFixedHeight(28);
    _colorPicker->setMinimumWidth(116);
    pickerLayout->addWidget(_colorPicker);
    layout->addWidget(pickerRow);
    return section;
}

void CameraInspector::notifyChanged() {
    if (!_syncing && _changeHandler) {
        _changeHandler(state());
    }
}

void CameraInspector::notifyLightingChanged() {
    if (!_syncing && _lightingChangeHandler) {
        _lightingChangeHandler(lightingState());
    }
}

void CameraInspector::updateColorPreview() {
    const auto light = lightingState();
    const QColor color = QColor::fromRgbF(
        std::clamp(light.color.r, 0.0f, 1.0f),
        std::clamp(light.color.g, 0.0f, 1.0f),
        std::clamp(light.color.b, 0.0f, 1.0f));
    QPixmap swatch(14, 14);
    swatch.fill(color);
    _colorPicker->setIcon(QIcon(swatch));
    _colorPicker->setText(color.name(QColor::HexRgb).toUpper());
}

} // namespace raum::sample
