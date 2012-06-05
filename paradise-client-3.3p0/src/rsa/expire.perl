
require "timelocal.pl";

@expire = localtime;

if($#ARGV == 0)
{
  if($ARGV[0] =~ /-/)
  {
    @f = split(/-/, $ARGV[0]);
  }
  elsif($ARGV[0] =~ /\//)
  {
    @f = split(/\//, $ARGV[0]);
  }
}

if($#f != 2)
{
  ($month, $day, $year) = ($expire[4], $expire[3], $expire[5] + 2);
}
else
{
  if($f[2] >= 1900)
  {
    $f[2] -= 1900;
    ($month, $day, $year) = ($f[0] - 1, $f[1], $f[2]);
  }
  else
  {
    ($month, $day, $year) = ($expire[4], $expire[3], $expire[5] + 2);
  }
}

print &timelocal(0, 0, 0, $day, $month, $year);
