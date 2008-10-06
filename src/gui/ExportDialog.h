/***************************************************************************
    File                 : ExportDialog.h
    Project              : LabPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2008 by Stefan Gerlach
    Email (use @ for *)  : stefan.gerlach*uni-konstanz.de
    Description          : dialog for exporting data
                           
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/

#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <KDialog>

class MainWin;
class Spreadsheet;
#include "ExportWidget.h"

/**
 * @brief Provides a dialog for exporting data from a spreadsheet.
 */
class ExportDialog: public KDialog {
	Q_OBJECT
public:
	ExportDialog(MainWin *mw);
private:
	MainWin *mainWin;
	ExportWidget *exportWidget;
	void setupGUI();
private slots:
	void apply() { exportWidget->apply(mainWin); }
};

#endif //EXPORTDIALOG_H
