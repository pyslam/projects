// ==========================================================================
// Program SOLVE_MAZE
// ==========================================================================
// Last updated on 11/5/16; 11/9/16; 11/13/16
// ==========================================================================

#include <iostream>
#include <string>
#include <vector>
#include "machine_learning/environment.h"
#include "games/maze.h"
#include "numrec/nrfuncs.h"
#include "general/outputfuncs.h"
#include "machine_learning/reinforce.h"
#include "general/stringfuncs.h"
#include "time/timefuncs.h"

int main (int argc, char* argv[])
{
   using std::cin;
   using std::cout;
   using std::endl;
   using std::string;
   using std::vector;

   timefunc::initialize_timeofday_clock();
   nrfunc::init_time_based_seed();

   int n_grid_size = 2;
   cout << "Enter grid size:" << endl;
   cin >> n_grid_size;
   int n_actions = 4;

// Construct one particular maze:

   maze curr_maze(n_grid_size);
   curr_maze.generate_maze();
   curr_maze.solve_maze_backwards();
   curr_maze.generate_all_turtle_states();

// Construct environment which acts as interface between reinforcement
// agent and particular game:

   environment game_world(environment::MAZE);
   game_world.set_maze(&curr_maze);

   int Din = curr_maze.get_occupancy_state()->get_mdim(); // Input dim
   int Dout = n_actions;
   int Tmax = 1;

   int H1 = 1 * Din;
//   int H1 = 2 * Din;
//   int H1 = 2 * Din;
//   int H1 = 4 * Din;
   int H2 = 0;
//   int H2 = 1 * Dout;

   string extrainfo="H1/Din="+stringfunc::number_to_string(H1/Din);
   if(H2 > 0)
   {
      "; H2/Dout="+stringfunc::number_to_string(H2/Dout);
   }

   vector<int> layer_dims;
   layer_dims.push_back(Din);
   layer_dims.push_back(H1);
   if(H2 > 0)
   {
      layer_dims.push_back(H2);
   }
   layer_dims.push_back(Dout);

// Construct reinforcement learning agent:

   int replay_memory_capacity = 3 * sqr(n_grid_size);
//   int replay_memory_capacity = 5 * sqr(n_grid_size);
   reinforce* reinforce_agent_ptr = new reinforce(
      layer_dims, Tmax, replay_memory_capacity);
//   reinforce_agent_ptr->set_debug_flag(true);
   reinforce_agent_ptr->set_environment(&game_world);
   curr_maze.set_qmap_ptr(reinforce_agent_ptr->get_qmap_ptr());

   int output_counter = 0;
   string output_subdir = "./output_solns";
   string basename = "maze";
   bool display_qmap_flag = true;
   curr_maze.DrawMaze(output_counter++, output_subdir, basename,
                      display_qmap_flag);

// Gamma = discount factor for reward:

   reinforce_agent_ptr->set_gamma(0.95);
   reinforce_agent_ptr->set_batch_size(3);
//   reinforce_agent_ptr->set_batch_size(10);   
//   reinforce_agent_ptr->set_batch_size(30);
   reinforce_agent_ptr->set_rmsprop_decay_rate(0.85);

   reinforce_agent_ptr->set_base_learning_rate(1E-3);
//   reinforce_agent_ptr->set_base_learning_rate(3E-4);
//   reinforce_agent_ptr->set_base_learning_rate(1E-4);
//   reinforce_agent_ptr->set_base_learning_rate(3E-5);
//   reinforce_agent_ptr->set_base_learning_rate(1E-5);
   double min_learning_rate = 1E-4;

   int n_max_episodes = 100 * 1000 * 1000;
   int n_anneal_steps = 1000;
   int n_update = 10000;
   int n_summarize = 20000;
   double Qmap_score = -1;

// Periodically decrease learning rate down to some minimal floor
// value:

   int n_episodes_period = 100 * 1000;

// Initialize Deep Q replay memory:

   game_world.start_new_episode();

   reinforce_agent_ptr->initialize_replay_memory();
   int update_old_weights_counter = 0;
   double total_loss = -1;

   while(reinforce_agent_ptr->get_episode_number() < n_max_episodes &&
         Qmap_score < 0.999999)
   {
      bool random_turtle_start = true;
      curr_maze.reset_game(random_turtle_start);
      reinforce_agent_ptr->initialize_episode();

      int curr_episode_number = reinforce_agent_ptr->get_episode_number();
      outputfunc::update_progress_and_remaining_time(
         curr_episode_number, 5 * n_summarize, n_max_episodes);

      if(curr_episode_number > 0 && curr_episode_number%n_episodes_period == 0)
      {
         double curr_learning_rate = reinforce_agent_ptr->get_learning_rate();
         if(curr_learning_rate > min_learning_rate)
         {
            reinforce_agent_ptr->set_learning_rate(0.8 * curr_learning_rate);
            n_episodes_period *= 1.2;
         }
      }

// -----------------------------------------------------------------------
// Current episode starts here:

//      cout << "************  Start of Game " << curr_episode_number
//           << " ***********" << endl;
//      curr_maze.print_occupancy_grid();
//      curr_maze.print_occupancy_state();

      double reward;
      genvector* next_s;
      while(!curr_maze.get_game_over())
      {
         genvector *curr_s = game_world.get_curr_state();
         int d = reinforce_agent_ptr->store_curr_state_into_replay_memory(
            *curr_s);
         int curr_a = reinforce_agent_ptr->select_action_for_curr_state();

         if(!game_world.is_legal_action(curr_a))
         {
            next_s = NULL;
            reward = -1;
            curr_maze.set_game_over(true);
         }
         else
         {
            next_s = game_world.compute_next_state(curr_a);
            reward = curr_maze.compute_turtle_reward();
         } // curr_a is legal action conditional

//         cout << " reward = " << reward 
//              << " game over = " << curr_maze.get_game_over() << endl;

         if(curr_maze.get_game_over())
         {
            reinforce_agent_ptr->store_arsprime_into_replay_memory(
               d, curr_a, reward, *curr_s, curr_maze.get_game_over());
         }
         else
         {
            reinforce_agent_ptr->store_arsprime_into_replay_memory(
               d, curr_a, reward, *next_s, curr_maze.get_game_over());
         }
      } // game_over while loop

// -----------------------------------------------------------------------

      reinforce_agent_ptr->increment_episode_number();


      if(curr_episode_number > 0 && curr_episode_number % 
         reinforce_agent_ptr->get_batch_size() == 0)
      {
         update_old_weights_counter++;
         if(update_old_weights_counter%100 == 0)
         {
            reinforce_agent_ptr->copy_weights_onto_old_weights();
         }

         total_loss = reinforce_agent_ptr->update_Q_network();
      }

      if(curr_episode_number > 0 && curr_episode_number % n_anneal_steps == 0)
      {
         double decay_factor = 0.995;
//         double min_epsilon = 1.0 / sqr(n_grid_size);
         double min_epsilon = 0.1;
         reinforce_agent_ptr->anneal_epsilon(decay_factor, min_epsilon);
      }

      if(curr_episode_number%n_update == 0)
      {
         cout << "episode_number = " << curr_episode_number << endl;
         
         reinforce_agent_ptr->compute_deep_Qvalues();
//          reinforce_agent_ptr->print_Qmap();

         curr_maze.compute_max_Qmap();
         Qmap_score = curr_maze.score_max_Qmap();
         cout << "Qmap_score = " << Qmap_score << endl;
         reinforce_agent_ptr->push_back_Qmap_score(Qmap_score);

         if(total_loss > 0)
         {
            cout << "Total loss = " << total_loss 
                 << " log10(total_loss) = " << log10(total_loss) << endl;
            reinforce_agent_ptr->push_back_log10_loss(log10(total_loss));
         }

         curr_maze.DrawMaze(output_counter++, output_subdir, basename,
                            display_qmap_flag);

//         reinforce_agent_ptr->set_epsilon(1 - Qmap_score);
      }

/*
      int n_episodes = curr_episode_number + 1;
      if(curr_episode_number > 10 && curr_episode_number % n_update == 0)
      {
//         curr_maze.print_turtle_path_history();


         cout << "  n_soln_steps = "
              << curr_maze.get_n_soln_steps() 
              << "  n_turtle_moves = "
              << curr_maze.get_n_turtle_moves() 
              << " ratio = " 
              << curr_n_turns_ratio << endl;

         reinforce_agent_ptr->snapshot_running_reward();
      }

      if(curr_episode_number > 10 && curr_episode_number % n_summarize == 0)
      {
         reinforce_agent_ptr->compute_weight_distributions();
         reinforce_agent_ptr->plot_loss_history(extrainfo);
         reinforce_agent_ptr->plot_reward_history(extrainfo, -1, 1);
      }
*/

//      outputfunc::enter_continue_char();
   } // n_episodes < n_max_episodes while loop

//   curr_maze.DrawMaze(output_counter++, output_subdir, basename,
//                      display_qmap_flag);
   reinforce_agent_ptr->plot_Qmap_score_history(output_subdir, extrainfo);
   reinforce_agent_ptr->plot_log10_loss_history(output_subdir, extrainfo);
//   reinforce_agent_ptr->print_Qmap();

   delete reinforce_agent_ptr;
}



