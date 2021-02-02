#ifndef MAINWINDOW_H
#define MAINWINDOW_H
//----------------------------------------------------------------------
#include "arduino.h"
#include <QMainWindow>
#include <QSerialPort>
#include <QListWidgetItem>
#include <QTimer>
//----------------------------------------------------------------------

namespace Ui {
class MainWindow;
}
//----------------------------------------------------------------------

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void on_openFileButton_clicked(void);
    void on_saveFileButton_clicked(void);
    void on_readChipButton_clicked(void);
    void on_writeChipButton_clicked(void);
    void on_verifyChipButton_clicked(void);
    void on_c16Button_clicked(void);
    void on_c32Button_clicked(void);
    void on_c64Button_clicked(void);
    void on_c128Button_clicked(void);
    void on_c256Button_clicked(void);
    void on_c512Button_clicked(void);
    void on_connectButton_clicked(void);
    void on_disconnectButton_clicked(void);
    void on_updateButton_clicked(void);
    void on_showButton_toggled(bool);
    void on_portList_itemClicked(QListWidgetItem *);
    void on_voltageChipButton_toggled(bool);

    void CheckClearChipSlot(void);
    void VerifyDataWrittenSlot(void);
    void ReloadPortsSlot(void);
    void ShowVoltageSlot(void);
    void UpdateVoltageValueSlot(double);
    void ChipOperationProgressBarSlot(uint16_t);
    void WriteCompleteAcknowledgeSlot(void);
    void UpdateCursorOnSerialOperationStartSlot(void);
    void UpdateCursorOnSerialOperationCompleteSlot(void);
    void WriteCompleteErrorSlot(uint16_t, char *);

private:
    const char CHECK_NO_ERROR = 0;
    const char CHECK_ERROR_WRITABLE = 1;
    const char CHECK_ERROR_UNWRITABLE = 2;
    const char *PROGRAMMER_NAME = "Arduino 27CXXX EEPROM programmer";

    Ui::MainWindow *ui;
    QSerialPort *serialPort = nullptr;
    Arduino *arduino = nullptr;

    QTimer updatePortsTimer;
    QTimer updateVoltageTimer;

    QMetaObject::Connection reloadPortsConnection;
    QMetaObject::Connection progressBarConnection;
    QMetaObject::Connection verifyDataWrittenConnection;
    QMetaObject::Connection checkClearConnection;
    QMetaObject::Connection updateBufferConnection;
    QMetaObject::Connection updateVoltageTimerConnection;
    QMetaObject::Connection updateVoltageValueConnection;
    QMetaObject::Connection writeEndConnection;
    QMetaObject::Connection writeErrorConnection;
    QMetaObject::Connection serialOperationStartConnection;
    QMetaObject::Connection serialOperationCompleteConnection;

    Arduino::CHIP_TYPE selectedChip = Arduino::NONE;

    QByteArray checkBuffer;
    QByteArray fileLoadBuffer;

    bool fileLoaded = false;
    bool chipRead = false;
    bool chipWritten = false;
    bool chipVerified = false;

    void ResetAllButtons(void);
    void Log(QString str);
    void OpenSerialPort(QString);
    void CloseSerialPort();
    void UpdateButtonsOnConnect(void);
    void ResetVaribles(void);
    void UpdateButtons(void);
    void ShowBuffer(void);
    QIcon* GetGuiIcon(void);
};
//----------------------------------------------------------------------
#endif // MAINWINDOW_H
