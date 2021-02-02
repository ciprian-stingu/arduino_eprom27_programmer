#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTimer>
#include "icon.h"
//----------------------------------------------------------------------

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    serialPort(new QSerialPort) 
{
    ui->setupUi(this);
    QIcon *mainIcon = GetGuiIcon();
    this->setWindowIcon(*mainIcon);
    delete mainIcon;

    ResetAllButtons();
    ResetVaribles();
    reloadPortsConnection = QObject::connect(&updatePortsTimer, SIGNAL(timeout()), this, SLOT(ReloadPortsSlot()));
    updatePortsTimer.setInterval(1000);
    updatePortsTimer.start();
    ReloadPortsSlot();

    this->setFixedSize(QSize(371, 431));
}
//----------------------------------------------------------------------

MainWindow::~MainWindow()
{
    delete ui;
}
//----------------------------------------------------------------------

QIcon* MainWindow::GetGuiIcon(void)
{
    QPixmap *pixData = new QPixmap(guiIcon);
    QIcon *icon = new QIcon(*pixData);
    delete pixData;
    return icon;
}
//----------------------------------------------------------------------

void MainWindow::UpdateCursorOnSerialOperationStartSlot(void)
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}
//----------------------------------------------------------------------

void MainWindow::UpdateCursorOnSerialOperationCompleteSlot(void)
{
    QApplication::restoreOverrideCursor();
}
//----------------------------------------------------------------------

void MainWindow::Log(QString str)
{
    ui->textBrowser->append(str);
    ui->textBrowser->moveCursor(QTextCursor::End);
}
//----------------------------------------------------------------------

void MainWindow::OpenSerialPort(QString path)
{
    serialPort->setPortName(path);
    serialPort->setBaudRate(QSerialPort::Baud115200);

    if(serialPort->open(QIODevice::ReadWrite))
    {
        QByteArray readData = serialPort->readAll();
        while(serialPort->waitForReadyRead(5000))
        {
            readData.append(serialPort->readAll());
            //Log(readData);
            if(readData.indexOf(PROGRAMMER_NAME, 0) != -1)
            {
                Log(QString("Connect successful"));

                arduino = new Arduino(serialPort);

                serialOperationStartConnection = QObject::connect(arduino, SIGNAL(SerialOperationStartSignal()), this, SLOT(UpdateCursorOnSerialOperationStartSlot()));
                serialOperationCompleteConnection = QObject::connect(arduino, SIGNAL(SerialOperationCompleteSignal()), this, SLOT(UpdateCursorOnSerialOperationCompleteSlot()));

                selectedChip = Arduino::NONE;
                arduino->SelectChip(selectedChip);

                UpdateButtonsOnConnect();

                return;
            }
        }
        Log(QString("Arduino programmer not found."));
        CloseSerialPort();
    } 
    else 
    {
        QMessageBox::critical(this, tr("Error"), serialPort->errorString());
    }
}
//----------------------------------------------------------------------

void MainWindow::ResetVaribles(void)
{
    fileLoaded = false;
    chipRead = false;
    chipWritten = false;
    chipVerified = false;
    checkBuffer.clear();
    fileLoadBuffer.clear();
}
//----------------------------------------------------------------------

void MainWindow::ResetAllButtons(void)
{
    ui->connectButton->setEnabled(false);
    ui->updateButton->setEnabled(false);
    ui->disconnectButton->setEnabled(false);
    ui->voltageChipButton->setEnabled(false);
    ui->openFileButton->setEnabled(false);
    ui->saveFileButton->setEnabled(false);
    ui->readChipButton->setEnabled(false);
    ui->writeChipButton->setEnabled(false);
    ui->verifyChipButton->setEnabled(false);

    ui->showButton->setChecked(false);
    ui->showButton->setEnabled(false);

    ui->c16Button->setEnabled(false);
    ui->c16Button->setAutoExclusive(false);
    ui->c16Button->setChecked(false);

    ui->c32Button->setEnabled(false);
    ui->c32Button->setAutoExclusive(false);
    ui->c32Button->setChecked(false);

    ui->c64Button->setEnabled(false);
    ui->c64Button->setAutoExclusive(false);
    ui->c64Button->setChecked(false);

    ui->c128Button->setEnabled(false);
    ui->c128Button->setAutoExclusive(false);
    ui->c128Button->setChecked(false);

    ui->c256Button->setEnabled(false);
    ui->c256Button->setAutoExclusive(false);
    ui->c256Button->setChecked(false);

    ui->c512Button->setEnabled(false);
    ui->c512Button->setAutoExclusive(false);
    ui->c512Button->setChecked(false);
}
//----------------------------------------------------------------------

