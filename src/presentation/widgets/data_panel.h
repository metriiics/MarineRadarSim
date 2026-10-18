#pragma once

#include <QFrame>

class QGridLayout;
class QLabel;

namespace mrs::ui
{

// Информационный блок в жёлтой рамке: строки «название — значение — единица».
// Название и единица — синим, значение — жёлтым. Текст выводится прописными буквами.
class DataPanel final : public QFrame
{
public:
    explicit DataPanel(QWidget* parent = nullptr);

    // Плотные строки без межстрочного отступа — для нижних блоков колонки.
    void setCompact(bool compact);

    void addTitle(const QString& title);

    // Возвращает метку значения, чтобы владелец мог обновлять её.
    QLabel* addRow(const QString& name, const QString& value = {}, const QString& unit = {});

    // Пустая строка высотой в одну строку текста.
    void addBlankLine();

    // Свободное место блока — под строками, а не между ними.
    void addStretch();

private:
    QGridLayout* grid_;
    int rows_{0};
};

} // namespace mrs::ui
