# Ruby class for Minute <= 99 / Second < 60 / Frame < 75
# AUTHOR: Eric Shattow
# LICENSE: Free to use and modify

class MSF
  include Comparable
  Second = 75
  Minute = 60 * Second
  attr_reader :value
  protected :value
  def initialize(value)
    self.value = value
  end
  def to_int
    @value
  end
  def coerce(other)
    case other
      when Integer
        [other, @value]
      when Float
        [other, Float(@value)]
    end
  end
  def to_i
    @value
  end
  def to_s
    value = @value
    msf = []
    for factor in [Minute, Second, 1]
      count, value = value.divmod(factor)
      msf << "%02d" % count
    end
    msf.join(':')
  end
  alias inspect to_s
  def value=(other)
    if MSF === other
      other = other.value
    elsif String === other
      if other =~ /^\d{1,2}:\d{1,2}:\d{1,2}$/
        s = other.split(':',3).map{|x| x.to_i}
        other = s[0] * 60 * 75 + s[1] * 75 + s[2]
      else
        other = other.to_i
      end
    end
    if Fixnum === other
      @value = other
    else
      @value = nil
    end
  end
  def <<(other)
    if MSF === other || String === other || Integer === other
      MSF.new(@value << MSF.new(other))
    else
      x, y = other.coerce(@value)
      x << y
    end
  end
  def >>(other)
    if MSF === other || String === other || Integer === other
      MSF.new(@value >> MSF.new(other))
    else
      x, y = other.coerce(@value)
      x >> y
    end
  end
  def %(other)
    if MSF === other || String === other || Integer === other
      MSF.new(@value % MSF.new(other))
    else
      x, y = other.coerce(@value)
      x % y
    end
  end
  def *(other)
    if MSF === other || String === other || Integer === other
      MSF.new(@value * MSF.new(other))
    else
      x, y = other.coerce(@value)
      x * y
    end
  end
  def /(other)
    if MSF === other || String === other || Integer === other
      MSF.new(@value / MSF.new(other))
    else
      x, y = other.coerce(@value)
      x / y
    end
  end
  def +(other)
    if MSF === other || String === other || Integer === other
      MSF.new(@value + MSF.new(other))
    else
      x, y = other.coerce(@value)
      x + y
    end
  end
  def -(other)
    if MSF === other || String === other || Integer === other
      MSF.new(@value - MSF.new(other))
    else
      x, y = other.coerce(@value)
      x - y
    end
  end
  def <=>(other)
    if MSF === other || String === other || Integer === other
      @value <=> MSF.new(other)
    else
      x, y = other.coerce(@value)
      x <=> y
    end
  end
end