void MainWindow::UpdateButtonsOnConnect(void)
{
    ui->updateButton->setEnabled(false);
    ui->disconnectButton->setEnabled(true);
    ui->connectButton->setEnabled(false);
    ui->voltageChipButton->setEnabled(true);

    ui->c16Button->setEnabled(true);
    ui->c16Button->setAutoExclusive(true);

    ui->c32Button->setEnabled(true);
    ui->c32Button->setAutoExclusive(true);

    ui->c64Button->setEnabled(true);
    ui->c64Button->setAutoExclusive(true);

    ui->c128Button->setEnabled(true);
    ui->c128Button->setAutoExclusive(true);

    ui->c256Button->setEnabled(true);
    ui->c256Button->setAutoExclusive(true);

    ui->c512Button->setEnabled(true);
    ui->c512Button->setAutoExclusive(true);
}
//----------------------------------------------------------------------

void MainWindow::UpdateButtons(void)
{
    //Log("UpdateButtons [" + QString::number(!!updateVoltageTimerConnection) + ", " + QString::number(selectedChip)
    //    + ", " + QString::number(fileLoaded) + ", " + QString::number(chipRead) + ", " + QString::number(chipWritten)
    //    + ", " + QString::number(!!checkClearConnection) + ", " + QString::number(!!writeEndConnection)
    //    + ", " + QString::number(!!verifyDataWrittenConnection) + "]");

    if(updateVoltageTimerConnection)
    {
        ui->disconnectButton->setEnabled(false);
        ui->openFileButton->setEnabled(false);
        ui->saveFileButton->setEnabled(false);
        ui->readChipButton->setEnabled(false);
        ui->writeChipButton->setEnabled(false);
        ui->verifyChipButton->setEnabled(false);
        ui->showButton->setEnabled(false);

        ui->c16Button->setEnabled(false);
        ui->c32Button->setEnabled(false);
        ui->c64Button->setEnabled(false);
        ui->c128Button->setEnabled(false);
        ui->c256Button->setEnabled(false);
        ui->c512Button->setEnabled(false);
    }
    else
    {
        ui->disconnectButton->setEnabled(true);
        ui->c16Button->setEnabled(true);
        ui->c32Button->setEnabled(true);
        ui->c64Button->setEnabled(true);
        ui->c128Button->setEnabled(true);
        ui->c256Button->setEnabled(true);
        ui->c512Button->setEnabled(true);

        ui->openFileButton->setEnabled(selectedChip != Arduino::NONE);
        ui->readChipButton->setEnabled(selectedChip != Arduino::NONE);

        if(selectedChip != Arduino::NONE)
        {
            if(fileLoaded) {
                ui->writeChipButton->setEnabled(true);
            }
            else {
                ui->writeChipButton->setEnabled(false);
            }


            if(chipRead)
            {
                ui->saveFileButton->setEnabled(true);
                ui->showButton->setEnabled(true);
            }
            else
            {
                ui->saveFileButton->setEnabled(false);
                ui->showButton->setChecked(false);
                ui->showButton->setEnabled(false);
            }

            if(chipWritten) {
                ui->verifyChipButton->setEnabled(true);
            }
            else {
                ui->verifyChipButton->setEnabled(false);
            }

            if(checkClearConnection || writeEndConnection || verifyDataWrittenConnection)
            {
                ui->disconnectButton->setEnabled(false);
                ui->openFileButton->setEnabled(false);
                ui->saveFileButton->setEnabled(false);
                ui->readChipButton->setEnabled(false);
                ui->writeChipButton->setEnabled(false);
                ui->verifyChipButton->setEnabled(false);
                ui->showButton->setEnabled(false);
                ui->voltageChipButton->setEnabled(false);

                ui->c16Button->setEnabled(false);
                ui->c32Button->setEnabled(false);
                ui->c64Button->setEnabled(false);
                ui->c128Button->setEnabled(false);
                ui->c256Button->setEnabled(false);
                ui->c512Button->setEnabled(false);
            }
            else
            {
                ui->voltageChipButton->setEnabled(true);
            }
        }
        else
        {
            ui->writeChipButton->setEnabled(false);
            ui->verifyChipButton->setEnabled(false);
            ui->saveFileButton->setEnabled(false);
            ui->showButton->setEnabled(false);
        }

    }
}
//----------------------------------------------------------------------

