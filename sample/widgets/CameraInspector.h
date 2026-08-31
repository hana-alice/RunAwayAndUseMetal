#pragma once

#include <array>
#include <functional>

#include <QWidget>

#include "../SampleBase.h"

class QDoubleSpinBox;
class QFrame;
class QPushButton;
class QSlider;

namespace raum::sample {

class CameraInspector final : public QWidget {
public:
    using ChangeHandler = std::function<void(const CameraControlState&)>;
    using LightingChangeHandler = std::function<void(const LightingControlState&)>;

    explicit CameraInspector(QWidget* parent = nullptr);

    void setChangeHandler(ChangeHandler handler);
    void setLightingChangeHandler(LightingChangeHandler handler);
    void setState(const CameraControlState& state);
    void setLightingState(const LightingControlState& state);

    CameraControlState state() const;
    LightingControlState lightingState() const;
    bool isEditing() const;

private:
    QFrame* makeSection(const QString& title, const QString& description);
    QWidget* makeValueEditor(
        const QString& name,
        const QString& axis,
        const QString& axisColor,
        QDoubleSpinBox*& editor,
        double minimum,
        double maximum,
        double step,
        int decimals,
        const QString& suffix,
        QWidget* parent);
    QFrame* buildTransformSection();
    QFrame* buildNavigationSection();
    QFrame* buildProjectionSection();
    QFrame* buildLightingSection();
    void notifyChanged();
    void notifyLightingChanged();
    void updateColorPreview();

    std::array<QDoubleSpinBox*, 3> _position{};
    QDoubleSpinBox* _yaw{nullptr};
    QDoubleSpinBox* _pitch{nullptr};
    QDoubleSpinBox* _fov{nullptr};
    QDoubleSpinBox* _moveSpeed{nullptr};
    QSlider* _fovSlider{nullptr};
    std::array<QDoubleSpinBox*, 3> _lightDirection{};
    std::array<QDoubleSpinBox*, 3> _lightColor{};
    QPushButton* _colorPicker{nullptr};
    ChangeHandler _changeHandler;
    LightingChangeHandler _lightingChangeHandler;
    bool _syncing{false};
};

} // namespace raum::sample
