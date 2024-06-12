# Workaround
Issues met will be record here.

## QT

因为尽可能引入轻量的依赖，单纯的把qt作为系统窗口的封装会遇到一些问题：
 - QWindow独立的作为window ID提供者可以被vk正确的引用并正常渲染，一旦使用`QWidget::createWindowContainer`想嵌入到其他地方时就不能正确渲染（灰色）。[官网类似问题参考](https://forum.qt.io/topic/123435/invalid-vksurface-from-qwidget/7)
    - 解决方案：https://www.niangames.com/articles/qt-vulkan-renderer， 所以引擎里会引入QVulkanInstance在QWindow初始化的时候做一些设置。另：`QVulkanInstance::surfaceForWindow`会返回无效值，除非先调用`QWindow::show()`，参考[此处](https://stackoverflow.com/questions/66033294/invalid-vksurfacekhr-handle-from-qtwidget-qtwindow)
- QWindow通过`QWindow::width`,`QWindow::height`,`QApplication::devicePixelRatio`获取实际长宽的时候会有±0.5误差，[类似问题参考](https://community.khronos.org/t/dpi-vksurfacecapabilitieskhr-extents-not-matching-qt-surface-extents/7625)
    - 解决方案：同上述问题参考，使用平台原生提供的函数。