void MainWindow::CloseSerialPort(void)
{
    QObject::disconnect(serialOperationStartConnection);
    QObject::disconnect(serialOperationCompleteConnection);

    delete arduino;
    selectedChip = Arduino::NONE;
    ResetAllButtons();
    ResetVaribles();

    ui->portList->clear();
    updatePortsTimer.start();

    if(serialPort->isOpen())
    {
        Log(QString("Disconnect..."));
        serialPort->close();
    }
}
//----------------------------------------------------------------------

void MainWindow::ReloadPortsSlot(void)
{
    ui->portList->clear();
    const auto infos = QSerialPortInfo::availablePorts();
    for(const QSerialPortInfo &info : infos)
    {
        QListWidgetItem *item = new QListWidgetItem(info.portName(), ui->portList);
        item->setData(Qt::UserRole, info.systemLocation());
        if(info.isBusy())
        {
            item->setText(info.portName() + " (Busy)");
            item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        }
    }
}
//----------------------------------------------------------------------

void MainWindow::CheckClearChipSlot(void)
{
    QObject::disconnect(checkClearConnection);
    QObject::disconnect(progressBarConnection);
    chipRead = true;

    UpdateButtons();

    uint8_t *dataRead = reinterpret_cast<uint8_t *>(arduino->GetReadBuffer()->data());
    for(int i = 0, j = arduino->GetReadBuffer()->length(); i < j; i++)
    {
        if(dataRead[i] != 0xFF)
        {
            Log(QString("Chip not clear."));
            return;
        }
    }
    Log(QString("Chip clear."));
}
//----------------------------------------------------------------------

void MainWindow::VerifyDataWrittenSlot(void)
{
    QObject::disconnect(verifyDataWrittenConnection);
    QObject::disconnect(progressBarConnection);
    chipRead = true;
    chipVerified = true;

    UpdateButtons();

    checkBuffer.clear();
    checkBuffer.resize(arduino->GetReadBuffer()->length());
    checkBuffer.fill(CHECK_NO_ERROR);

    uint8_t *dataRead = reinterpret_cast<uint8_t *>(arduino->GetReadBuffer()->data());
    uint8_t *fileData = reinterpret_cast<uint8_t *>(fileLoadBuffer.data());

    int errorsCount = 0, warningsCount = 0;
    for(int i = 0, j = arduino->GetReadBuffer()->length(); i < j; i++)
    {
        if((dataRead[i] ^ fileData[i]) & fileData[i])
        {
            checkBuffer[i] = CHECK_ERROR_UNWRITABLE;
            errorsCount++;
        }
        else if (dataRead[i] != fileData[i])
        {
            checkBuffer[i] = CHECK_ERROR_WRITABLE;
            warningsCount++;
        }
    }

    if (errorsCount || warningsCount)
    {
        Log(QString("Verification failed."));
        Log(QString("Errors: %1.").arg(errorsCount));
        Log(QString("Warnings: %1.").arg(warningsCount));
    }
    else
    {
        Log(QString("Verification successful."));
    }
}
//----------------------------------------------------------------------

