#define TRUE 1
#define FALSE 0

/* 74HC595 control (address lines) */
#define SHIFT_LATCH_PIN A1
#define SHIFT_CLOCK_PIN A2
#define SHIFT_DATA_PIN  A0
#define ADDRESS_A10_PIN 13

/* Data pins */
#define DATA_B0_PIN 2
#define DATA_B1_PIN 3
#define DATA_B2_PIN 4
#define DATA_B3_PIN 5
#define DATA_B4_PIN 6
#define DATA_B5_PIN 7
#define DATA_B6_PIN 8
#define DATA_B7_PIN 10

/* Chip control */
#define CHIP_ENABLE_PIN   A3
#define OUTPUT_ENABLE_PIN A4
#define POWER_ENABLE_PIN  A5 // For 27C16 and 27C32
#define READ_VOLTAGE_ENABLE_PIN 13 // For 27C16
#define PROGRAMMING_VOLTAGE_ENABLE_C16_PIN   9 // For 27C16
#define PROGRAMMING_VOLTAGE_ENABLE_C32_PIN   12 // For 27C32 and 27C512
#define PROGRAMMING_VOLTAGE_ENABLE_OTHER_PIN 11 // For other

/* Voltage control (for programming chips) */
#define VOLTAGE_CONTROL_PIN A6
#define RESISTOR_TOP_VALUE 9870.0
#define RESISTOR_BOTTOM_VALUE 1482.0

// read buffer length
#define BUF_LEN 16

// commands
#define MESSAGE_COMMAND_FLAG        "!@#$"
#define MESSAGE_SELECT_NONE         "NONE"
#define MESSAGE_SELECT_C16          "C16 "
#define MESSAGE_SELECT_C32          "C32 "
#define MESSAGE_SELECT_C64          "C64 "
#define MESSAGE_SELECT_C128         "C128"
#define MESSAGE_SELECT_C256         "C256"
#define MESSAGE_SELECT_C512         "C512"
#define MESSAGE_VOLTAGE_INFO        "VINF"
#define MESSAGE_READ_CHIP           "READ"
#define MESSAGE_WRITE_CHIP          "WRIT"
#define MESSAGE_RESPONSE_FLAG       "$#@!"
#define MESSAGE_OK                  "OK  "
#define MESSAGE_ERROR               "ERR "
#define MESSAGE_BLOCK               "BLCK"
#define MESSAGE_READ_BYTE           "RDBT"
#define MESSAGE_WRITE_BYTE          "WRBT"

enum CHIP_TYPE {
  NONE = 0,
  C16 = 1,
  C32 = 2,
  C64 = 3,
  C128 = 4,
  C256 = 5,
  C512 = 6
};

enum COMMAND_MODE {
  WAIT,
  READ,
  WRITE,
  VOLTAGE,
  READ_BYTE,
  WRITE_BYTE
};

void SetWriteMode(void);
void SetReadMode(void);
void SetAddress(uint16_t address);
uint8_t GetData(void);
void SetData(uint8_t data);
uint8_t ReadByte(uint16_t address);
void WriteByte(uint16_t address, uint8_t data);
double GetVoltage(void);
void SetProgrammingVoltage(bool enable);
uint16_t GenerateAddress(uint16_t address);
void SelectChip(CHIP_TYPE newChip);
void WaitForData(void);
uint8_t VerifyData(void);
void WaitMillis(unsigned long period);

CHIP_TYPE ChipSelected = NONE;
COMMAND_MODE CommandMode = WAIT;
uint16_t StartAddress = 0x0000;
uint16_t EndAddress = 0x0000;
uint8_t ReadingBuffer[BUF_LEN];
double programmingVoltage = 0.0;

