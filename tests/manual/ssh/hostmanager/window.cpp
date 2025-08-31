/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "window.h"
#include "ui_window.h"

#include <qssh/sftpfilesystemmodel.h>
#include <qssh/sshconnection.h>

#include <QSqlDatabase>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QModelIndexList>
#include <QItemSelectionModel>
#include <QString>

using namespace Qt::StringLiterals;

using namespace QSsh;

SftpFsWindow::SftpFsWindow(QWidget *parent) : QDialog(parent), m_ui(new Ui::Window)
{
    m_ui->setupUi(this);
    connect(m_ui->connectButton, &QAbstractButton::clicked, this, &SftpFsWindow::connectToHost);
    connect(m_ui->downloadButton, &QAbstractButton::clicked, this, &SftpFsWindow::downloadFile);
    connect(m_ui->treeViewHosts, &QTreeView::clicked, this, &SftpFsWindow::treeViewHostsClicked);

    QFile file("hosts.txt"_L1);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    hostModel = new TreeModel(QString::fromUtf8(file.readAll()));
    file.close();

    m_ui->treeViewHosts->setModel(hostModel);
    QSqlError err = connectToDatabase();
}

SftpFsWindow::~SftpFsWindow()
{
    delete m_ui;
}

void SftpFsWindow::connectToHost()
{
    m_ui->connectButton->setEnabled(false);
    SshConnectionParameters sshParams;
    sshParams.setHost(m_ui->hostLineEdit->text());
    sshParams.setUserName(m_ui->userLineEdit->text());
    sshParams.authenticationType
            = SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods;
    sshParams.setPassword(m_ui->passwordLineEdit->text());
    sshParams.setPort(m_ui->portSpinBox->value());
    sshParams.timeout = 10;
    m_fsModel = new SftpFileSystemModel(this);
    connect(m_fsModel, &SftpFileSystemModel::sftpOperationFailed,
            this, &SftpFsWindow::handleSftpOperationFailed);
    connect(m_fsModel, &SftpFileSystemModel::connectionError,
            this, &SftpFsWindow::handleConnectionError);
    connect(m_fsModel, &SftpFileSystemModel::sftpOperationFinished,
            this, &SftpFsWindow::handleSftpOperationFinished);

    connect(m_fsModel, &SftpFileSystemModel::sftpConnectionSuccess,
            this, &SftpFsWindow::handleConnectionSuccess);

    m_fsModel->setSshConnection(sshParams);
    m_ui->fsView->setModel(m_fsModel);
}

void SftpFsWindow::downloadFile()
{
    const QModelIndexList selectedIndexes = m_ui->fsView->selectionModel()->selectedIndexes();
    if (selectedIndexes.count() != 2)
        return;
    const QString targetFilePath = QFileDialog::getSaveFileName(this, tr("Choose Target File"),
        QDir::tempPath());
    if (targetFilePath.isEmpty())
        return;
    const SftpJobId jobId = m_fsModel->downloadFile(selectedIndexes.at(1), targetFilePath);
    QString message;
    if (jobId == SftpInvalidJob)
        message = tr("Download failed.");
    else
        message = tr("Queuing download operation %1.").arg(jobId);
    m_ui->outputTextEdit->appendPlainText(message);
}

void SftpFsWindow::treeViewHostsClicked(const QModelIndex &index)
{
    QString hostname = index.data().toString();
    QString lastDirectory;
    QString userName;
    QString password;

    qDebug() << "SftpFsWindow::treeViewHostsClicked Clicked on column: " << index.column() << ", row : " << index.row() <<  ", data :" << index.data().toString();
    qDebug() << "Retrieving credentials from DB in any exist";

    QSqlQuery query("SELECT * FROM hosts where hostname = '" + hostname + "'");
    while (query.next()) {
        hostname = query.value(0).toString();
        userName = query.value(1).toString();
        password = query.value(2).toString();
        lastDirectory = query.value(3).toString();
        qDebug() << "Found hostname: " <<hostname;
        qDebug() << "Found userName: " <<userName;
        qDebug() << "Found password: " <<password;
        qDebug() << "Found lastDirectory: " <<lastDirectory;
    }

    setHostNameToConnectTo(hostname, userName, password, lastDirectory);

    // Check if connectOnClick Checkbox is ticked, and if yes, invoke the connect as well here
    if(m_ui->checkBoxConnectOnClick->isChecked())
        connectToHost();
}

void SftpFsWindow::handleSftpOperationFailed(const QString &errorMessage)
{
    m_ui->outputTextEdit->appendPlainText(errorMessage);
}

void SftpFsWindow::handleSftpOperationFinished(SftpJobId jobId, const QString &error)
{
    QString message;
    if (error.isEmpty())
        message = tr("Operation %1 finished successfully.").arg(jobId);
    else
        message = tr("Operation %1 failed: %2.").arg(jobId).arg(error);
    m_ui->outputTextEdit->appendPlainText(message);
}

void SftpFsWindow::setHostNameToConnectTo(QString _hostName, QString _userName, QString _password, QString _lastPath)
{
    m_ui->hostLineEdit->setText(_hostName);
    m_ui->userLineEdit->setText(_userName);
    m_ui->passwordLineEdit->setText(_password);
}

void SftpFsWindow::handleConnectionError(const QString &errorMessage)
{
    QMessageBox::warning(this, tr("Connection Error"),
        tr("Fatal SSH error: %1").arg(errorMessage));
    //QCoreApplication::quit();
}

void SftpFsWindow::handleConnectionSuccess()
{
    qDebug() << "Connection success, updating credentials...";

    QString hostName = m_ui->hostLineEdit->text();
    QString userName = m_ui->userLineEdit->text();
    QString password = m_ui->passwordLineEdit->text();

    QSqlQuery query("update hosts set username = '" + userName + "' where hostname = '" + hostName + "'" );
    //bool updateSuccess = query.exec();

    //qDebug() << "Succ " << updateSuccess;
}

QSqlError SftpFsWindow::connectToDatabase()
{
    QSqlError err;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("hostmanager.db");

    if (!db.open()) {
        err = db.lastError();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(db.connectionName());
    }

    return err;
}
