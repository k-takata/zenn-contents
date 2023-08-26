#!mruby

class LCD
  ID = 0x3E

  def initialize
    @i2c = I2c.new(1)
  end

  def init
    #delay(10)
    send_cmd(0x38)     # Function Set: 8 bits, 2 lines, normal height, Normal mode
    delay(2)
    send_seq([0x39,    # Function Set: 8 bits, 2 lines, normal height, Extend mode
              0x14])   # Internal OSC frequency: 1/5 bias, 183 Hz

    # Contrast
    contrast = 0x20
    send_seq([0x70 + (contrast & 0x0F),          # Contrast set: low byte
              0x5C + ((contrast >> 4) & 0x03),   # Icon on, Booster on, Contrast high byte
              0x6C])   # Follower control: follower circuit on, amplified ratio = 2 (for 3.3 V)
    delay(200)
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

lcd = LCD.new

lcd.init

lcd.set_cursor(0, 0)  # 0桁, 0行目
lcd.print("Hello World")