void setup()
{
  // 74HC595 (*2)
  pinMode(SHIFT_LATCH_PIN, OUTPUT);
  pinMode(SHIFT_CLOCK_PIN, OUTPUT);
  pinMode(SHIFT_DATA_PIN,  OUTPUT);
  pinMode(ADDRESS_A10_PIN, OUTPUT);

  // Chip control
  pinMode(CHIP_ENABLE_PIN, OUTPUT);
  pinMode(OUTPUT_ENABLE_PIN, OUTPUT);
  pinMode(POWER_ENABLE_PIN, OUTPUT);
  pinMode(READ_VOLTAGE_ENABLE_PIN, OUTPUT);
  pinMode(PROGRAMMING_VOLTAGE_ENABLE_C16_PIN, OUTPUT);
  pinMode(PROGRAMMING_VOLTAGE_ENABLE_C32_PIN, OUTPUT);
  pinMode(PROGRAMMING_VOLTAGE_ENABLE_OTHER_PIN, OUTPUT);
  digitalWrite(OUTPUT_ENABLE_PIN, HIGH);
  digitalWrite(POWER_ENABLE_PIN, HIGH);
  digitalWrite(READ_VOLTAGE_ENABLE_PIN, HIGH);
  digitalWrite(PROGRAMMING_VOLTAGE_ENABLE_C16_PIN, LOW);
  digitalWrite(PROGRAMMING_VOLTAGE_ENABLE_C32_PIN, LOW);
  digitalWrite(PROGRAMMING_VOLTAGE_ENABLE_OTHER_PIN, LOW);
  digitalWrite(CHIP_ENABLE_PIN, LOW);

  // Data pins
  SetReadMode();

  Serial.begin(115200);
  Serial.println("Arduino 27CXXX EEPROM programmer");
}