void MainWindow::ShowBuffer(void)
{
    if(!chipRead) {
        return;
    }

    QTableWidget *tableWidget = ui->tableWidget;
    QTableWidgetItem *newItem;

    //Font
    QFont font("Monospace", 9);
    font.setStyleHint(QFont::Monospace);
    font.setWeight(QFont::Bold);

    tableWidget->setFixedWidth(700);

    // clear table
    tableWidget->setRowCount(0);

    // prepare layout
    tableWidget->setStyleSheet("QTableWidget::item { padding: 0px, margin: 0px }");
    tableWidget->horizontalHeader()->setVisible(true);
    tableWidget->verticalHeader()->setVisible(true);
    tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // columns
    tableWidget->setColumnCount(33);
    for(int i = 0; i < 33; i++) {
        tableWidget->setColumnWidth(i, 5);
    }
    // headers for hex data
    for(int i = 0; i < 16; i++)
    {
        newItem = new QTableWidgetItem(tr("%1").arg(QString::asprintf("%02X", i)));
        newItem->setFont(font);
        tableWidget->setHorizontalHeaderItem(i, newItem);
    }
    // headers for other items
    for(int i = 16; i < 33; i++)
    {
        newItem = new QTableWidgetItem(tr(""));
        newItem->setFont(font);
        tableWidget->setHorizontalHeaderItem(i, newItem);
    }

    // rows
    tableWidget->setRowCount((arduino->GetChipSize() + 1) / 16);

    for(int row = 0; row < tableWidget->rowCount(); ++row)
    {
        tableWidget->setRowHeight(row, 5);

        // set row header
        newItem = new QTableWidgetItem(tr("%1").arg(QString::asprintf("%04X", (row * 16))));
        newItem->setFont(font);
        tableWidget->setVerticalHeaderItem(row, newItem);

        for(int column = 0; column < 16; ++column)
        {
            // hex data
            newItem = new QTableWidgetItem(QString::asprintf("%02X", static_cast<uint8_t>(arduino->GetReadBuffer()->data()[row * 16 + column])));
            newItem->setFont(font);
            newItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

            if(chipVerified)
            {
                if (checkBuffer.data()[row * 16 + column] == CHECK_ERROR_UNWRITABLE) {
                    newItem->setForeground(QColor::fromRgb(255, 0, 0));
                }
                else if (checkBuffer.data()[row * 16 + column] == CHECK_ERROR_WRITABLE) {
                    newItem->setForeground(QColor::fromRgb(0, 0, 255));
                }
                else {
                    newItem->setForeground(QColor::fromRgb(0, 0, 0));
                }
            }
            else {
                newItem->setForeground(QColor::fromRgb(0, 0, 0));
            }

            tableWidget->setItem(row, column, newItem);

            // char data
            newItem = new QTableWidgetItem(QString::asprintf("%c", static_cast<uint8_t>(arduino->GetReadBuffer()->data()[row * 16 + column])));
            newItem->setFont(font);
            newItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            tableWidget->setItem(row, column + 17, newItem);
        }
    }
}
//----------------------------------------------------------------------

void MainWindow::WriteCompleteAcknowledgeSlot(void)
{
    QObject::disconnect(writeEndConnection);
    QObject::disconnect(progressBarConnection);
    QObject::disconnect(writeErrorConnection);
    chipWritten = true;

    UpdateButtons();
}
//----------------------------------------------------------------------

void MainWindow::WriteCompleteErrorSlot(uint16_t address, char *message)
{
    QObject::disconnect(writeEndConnection);
    QObject::disconnect(progressBarConnection);
    QObject::disconnect(writeErrorConnection);
    chipWritten = false;
    UpdateButtons();

    QString errorMessage = "Write error for block 0x";
    errorMessage.append(QString::number(address, 16));
    errorMessage.append(", ");
    errorMessage.append(message);
    Log(errorMessage);
}
//----------------------------------------------------------------------

void MainWindow::ChipOperationProgressBarSlot(uint16_t value)
{
    ui->progressBar->setValue(static_cast<int>(value));
}
//----------------------------------------------------------------------

void MainWindow::UpdateVoltageValueSlot(double value)
{
    QObject::disconnect(updateVoltageValueConnection);
    if(ui->voltageChipButton->isChecked())
    {
        updateVoltageTimer.setInterval(250);
        updateVoltageTimer.start();
        ui->progressBar->setValue(static_cast<int>(value));

        QString text;
        text.sprintf("%02.2f", value / 100.0);
        ui->progressBar->setFormat(text + QString(" V"));
    }
}
//----------------------------------------------------------------------

void MainWindow::ShowVoltageSlot(void)
{
    updateVoltageTimer.stop();
    updateVoltageValueConnection = QObject::connect(arduino, SIGNAL(VoltageUpdatedSignal(double)), this, SLOT(UpdateVoltageValueSlot(double)));
    arduino->ReadVoltage();
}
//----------------------------------------------------------------------

void MainWindow::on_showButton_toggled(bool checked)
{
    if(checked && chipRead)
    {
        this->setFixedSize(QSize(1090, 431));
        ShowBuffer();
    }
    else {
        this->setFixedSize(QSize(371, 431));
    }
}
//----------------------------------------------------------------------

