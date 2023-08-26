#!mruby

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

aht25 = AHT25.new
aht25.init

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

  hum = aht25.calc_hum
  s = adj_digit(hum, 3)
  puts("hum: #{s} %")

  # Discomfort Index (100x)
  #  DI = 0.81Td + 0.01H(0.99Td - 14.3) + 46.3
  di = (81 * temp + (hum * ((99 * temp).div(100) - 1430)).div(1000)).div(100) + 4630
  s = adj_digit(di, 2)
  puts("DI: #{s}")

  puts

  delay(1000 - t_delta)
  t_end = millis()
  t_delta = (t_end - t_start) - (1000 - t_delta)
  puts("interval: #{t_end - t_start}, t_delta: #{t_delta}")
end
