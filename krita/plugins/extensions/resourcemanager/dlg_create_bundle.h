/*
 *  Copyright (c) 2014 Victor Lafon metabolic.ewilan@hotmail.fr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#ifndef KOBUNDLECREATIONWIDGET_H
#define KOBUNDLECREATIONWIDGET_H

#include <kdialog.h>

class ResourceBundle;

namespace Ui
{
class WdgDlgCreateBundle;
}

class DlgCreateBundle : public KDialog
{
    Q_OBJECT

public:
    explicit DlgCreateBundle(ResourceBundle *bundle = 0, QWidget *parent = 0);
    ~DlgCreateBundle();

    QString bundleName() const;
    QString authorName() const;
    QString email() const;
    QString website() const;
    QString license() const;
    QString description() const;
    QString saveLocation() const;
    QString previewImage() const;

    QStringList selectedBrushes() const { return m_selectedBrushes; }
    QStringList selectedPresets() const { return m_selectedPresets; }
    QStringList selectedGradients() const { return m_selectedGradients; }
    QStringList selectedPatterns() const { return m_selectedPatterns; }
    QStringList selectedPalettes() const { return m_selectedPalettes; }
    QStringList selectedWorkspaces() const { return m_selectedWorkspaces; }

private slots:

    void accept();
    void selectSaveLocation();
    void addSelected();
    void removeSelected();
    void resourceTypeSelected(int idx);
    void getPreviewImage();

private:
    QWidget *m_page;
    Ui::WdgDlgCreateBundle *m_ui;

    QStringList m_selectedBrushes;
    QStringList m_selectedPresets;
    QStringList m_selectedGradients;
    QStringList m_selectedPatterns;
    QStringList m_selectedPalettes;
    QStringList m_selectedWorkspaces;

    QString m_previewImage;

    ResourceBundle *m_bundle;
};

#endif // KOBUNDLECREATIONWIDGET_H
