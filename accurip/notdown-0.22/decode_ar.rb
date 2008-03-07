#!/usr/bin/ruby

# decode_ar.rb - decode AccurateRip datachunk
# AUTHOR: Eric Shattow
# LICENSE: Free to use and modify

require 'toc'

unless ARGV.length == 1
  puts "Usage: #{__FILE__} dBAR-xxx-xxxxxxxx-xxxxxxxxx.bin"
  exit
end

fh = File.new(ARGV[0])

data = fh.read

t = Toc::Sheet.new

track_confidences = t.ar_db_parse(data)

track_confidences.each_with_index do |confidence, idx|
  print "%02d: " % [idx + 1]
  puts confidence.map {|k,v| "%08x@%02d" % [k,v]}.join(' ')
end

