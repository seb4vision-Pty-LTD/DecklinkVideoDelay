#include <QApplication>
#include <gst/gst.h>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    // Initialize GStreamer runtime prior to Qt event loop execution
    gst_init(&argc, &argv);

    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    int result = app.exec();

    // Clean up GStreamer execution resources on exit
    gst_deinit();

    return result;
}