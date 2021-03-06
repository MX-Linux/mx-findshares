/*****************************************************************************
 * mxfindshares.h
 *****************************************************************************
 * Copyright (C) 2014 MX Authors
 *
 * Authors: Adrian
 *          MX Linux <http://mxlinux.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MX Find Shares is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MX Find Shares.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/


#ifndef MXFINDSHARES_H
#define MXFINDSHARES_H

#include <QMessageBox>
#include <QProcess>
#include <QTimer>

namespace Ui {
class mxfindshares;
}

class mxfindshares : public QDialog
{
    Q_OBJECT

protected:
    QProcess *proc;
    QTimer *timer;

public:
    explicit mxfindshares(QWidget *parent = 0);
    ~mxfindshares();

    QString getCmdOut(QString cmd);

    void listShares(QString option);
    QString getVersion(QString name);
    void setup();

public slots:
    void procStart();
    void procTime();
    void procDone(int exitCode);
    void setConnections(QTimer* timer, QProcess* proc);
    void onStdoutAvailable();

    virtual void on_buttonStart_clicked();
    virtual void on_buttonAbout_clicked();
    virtual void on_buttonHelp_clicked();

private slots:
    void on_buttonSave_clicked();

private:
    Ui::mxfindshares *ui;
};

#endif // MXFINDSHARES_H
