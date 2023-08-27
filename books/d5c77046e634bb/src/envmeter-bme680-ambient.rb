#!mruby

# Wi-Fi
SSID = '**************'
PASSWD = '*************'

# Ambient
CHANNELID = '*****'
WRITEKEY = '****************'

SEND_CYCLE = 600  # 10 min


class Ambient
  def initialize(channelId, writeKey)
    @channelId = channelId
    @writeKey = writeKey
  end

  def send(hash)
    arr = []
    hash.each {|k, v|
      arr << "\"#{k}\":#{v}"
    }
    body = "{\"writeKey\":\"#{@writeKey}\",\"data\":[{#{arr.join(',')}}]}"
    puts("body: #{body}")
    url = "ambidata.io/api/v2/channels/#{@channelId}/dataarray"
    puts("url: #{url}")
    ret = WiFi.httpPost(url, ['Content-Type: application/json'], body)
    puts("httpPost: #{ret}")
  end
end

class LCD
  ID = 0x3E

  def initialize
    @i2c = I2c.new(1)
  end

  def init
    #delay(10)
    send_cmd(0x38)     # Function Set: 8 bits, 2 lines, normal height, Normal mode
  end
  def init2
    delay(2)
    send_seq([0x39,    # Function Set: 8 bits, 2 lines, normal height, Extend mode
              0x14])   # Internal OSC frequency: 1/5 bias, 183 Hz

    # Contrast
    contrast = 0x20
    send_seq([0x70 + (contrast & 0x0F),          # Contrast set: low byte
              0x5C + ((contrast >> 4) & 0x03),   # Icon on, Booster on, Contrast high byte
              0x6C])   # Follower control: follower circuit on, amplified ratio = 2 (for 3.3 V)
    @prev_ms = millis()
  end
  def init3
    delay(200 - (millis() - @prev_ms))
    send_seq([0x38,    # Function Set: Normal mode
              0x0C,    # Display on, cursor off, cursor position off
              0x01])   # Clear Display
  end

  # Send a command sequence
  def send_seq(cmds, dataarr=[])
    @i2c.begin(ID)
    if dataarr.empty? then
      # Only command data
      @i2c.lwrite(0x00)   # Control byte: Co=0, RS=0
      cmds.each {|cmd|
        @i2c.lwrite(cmd)  # Command data byte
      }
    else
      # Send command words (if any)
      cmds.each {|cmd|
        @i2c.lwrite(0x80) # Control byte: Co=1, RS=0
        @i2c.lwrite(cmd)  # Command data byte
      }
      # Send RAM data bytes
      @i2c.lwrite(0x40)   # Control byte: Co=0, RS=1
      dataarr.each {|dat|
        @i2c.lwrite(dat)  # RAM data byte
      }
    end
    @i2c.end
  end

  # Send a command
  def send_cmd(cmd)
    send_seq([cmd])
  end

  # Set cursor position
  def set_cursor(col, row)
    send_cmd(0x80 + 0x40*row + col)
  end

  # Clear all display
#  def clear()
#    send_cmd(0x01)   # Clear Display
#    delay(2)
#  end

  # Show the string
  def print(cs)
    send_seq([], cs.bytes)
  end

  # Set CGRAM
  #   num: 0-7
  #   pat: Character pattern (8-bytes per character)
  def set_cgram(num, pat)
    send_seq([0x40 + num*8], pat)
  end

  # Shift the display to left or right
  #   dir: shift direction: 0=left, !0=right
#  def shiftdisp(dir=0)
#    send_cmd(0x18 + ((dir != 0) ? 0x04 : 0x00))
#  end
end