void loop()
{
  switch (CommandMode)
  {
    case READ:
      if (ChipSelected == NONE)
      {
        CommandMode = WAIT;
        break;
      }

      Serial.print(MESSAGE_RESPONSE_FLAG);
      Serial.println(MESSAGE_READ_CHIP);

      SetReadMode();

      if (ChipSelected == C16) {
        digitalWrite(READ_VOLTAGE_ENABLE_PIN, LOW);
      }
      digitalWrite(CHIP_ENABLE_PIN, LOW);
      digitalWrite(OUTPUT_ENABLE_PIN, LOW);

      uint8_t buffer[BUF_LEN];
      for (uint16_t i = StartAddress; i < EndAddress; i += BUF_LEN)
      {
        for (uint8_t j = 0; j < BUF_LEN; j++) {
          buffer[j] = ReadByte(i + j);
        }
        Serial.write(buffer, BUF_LEN);
      }

      digitalWrite(OUTPUT_ENABLE_PIN, HIGH);
      digitalWrite(CHIP_ENABLE_PIN, HIGH);

      if (ChipSelected == C16) {
        digitalWrite(READ_VOLTAGE_ENABLE_PIN, HIGH);
      }

      Serial.print(MESSAGE_RESPONSE_FLAG);
      Serial.println(MESSAGE_OK);

      CommandMode = WAIT;
      break;
    case WRITE:
      if (ChipSelected == NONE)
      {
        CommandMode = WAIT;
        break;
      }

      programmingVoltage = GetVoltage();
      if (programmingVoltage <= 6.0)
      {
        Serial.print(MESSAGE_RESPONSE_FLAG);
        Serial.print(MESSAGE_ERROR);
        Serial.print("Low programming voltage (");
        Serial.print(programmingVoltage, 2);
        Serial.println("V)");
        CommandMode = WAIT;
        break;
      }

      Serial.print(MESSAGE_RESPONSE_FLAG);
      Serial.println(MESSAGE_WRITE_CHIP);

      for (uint16_t i = StartAddress; i < EndAddress; i += BUF_LEN)
      {
        Serial.print(MESSAGE_RESPONSE_FLAG);
        Serial.print(MESSAGE_BLOCK);
        Serial.println(i);

        WaitForData();

        uint8_t count = Serial.readBytes((char*)ReadingBuffer, BUF_LEN);
        if (count != BUF_LEN)
        {
          Serial.print(MESSAGE_RESPONSE_FLAG);
          Serial.print(MESSAGE_ERROR);
          Serial.print(count);
          Serial.print(" bytes received for block 0x");
          Serial.println(i, HEX);
          CommandMode = WAIT;
          break;
        }

        for (uint16_t j = 0; j < BUF_LEN; j++)
        {
          // Write byte
          SetWriteMode();
          SetProgrammingVoltage(true);
          WriteByte((i + j), ReadingBuffer[j]);
          SetProgrammingVoltage(false);

          // Verify byte
          uint8_t verify = VerifyData();
          if(ReadingBuffer[j] != verify)
          {
            WaitMillis(1);
            verify = VerifyData();
          }
          
          if(ReadingBuffer[j] != verify)
          {
            Serial.print(MESSAGE_RESPONSE_FLAG);
            Serial.print(MESSAGE_ERROR);
            Serial.print("Wrote 0x");
            Serial.print(ReadingBuffer[j], HEX);
            Serial.print(", read 0x");
            Serial.print(verify, HEX);
            Serial.print(", address 0x");
            Serial.println(i + j, HEX);
            CommandMode = WAIT;
            break;
          }
        }

        if (CommandMode != WRITE) {
          break;
        }

        Serial.print(MESSAGE_RESPONSE_FLAG);
        Serial.println(MESSAGE_OK);
      }

      CommandMode = WAIT;
      break;

    case VOLTAGE:
      Serial.print(MESSAGE_RESPONSE_FLAG);
      Serial.print(MESSAGE_VOLTAGE_INFO);
      Serial.println(GetVoltage(), 2);
      CommandMode = WAIT;
      break;

    case READ_BYTE:
      CommandMode = WAIT;
      break;

    case WRITE_BYTE:
      CommandMode = WAIT;
      break;

    default:
      if (!Serial.available()) {
        break;
      }

      uint8_t count = Serial.readBytes((char*)ReadingBuffer, BUF_LEN);

      while (Serial.available()) {
        Serial.read();
      }

      if (count)
      {
        String command((char*)ReadingBuffer);

        int8_t commandFlagIndex = command.indexOf(MESSAGE_COMMAND_FLAG);
        Serial.print(MESSAGE_RESPONSE_FLAG);
        if (commandFlagIndex != -1)
        {
          if (command.indexOf(MESSAGE_SELECT_C16, commandFlagIndex + 4) != -1)
          {
            SelectChip(C16);
            Serial.println(MESSAGE_OK);
          }
          else if (command.indexOf(MESSAGE_SELECT_C32, commandFlagIndex + 4) != -1)
          {
            SelectChip(C32);
            Serial.println(MESSAGE_OK);
          }
          else if (command.indexOf(MESSAGE_SELECT_C64, commandFlagIndex + 4) != -1)
          {
            SelectChip(C64);
            Serial.println(MESSAGE_OK);
          }
          else if (command.indexOf(MESSAGE_SELECT_C128, commandFlagIndex + 4) != -1)
          {
            SelectChip(C128);
            Serial.println(MESSAGE_OK);
          }
          else if (command.indexOf(MESSAGE_SELECT_C256, commandFlagIndex + 4) != -1)
          {
            SelectChip(C256);
            Serial.println(MESSAGE_OK);
          }
          else if (command.indexOf(MESSAGE_SELECT_C512, commandFlagIndex + 4) != -1)
          {
            SelectChip(C512);
            Serial.println(MESSAGE_OK);
          }
          else if (command.indexOf(MESSAGE_VOLTAGE_INFO, commandFlagIndex + 4) != -1)
          {
            CommandMode = VOLTAGE;
            Serial.println(MESSAGE_OK);
          }
          else if (command.indexOf(MESSAGE_READ_CHIP, commandFlagIndex + 4) != -1)
          {
            CommandMode = READ;
            Serial.println(MESSAGE_OK);
          }
          else if (command.indexOf(MESSAGE_WRITE_CHIP, commandFlagIndex + 4) != -1)
          {
            CommandMode = WRITE;
            Serial.println(MESSAGE_OK);
          }
          else if (command.indexOf(MESSAGE_READ_BYTE, commandFlagIndex + 4) != -1)
          {
            CommandMode = READ_BYTE;
            Serial.println(MESSAGE_OK);
          }
          else if (command.indexOf(MESSAGE_WRITE_BYTE, commandFlagIndex + 4) != -1)
          {
            CommandMode = WRITE_BYTE;
            Serial.println(MESSAGE_OK);
          }
          else if (command.indexOf(MESSAGE_SELECT_NONE, commandFlagIndex + 4) != -1)
          {
            SelectChip(NONE);
            Serial.println(MESSAGE_OK);
          }
          else {
            Serial.println(MESSAGE_ERROR);
          }
        }
        else {
          Serial.println(MESSAGE_ERROR);
        }
      }
  }
}

