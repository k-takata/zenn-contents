#include <Wire.h>

class Lcd {
private:
  const int addr = 0x3E;

public:
  Lcd() = default;
  ~Lcd() = default;

  void init() {
    delay(10);
    send_cmd(0x38);
    delay(2);
    send_seq({0x39, 0x14});

    uint8_t contrast = 0x20;
    send_seq({
      uint8_t(0x70 + (contrast & 0x0F)),
      uint8_t(0x5C + ((contrast >> 4) & 0x03)),
      0x6C});
    delay(200);

    send_seq({0x38, 0x0C, 0x01});
    delay(2);
  }

  template <size_t cmdlen>
  void send_seq(const uint8_t (&cmds)[cmdlen], const uint8_t *data=nullptr, size_t datalen=0) {
    send_seq(cmds, cmdlen, data, datalen);
  }

  void send_seq(const uint8_t *cmds, size_t cmdlen, const uint8_t *data=nullptr, size_t datalen=0) {
    Wire.beginTransmission(addr);
    if (data == nullptr) {
      // Only command data
      Wire.write(0x00);       // Command byte: Co=0, RS=0
      for (size_t i = 0; i < cmdlen; ++i) {
        Wire.write(cmds[i]);  // Command data byte
      }
    } else {
      // Send command words (if any)
      for (size_t i = 0; i < cmdlen; ++i) {
        Wire.write(0x80);     // Command byte: Co=1, RS=0
        Wire.write(cmds[i]);  // Command data byte
      }
      // Send RAM data bytes
      Wire.write(0x40);       // Command byte: Co=0, RS=1
      for (size_t i = 0; i < datalen; ++i) {
        Wire.write(data[i]);  // RAM data byte
      }
    }
    Wire.endTransmission();
  }

  void send_cmd(const uint8_t cmd) {
    send_seq({cmd});
  }

  template <size_t datalen>
  void send_data(const uint8_t (&data)[datalen]) {
    send_data(data, datalen);
  }

  void send_data(const uint8_t *data, size_t datalen) {
    send_seq(nullptr, 0, data, datalen);
  }

  void set_cursor(int col, int row) {
    send_cmd(uint8_t(0x80 + 0x40*row + col));
  }

  void clear() {
    send_cmd(0x01);
    delay(2);
  }

  void print(String s) {
    send_data(reinterpret_cast<const uint8_t *>(s.c_str()), s.length());
  }

  template <size_t patlen>
  void set_cgram(int num, const uint8_t (&pat)[patlen]) {
    set_cgram(num, pat, patlen);
  }

  void set_cgram(int num, const uint8_t *pat, size_t patlen) {
    send_seq({uint8_t(0x40 + num*8)}, pat, patlen);
  }
};

Lcd lcd;

void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  lcd.init();
}

void loop() {
  // put your main code here, to run repeatedly:
  lcd.set_cursor(0, 0);
  lcd.print("Hello World");
  delay(1000);
}
