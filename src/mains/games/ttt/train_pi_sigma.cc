// ==========================================================================
// Program TRAIN_PI_SIGMA
// ==========================================================================
// Last updated on 1/15/17; 1/16/17; 1/17/17; 1/18/17
// ==========================================================================

#include <iostream>
#include <string>
#include <unistd.h>     // needed for getpid()
#include <vector>
#include "general/filefuncs.h"
#include "machine_learning/machinelearningfuncs.h"
#include "machine_learning/neural_net.h"
#include "numrec/nrfuncs.h"
#include "general/outputfuncs.h"
#include "general/stringfuncs.h"
#include "general/sysfuncs.h"
#include "games/tictac3d.h"
#include "time/timefuncs.h"

int main (int argc, char* argv[])
{
   using std::cin;
   using std::cout;
   using std::endl;
   using std::ofstream;
   using std::string;
   using std::vector;

   timefunc::initialize_timeofday_clock();

   int n_cells = 64;
   int Din = n_cells;   	// Number of input layer nodes
//   int Din = n_cells + 1;   	// Number of input layer nodes
   int H1 = 64;			// Number of first hidden layer nodes
   int H2 = 32;			// Number of 2nd hidden layer nodes
   int H3 = 0;			// Number of 3rd hidden layer nodes
   cout << "Enter H1:" << endl;
   cin >> H1;
   cout << "Enter H2:" << endl;
   cin >> H2;
   cout << "Enter H3:" << endl;
   cin >> H3;
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
   double lambda = 0;  // L2 regularization coefficient
//   double lambda = 1E-3;  // L2 regularization coefficient
   cout << "Enter L2 regularization coefficient lambda:" << endl;
   cin >> lambda;
   double rmsprop_decay_rate = 0.95;

   neural_net NN(mini_batch_size, lambda, rmsprop_decay_rate, layer_dims);
   machinelearning_func::set_leaky_ReLU_small_slope(0.01);    

   double blr;
   cout << "Enter base learning rate:" << endl;
   cin >> blr;
   NN.set_base_learning_rate(blr);

//   NN.set_base_learning_rate(3E-4);
//   NN.set_base_learning_rate(1E-4);
//   NN.set_base_learning_rate(3E-5);

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
   filefunc::dircreate(output_subdir);
   NN.set_output_subdir(output_subdir);

// Import afterstate-action pairs generated by program minimax2.
// Convert them into training and testing data samples:

   vector<neural_net::DATA_PAIR> training_samples;
   vector<neural_net::DATA_PAIR> testing_samples;
   const double testing_sample_frac = 0.1;
   int n_training_samples = 0;
   int n_testing_samples = 0;

   string input_subdir = "./afterstate_action_pairs/";
   string input_filename = input_subdir + "afterstate_action_pairs.txt";
   filefunc::ReadInfile(input_filename);

   vector<vector<string> > line_substrings = 
      filefunc::ReadInSubstrings(input_filename);

   neural_net::DATA_PAIR curr_data_pair;

   int max_move_rel_to_game_end = 1;
   cout << "Enter maximum move relative to game end:" << endl;
   cin >> max_move_rel_to_game_end;

   for(unsigned int i = 0; i < line_substrings.size(); i++)
   {
      string board_state_str = line_substrings[i].at(0);
      int move_rel_to_game_end = stringfunc::string_to_integer(
         line_substrings[i].at(1));

      if(move_rel_to_game_end >= max_move_rel_to_game_end) continue;

      int aplus1 = stringfunc::string_to_integer(line_substrings[i].at(2));
      double player_value = 1;
      if(aplus1 < 0) player_value = -1;
      int a = abs(aplus1) - 1;
      curr_data_pair.second = a;

//      cout << board_state_str << "  "
//           << " player_value = " << player_value
//           << " a = " << a << endl;

// Store player_value = +1 or -1 within last entry in
// *player_board_state_ptr:

// FAKE FAKE:  Weds Jan 18 at 8:06 am

// Experiment with forming 64-dim player_board states in which action ALWAYS
// corresponds to agent_value = +1:

      genvector *player_board_state_ptr = new genvector(n_cells);
//      genvector *player_board_state_ptr = new genvector(n_cells + 1);
      curr_data_pair.first = player_board_state_ptr;
      player_board_state_ptr->clear_values();
      
      for(int c = 0; c < n_cells; c++)
      {
         if(c != a)
         {
            if(board_state_str[c] == 'X')
            {
               player_board_state_ptr->put(c, -1);
            }
            else if(board_state_str[c] == 'O')
            {
               player_board_state_ptr->put(c, 1);
            }
         }
      }

      *player_board_state_ptr *= player_value;
      player_board_state_ptr->put(n_cells, player_value);

      if(nrfunc::ran1() < testing_sample_frac)
      {
         testing_samples.push_back(curr_data_pair);
      }
      else
      {
         training_samples.push_back(curr_data_pair);
      }
      
      n_training_samples = training_samples.size();
      n_testing_samples = testing_samples.size();
   } // loop over index i labeling input text lines

   int n_data_samples = n_training_samples + n_testing_samples;
   cout << "n_training_samples = " << n_training_samples
        << " n_testing_samples = " << n_testing_samples
        << " n_data_samples = " << n_data_samples
        << endl;

   NN.import_training_data(training_samples);
   NN.import_test_data(testing_samples);
//   int n_epochs = 1 * 1000;
   int n_epochs = 10 * 1000;

// Generate text file summary of parameter values:

   string params_filename = output_subdir + "params.dat";
   NN.summarize_parameters(params_filename);
   ofstream params_stream;
   filefunc::appendfile(params_filename, params_stream);

   params_stream << "Max n_training_epochs = " << n_epochs << endl;
   params_stream << "Leaky ReLU small slope = "
                 << machinelearning_func::get_leaky_ReLU_small_slope() << endl;
   params_stream << "Maximum move relative to game end = " 
                 << max_move_rel_to_game_end << endl;
//   params_stream << "Learning rate decrease period = " 
//                 << n_lr_episodes_period << " episodes" << endl;
   params_stream << "Process ID = " << getpid() << endl;
   filefunc::closefile(params_filename, params_stream);

   NN.train_network(n_epochs);

   vector<int> incorrect_classifications = NN.get_incorrect_classifications();
   double incorrect_testing_frac = incorrect_classifications.size() / 
      n_testing_samples;
   cout << "Number of testing samples incorrectly classified = "
        << incorrect_classifications.size() << endl;
   cout << "incorrect_testing_frac = " << incorrect_testing_frac << endl;

// Delete dynamically allocated memory:

   for(int i = 0; i < n_training_samples; i++)
   {
      delete training_samples[i].first;
   }
   for(int i = 0; i < n_testing_samples; i++)
   {
      delete testing_samples[i].first;
   }

}



