// ==========================================================================
// Program SELF_PLAY allows a minimax AI to play again a minimax
// It periodically exports afterstate-action pairs for both players to
// output text files for later supervised learning policy training
// purposes.  It also can export a frequency histogram for the total
// number of turns (AI + agent) played per game.
// ==========================================================================
// Last updated on 1/22/17
// ==========================================================================

#include <iostream>
#include <string>
#include <vector>
#include "general/filefuncs.h"
#include "machine_learning/neural_net.h"
#include "numrec/nrfuncs.h"
#include "general/outputfuncs.h"
#include "general/sysfuncs.h"
#include "games/tictac3d.h"
#include "time/timefuncs.h"

int main (int argc, char* argv[])
{
   using std::cin;
   using std::cout;
   using std::endl;
   using std::string;
   using std::vector;

   std::set_new_handler(sysfunc::out_of_memory);
   timefunc::initialize_timeofday_clock();
   long seed = nrfunc::init_time_based_seed();
//   long seed = -11;
//   cout << "Enter negative seed:" << endl;
//   cin >> seed;
//   nrfunc::init_default_seed(seed);

   int nsize = 4;
   int n_zlevels = 4;
   tictac3d* ttt_ptr = new tictac3d(nsize, n_zlevels);
   int n_cells = ttt_ptr->get_n_total_cells();

   int Din = n_cells;
   int H1 = 256;			// Number of first hidden layer nodes
   int H2 = 256;			// Number of 2nd hidden layer nodes
   int H3 = 0;				// Number of 3rd hidden layer nodes
   int Dout = n_cells;   	// Number of output layer nodes

   vector<int> layer_dims;
   layer_dims.push_back(Din);
   layer_dims.push_back(H1);
   if(H2 > 0)
   {
      layer_dims.push_back(H2);
   }
   if(H3 > 0)
   {
      layer_dims.push_back(H3);
   }
   layer_dims.push_back(Dout);

// Set up neural network:

   int mini_batch_size = 100;
   double lambda = 1E-4;  // L2 regularization coefficient
//   cout << "Enter L2 regularization coefficient lambda:" << endl;
//   cin >> lambda;
   double rmsprop_decay_rate = 0.95;
   neural_net NN(mini_batch_size, lambda, rmsprop_decay_rate, layer_dims);

// Initialize output subdirectory within an experiments folder:

   string experiments_subdir="./experiments/";
   filefunc::dircreate(experiments_subdir);
   string pi_sigma_subdir = experiments_subdir + "pi_sigma/";
   filefunc::dircreate(pi_sigma_subdir);

   int expt_number;
   cout << "Enter experiment number:" << endl;
   cin >> expt_number;
   NN.set_expt_number(expt_number);
   string output_subdir=pi_sigma_subdir+
      "expt"+stringfunc::integer_to_string(expt_number,3)+"/";
   NN.set_output_subdir(output_subdir);

// Import neural network snapshot:

   string snapshots_subdir = output_subdir + "snapshots/";
   string snapshot_filename = snapshots_subdir + "snapshot.txt";
   NN.import_snapshot(snapshot_filename);

   int AI_value = -1;   // "X"
   int agent_value = 1;   // "O"
   int n_AI_wins = 0;
   int n_agent_wins = 0;
   double AI_win_frac = 0;
   double agent_win_frac = 0;
   vector<double> class_probs;
   vector<int> class_IDs;
   int n_games = 100;
   for(int g = 0; g < n_games; g++)
   {
      cout << "************************************************************" 
           << endl;
      cout << "Starting game " << g << endl;
      outputfunc::update_progress_and_remaining_time(g, 10, n_games);

      bool AI_move_first = false;
      if(nrfunc::ran1() < 0.5)
      {
         AI_move_first = true;
         cout << "AI moves first" << endl;
      }
      else
      {
         cout << "Agent moves first" << endl;
      }

      ttt_ptr->reset_board_state();
      while(!ttt_ptr->get_game_over())
      {

// AI move:

         if(AI_move_first || ttt_ptr->get_n_agent_turns() > 0)
         {
            genvector* input_state_ptr = 
               ttt_ptr->update_inverse_board_state_ptr();
            bool sort_probs_flag = false;
            NN.get_class_probabilities(
               input_state_ptr, sort_probs_flag, class_probs, class_IDs);
            double prob_sum = 0;
            for(unsigned int c = 0; c < class_IDs.size(); c++)
            {
               if(!ttt_ptr->legal_player_move(c))
               {
                  class_probs[c] = 0;
               }
               prob_sum += class_probs[c];
            }

// Renormalize legal moves' probability distribution:

            for(unsigned int c = 0; c < class_IDs.size(); c++)
            {
               class_probs[c] /= prob_sum;
            }

            int AI_move = NN.get_class_prediction_given_probs(class_probs);
            ttt_ptr->set_player_move(AI_move, AI_value);
            ttt_ptr->record_latest_move(AI_value, AI_move);

            ttt_ptr->increment_n_AI_turns();
            if(ttt_ptr->check_player_win(AI_value) > 0)
            {
               ttt_ptr->set_game_over(true);
               break;
            }
            if(ttt_ptr->check_filled_board()) break;
         }

// Agent move:

         genvector* input_state_ptr = ttt_ptr->update_board_state_ptr();
         bool sort_probs_flag = false;
         NN.get_class_probabilities(
            input_state_ptr, sort_probs_flag, class_probs, class_IDs);
         double prob_sum = 0;
         for(unsigned int c = 0; c < class_IDs.size(); c++)
         {
            if(!ttt_ptr->legal_player_move(c))
            {
               class_probs[c] = 0;
            }
            prob_sum += class_probs[c];
         }

// Renormalize legal moves' probability distribution:

         for(unsigned int c = 0; c < class_IDs.size(); c++)
         {
            class_probs[c] /= prob_sum;
         }

         int agent_move = NN.get_class_prediction_given_probs(class_probs);
         ttt_ptr->set_player_move(agent_move, agent_value);
         ttt_ptr->record_latest_move(agent_value, agent_move);
         ttt_ptr->display_board_state();
         ttt_ptr->increment_n_agent_turns();

         outputfunc::enter_continue_char();

         if(ttt_ptr->check_player_win(agent_value) > 0)
         {
            ttt_ptr->set_game_over(true);
            break;
         }

         if(ttt_ptr->check_filled_board()) break;

         int n_completed_turns = ttt_ptr->get_n_completed_turns();
//         cout << "n_completed_turns = " << n_completed_turns << endl;
      } // !game_over while loop

      if(ttt_ptr->check_player_win(AI_value) > 0)
      {
         n_AI_wins++;
         AI_win_frac = double(n_AI_wins) / n_games;
      }
      else if (ttt_ptr->check_player_win(agent_value) > 0)
      {
         n_agent_wins++;
         agent_win_frac = double(n_agent_wins) / n_games;
      }
      ttt_ptr->record_n_total_game_turns();

      ttt_ptr->print_winning_pattern();
//      cout << "n_AI_turns = " << ttt_ptr->get_n_AI_turns() << endl;
//      cout << "n_agent_turns = " << ttt_ptr->get_n_agent_turns() << endl;
      cout << "n_total_game_turns = " << ttt_ptr->get_n_total_game_turns() 
           << endl;
      cout << "n_AI_wins = " << n_AI_wins 
           << " n_agent_wins = " << n_agent_wins << endl;
   } // loop over index g labeling games

   cout << endl << endl;
   cout << "======================================================== " << endl;
   cout << "n_games = " << n_games 
        << " n_AI_wins = " << n_AI_wins 
        << " n_agent_wins = " << n_agent_wins << endl;
   cout << "Winning game fractions: AI = " << AI_win_frac
        << " agent = " << agent_win_frac << endl;
   cout << "======================================================== " << endl;

   delete ttt_ptr;
}


