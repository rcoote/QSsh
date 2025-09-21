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

#include "HostManagerWindow.h"
#include "ui_HostManagerWindow.h"

#include "remoteprocesstest.h"

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

HostManagerWindow::HostManagerWindow(QWidget *parent) : QDialog(parent), m_ui(new Ui::Window)
{
    m_ui->setupUi(this);

    //Connect all SIGNALS
    connect(m_ui->connectButton, &QAbstractButton::clicked, this, &HostManagerWindow::connectToHost);
    connect(m_ui->downloadButton, &QAbstractButton::clicked, this, &HostManagerWindow::downloadFile);
    connect(m_ui->treeViewHosts, &QTreeView::clicked, this, &HostManagerWindow::treeViewHostsClicked);
    connect(m_ui->saveNotesButton, &QPushButton::clicked, this, &HostManagerWindow::buttonSaveNotesClicked);
    connect(m_ui->pushButtonSendCommand, &QPushButton::clicked, this, &HostManagerWindow::buttonSendCommandClicked);
    connect(m_ui->fileSystemView, &QTreeView::clicked, this, &HostManagerWindow::fileSystemFileClicked);

    QFile file("hosts.txt"_L1);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    hostsModel = new TreeModel(QString::fromUtf8(file.readAll()));
    file.close();

    m_ui->treeViewHosts->setModel(hostsModel);
    QSqlError err = connectToDatabase();

    remoteProcessTest = new RemoteProcessTest();
}

HostManagerWindow::~HostManagerWindow()
{
    delete m_ui;
}

void HostManagerWindow::connectToHost()
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
            this, &HostManagerWindow::handleSftpOperationFailed);
    connect(m_fsModel, &SftpFileSystemModel::connectionError,
            this, &HostManagerWindow::handleConnectionError);
    connect(m_fsModel, &SftpFileSystemModel::sftpOperationFinished,
            this, &HostManagerWindow::handleSftpOperationFinished);

    connect(m_fsModel, &SftpFileSystemModel::sftpConnectionSuccess,
            this, &HostManagerWindow::handleConnectionSuccess);

    m_fsModel->setSshConnection(sshParams);
    m_ui->fileSystemView->setModel(m_fsModel);
}

void HostManagerWindow::downloadFile()
{
    const QModelIndexList selectedIndexes = m_ui->fileSystemView->selectionModel()->selectedIndexes();
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

void HostManagerWindow::treeViewHostsClicked(const QModelIndex &index)
{
    QString hostname = index.data().toString();
    m_currentHostName = hostname;
    QString lastDirectory;
    QString userName;
    QString password;
    QString notes;

    qDebug() << "SftpFsWindow::treeViewHostsClicked Clicked on column: " << index.column() << ", row : " << index.row() <<  ", data :" << index.data().toString();
    qDebug() << "Retrieving credentials from DB in any exist";

    QSqlQuery query("SELECT * FROM hosts where hostname = '" + hostname + "'");
    while (query.next()) {
        hostname = query.value(0).toString();
        userName = query.value(1).toString();
        password = query.value(2).toString();
        lastDirectory = query.value(3).toString();
        notes = query.value(4).toString();
        qDebug() << "Found hostname: " <<hostname;
        qDebug() << "Found userName: " <<userName;
        qDebug() << "Found password: " <<password;
        qDebug() << "Found lastDirectory: " <<lastDirectory;
        qDebug() << "Found notes : " << notes ;

    }

    setHostNameToConnectTo(hostname, userName, password, lastDirectory, notes);
    setSshParams(hostname, userName, password);

    // Check if connectOnClick Checkbox is ticked, and if yes, invoke the connect as well here
    if(m_ui->checkBoxConnectOnClick->isChecked())
        connectToHost();
}

void HostManagerWindow::buttonSaveNotesClicked()
{
    QString hostname = m_currentHostName;
    QString notes = m_ui->hostNotesPlainTextEdit->toPlainText();

    qDebug() << "Storing notes in DB";
    QSqlQuery query("UPDATE hosts set notes = '" + notes + "' WHERE hostname = '" + hostname + "'" );
    auto result  = query.exec();
    qDebug() << "Result: " << result ;
}

void HostManagerWindow::buttonSendCommandClicked()
{
    m_currentCommand = m_ui->remoteCommand->toPlainText();
    remoteProcessTest->setSshParams(m_sshParams);
    remoteProcessTest->run(m_currentCommand);
}

void HostManagerWindow::fileSystemFileClicked(const QModelIndex &index)
{
    QString fulpat = m_fsModel->getFullPath(index);
    qDebug() << "Fullpath. " << m_fsModel->getFullPath(index);
    QString hostname = m_currentHostName;

    qDebug() << "Storing path in DB";
    QSqlQuery query("UPDATE hosts set lastpath = '" + fulpat + "' WHERE hostname = '" + hostname + "'" );
    auto result  = query.exec();
    qDebug() << "Result: " << result ;

}

void HostManagerWindow::handleSftpOperationFailed(const QString &errorMessage)
{
    m_ui->outputTextEdit->appendPlainText(errorMessage);
}

void HostManagerWindow::handleSftpOperationFinished(SftpJobId jobId, const QString &error)
{
    QString message;
    if (error.isEmpty())
        message = tr("Operation %1 finished successfully.").arg(jobId);
    else
        message = tr("Operation %1 failed: %2.").arg(jobId).arg(error);
    m_ui->outputTextEdit->appendPlainText(message);
}

void HostManagerWindow::setHostNameToConnectTo(QString _hostName, QString _userName, QString _password, QString _lastPath, QString _notes)
{
    m_ui->hostLineEdit->setText(_hostName);
    m_ui->userLineEdit->setText(_userName);
    m_ui->passwordLineEdit->setText(_password);
    m_ui->hostNotesPlainTextEdit->setPlainText(_notes);
}

void HostManagerWindow::handleConnectionError(const QString &errorMessage)
{
    QMessageBox::warning(this, tr("Connection Error"),
        tr("Fatal SSH error: %1").arg(errorMessage));
    //QCoreApplication::quit();
}

void HostManagerWindow::handleConnectionSuccess()
{
    qDebug() << "Connection success, updating credentials...";

    QString hostName = m_ui->hostLineEdit->text();
    QString userName = m_ui->userLineEdit->text();
    QString password = m_ui->passwordLineEdit->text();

    QSqlQuery query("update hosts set username = '" + userName + "' where hostname = '" + hostName + "'" );
    //bool updateSuccess = query.exec();

    //qDebug() << "Succ " << updateSuccess;
}

QSqlError HostManagerWindow::connectToDatabase()
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

void HostManagerWindow::setSshParams(QString _hostname, QString _username, QString _password)
{
    m_sshParams.setHost(_hostname);
    m_sshParams.setUserName(_username);
    m_sshParams.authenticationType = SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods;
    m_sshParams.setPassword(_password);
    m_sshParams.setPort(22);
    m_sshParams.timeout = 10;
}