class BME680
  ID = 0x77

  #REG_COEFF3        = 0x00
  REG_FIELD0        = 0x1D
  REG_IDAC_HEAT0    = 0x50
  REG_RES_HEAT0     = 0x5A
  REG_GAS_WAIT0     = 0x64
  REG_SHD_HEATR_DUR = 0x6E
  REG_CTRL_GAS_0    = 0x70
  REG_CTRL_GAS_1    = 0x71
  REG_CTRL_HUM      = 0x72
  REG_CTRL_MEAS     = 0x74
  REG_CONFIG        = 0x75
  REG_UNIQUE_ID     = 0x83
  REG_COEFF1        = 0x8A
  REG_CHIP_ID       = 0xD0
  REG_RESET         = 0xE0
  REG_COEFF2        = 0xE1
  REG_VARIANT_ID    = 0xF0

  LEN_COEFF1    = 23
  LEN_COEFF2    = 14
  #LEN_COEFF3    = 5
  #LEN_COEFF_ALL = LEN_COEFF1 + LEN_COEFF2 + LEN_COEFF3

  LEN_FIELD = 17

  def initialize
    @i2c = I2c.new(1)
  end

  def init
    @i2c.write(ID, REG_RESET, 0xB6)
    if @i2c.read(ID, REG_CHIP_ID) != 0x61 then
      puts "BME680 not found."
    end
  end

  def u16(msb, lsb)
    return (msb << 8) + lsb
  end
  def s16(msb, lsb)
    return (msb << 8) + lsb + ((msb < 0x80) ? 0 : ~0xFFFF)
  end
  def u8(b)
    return b
  end
  def s8(b)
    return b + ((b < 0x80) ? 0 : ~0xFF)
  end

  # Get calibration data
  def get_calib_data
    coeff = []
    delay 10
    @i2c.begin(ID)
    @i2c.lwrite(REG_COEFF1)
    @i2c.end(0)
    @i2c.request(ID, LEN_COEFF1)
    LEN_COEFF1.times do
      coeff << @i2c.lread()
    end
    @i2c.begin(ID)
    @i2c.lwrite(REG_COEFF2)
    @i2c.end(0)
    @i2c.request(ID, LEN_COEFF2)
    LEN_COEFF2.times do
      coeff << @i2c.lread()
    end
    #@i2c.begin(ID)
    #@i2c.lwrite(REG_COEFF3)
    #@i2c.end(0)
    #@i2c.request(ID, LEN_COEFF3)
    #LEN_COEFF3.times do
    #  coeff << @i2c.lread()
    #end

    #puts(["coeff:", coeff])

    @par_t1 = u16(coeff[32], coeff[31])
    @par_t2 = s16(coeff[1], coeff[0])
    @par_t3 = s8(coeff[2])
    puts("par_t1: #{@par_t1}")
    puts("par_t2: #{@par_t2}")
    puts("par_t3: #{@par_t3}")

    @par_p1 = u16(coeff[5], coeff[4])
    @par_p2 = s16(coeff[7], coeff[6])
    @par_p3 = s8(coeff[8])
    @par_p4 = s16(coeff[11], coeff[10])
    @par_p5 = s16(coeff[13], coeff[12])
    @par_p6 = s8(coeff[15])
    @par_p7 = s8(coeff[14])
    @par_p8 = s16(coeff[19], coeff[18])
    @par_p9 = s16(coeff[21], coeff[20])
    @par_p10 = u8(coeff[22])
    puts("par_p1: #{@par_p1}")
    puts("par_p2: #{@par_p2}")
    puts("par_p3: #{@par_p3}")
    puts("par_p4: #{@par_p4}")
    puts("par_p5: #{@par_p5}")
    puts("par_p6: #{@par_p6}")
    puts("par_p7: #{@par_p7}")
    puts("par_p8: #{@par_p8}")
    puts("par_p9: #{@par_p9}")
    puts("par_p10: #{@par_p10}")

    @par_h1 = (coeff[25] << 4) + (coeff[24] & 0x0F)
    @par_h2 = (coeff[23] << 4) + ((coeff[24] >> 4) & 0x0F)
    @par_h3 = s8(coeff[26])
    @par_h4 = s8(coeff[27])
    @par_h5 = s8(coeff[28])
    @par_h6 = u8(coeff[29])
    @par_h7 = s8(coeff[30])
    puts("par_h1: #{@par_h1}")
    puts("par_h2: #{@par_h2}")
    puts("par_h3: #{@par_h3}")
    puts("par_h4: #{@par_h4}")
    puts("par_h5: #{@par_h5}")
    puts("par_h6: #{@par_h6}")
    puts("par_h7: #{@par_h7}")

    @par_g1 = s8(coeff[35])
    @par_g2 = s16(coeff[34], coeff[33])
    @par_g3 = s8(coeff[36])
    puts("par_g1: #{@par_g1}")
    puts("par_g2: #{@par_g2}")
    puts("par_g3: #{@par_g3}")
    puts
  end

  # Set operation mode
  #   mode: 0=Sleep mode, 1=Force mode
  def set_op_mode(mode)
    # Wait until it becomes to sleep mode.
    cur_mode = 0
    loop do
      cur_mode = @i2c.read(ID, REG_CTRL_MEAS)
      break if (cur_mode & 0x03) == 0
      delay 10
    end
    if mode != 0 then
      @i2c.write(ID, REG_CTRL_MEAS, (cur_mode & ~0x03) | (mode & 0x03))
    end
  end

  # Set configuration for temperature, pressure, humidity
  #   osrs_x (0..5): Oversampling rate: 0: skip, 1..5: 2^(osrs_x - 1)
  #   filter (0..7): Filter coefficient: 2^(filter) - 1
  def set_conf(osrs_t, osrs_p, osrs_h, filter)
    set_op_mode(0)

    tmp = @i2c.read(ID, REG_CTRL_MEAS)
    @i2c.write(ID, REG_CTRL_MEAS, ((osrs_t & 0x07) << 5) | ((osrs_p & 0x07) << 2) | (tmp & 0x03))

    tmp = @i2c.read(ID, REG_CTRL_HUM)
    @i2c.write(ID, REG_CTRL_HUM, (tmp & ~0x07) | (osrs_h & 0x07))

    tmp = @i2c.read(ID, REG_CONFIG)
    @i2c.write(ID, REG_CONFIG, (tmp & ~(0x07 << 2)) | ((filter & 0x07) << 2))
  end

  def set_heater_conf()
    # TODO
  end

  def read_field_data
    data = []
    @i2c.begin(ID)
    @i2c.lwrite(REG_FIELD0)
    @i2c.end(0)
    @i2c.request(ID, LEN_FIELD)
    LEN_FIELD.times do
      data << @i2c.lread()
    end

    @adc_pres = (data[2] << 12) | (data[3] << 4) | (data[4] >> 4)
    @adc_temp = (data[5] << 12) | (data[6] << 4) | (data[7] >> 4)
    @adc_hum = (data[8] << 8) | data[9]
    puts("adc_temp: #{@adc_temp}")
    puts("adc_pres: #{@adc_pres}")
    puts("adc_hum: #{@adc_hum}")
  end

  # Calculate temperature
  #   return: 100x degrees Celsius
  def calc_temp
    var1 = (@adc_temp >> 3) - (@par_t1 << 1)
    var2 = (var1 * @par_t2) >> 11
    var3 = ((((var1 >> 1) * (var1 >> 1)) >> 12) * (@par_t3 << 4)) >> 14
    @t_fine = var2 + var3
    @temp_comp = ((@t_fine * 5) + 128) >> 8
    return @temp_comp
  end

  # Calculate pressure
  #   return: 100x hPa
  def calc_pres
    var1 = (@t_fine >> 1) - 64000
    var2 = ((((var1 >> 2) * (var1 >> 2)) >> 11) * @par_p6) >> 2
    var2 = var2 + ((var1 * @par_p5) << 1)
    var2 = (var2 >> 2) + (@par_p4 << 16)
    var1 = (((((var1 >> 2) * (var1 >> 2)) >> 13) * (@par_p3 << 5)) >> 3) + ((@par_p2 * var1) >> 1)
    var1 = var1 >> 18
    var1 = ((32768 + var1) * @par_p1) >> 15
    press_comp = 1048576 - @adc_pres
    press_comp = (press_comp - (var2 >> 12)) * 3125
    if press_comp >= (1 << 30) then
      press_comp = (press_comp.div(var1)) << 1
    else
      press_comp = (press_comp << 1).div(var1)
    end
    var1 = (@par_p9 * (((press_comp >> 3) * (press_comp >> 3)) >> 13)) >> 12
    var2 = ((press_comp >> 2) * @par_p8) >> 13
    var3 = ((press_comp >> 8) * (press_comp >> 8) * (press_comp >> 8) * @par_p10) >> 17
    press_comp = press_comp + ((var1 + var2 + var3 + (@par_p7 << 7)) >> 4)
    return press_comp
  end

  # Calculate humidity
  #   return: 1000x %rH
  def calc_hum
    temp_scaled = @temp_comp
    var1 = @adc_hum - (@par_h1 << 4) - (((temp_scaled * @par_h3).div(100)) >> 1)
    var2 = (@par_h2 * (((temp_scaled * @par_h4).div(100)) + (((temp_scaled * ((temp_scaled * @par_h5).div(100))) >> 6).div(100)) + (1 << 14))) >> 10
    var3 = var1 * var2
    var4 = ((@par_h6 << 7) + ((temp_scaled * @par_h7).div(100))) >> 4
    var5 = ((var3 >> 14) * (var3 >> 14)) >> 10
    var6 = (var4 * var5) >> 1
    comp_hum = (((var3 + var6) >> 10) * 1000) >> 12

    # Limit the result between 0 and 100.000.
    comp_hum = [[comp_hum, 100000].min, 0].max
    return comp_hum
  end
