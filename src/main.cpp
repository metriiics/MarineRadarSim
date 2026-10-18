#include <QApplication>

#include "presentation/main_window/main_window.h"
#include "presentation/theme/theme.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationVersion(QStringLiteral(MRS_VERSION));
    mrs::ui::theme::initFonts();

    // Таблицу стилей, шрифт и цветовую схему задаёт окно по сохранённым настройкам.
    mrs::ui::MainWindow window;
    window.showInitial();

    return app.exec();
}
