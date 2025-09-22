#!/usr/bin/perl -w
# Generate table of fixed-point velocity values for each angle.

use strict;
use constant PI => 3.14159265358979323846264338327950288419716939937510;
use constant VELOCITY => 900;

# Angle uses Playdate convention, so 0 degrees is up and 90 degrees is right.
for(my $a = 0; $a < 360; $a++)
{
   my $r = $a * PI / 180.0;
   my $dx = sin($r);
   my $dy = -cos($r);
   print "{", int($dx * VELOCITY), ",", int($dy * VELOCITY), "},\n",
}
