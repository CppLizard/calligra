/*
 *  Copyright (c) 2015 Jouni Pentikäinen <joupent@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KIS_ONION_SKIN_DIALOG_H
#define KIS_ONION_SKIN_DIALOG_H

#include <QDialog>
#include <QSlider>

namespace Ui {
class KisOnionSkinDialog;
}

class KisOnionSkinDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KisOnionSkinDialog(QWidget *parent = 0);
    ~KisOnionSkinDialog();

private:
    Ui::KisOnionSkinDialog *ui;

    QSlider numberOfSkins;
    QVector<QSlider*> forwardOpacities;
    QVector<QSlider*> backwardOpacities;

private slots:
    void changed();
};

#endif // KIS_ONION_SKIN_DIALOG_H
