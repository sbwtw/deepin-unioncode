/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhouyi<zhouyi1@uniontech.com>
 *
 * Maintainer: zhouyi<zhouyi1@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "hotkeylineedit.h"

;
#pragma pack(push, 1)
class HotkeyLineEditPrivate
{
    HotkeyLineEditPrivate();
    bool hotkeyMode;
    int key;

    friend class HotkeyLineEdit;
};

HotkeyLineEditPrivate::HotkeyLineEditPrivate()
    : hotkeyMode(false)
    , key(Qt::Key_unknown)
{

}
#pragma pack(pop)

HotkeyLineEdit::HotkeyLineEdit(QWidget *parent)
    : QLineEdit(parent)
    , d(new HotkeyLineEditPrivate())
{
    setAttribute(Qt::WA_InputMethodEnabled, false);
}

HotkeyLineEdit::~HotkeyLineEdit()
{

}

void HotkeyLineEdit::setHotkeyMode(bool hotkeyMode)
{
    d->hotkeyMode = hotkeyMode;
}

bool HotkeyLineEdit::isHotkeyMode()
{
    return d->hotkeyMode;
}

void HotkeyLineEdit::setKey(int key)
{
    Qt::Key qKey = static_cast<Qt::Key>(key);
    if (qKey != Qt::Key_unknown)
    {
        d->key = qKey;
        setText(QKeySequence(qKey).toString());
    }
    else
    {
        d->key = Qt::Key_unknown;
        setText("");
    }
}

int HotkeyLineEdit::getKey()
{
    return d->key;
}

void HotkeyLineEdit::setText(QString text)
{
    QLineEdit::setText(text);
}

void HotkeyLineEdit::keyPressEvent(QKeyEvent *e)
{
    if (!d->hotkeyMode) {
        QLineEdit::keyPressEvent(e);
        return;
    }

    int key = e->key();
    Qt::Key qKey = static_cast<Qt::Key>(key);
    if (qKey == Qt::Key_unknown
        || qKey == Qt::Key_Control
        || qKey == Qt::Key_Shift
        || qKey == Qt::Key_Alt
        || qKey == Qt::Key_Enter
        || qKey == Qt::Key_Return
        || qKey == Qt::Key_Tab
        || qKey == Qt::Key_CapsLock
        || qKey == Qt::Key_Escape
        || qKey == Qt::Key_Meta)
    {
        return;
    }

    Qt::KeyboardModifiers modifiers = e->modifiers();
    if (!(modifiers & Qt::ShiftModifier
            || modifiers & Qt::ControlModifier
            || modifiers & Qt::AltModifier
            || (key >= Qt::Key_F1 && key <= Qt::Key_F35)))
    {
        return;
    }

    if (modifiers & Qt::ShiftModifier) {
        key += Qt::SHIFT;
    }

    if (modifiers & Qt::ControlModifier) {
        key += Qt::CTRL;
    }

    if (modifiers & Qt::AltModifier) {
        key += Qt::ALT;
    }

    setText(QKeySequence(key).toString());
    d->key = key;
    emit sigKeyChanged(key);
}