void MainWindow::on_openFileButton_clicked(void)
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load binary file"), "", tr("Binary (*.bin *.rom);;All Files (*)"));

    if(fileName.isEmpty()) {
        return;
    }
    else
    {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly))
        {
            QMessageBox::information(this, tr("Unable to open file"), file.errorString());
            return;
        }

        fileLoadBuffer.clear();
        fileLoadBuffer.append(file.readAll());

        Log(QString("Load from %1 file").arg(fileName));
        Log(QString("Read %1 bytes").arg(fileLoadBuffer.count()));
        if (fileLoadBuffer.count() < arduino->GetChipSize())
        {
            Log(QString("Padding by %1 bytes").arg(arduino->GetChipSize() - fileLoadBuffer.count()));
            fileLoadBuffer.append((arduino->GetChipSize() - fileLoadBuffer.count()), static_cast<char>(0xFF));
        }
        else if(fileLoadBuffer.count() > arduino->GetChipSize())
        {
            Log(QString("Deleted %1 bytes").arg(fileLoadBuffer.count() - arduino->GetChipSize()));
            fileLoadBuffer.resize(arduino->GetChipSize());
        }
        fileLoaded = true;
        ui->showButton->setChecked(false);

        UpdateButtons();
    }
}
//----------------------------------------------------------------------

