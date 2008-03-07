#!/usr/bin/ruby

# cdwavheader.rb - tocfile/cuesheet information
# AUTHOR: Eric Shattow
# LICENSE: Free to use and modify

require 'toc'

unless ARGV.length == 1
  puts "Usage: #{__FILE__} filename.wav"
  exit
end

FramesPerSecond = 75

t = Toc::Sheet.new

fields = t.read_wave_header(ARGV[0])

samplesPerFrame = fields[:sample_rate] / FramesPerSecond
bytesPerFrame = samplesPerFrame * fields[:block_align]
cdFramesTotal = fields[:subchunk2_size] / bytesPerFrame

puts "samplesPerFrame: #{samplesPerFrame}"
puts "bytesPerFrame: #{bytesPerFrame}"
puts "cdframesTotal: #{cdFramesTotal} (#{MSF.new(cdFramesTotal)})"

