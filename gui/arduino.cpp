#include "arduino.h"
#include <QDebug>

//----------------------------------------------------------------------

Arduino::Arduino(QSerialPort *port)
{
    serialPort = port;
    ResetVariables();
}
//----------------------------------------------------------------------

void Arduino::ResetVariables(void)
{
    maxBufferSize = 0;
    readBuffer.clear();
    writeBuffer.clear();
}
//----------------------------------------------------------------------

int Arduino::GetChipSize(void)
{
    return maxBufferSize;
}
//----------------------------------------------------------------------

QByteArray *Arduino::GetReadBuffer(void)
{
    return &readBuffer;
}
//----------------------------------------------------------------------

void Arduino::Send(const QByteArray &data)
{
    emit SerialOperationStartSignal();
    serialPort->write(data);
}
//----------------------------------------------------------------------

void Arduino::ReadChip(void)
{
    readBuffer.clear();
    serialDataConnection = QObject::connect(serialPort, SIGNAL(readyRead()), this, SLOT(ReadChipSlot()));
    Send(MESSAGE_READ_CHIP);
}
//----------------------------------------------------------------------

void Arduino::ReadChipSlot(void)
{
    while (!serialPort->atEnd()) 
    {
        QByteArray readData = serialPort->read(64);
        QString str = RESPONSE_OK;
        str.append("\r\n");

        int index = 0;
        if((index = readData.indexOf(str, 0)) != -1)
        {
            readData.remove(readBuffer.length() ? index : 0, (readBuffer.length() ? 0 : index) + str.length());

            str = RESPONSE_READ_CHIP;
            str.append("\r\n");

            if((index = readData.indexOf(str, 0)) != -1) {
                readData.remove(0, index + str.length());
            }
        }

        readBuffer.append(readData);
        emit ReadBlockSignal(static_cast<uint16_t>(readBuffer.length()));
    }
    if (readBuffer.length() >= maxBufferSize) 
    {
        if(readBuffer.length() > maxBufferSize) {
            readBuffer.resize(maxBufferSize);
        }
        QObject::disconnect(serialDataConnection);
        emit ReadCompleteSignal();
        emit SerialOperationCompleteSignal();
    }
}
//----------------------------------------------------------------------

void Arduino::WriteChip(QByteArray data)
{
    writeBuffer.clear();
    writeBuffer.append(data);
    if(writeBuffer.length() != maxBufferSize)
    {
        QString errorMessage = "Invalid data length of ";
        errorMessage.append(QString::number(writeBuffer.length()));
        emit WriteErrorSignal(0, reinterpret_cast<char *>(errorMessage.data()));
        return;
    }

    serialDataConnection = QObject::connect(serialPort, SIGNAL(readyRead()), this, SLOT(WriteChipSlot()));
    Send(MESSAGE_WRITE_CHIP);
}
//----------------------------------------------------------------------