void WaitForData(void)
{
  unsigned long start = millis();
  while (Serial.available() < 0)
  {
    if (millis() - start > 1000) {
      break;
    }
    WaitMillis(10);
  }
}

void SelectChip(CHIP_TYPE newChip)
{
  ChipSelected = newChip;
  digitalWrite(POWER_ENABLE_PIN, HIGH);
  switch (newChip)
  {
    case C16:
      digitalWrite(POWER_ENABLE_PIN, LOW);
      EndAddress = 0x07ff;
      break;
    case C32:
      digitalWrite(POWER_ENABLE_PIN, LOW);
      EndAddress = 0x0fff;
      break;
    case C64:
      EndAddress = 0x1fff;
      break;
    case C128:
      EndAddress = 0x3fff;
      break;
    case C256:
      EndAddress = 0x7fff;
      break;
    case C512:
      EndAddress = 0xffff;
      break;
    default:
      ChipSelected = NONE;
      EndAddress = 0x0000;
  }
}


void SetProgrammingVoltage(bool enable)
{
  switch (ChipSelected)
  {
    case C16:
      digitalWrite(PROGRAMMING_VOLTAGE_ENABLE_C16_PIN, enable);
      break;
    case C32:
    case C512:
      digitalWrite(PROGRAMMING_VOLTAGE_ENABLE_C32_PIN, enable);
      break;
    case C64:
    case C128:
    case C256:
    default:
      digitalWrite(PROGRAMMING_VOLTAGE_ENABLE_OTHER_PIN, enable);
  }
}

void SetWriteMode(void)
{
  pinMode(DATA_B0_PIN, OUTPUT);
  pinMode(DATA_B1_PIN, OUTPUT);
  pinMode(DATA_B2_PIN, OUTPUT);
  pinMode(DATA_B3_PIN, OUTPUT);
  pinMode(DATA_B4_PIN, OUTPUT);
  pinMode(DATA_B5_PIN, OUTPUT);
  pinMode(DATA_B6_PIN, OUTPUT);
  pinMode(DATA_B7_PIN, OUTPUT);
}

void SetReadMode(void)
{
  pinMode(DATA_B0_PIN, INPUT_PULLUP);
  pinMode(DATA_B1_PIN, INPUT_PULLUP);
  pinMode(DATA_B2_PIN, INPUT_PULLUP);
  pinMode(DATA_B3_PIN, INPUT_PULLUP);
  pinMode(DATA_B4_PIN, INPUT_PULLUP);
  pinMode(DATA_B5_PIN, INPUT_PULLUP);
  pinMode(DATA_B6_PIN, INPUT_PULLUP);
  pinMode(DATA_B7_PIN, INPUT_PULLUP);
}

uint16_t GenerateAddress(uint16_t address)
{
  byte high = highByte(address);
  byte low = lowByte(address);
  switch (ChipSelected)
  {
    case C16:
      break;
    //      if(CommandMode == READ) {
    //        high |= 1 << 3; // A11 (C32+) is Vpp for C16 (5v for read)
    //      }
    //      break;
    case C64:
    case C128:
      if (CommandMode == READ) {
        high |= 1 << 6; // A14 (C256 and C512) is ~PGM for C64 and C128
      }
      break;
    case C32:
    case C256:
    case C512:
    default:
      break;
  }
  return (high << 8) | low;
}

