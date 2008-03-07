# toc.rb - Ruby class for CD TOC / CUESHEET
# AUTHOR: Eric Shattow
# LICENSE: Free to use and modify

module Toc

require 'msf'
require 'toc_cddb'
require 'toc_ar'

class Track
  attr_accessor :num, :offset, :length, :pregap, :wavpath
end

class Sheet
  attr_reader :file, :tracks
  def initialize(filename=ENV['PWD'])
    read_sheet(filename)
  end
  def butcher(line)
    line.chomp.scan(/(?:\A| )\s*"([^"]*)"|([^" ]+)/).flatten.compact
  end
  def read_wave_header(filename)
    headerNames = [:chunk_id, :chunk_size, :format, :subchunk1_id,
      :subchunk1_size, :audio_format, :num_channels, :sample_rate,
      :byte_rate, :block_align, :bits_per_sample, :subchunk2_id,
      :subchunk2_size]
    fields = nil
    if (File.exist? filename) && (headerData = File.read(filename, 44))
      fields = {}
      headerNames.zip(headerData.unpack("a4Va4a4VvvVVvva4V")) do |key, value|
        fields[key] = value
      end
    end
    fields
  end
  def read_sheet(filename)
    @file = File.new(filename, "r")
    if @file
      case File.extname(@file.path)
        when /toc/i
          read_toc(@file)
        when /cue/i
          read_cue(@file)
      end
    end
  end
  def read_toc(file)
    @tracks = []
    n = -1
    @file.each_line do |line|
      meat = butcher(line)
      case meat[0]
        when /^TRACK$/
          n += 1
          @tracks[n] = Track.new
        when /^SILENCE$/
          @tracks.first.pregap = MSF.new(meat[1]) if n < 0
        when /^FILE$/
          @tracks[n].wavpath = meat[1]
          @tracks[n].offset = MSF.new(meat[2])
          unless n.zero?
            @tracks[n].offset += @tracks.first.pregap if @tracks.first.pregap
          end
          @tracks[n].length = MSF.new(meat[3])
          @tracks[n].num = n + 1
        when /^START$/
          @tracks[n].pregap = MSF.new(meat[1])
          @tracks[n].offset += @tracks[n].pregap
          unless n.zero?
            @tracks[n].length -= @tracks[n].pregap
          end
      end
    end
    unless @tracks.last.length
      @tracks.last.length = lead_out_from_wave.offset - @tracks.last.offset
    end
  end
  def read_cue(file)
    @tracks = []
    n = -1
    wavpath = nil
    @file.each_line do |line|
      meat = butcher(line)
      case meat[0]
        when /^FILE$/
          wavpath = meat[1]
        when /^TRACK$/
          n += 1
          @tracks[n] = Track.new
          @tracks[n].num = n + 1
          @tracks[n].wavpath = wavpath
        when /^INDEX$/
          case meat[1].to_i
            when 0
              @tracks[n].pregap = MSF.new(meat[2])
            when 1
              @tracks[n].offset = MSF.new(meat[2])
              if @tracks[n].pregap
                @tracks[n - 1].length = \
                  @tracks[n].pregap - @tracks[n - 1].offset if n > 0
                @tracks[n].pregap = @tracks[n].offset - @tracks[n].pregap
              else
                @tracks[n - 1].length = \
                  @tracks[n].offset - @tracks[n - 1].offset if n > 0
              end
          end
      end
    end
    unless @tracks.last.length
      @tracks.last.length = lead_out_from_wave.offset - @tracks.last.offset
    end
  end
  def lead_out_from_wave()
    result = Track.new
    filename = File.dirname(@file.path) + '/' + @tracks.last.wavpath
    header = read_wave_header(filename)
    if header
      result.num = @tracks.last.num + 1
      result.offset = MSF.new(header[:subchunk2_size] / 2352)
    else
      fail "Unable to suss offset of lead-out"
    end
    result
  end
  def lead_out()
    val = Track.new
    val.num = @tracks.last.num + 1
    val.offset = @tracks.last.offset + @tracks.last.length
    val
  end

  def process_track_audio(track, pregap_layout, skip=0)
    wavfilename = File.dirname(@file.path) + '/' + track.wavpath
    wavfile = File.new(wavfilename, "r")
    startframe = track.offset
    endframe = track.length
    case pregap_layout
      when :index   # track(n) index 01
      when :track   # track(n) pregap + track(n) index 01
        if track.pregap
          startframe -= track.pregap
          endframe += track.pregap
        end
      when :eac   # track(n) index 01 + track(n+1) pregap
        if @tracks[track.num] && @tracks[track.num].pregap
          endframe += @tracks[track.num].pregap
        end
      else
        fail "Invalid pregap layout type #{pregap_layout}"
    end
    wavfile.seek(44 + skip * 4 + startframe * 2352)
    endframe.to_int.times do |framenum|
      framedata = wavfile.read(2352)
      if framedata.nil? || framedata.length != 2352
        if track.num == @tracks.last.num && framedata.length == 2352 - skip * 4
          puts "Warning: #{skip} sample compensation resulted in short read"
        else
          puts "Warning: short read on track ##{track.num} frame ##{framenum}"
        end
        next
      end
      yield framedata, framenum, track.num
    end
  end
end

end

