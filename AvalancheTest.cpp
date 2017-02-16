#include "AvalancheTest.h"

//-----------------------------------------------------------------------------
// Print out a diagram in a hacked up base-51 representation.
// We are printing out counts of bit changes, where we expect to see the bits
// change half the time. This lends itself naturally to a base-51 representation
// which means we can use one character per bit/count.
// The 'scale' parameter controls how magnified the data should be. So setting it
// to 1 shows 2% per digit, with the absolute worst being "#". Setting it to 2
// shows roughly 1% per digit with anything higher than 0.5 being shown as "#",
// setting to 10 shows 0.2% per character, with anything higher than 0.1 being
// shown as "#", etc.
// A good hash function should show all "." for scale 2 at least.

void PrintAvalancheDiagram ( int seedbits, int keybits, int hashbits, int reps, int scale, std::vector<int> & bins )
{
  int rows = seedbits + keybits;
  /*                                1         2         3         4         5    */
  /*                      0        90        90        90        90        90    */
  const char * symbols = ".1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ[]{}()<>*&%$@#";
  int *cursor= &bins[0];
  if(!scale) scale = 1;

  for(int i = 0; i < rows; i++)
  {
    printf("%-4s bit %3d [", i < seedbits ? "seed" : "key",
            i < seedbits ? i : i - seedbits);
    for(int j = 0; j < hashbits; j++)
    {
      double b = double(*cursor++) / double(reps);
      b = fabs( 2 * b - 1);                     /*interval: [0,  1] */
      int s = (int)floor(b * (50.0 * scale));   /*interval: [0, 50] */
      printf("%c",symbols[s >= 50 ? 50 : s]);
    }
    printf("]\n");
  }
}

//----------------------------------------------------------------------------

double calcBiasStats ( std::vector<int> & counts, int reps, double *err, double *chi, double *gval )
{
  double worst = 0;
  double _err;
  double _chi;
  double _gval;
  double dreps= double(reps);
  double expect= dreps / 2;

  if (!err) err= &_err;
  if (!chi) chi= &_chi;
  if (!_gval) gval= &_gval;
  *err= 0.0;
  *chi= 0.0;
  *gval= 0.0;

  for(int i = 0; i < (int)counts.size(); i++)
  {
    double changed= double(counts[i]);
    double same= double(reps-counts[i]);

    double ratio = changed / dreps;
    double diff = 0.5 - ratio;
    double d_pct = fabs(diff * 2);
    *err += (diff * diff);

    *chi += pow(changed - expect, 2) / expect;
    *chi += pow(same - expect, 2) / expect;
    if (changed > 0) *gval += changed * log( changed / expect );
    if (same > 0) *gval += same * log( same / expect );
      
    if(d_pct > worst)
    {
      worst = d_pct;
    }
  }
  return worst;
}

//-----------------------------------------------------------------------------