void SetAddress(uint16_t address)
{
  address = GenerateAddress(address);
  digitalWrite(SHIFT_LATCH_PIN, LOW);
  byte registerTwo = highByte(address);
  byte registerOne = lowByte(address);
  shiftOut(SHIFT_DATA_PIN, SHIFT_CLOCK_PIN, MSBFIRST, registerTwo);
  shiftOut(SHIFT_DATA_PIN, SHIFT_CLOCK_PIN, MSBFIRST, registerOne);
  digitalWrite(SHIFT_LATCH_PIN, HIGH);
}

uint8_t GetData(void)
{
  uint8_t data = 0;
  data |= digitalRead(DATA_B0_PIN) << 0;
  data |= digitalRead(DATA_B1_PIN) << 1;
  data |= digitalRead(DATA_B2_PIN) << 2;
  data |= digitalRead(DATA_B3_PIN) << 3;
  data |= digitalRead(DATA_B4_PIN) << 4;
  data |= digitalRead(DATA_B5_PIN) << 5;
  data |= digitalRead(DATA_B6_PIN) << 6;
  data |= digitalRead(DATA_B7_PIN) << 7;
  return data;
}

void SetData(uint8_t data)
{
  digitalWrite(DATA_B0_PIN, (data & (1 << 0)));
  digitalWrite(DATA_B1_PIN, (data & (1 << 1)));
  digitalWrite(DATA_B2_PIN, (data & (1 << 2)));
  digitalWrite(DATA_B3_PIN, (data & (1 << 3)));
  digitalWrite(DATA_B4_PIN, (data & (1 << 4)));
  digitalWrite(DATA_B5_PIN, (data & (1 << 5)));
  digitalWrite(DATA_B6_PIN, (data & (1 << 6)));
  digitalWrite(DATA_B7_PIN, (data & (1 << 7)));
}

uint8_t ReadByte(uint16_t address)
{
  SetAddress(address);
  return GetData();
}

void WriteByte(uint16_t address, uint8_t data)
{
  SetAddress(address);
  SetData(data);
  switch (ChipSelected)
  {
    case C16:
      digitalWrite(CHIP_ENABLE_PIN, HIGH);
      WaitMillis(15);
      digitalWrite(CHIP_ENABLE_PIN, LOW);
      break;
    case C32:
    case C64:
    case C128:
    case C256:
    case C512:
    default:
      digitalWrite(CHIP_ENABLE_PIN, LOW);
      delayMicroseconds(110);
      digitalWrite(CHIP_ENABLE_PIN, HIGH);
      break;
  }
}

double GetVoltage(void)
{
  double programmingVoltage = (analogRead(VOLTAGE_CONTROL_PIN) * 4.72) / 1024.;
  programmingVoltage /= RESISTOR_BOTTOM_VALUE;
  return (programmingVoltage * (RESISTOR_TOP_VALUE + RESISTOR_BOTTOM_VALUE));
}

uint8_t VerifyData(void)
{
  SetReadMode();
  if (ChipSelected == C16) {
    digitalWrite(READ_VOLTAGE_ENABLE_PIN, LOW);
  }

  digitalWrite(CHIP_ENABLE_PIN, LOW);
  digitalWrite(OUTPUT_ENABLE_PIN, LOW);
  uint8_t verify = GetData();

  digitalWrite(OUTPUT_ENABLE_PIN, HIGH);
  digitalWrite(CHIP_ENABLE_PIN, HIGH);

  if (ChipSelected == C16) {
    digitalWrite(READ_VOLTAGE_ENABLE_PIN, HIGH);
  }

  return verify;
}

void WaitMillis(unsigned long period)
{
  unsigned long time_now = millis();
  while(millis() < time_now + period)
  {
    //__asm__ __volatile__ ("nop\n\t"); 
    delayMicroseconds(1000); 
  }
}
