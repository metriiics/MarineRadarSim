#include <QApplication>

#include "presentation/main_window/main_window.h"
#include "presentation/theme/theme.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationVersion(QStringLiteral(MRS_VERSION));
    app.setStyleSheet(mrs::ui::theme::styleSheet());

    mrs::ui::MainWindow window;
    window.showMaximized();

    return app.exec();
}
