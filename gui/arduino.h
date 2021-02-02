#ifndef ARDUINO_H
#define ARDUINO_H
//----------------------------------------------------------------------
#include <QObject>
#include <QSerialPort>
//----------------------------------------------------------------------

class Arduino : public QObject
{
    Q_OBJECT

private:
    const char *MESSAGE_SELECT_NONE    = "!@#$NONE";
    const char *MESSAGE_SELECT_C16     = "!@#$C16 ";
    const char *MESSAGE_SELECT_C32     = "!@#$C32 ";
    const char *MESSAGE_SELECT_C64     = "!@#$C64 ";
    const char *MESSAGE_SELECT_C128    = "!@#$C128";
    const char *MESSAGE_SELECT_C256    = "!@#$C256";
    const char *MESSAGE_SELECT_C512    = "!@#$C512";
    const char *MESSAGE_VOLTAGE_INFO   = "!@#$VINF";
    const char *MESSAGE_READ_CHIP      = "!@#$READ";
    const char *MESSAGE_WRITE_CHIP     = "!@#$WRIT";
    const char *RESPONSE_READ_CHIP     = "$#@!READ";
    const char *RESPONSE_WRITE_CHIP    = "$#@!WRIT";
    const char *RESPONSE_ERROR         = "$#@!ERR ";
    const char *RESPONSE_BLOCK_REQUEST = "$#@!BLCK";
    const char *RESPONSE_OK            = "$#@!OK  ";
    const char *RESPONSE_VOLTAGEINFO   = "$#@!VINF";

    int maxBufferSize = 0;
    QByteArray readBuffer;
    QByteArray writeBuffer;
    QSerialPort *serialPort = nullptr;
    QMetaObject::Connection serialDataConnection;

    void Send(const QByteArray &data);

private slots:
    void SelectChipSlot(void);
    void ReadChipSlot(void);
    void WriteChipSlot(void);
    void ReadVoltageSlot(void);

public:
    enum CHIP_TYPE {
        NONE,
        C16,
        C32,
        C64,
        C128,
        C256,
        C512
    };

    explicit Arduino(QSerialPort *);
    int GetChipSize(void);
    QByteArray *GetReadBuffer(void);
    void SelectChip(CHIP_TYPE);
    void ReadChip(void);
    void WriteChip(QByteArray);
    void ReadVoltage(void);
    void ResetVariables(void);

signals:
    void ReadBlockSignal(uint16_t);
    void ReadCompleteSignal(void);
    void WriteBlockSignal(uint16_t);
    void WriteCompleteSignal(void);
    void WriteErrorSignal(uint16_t, char *);
    void VoltageUpdatedSignal(double);
    void SerialOperationStartSignal(void);
    void SerialOperationCompleteSignal(void);

private:
    CHIP_TYPE selectedChipType = CHIP_TYPE::NONE;

};
//----------------------------------------------------------------------
#endif // ARDUINO_H
