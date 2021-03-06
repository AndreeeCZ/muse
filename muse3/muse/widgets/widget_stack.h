//=========================================================
//  MusE
//  Linux Music Editor
//    widget_stack.h
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __WIDGET_STACK_H__
#define __WIDGET_STACK_H__

#include <QWidget>

class QWheelEvent;
class QResizeEvent;

namespace MusEGui {

//---------------------------------------------------------
//   WidgetStack
//---------------------------------------------------------

class WidgetStack : public QWidget {
      Q_OBJECT
   public:
     // StackHint:   Use the maximum of all the stack widgets.
     // VisibleHint: Use only the current visible widget. 
     //              If it is invalid, use the StackHint.
     enum SizeHintMode { StackHint=0, VisibleHint=1 };
     
   private:
      SizeHintMode _sizeHintMode;
      
      std::vector<QWidget*> stack;
      int top;

     void resizeStack(const QSize&);
     
   protected:
      virtual void wheelEvent(QWheelEvent* e);
      virtual void resizeEvent(QResizeEvent* e);
      
   signals:
      void redirectWheelEvent(QWheelEvent*);
      
   public:
      WidgetStack(QWidget* parent, const char* name = 0, SizeHintMode sizeHintMode = VisibleHint);
      void raiseWidget(int idx);
      void addWidget(QWidget* w, unsigned int idx);
      QWidget* getWidget(unsigned int idx);
      QWidget* visibleWidget() const;
      int curIdx() const { return top; }
      virtual QSize minimumSizeHint() const;
      SizeHintMode sizeHintMode() const { return _sizeHintMode; }
      void setSizeHintMode(SizeHintMode mode) { _sizeHintMode = mode; update(); }
      
      QSize sizeHint() const;
      
      };

} // namespace MusEGui

#endif
