#pragma once

class QPainter;
class QRectF;

namespace mrs::ui
{

// Эмблема Дальрыбвтуза для боковых полей: карточка во всю высоту поля со знаком «рыба-глобус»
// (по мотивам логотипа на dalrybvtuz.ru), кольцами сонара, шкалами с рисками по краям, волнами,
// названием и полным наименованием. Цвета — из текущей цветовой схемы.
//
// Это не официальный файл логотипа: если рядом с программой лежит data/branding/dalrybvtuz.png
// (или .svg), используется он.
void drawDalrybvtuzEmblem(QPainter& painter, const QRectF& area);

// Второй вариант: сетка во всю высоту поля, поверх — знак в кольцах с рисками, перекрестие,
// уголки-скобки, название и координаты Владивостока. Без жёлтого: тона текста и шкалы ИКО.
void drawDalrybvtuzGridEmblem(QPainter& painter, const QRectF& area);

} // namespace mrs::ui
