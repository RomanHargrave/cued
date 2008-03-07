#!/usr/bin/ruby

# clashcheck.rb - compare AccurateRip checksums against AccurateRip database
# AUTHOR: Eric Shattow
# LICENSE: Free to use and modify

# If you intend to access the AccurateRip database, please visit their site
# at http://www.accuraterip.com/ for terms of use.

# compensation_samples:
#   AccurateRip is consistently inaccurate by 30 samples.
#   set to 0 for AccurateRip compatibility
compensation_samples = 30

require 'toc'

require 'net/http'
require 'uri'

unless ARGV.length == 1 || ARGV.length == 2
  puts "Usage: #{__FILE__}" +
    "album.toc|album.cue [dBAR-xxx-xxxxxxxx-xxxxxxxxx.bin]"
  exit
end

t = Toc::Sheet.new(ARGV[0])

dbdata = nil

if ARGV.length == 2
  begin
    dbdata = File.read(ARGV[1])
  rescue
    puts "No such file #{ARGV[1]}"
    exit
  end
else
  url = URI.parse(t.ar_db_url)
  response = Net::HTTP.start(url.host, url.port) { |http| http.get(url.path) }
  case response.code
    when /200/  # HTTPOK
      dbdata = response.body
    when /404/  # HTTPNotFound
      puts "Album not found in AccurateRip database."
      exit
  end
end

arconfs = t.ar_db_parse(dbdata)

t.ar_each_crc(compensation_samples) do |arcrc, num|
  line = "ARCRC %s: %08x" % [num, arcrc]
  if arconfs[num - 1][arcrc]
    line += " Match found (%d)" % [arconfs[num - 1][arcrc]]
  else
    line += " No Match found."
  end
  puts line
end