end

@ambi = nil
@init_step = 0
def init_wifi(start=false)
  @init_step = 0 if start
  if @init_step >= 3 then
    return
  elsif @init_step == 0 then
    # Reset ESP8266
    pinMode(5, OUTPUT)
    digitalWrite(5, LOW)  # Disable
    #delay 500
  elsif @init_step == 1 then
    digitalWrite(5, HIGH) # Enable
    #delay 500
  elsif @init_step == 2 then
    if (System.use?('WiFi')) then
      WiFi.setMode(1)  # Station mode
      WiFi.connect(SSID, PASSWD)
      s = WiFi.ipconfig
      puts s
      if s.index("OK") then
        @ambi = Ambient.new(CHANNELID, WRITEKEY)
      end
    end
  end
  @init_step += 1
end

def adj_digit(num, digit)
  s = num.to_s
  s[-digit,0] = '.'
  return s
end

def align_right(s, digit)
  return " " * (digit - s.length) + s
end

lcd = LCD.new
bme680 = BME680.new

bme680.init
lcd.init
init_wifi(true)
lcd.init2
bme680.get_calib_data
bme680.set_conf(5, 5, 5, 3)
lcd.init3
delay(2)

# Custom symbols
# Degree symbol
lcd.set_cgram(0, [0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
# DI (Discomfort Index)
lcd.set_cgram(1, [0x18, 0x14, 0x14, 0x18, 0x07, 0x02, 0x02, 0x07])


t_end = millis()
t_delta = 0
count = 20
loop do
  t_start = t_end
  init_wifi

  led(1)
  bme680.set_op_mode(1)
  led(0)
  bme680.set_op_mode(0)
  bme680.read_field_data

  temp = bme680.calc_temp
  s = adj_digit(temp, 2)
  puts("temp: #{s} C")
  lcd.set_cursor(0, 0)
  lcd.print("#{align_right(s[0..-2], 5)}\x00C")

  hum = bme680.calc_hum
  s = adj_digit(hum, 3)
  puts("hum: #{s} %")
  #lcd.set_cursor(7, 0)
  lcd.print(" #{align_right(s[0..-3], 5)} %")

  pres = bme680.calc_pres
  s = adj_digit(pres, 2)
  puts("pres: #{s} hPa")
  lcd.set_cursor(0, 1)
  lcd.print("#{align_right(s[0..-2], 6)} hPa")

  # Discomfort Index (100x)
  #  DI = 0.81Td + 0.01H(0.99Td - 14.3) + 46.3
  di = (81 * temp + (hum * ((99 * temp).div(100) - 1430)).div(1000)).div(100) + 4630
  s = adj_digit(di, 2)
  puts("DI: #{s}")
  #lcd.set_cursor(10, 1)
  lcd.print("  \x01#{align_right(s[0..-4], 3)}")

  count -= 1
  if count == 0 then
    count = SEND_CYCLE
    if @ambi then
      @ambi.send({"d1" => temp / 100, "d2" => hum / 1000, "d3" => pres / 100, "d4" => di / 100})
    end
  end
  puts

  delay(1000 - t_delta)
  t_end = millis()
  t_delta = (t_end - t_start) - (1000 - t_delta)
  puts("interval: #{t_end - t_start}, t_delta: #{t_delta}")
end
