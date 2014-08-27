/*****************************************************************************
 *  
 *   This file is part of the Utopia Documents application.
 *       Copyright (c) 2008-2014 Lost Island Labs
 *           <info@utopiadocs.com>
 *   
 *   Utopia Documents is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU GENERAL PUBLIC LICENSE VERSION 3 as
 *   published by the Free Software Foundation.
 *   
 *   Utopia Documents is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 *   Public License for more details.
 *   
 *   In addition, as a special exception, the copyright holders give
 *   permission to link the code of portions of this program with the OpenSSL
 *   library under certain conditions as described in each individual source
 *   file, and distribute linked combinations including the two.
 *   
 *   You must obey the GNU General Public License in all respects for all of
 *   the code used other than OpenSSL. If you modify file(s) with this
 *   exception, you may extend this exception to your version of the file(s),
 *   but you are not obligated to do so. If you do not wish to do so, delete
 *   this exception statement from your version.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with Utopia Documents. If not, see <http://www.gnu.org/licenses/>
 *  
 *****************************************************************************/

/****************************************************************************
 **
 ** This file is part of a Qt Solutions component.
 **
 ** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
 **
 ** Contact:  Qt Software Information (qt-info@nokia.com)
 **
 ** Commercial Usage
 ** Licensees holding valid Qt Commercial licenses may use this file in
 ** accordance with the Qt Solutions Commercial License Agreement provided
 ** with the Software or, alternatively, in accordance with the terms
 ** contained in a written agreement between you and Nokia.
 **
 ** GNU Lesser General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU Lesser
 ** General Public License version 2.1 as published by the Free Software
 ** Foundation and appearing in the file LICENSE.LGPL included in the
 ** packaging of this file.  Please review the following information to
 ** ensure the GNU Lesser General Public License version 2.1 requirements
 ** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
 **
 ** In addition, as a special exception, Nokia gives you certain
 ** additional rights. These rights are described in the Nokia Qt LGPL
 ** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
 ** package.
 **
 ** GNU General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU
 ** General Public License version 3.0 as published by the Free Software
 ** Foundation and appearing in the file LICENSE.GPL included in the
 ** packaging of this file.  Please review the following information to
 ** ensure the GNU General Public License version 3.0 requirements will be
 ** met: http://www.gnu.org/copyleft/gpl.html.
 **
 ** Please note Third Party Software included with Qt Solutions may impose
 ** additional restrictions and it is the user's responsibility to ensure
 ** that they have met the licensing requirements of the GPL, LGPL, or Qt
 ** Solutions Commercial license and the relevant license of the Third
 ** Party Software they are using.
 **
 ** If you are unsure which license is appropriate for your use, please
 ** contact the sales department at qt-sales@nokia.com.
 **
 ****************************************************************************/

#include <QObject>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QPainter>
#include <QtGui/QPushButton>
#include <QtGui/QColorDialog>
#include <QtCore/QMap>
#include <QtGui/QLayout>
#include <QtGui/QStyle>
#include <QtGui/QLabel>
#include <QtGui/QToolTip>
#include <QtGui/QPixmap>
#include <QtGui/QFocusEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QGridLayout>
#include <QtGui/QHideEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QShowEvent>
#include <QtGui/QMouseEvent>
#include <math.h>

#include <QtDebug>

#include <graffiti/qtcolorpicker.h>

