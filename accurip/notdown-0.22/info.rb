#!/usr/bin/ruby

# info.rb - tocfile/cuesheet information
# AUTHOR: Eric Shattow
# LICENSE: Free to use and modify

require 'toc'

unless ARGV.length == 1
  puts "Usage: #{__FILE__} album.toc|album.cue"
  exit
end

t = Toc::Sheet.new(ARGV[0])

puts "Track\tOffset   Length   Ending   [Pregap]"
t.tracks.each do |track|
  pg = (track.pregap ? ' ' + track.pregap.to_s : '')
  print "%02d" % track.num
  puts ":\t#{track.offset} #{track.length} #{track.offset + track.length}#{pg}"
end

print "CDDB: %i seconds, discid %08x\n" % [t.cddb_seconds, t.cddb_id]

