/*****************************************************************************
 * mxfindshares.cpp
 *****************************************************************************
 * Copyright (C) 2014 MX Authors
 *
 * Authors: Adrian
 *          MEPIS Community <http://forum.mepiscommunity.org>
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


#include "mxfindshares.h"
#include "ui_mxfindshares.h"

#include <QFileDialog>
#include <QScrollBar>
#include <QTextStream>


mxfindshares::mxfindshares(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::mxfindshares)
{
    ui->setupUi(this);
    setup();
}

mxfindshares::~mxfindshares()
{
    delete ui;
}

// setup versious items first time program runs
void mxfindshares::setup() {
    proc = new QProcess(this);
    timer = new QTimer(this);
    proc->setReadChannel(QProcess::StandardOutput);
    proc->setReadChannelMode(QProcess::MergedChannels);
    ui->stackedWidget->setCurrentIndex(0);
    ui->buttonCancel->setEnabled(true);
    ui->buttonStart->setEnabled(true);
    ui->radioAll->setChecked(true);
    ui->buttonSave->setEnabled(false);
}

// Util function
QString mxfindshares::getCmdOut(QString cmd) {
    proc = new QProcess(this);
    proc->start("/bin/bash", QStringList() << "-c" << cmd);
    proc->setReadChannel(QProcess::StandardOutput);
    proc->setReadChannelMode(QProcess::MergedChannels);
    proc->waitForFinished(-1);
    return proc->readAllStandardOutput().trimmed();
}

// Get version of the program
QString mxfindshares::getVersion(QString name) {
    QString cmd = QString("dpkg -l %1 | awk 'NR==6 {print $3}'").arg(name);
    return getCmdOut(cmd);
}

// List network shares
void mxfindshares::listShares(QString option) {
    setCursor(QCursor(Qt::WaitCursor));
    QString cmd = QString("findshares %1").arg(option);
    ui->stackedWidget->setCurrentWidget(ui->outputPage);
    setConnections(timer, proc);
    proc->start(cmd);
}

//// sync process events ////

void mxfindshares::procStart() {
    timer->start(100);
}

void mxfindshares::procTime() {
    int i = ui->progressBar->value() + 1;
    if (i > 100) {
        i = 0;
    }
    ui->progressBar->setValue(i);
}

void mxfindshares::procDone(int exitCode) {
    timer->stop();
    ui->progressBar->setValue(100);
    setCursor(QCursor(Qt::ArrowCursor));

    if (exitCode == 0) {
        ui->outputLabel->setText(tr("Finished searching for shares."));
    } else {
        QMessageBox::critical(this, tr("Error"),
                              tr("Process finished. Errors have occurred."));
    }
    ui->buttonSave->setEnabled(true);
    ui->buttonStart->setEnabled(true);
    ui->buttonStart->setText(tr("< Back"));
    ui->buttonStart->setIcon(QIcon());
}


// set proc and timer connections
void mxfindshares::setConnections(QTimer* timer, QProcess* proc) {
    disconnect(timer, SIGNAL(timeout()), 0, 0);
    connect(timer, SIGNAL(timeout()), SLOT(procTime()));
    disconnect(proc, SIGNAL(started()), 0, 0);
    connect(proc, SIGNAL(started()), SLOT(procStart()));
    disconnect(proc, SIGNAL(finished(int)), 0, 0);
    connect(proc, SIGNAL(finished(int)),SLOT(procDone(int)));
    disconnect(proc, SIGNAL(readyReadStandardOutput()), 0, 0);
    connect(proc, SIGNAL(readyReadStandardOutput()), SLOT(onStdoutAvailable()));
}


//// slots ////

// update output box on Stdout
void mxfindshares::onStdoutAvailable() {
    QByteArray output = proc->readAllStandardOutput();
    QString out = ui->outputBox->toPlainText() + QString::fromUtf8(output);
    ui->outputBox->setPlainText(out);
    QScrollBar *sb = ui->outputBox->verticalScrollBar();
    //sb->setValue(sb->maximum());
}


// Start button clicked
void mxfindshares::on_buttonStart_clicked() {
    // on first page
    if (ui->stackedWidget->currentIndex() == 0) {
        ui->buttonStart->setEnabled(false);
        if (ui->radioAll->isChecked()) {
            listShares("");
        } else if (ui->radioNoNFS->isChecked()) {
            listShares("-nn");
        } else if (ui->radioNoSMB->isChecked()) {
            listShares("-ns");
        }
    // on output page
    } else if (ui->stackedWidget->currentWidget() == ui->outputPage) {
        ui->stackedWidget->setCurrentIndex(0);
        // restore Start button
        ui->buttonStart->setText(tr("Start"));
        ui->buttonStart->setIcon(QIcon("/usr/share/mx-findshares/icons/dialog-ok.png"));
        ui->buttonSave->setEnabled(false);
        ui->outputBox->clear();
    } else {
        qApp->exit(0);
    }
}


// About button clicked
void mxfindshares::on_buttonAbout_clicked() {
    QMessageBox msgBox(QMessageBox::NoIcon,
                       tr("About MX Find Shares"), "<p align=\"center\"><b><h2>" +
                       tr("MX Find Shares") + "</h2></b></p><p align=\"center\">" + tr("Version: ") +
                       getVersion("mx-findshares") + "</p><p align=\"center\"><h3>" +
                       tr("Simple package for finding network shares for antiX MX") + "</h3></p><p align=\"center\"><a href=\"http://www.mepiscommunity.org/mx\">http://www.mepiscommunity.org/mx</a><br /></p><p>" +
                       tr("This program is composed of two packages:") +
                       "<p>findshares, the CLI utility: Copyright (c) Richard A. Rost</p>" +
                       "<p>mx-findshares, the GUI wrapper: " + tr("Copyright (c) antiX\n") + "<br /><br /></p>", 0, this);
    msgBox.addButton(tr("Cancel"), QMessageBox::AcceptRole); // because we want to display the buttons in reverse order we use counter-intuitive roles.
    msgBox.addButton(tr("License"), QMessageBox::RejectRole);
    if (msgBox.exec() == QMessageBox::RejectRole)
        system("mx-viewer http://www.mepiscommunity.org/doc_mx/mx-findshares-license.html 'MX Find Shares License'");
}


// Help button clicked
void mxfindshares::on_buttonHelp_clicked() {
    system("mx-viewer http://www.mepiscommunity.org/user_manual_mx15/mxum.html#findshares 'MX Find Shares Help'");
}


// Save output
void mxfindshares::on_buttonSave_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                    tr("network_shares.txt"));
    if (fileName != "") {
        QFile file(fileName);
        file.open(QIODevice::WriteOnly);
        QTextStream out(&file);
        out << ui->outputBox->toPlainText();
        file.close();
    }
}