void MainWindow::on_saveFileButton_clicked(void)
{
    if(!chipRead) {
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save buffer"), "", tr("Binary (*.bin);;Rom (*.rom);;All Files (*)"));

    if(fileName.isEmpty()) {
        return;
    }
    else
    {
        if(fileName.right(4).indexOf(".bin", 0) == -1 && fileName.right(4).indexOf(".rom", 0) == -1) {
            fileName.append(QString(".bin"));
        }

        QFile file(fileName);
        if(!file.open(QIODevice::WriteOnly))
        {
            QMessageBox::information(this, tr("Unable to open file"), file.errorString());
            return;
        }

        file.write(*arduino->GetReadBuffer());
        file.close();
        Log(QString("Buffer saved to %1 file").arg(fileName));
    }
}
//----------------------------------------------------------------------

void MainWindow::on_writeChipButton_clicked(void)
{
    if(!fileLoaded) {
        return;
    }

    ui->progressBar->setMaximum(arduino->GetChipSize());
    Log(QString("Writing %1 bytes to chip...").arg(arduino->GetChipSize()));
    progressBarConnection = QObject::connect(arduino, SIGNAL(WriteBlockSignal(uint16_t)), this, SLOT(ChipOperationProgressBarSlot(uint16_t)));
    writeEndConnection = QObject::connect(arduino, SIGNAL(WriteCompleteSignal()), this, SLOT(WriteCompleteAcknowledgeSlot()));
    writeErrorConnection = QObject::connect(arduino, SIGNAL(WriteErrorSignal(uint16_t, char*)), this, SLOT(WriteCompleteErrorSlot(uint16_t, char*)));
    chipRead = false;
    chipWritten = false;
    chipVerified = false;
    UpdateButtons();
    arduino->WriteChip(fileLoadBuffer);
}
//----------------------------------------------------------------------

void MainWindow::on_readChipButton_clicked(void)
{
    ui->progressBar->setMaximum(arduino->GetChipSize());
    Log(QString("Reading %1 bytes from chip...").arg(arduino->GetChipSize()));
    progressBarConnection = QObject::connect(arduino, SIGNAL(ReadBlockSignal(uint16_t)), this, SLOT(ChipOperationProgressBarSlot(uint16_t)));
    checkClearConnection = QObject::connect(arduino, SIGNAL(ReadCompleteSignal()), this, SLOT(CheckClearChipSlot()));
    chipRead = false;
    chipVerified = false;
    UpdateButtons();
    arduino->ReadChip();
}
//----------------------------------------------------------------------

void MainWindow::on_updateButton_clicked(void)
{
    ui->updateButton->setEnabled(false);
    ui->connectButton->setEnabled(false);
    ReloadPortsSlot();
    updatePortsTimer.start();
}
//----------------------------------------------------------------------

void MainWindow::on_portList_itemClicked(QListWidgetItem *item)
{
    (void)item;
    ui->connectButton->setEnabled(true);
    ui->updateButton->setEnabled(true);
    ui->disconnectButton->setEnabled(false);

    updatePortsTimer.stop();
}
//----------------------------------------------------------------------

void MainWindow::on_verifyChipButton_clicked(void)
{
    ui->progressBar->setMaximum(arduino->GetChipSize());
    Log(QString("Verifying %1 bytes from chip...").arg(arduino->GetChipSize()));
    progressBarConnection = QObject::connect(arduino, SIGNAL(ReadBlockSignal(uint16_t)), this, SLOT(ChipOperationProgressBarSlot(uint16_t)));
    verifyDataWrittenConnection = QObject::connect(arduino, SIGNAL(ReadCompleteSignal()), this, SLOT(VerifyDataWrittenSlot()));
    chipRead = false;
    chipVerified = false;
    UpdateButtons();
    arduino->ReadChip();
}
//----------------------------------------------------------------------

void MainWindow::on_connectButton_clicked(void)
{
    QListWidgetItem* item = ui->portList->currentItem();

    if(item == nullptr)
    {
        QMessageBox::critical(this, tr("EPROM Programmer"), tr("Select serial port!"));
        return;
    }

    if(!(item->flags() & Qt::ItemIsSelectable))
    {
        QMessageBox::critical(this, tr("EPROM Programmer"), tr("Port is busy!"));
        return;
    }

    Log(QString("Connect to %1").arg(item->data(Qt::UserRole).toString()));
    OpenSerialPort(item->data(Qt::UserRole).toString());
}
//----------------------------------------------------------------------

void MainWindow::on_disconnectButton_clicked(void)
{
    CloseSerialPort();
}
//----------------------------------------------------------------------

void MainWindow::on_c16Button_clicked(void)
{
    chipRead = false;
    chipWritten = false;
    chipVerified = false;
    fileLoaded = false;
    selectedChip = Arduino::C16;
    Log("Select 27C16 chip");
    arduino->SelectChip(selectedChip);
    UpdateButtons();
}
//----------------------------------------------------------------------

void MainWindow::on_c32Button_clicked(void)
{
    chipRead = false;
    chipWritten = false;
    chipVerified = false;
    fileLoaded = false;
    selectedChip = Arduino::C32;
    Log("Select 27C32 chip");
    arduino->SelectChip(selectedChip);
    UpdateButtons();
}
//----------------------------------------------------------------------

void MainWindow::on_c64Button_clicked(void)
{
    chipRead = false;
    chipWritten = false;
    chipVerified = false;
    fileLoaded = false;
    selectedChip = Arduino::C64;
    Log("Select 27C64 chip");
    arduino->SelectChip(selectedChip);
    UpdateButtons();
}
//----------------------------------------------------------------------

void MainWindow::on_c128Button_clicked(void)
{
    chipRead = false;
    chipWritten = false;
    chipVerified = false;
    fileLoaded = false;
    selectedChip = Arduino::C128;
    Log("Select 27C128 chip");
    arduino->SelectChip(selectedChip);
    UpdateButtons();
}
//----------------------------------------------------------------------

void MainWindow::on_c256Button_clicked(void)
{
    chipRead = false;
    chipWritten = false;
    chipVerified = false;
    fileLoaded = false;
    selectedChip = Arduino::C256;
    Log("Select 27C256 chip");
    arduino->SelectChip(selectedChip);
    UpdateButtons();
}
//----------------------------------------------------------------------

void MainWindow::on_c512Button_clicked(void)
{
    chipRead = false;
    chipWritten = false;
    chipVerified = false;
    fileLoaded = false;
    selectedChip = Arduino::C512;
    Log("Select 27C512 chip");
    arduino->SelectChip(selectedChip);
    UpdateButtons();
}
//----------------------------------------------------------------------

void MainWindow::on_voltageChipButton_toggled(bool checked)
{
    if(checked)
    {
        updateVoltageTimerConnection = QObject::connect(&updateVoltageTimer, SIGNAL(timeout()), this, SLOT(ShowVoltageSlot()));

        // Change progressBar params
        ui->progressBar->setMinimum(0);
        ui->progressBar->setMaximum(2500);
        ui->progressBar->setFormat("%v");
        ui->progressBar->setTextVisible(true);

        UpdateButtons();
        ShowVoltageSlot();
    }
    else
    {
        QObject::disconnect(updateVoltageTimerConnection);
        updateVoltageTimer.stop();
        ui->progressBar->setMinimum(0);
        ui->progressBar->setMaximum(100);
        ui->progressBar->setFormat("%p%");
        ui->progressBar->setValue(0);

        UpdateButtons();
    }
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
