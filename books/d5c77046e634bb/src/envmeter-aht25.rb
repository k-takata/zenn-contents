#!mruby

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

class AHT25
  ID = 0x38

  def initialize
    @i2c = I2c.new(1)
  end

  def init
    # Reset
    if @i2c.read(ID, 0x71) != 0x18 then
      # TODO: initialize
    end
  end

  def trigger
    @i2c.begin(ID)
    @i2c.lwrite(0xAC)
    @i2c.lwrite(0x33)
    @i2c.lwrite(0x00)
    @i2c.end()
  end

  LEN_DATA = 7
  def read_data
    @data = []
    @i2c.request(ID, LEN_DATA)
    LEN_DATA.times do
      @data << @i2c.lread()
    end
  end

  # Calculate temperature
  #   return: 100x degrees Celsius
  def calc_temp
    temp_raw = ((@data[3] & 0x0F) << 16) | (@data[4] << 8) | @data[5]
    return ((temp_raw * 625) >> 15) - 5000
  end

  # Calculate humidity
  #   return: 1000x %rH
  def calc_hum
    hum_raw = (@data[1] << 12) | (@data[2] << 4) | ((@data[3] & 0x0F) >> 4)
    return ((hum_raw * 625) >> 15) * 5
  end
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
aht25 = AHT25.new

lcd.init
aht25.init
lcd.init2
lcd.init3
delay(2)

# Custom symbols
# Degree symbol
lcd.set_cgram(0, [0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
# DI (Discomfort Index)
lcd.set_cgram(1, [0x18, 0x14, 0x14, 0x18, 0x07, 0x02, 0x02, 0x07])


t_end = millis()
t_delta = 0
loop do
  t_start = t_end

  led(1)
  delay(10)
  led(0)
  aht25.trigger
  delay(80)
  aht25.read_data

  temp = aht25.calc_temp
  s = adj_digit(temp, 2)
  puts("temp: #{s} C")
  lcd.set_cursor(0, 0)
  lcd.print("#{align_right(s[0..-2], 5)}\x00C")

  hum = aht25.calc_hum
  s = adj_digit(hum, 3)
  puts("hum: #{s} %")
  #lcd.set_cursor(7, 0)
  lcd.print(" #{align_right(s[0..-3], 5)} %")

  # Discomfort Index (100x)
  #  DI = 0.81Td + 0.01H(0.99Td - 14.3) + 46.3
  di = (81 * temp + (hum * ((99 * temp).div(100) - 1430)).div(1000)).div(100) + 4630
  s = adj_digit(di, 2)
  puts("DI: #{s}")
  lcd.set_cursor(12, 1)
  lcd.print("\x01#{align_right(s[0..-4], 3)}")

  puts

  delay(1000 - t_delta)
  t_end = millis()
  t_delta = (t_end - t_start) - (1000 - t_delta)
  puts("interval: #{t_end - t_start}, t_delta: #{t_delta}")
end