/*! \class QtColorPicker

  \brief The QtColorPicker class provides a widget for selecting
  colors from a popup color grid.

  Users can invoke the color picker by clicking on it, or by
  navigating to it and pressing Space. They can use the mouse or
  arrow keys to navigate between colors on the grid, and select a
  color by clicking or by pressing Enter or Space. The
  colorChanged() signal is emitted whenever the color picker's color
  changes.

  The widget also supports negative selection: Users can click and
  hold the mouse button on the QtColorPicker widget, then move the
  mouse over the color grid and release the mouse button over the
  color they wish to select.

  The color grid shows a customized selection of colors. An optional
  ellipsis "..." button (signifying "more") can be added at the
  bottom of the grid; if the user presses this, a QColorDialog pops
  up and lets them choose any color they like. This button is made
  available by using setColorDialogEnabled().

  When a color is selected, the QtColorPicker widget shows the color
  and its name. If the name cannot be determined, the translatable
  name "Custom" is used.

  The QtColorPicker object is optionally initialized with the number
  of columns in the color grid. Colors are then added left to right,
  top to bottom using insertColor(). If the number of columns is not
  set, QtColorPicker calculates the number of columns and rows that
  will make the grid as square as possible.

  \code
  DrawWidget::DrawWidget(QWidget *parent, const char *name)
  {
  QtColorPicker *picker = new QtColorPicker(this);
  picker->insertColor(red, "Red"));
  picker->insertColor(QColor("green"), "Green"));
  picker->insertColor(QColor(0, 0, 255), "Blue"));
  picker->insertColor(white);

  connect(colors, SIGNAL(colorChanged(const QColor &)), SLOT(setCurrentColor(const QColor &)));
  }
  \endcode

  An alternative to adding colors manually is to initialize the grid
  with QColorDialog's standard colors using setStandardColors().

  QtColorPicker also provides a the static function getColor(),
  which pops up the grid of standard colors at any given point.

  \img colorpicker1.png
  \img colorpicker2.png

  \sa QColorDialog
*/

/*! \fn QtColorPicker::colorChanged(const QColor &color)

  This signal is emitted when the QtColorPicker's color is changed.
  \a color is the new color.

  To obtain the color's name, use text().
*/

/*
  A class  that acts very much  like a QPushButton. It's not styled,
  so we  can  expect  the  exact  same    look,  feel and   geometry
  everywhere.     Also,  this  button     always emits   clicked  on
  mouseRelease, even if the mouse button was  not pressed inside the
  widget.
*/
class ColorPickerButton : public QFrame
{
    Q_OBJECT

public:
    ColorPickerButton(QWidget *parent);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void keyReleaseEvent(QKeyEvent *e);
    void paintEvent(QPaintEvent *e);
    void focusInEvent(QFocusEvent *e);
    void focusOutEvent(QFocusEvent *e);
};

/*
  This class represents each "color" or item in the color grid.
*/

class ColorPickerPopup;

class ColorPickerItem : public QFrame
{
    Q_OBJECT

public:
    ColorPickerItem(ColorPickerPopup *popup, const QColor &color = Qt::white, const QString &text = QString::null,
                    QWidget *parent = 0);
    ~ColorPickerItem();

    QColor color() const;
    QString text() const;

    void setSelected(bool);
    bool isSelected() const;

    ColorPickerPopup *popup();
signals:
    void clicked();
    void selected();

public slots:
    void setColor(const QColor &color, const QString &text = QString());

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void paintEvent(QPaintEvent *e);

private:
    QColor c;
    QString t;
    bool sel;
    ColorPickerPopup *_popup;
};

/*

 */
class ColorPickerPopup : public QFrame
{
    Q_OBJECT

public:
    ColorPickerPopup(QtColorPicker *picker, int width, bool withColorDialog,
                     QWidget *parent = 0);
    ~ColorPickerPopup();

    void insertColor(const QColor &col, const QString &text, int index);
    void exec();

    void setExecFlag();

    QColor lastSelected() const;

    ColorPickerItem *find(const QColor &col) const;
    QColor color(int index) const;

    QtColorPicker *picker();

signals:
    void selected(const QColor &);
    void hid();

public slots:
    void getColorFromDialog();

protected slots:
    void updateSelected();

protected:
    void keyPressEvent(QKeyEvent *e);
    void showEvent(QShowEvent *e);
    void hideEvent(QHideEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

    void regenerateGrid();

private:
    QMap<int, QMap<int, QWidget *> > widgetAt;
    QList<ColorPickerItem *> items;
    QGridLayout *grid;
    ColorPickerButton *moreButton;
    QEventLoop *eventLoop;

    int lastPos;
    int cols;
    QColor lastSel;

    QtColorPicker *_picker;
};
