#include "data_panel.h"

#include "presentation/theme/theme.h"

#include <QFontMetrics>
#include <QGridLayout>
#include <QLabel>

namespace mrs::ui
{

namespace
{

enum Column
{
    kNameColumn,
    kValueColumn,
    kUnitColumn,
    kColumnCount
};

constexpr int kUnitMinWidth = 40;
constexpr int kRowSpacing = 3;
constexpr int kColumnSpacing = 6;  // между названием, значением и единицей — не зависит от шага между блоками

} // namespace

DataPanel::DataPanel(QWidget* parent)
    : QFrame(parent)
    , grid_(new QGridLayout(this))
{
    setProperty("panel", true);

    grid_->setContentsMargins(theme::kPadding, 4, theme::kPadding, 4);
    grid_->setHorizontalSpacing(kColumnSpacing);
    grid_->setVerticalSpacing(kRowSpacing);
    grid_->setColumnStretch(kNameColumn, 1);
    grid_->setColumnMinimumWidth(kUnitColumn, kUnitMinWidth);
}

void DataPanel::setCompact(bool compact)
{
    const int vertical = compact ? 1 : 4;
    grid_->setContentsMargins(theme::kPadding, vertical, theme::kPadding, vertical);
    grid_->setVerticalSpacing(compact ? 0 : kRowSpacing);
}

void DataPanel::addTitle(const QString& title)
{
    auto* label = theme::createText(title.toUpper(), this);
    label->setAlignment(Qt::AlignCenter);
    grid_->addWidget(label, rows_++, kNameColumn, 1, kColumnCount);
}

void DataPanel::addBlankLine()
{
    QFont font = this->font();
    font.setPixelSize(theme::kFontPx);
    grid_->setRowMinimumHeight(rows_++, QFontMetrics(font).height());
}

void DataPanel::addStretch()
{
    grid_->setRowStretch(rows_++, 1);
}

QLabel* DataPanel::addRow(const QString& name, const QString& value, const QString& unit)
{
    auto* nameLabel = theme::createText(name.toUpper(), this);
    auto* valueLabel = theme::createValue(value.toUpper(), this);
    auto* unitLabel = theme::createText(unit.toUpper(), this);

    valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    grid_->addWidget(nameLabel, rows_, kNameColumn);
    grid_->addWidget(valueLabel, rows_, kValueColumn);
    grid_->addWidget(unitLabel, rows_, kUnitColumn);
    ++rows_;

    return valueLabel;
}

} // namespace mrs::ui
