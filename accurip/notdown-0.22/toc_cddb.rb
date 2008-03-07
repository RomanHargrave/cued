# toc_cddb.rb - adds to Toc::Sheet for computing CDDB
# AUTHOR: Eric Shattow
# LICENSE: Free to use and modify

require 'toc'

class Toc::Sheet
  def cddb_sum_compat(num)
    result = 0
    while num > 0
      result += num % 10
      num /= 10
    end
    result
  end
  def cddb_sum(num)
    num.to_i.to_s.split(//).inject(0) { |sum, x| sum + x.to_i }
  end
  def cddb_first()
    @tracks.first.offset + 150
  end
  def cddb_last()
    @tracks.last.offset + @tracks.last.length + 150
  end
  def cddb_seconds()
    cddb_last / 75
  end
  def cddb_len()
    cddb_last / 75 - cddb_first / 75
  end
  def cddb_id()
    cksum = 0
    @tracks.map { |t| cksum += cddb_sum((t.offset + 150) / 75) }
    result = ((cksum % 0xff) << 24) | (cddb_len << 8) | tracks.last.num
    result & 0xffffffff
  end
end