void Arduino::WriteChipSlot(void)
{
    QByteArray readData;
    QObject::disconnect(serialDataConnection);
    while(!serialPort->atEnd())
    {
        readData = serialPort->readAll();

        QString str = RESPONSE_OK;
        str.append("\r\n");

        int index = 0;
        if((index = readData.indexOf(str, 0)) != -1)
        {
            str = RESPONSE_ERROR;

            if((index = readData.indexOf(str, 0)) != -1)
            {
                serialPort->waitForReadyRead(100);
                readData.append(serialPort->readAll());
                readData.remove(0, index + str.length());
                emit WriteErrorSignal(0, readData.data());
                emit SerialOperationCompleteSignal();
                return;
            }

            str = RESPONSE_WRITE_CHIP;
            str.append("\r\n");

            if((index = readData.indexOf(str, 0)) != -1)
            {
                readData.remove(0, index + str.length());
                break;
            }
        }
    }

    serialPort->waitForReadyRead(100);

    for(int i = 0; i < maxBufferSize; i += 16)
    {
        readData.append(serialPort->readAll());

        QString str = RESPONSE_BLOCK_REQUEST;
        int index = 0, blockIndex = 0;
        if((index = readData.indexOf(str, 0)) != -1)
        {
            readData.remove(0, index + str.length());
            blockIndex = QString(readData).simplified().toInt();
        }
        else
        {
            QString errorMessage = "Invalid acknowledge data received";
            emit WriteErrorSignal(0, reinterpret_cast<char *>(errorMessage.data()));
            emit SerialOperationCompleteSignal();
            break;
        }

        if(i != blockIndex)
        {
            QString errorMessage = "Invalid block ";
            errorMessage.append(QString::number(blockIndex, 16));
            errorMessage.append(" received, expected ");
            errorMessage.append(QString::number(i, 16));
            emit WriteErrorSignal(0, reinterpret_cast<char *>(errorMessage.data()));
            emit SerialOperationCompleteSignal();
            break;
        }

        char data[16];
        memcpy(data, &writeBuffer.data()[i], 16);

        serialPort->write(data, 16);

        serialPort->waitForReadyRead(selectedChipType == C16 ? 320 : 100);
        readData.clear();
        readData.append(serialPort->readAll());

        str = RESPONSE_ERROR;

        if((index = readData.indexOf(str, 0)) != -1)
        {
            serialPort->waitForReadyRead(100);
            readData.append(serialPort->readAll());
            readData.remove(0, index + str.length());
            emit WriteErrorSignal(0, readData.data());
            emit SerialOperationCompleteSignal();
            return;
        }

        str = RESPONSE_OK;
        str.append("\r\n");

        if((index = readData.indexOf(str, 0)) == -1) {
            serialPort->waitForReadyRead(250);
        }

        if((index = readData.indexOf(str, 0)) == -1)
        {
            QString errorMessage = "Can't acknowledge block ";
            errorMessage.append(QString::number(i, 16));
            emit WriteErrorSignal(0, reinterpret_cast<char *>(errorMessage.data()));
            emit SerialOperationCompleteSignal();
        }
        readData.remove(0, index + str.length());

        emit WriteBlockSignal(static_cast<uint16_t>(i));
    }

    emit WriteCompleteSignal();
    emit SerialOperationCompleteSignal();
}
//----------------------------------------------------------------------

void Arduino::SelectChip(CHIP_TYPE type)
{
    serialDataConnection = QObject::connect(serialPort, SIGNAL(readyRead()), this, SLOT(SelectChipSlot()));

    switch(type)
    {
        case C16:
            maxBufferSize = 0x07FF + 1;
            Send(MESSAGE_SELECT_C16);
            break;
        case C32:
            maxBufferSize = 0x0FFF + 1;
            Send(MESSAGE_SELECT_C32);
            break;
        case C64:
            maxBufferSize = 0x1FFF + 1;
            Send(MESSAGE_SELECT_C64);
            break;
        case C128:
            maxBufferSize = 0x3FFF + 1;
            Send(MESSAGE_SELECT_C128);
            break;
        case C256:
            maxBufferSize = 0x7FFF + 1;
            Send(MESSAGE_SELECT_C256);
            break;
        case C512:
            maxBufferSize = 0xFFFF + 1;
            Send(MESSAGE_SELECT_C512);
            break;
        default:
            maxBufferSize = 0;
            Send(MESSAGE_SELECT_NONE);
    }

    selectedChipType = type;
}
//----------------------------------------------------------------------

void Arduino::SelectChipSlot(void)
{
    while(!serialPort->atEnd())
    {
        QByteArray readData = serialPort->readAll();
        if(readData.indexOf(RESPONSE_OK, 0) != -1)
        {
            QObject::disconnect(serialDataConnection);
            emit SerialOperationCompleteSignal();
        }
    }
}
//----------------------------------------------------------------------

void Arduino::ReadVoltage(void)
{
    serialDataConnection = QObject::connect(serialPort, SIGNAL(readyRead()), this, SLOT(ReadVoltageSlot()));
    Send(MESSAGE_VOLTAGE_INFO);
}
//----------------------------------------------------------------------

void Arduino::ReadVoltageSlot(void)
{
    while(!serialPort->atEnd())
    {
        QByteArray readData = serialPort->readAll();

        QString str = RESPONSE_OK;
        str.append("\r\n");

        int index = 0;
        if((index = readData.indexOf(str, 0)) != -1)
        {
            readData.remove(0, index + str.length());
            str = RESPONSE_VOLTAGEINFO;
            if((index = readData.indexOf(str, 0)) != -1)
            {
                readData.remove(0, index + str.length());
                QObject::disconnect(serialDataConnection);
                emit VoltageUpdatedSignal(QString(readData).simplified().toDouble() * 100);
                emit SerialOperationCompleteSignal();
            }
        }
    }
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
