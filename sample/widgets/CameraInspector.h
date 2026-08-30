#pragma once

#include <array>
#include <functional>

#include <QWidget>

#include "../SampleBase.h"

class QDoubleSpinBox;
class QFrame;
class QSlider;

namespace raum::sample {

class CameraInspector final : public QWidget {
public:
    using ChangeHandler = std::function<void(const CameraControlState&)>;

    explicit CameraInspector(QWidget* parent = nullptr);

    void setChangeHandler(ChangeHandler handler);
    void setState(const CameraControlState& state);

    CameraControlState state() const;
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
    QFrame* buildProjectionSection();
    void notifyChanged();

    std::array<QDoubleSpinBox*, 3> _position{};
    QDoubleSpinBox* _yaw{nullptr};
    QDoubleSpinBox* _pitch{nullptr};
    QDoubleSpinBox* _fov{nullptr};
    QSlider* _fovSlider{nullptr};
    ChangeHandler _changeHandler;
    bool _syncing{false};
};

} // namespace raum::sample
