#!/usr/bin/perl -w
# Generate a set of 8x8 bit patterns indexed by intensity.

use strict;

# Bayer 8x8 dither pattern.
my @bayer8x8 =
(
   [ 0, 32,  8, 40,  2, 34, 10, 42],
   [48, 16, 56, 24, 50, 18, 58, 26],
   [12, 44,  4, 36, 14, 46,  6, 38],
   [60, 28, 52, 20, 62, 30, 54, 22],
   [ 3, 35, 11, 43,  1, 33,  9, 41],
   [51, 19, 59, 27, 49, 17, 57, 25],
   [15, 47,  7, 39, 13, 45,  5, 37],
   [63, 31, 55, 23, 61, 29, 53, 21]
);

print "static const uint8_t kGrayPattern[65][16] =\n{\n";

for(my $intensity = 0; $intensity <= 64; $intensity++)
{
   # Build bit patterns for the selected intensity.
   my @pattern = ();
   for(my $i = 0; $i < 8; $i++)
   {
      my $bits = 0;
      for(my $j = 0; $j < 8; $j++)
      {
         if( $intensity > $bayer8x8[$i][$j] )
         {
            $bits |= (1 << $j);
         }
      }
      push @pattern, $bits;
   }

   # Output bits.  Two copies of the same pattern is printed to output, so
   # we can get a vertically shifted bit pattern by copying with an offset.
   print "\t{",
         (join ",", map {sprintf('0x%02x', $_)} @pattern),
         ", ",
         (join ",", map {sprintf('0x%02x', $_)} @pattern),
         "},\n";
}

print "};\n";
