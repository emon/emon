#!/usr/local/bin/ruby
#
# $Id: jpegrenum.rb,v 1.1 2008/09/26 15:10:34 emon Exp $
#
# 1. jpegsave -D1 -c < sample.jpgs >& stat.txt
# 2. mkdir $OUTDIR
# 3. jpegrenum < stat.txt
#
# options:
# -D <n> : debug level (default 0)
# -n     : dry-run
#

# modify below variable (^^;
OUTDIR="out"
OUTFMT="yuv"			# output format
FREQ=1470

require 'fileutils'
require 'getopts'
getopts("nD:")
$debug_level = $OPT_D.to_i

ofnum = 0			# output file number

lifn = nil			# last input file name
cofn = nil			# last clear output file


def my_system(cmd)
  printf(cmd) if $debug_level >= 1
  if $OPT_n == false
    code = system(cmd)
    if $debug_level >= 2
      ret = $?
      printf(" (%s/%d)", code == true ? "true" : "false", ret)
    end
  end
  printf("\n") if $debug_level >= 1
  return ret
end


def my_ln(old, new)
  printf(" ln %s %s\n", old, new) if $debug_level >= 1
  if $OPT_n == false
    FileUtils.ln(old, new, :force)
  end
end


while line=STDIN.gets
  next if not line =~ /^open/
  break if line == nil

  ifn = line.scan(/open:([^\s]*)/)[0][0] # input file name
  itvl = line.scan(/ts:\d+ \(\s*(\d+)\)/)[0][0].to_i

  if lifn != nil
    if itvl < FREQ
      printf("skip:%s itvl:%s\n", ifn, itvl)
      next
    end
    num = (itvl / FREQ).to_i
#    printf("#{lifn} : ")

    ofn = sprintf("%s/%04d.%s", OUTDIR, ofnum, OUTFMT)
    if my_system("convert #{lifn} #{ofn}") == 0 # convert success
      cofn = ofn
    elsif (cofn != nil)			# convert fail, link to old picture
      my_ln(cofn, ofn)
    end

    num -= 1
    ofnum += 1
    while num > 0
      ofn2 = sprintf("%s/%04d.%s", OUTDIR, ofnum, OUTFMT)
      my_ln(ofn, ofn2)
      num -= 1
      ofnum += 1
    end
  end
  lifn = ifn
end
