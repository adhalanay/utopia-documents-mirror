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

#ifndef ATHENAEUM_LIBRARYDELEGATE_H
#define ATHENAEUM_LIBRARYDELEGATE_H

#include <QStyledItemDelegate>

namespace Athenaeum
{

    class LibraryDelegate : public QStyledItemDelegate
    {
        Q_OBJECT

    public:
        LibraryDelegate(QObject * parent = 0);
        virtual ~LibraryDelegate();

        QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
        void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;

    }; // class LibraryDelegate

}

#endif // ATHENAEUM_LIBRARYDELEGATE_H
