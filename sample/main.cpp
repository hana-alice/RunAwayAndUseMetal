#include "ui.h"
#include <QApplication>
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    raum::sample::UI ui(argc, argv);
    ui.show();
    return app.exec();
}
