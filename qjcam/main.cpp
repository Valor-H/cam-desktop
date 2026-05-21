#include <ultramill/nmainwindow.h>
#include <ultramill/napplication.h>
#include <QtWidgets/QApplication>

using namespace qianjizn::ultramill;

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    NApplication app(argc, argv);
    app.Initialize();

    NMainWindow window;
    window.showMaximized();
    return app.exec();
}
