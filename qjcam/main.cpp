#include <ultramill/nmainwindow.h>
#include <ultramill/napplication.h>
#include <QIcon>
#include <QtWidgets/QApplication>

using namespace qianjizn::ultramill;

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    NApplication app(argc, argv);
    app.Initialize();

    NMainWindow window;
    window.setWindowTitle(QObject::tr("CamDemo"));
    window.setWindowIcon(QIcon(QStringLiteral(":/qjcam/resource/logo.ico")));
    window.setMinimumSize(1000, 800);
    window.showMaximized();
    return app.exec();
}
