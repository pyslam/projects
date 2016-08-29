// ==========================================================================
// Header file for tictac3d class 
// ==========================================================================
// Last modified on 8/28/16
// ==========================================================================

#ifndef TICTAC3D_H
#define TICTAC3D_H

#include <map>
#include <vector>
#include "math/lttriple.h"
#include "math/threevector.h"

class tictac3d
{
   
  public:

   typedef std::map<triple, int, lttriple> WINNING_POSNS_MAP;

// independent triple:  winning board posns
// dependent int:  winner ID

// Initialization, constructor and destructor functions:

   tictac3d();
   tictac3d(const tictac3d& C);
   ~tictac3d();
   friend std::ostream& operator<< 
      (std::ostream& outstream,const tictac3d& C);

   void randomize_board_state();
   void display_board_state();
   int check_player_win(int player_ID);

  private: 

   int n_size;
   std::vector<int> curr_board_state;

   WINNING_POSNS_MAP winning_posns_map;
   WINNING_POSNS_MAP::iterator winning_posns_iter;

   void allocate_member_objects();
   void initialize_member_objects();

   int get_cell_value(int px, int py, int pz);
   void display_Zgrid_state(int pz);

   bool winning_cell_posn(int player_ID, int px, int py, int pz);
   void print_winning_pattern();

   bool Zplane_win(int player_ID, int pz);
   bool Zcolumn_win(int player_ID, int px, int py);
   bool corner_2_corner_win(int player_ID);
   bool Zslant_xconst_win(int player_ID, int px);
   bool Zslant_yconst_win(int player_ID, int py);
};

// ==========================================================================
// Inlined methods:
// ==========================================================================

// Set and get member functions:



#endif  // tictac3d.h

