//=========================================================
//  MusE
//  Linux Music Editor
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  elided_label.h
//  (C) Copyright 2015-2016 Tim E. Real (terminator356 on sourceforge)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#ifndef __ELIDED_LABEL_H__
#define __ELIDED_LABEL_H__

#include <QFrame>

class QEvent;
class QPaintEvent;
class QSize;
class QResizeEvent;
class QMouseEvent;

namespace MusEGui {

//---------------------------------------------------------
//   ElidedLabel
//---------------------------------------------------------

class ElidedLabel : public QFrame
{
  Q_OBJECT
  Q_PROPERTY(QString text READ text WRITE setText)
  Q_PROPERTY(Qt::TextElideMode elideMode READ elideMode WRITE setElideMode)

  private:
    int _id;
    Qt::TextElideMode _elideMode;
    int _fontPointMin;
    bool _fontIgnoreHeight;
    bool _fontIgnoreWidth;
    QString _text;
    QFont _curFont;
    
    bool autoAdjustFontSize();
    
  protected:
    virtual void paintEvent(QPaintEvent*);
    virtual void resizeEvent(QResizeEvent*);
    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);

  signals:
    void pressed(QPoint p, int id, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys);
    void released(QPoint p, int id, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys);
    
  public:
    explicit ElidedLabel(QWidget* parent = 0, 
                         Qt::TextElideMode elideMode = Qt::ElideNone, 
                         //int maxFontPoint = 10, 
                         int minFontPoint = 5,
                         bool ignoreHeight = true, bool ignoreWidth = false,
                         const QString& text = QString(), 
                         Qt::WindowFlags flags = 0);
  
    int id() const             { return _id; }
    void setId(int i)          { _id = i; }
    
    Qt::TextElideMode elideMode() const { return _elideMode; }
    void setElideMode(Qt::TextElideMode mode) { _elideMode = mode; update(); }

    const QString& text() const { return _text; }
    void setText(const QString& txt);
    
    int fontPointMin() const { return _fontPointMin; }
    void setFontPointMin(int point);

    bool fontIgnoreWidth() const { return _fontIgnoreWidth; }
    bool fontIgnoreHeight() const { return _fontIgnoreHeight; }
    void setFontIgnoreDimensions(bool ignoreHeight, bool ignoreWidth = false);
};

} // namespace MusEGui

#endif
