
srand(time);

while($#ARGV >= 0)
{
  $i = int(rand($#ARGV+1));
  print splice(@ARGV, $i, 1)," ";
}

print "\n";

0;
