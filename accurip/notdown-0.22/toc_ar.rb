# toc_ar.rb - adds to Toc::Sheet for computing AccurateRip
# AUTHOR: Eric Shattow
# LICENSE: Free to use and Modify

require 'toc'

require 'inline'

class Array
  # build the Array#accuraterip_crc method in C using Ruby
  # takes a single argument which is the offset in bytes to seed with
  inline do |builder|
  builder.c_raw "
    static VALUE accuraterip_crc(int argc, VALUE *argv, VALUE self) {
      unsigned long acrc;
      unsigned long offset;
      unsigned long i;
      long len;

      VALUE *arr = RARRAY(self)->ptr;
      len = RARRAY(self)->len;

      acrc = 0;
      if(argc == 1) {
        offset = rb_num2ulong(argv[0]);
        for(i = 0; i < len; ++i) {
          acrc += rb_num2ulong(arr[i]) * (offset + i);
        }
        acrc &= 0xffffffff;
      } else {
        return 0;
      }

      return LONG2NUM(acrc);
    }
    "
  end
  def plain_accuraterip_crc(offset)
    fcrc = 0
    self.each_with_index do |v, i|
      fcrc += (v * (offset + i))
    end
    fcrc & 0xffffffff
  end
end

class Toc::Sheet
  def ar_tracks()
    @tracks + [lead_out()]
  end
  def ar_id1()
    cksum = 0
    lead_out = @tracks.last.offset + @tracks.last.length
    ar_tracks.each { |t| cksum += t.offset }
    cksum & 0xffffffff
  end
  def ar_id2()
    cksum = 0
    lead_out = @tracks.last.offset + @tracks.last.length
    ar_tracks.each { |t| cksum += (t.offset == 0 ? 1 : t.offset) * (t.num) }
    cksum & 0xffffffff
  end
  def ar_each_crc(skip = 0)
    @tracks.each do |track|
      result = 0
      process_track_audio(track, :eac, skip) do |framedata, framenum, tracknum|
        next if (tracknum == 1) && (framenum < 4)
        next if (tracknum == @tracks.last.num) && (track.length - framenum) < 6
        values = framedata.unpack("V*")
        if tracknum == 1 && framenum == 4
          result += values.last * 588 * 5
        else
          result += values.accuraterip_crc(framenum * 588 + 1)
        end
      end
      yield result & 0xffffffff, track.num
    end
  end
  def ar_db_parse(data)
    result = []
    until data.length.zero?
      trackcount, id1, id2, cddbid = data.slice!(0..12).unpack("cVVV")
      trackcount.times do |n|
        confidence, crc = data.slice!(0..8).unpack("cV")
        result[n] ||= {}
        result[n][crc] = confidence
      end
    end
    result
  end
  def ar_db_url()
    id1, id2, id3 = ar_id1, ar_id2, cddb_id
    terms = [id1 & 0xf, (id1 >> 4) & 0xf, (id1 >> 8) & 0xf]
    terms << @tracks.last.num
    terms << ar_id1
    terms << ar_id2
    terms << cddb_id
    result = "http://www.accuraterip.com/accuraterip/"
    result + "%.1x/%.1x/%.1x/dBAR-%.3d-%.8x-%.8x-%.8x.bin" % terms
  end
end

