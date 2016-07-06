// ==========================================================================
// Program FIT_HORIZONS loops over input DIME WISP panoramas.  For
// each image, it imports horizon line segments generated by program
// EXTRACT_HORIZON_SEGMENTS from an input text file.  Line segments
// lying too far above or below the panorama's horizontal midline are
// ignored.  Segments with large slope magnitudes are similarly
// ignored.  Surviving segments are then ordered according to their
// midpoint U coordinates.  Initial estimates for the A, phi and v_avg
// parameters entering into the sinusoidal function for the projected
// horizon are computed based upon 3 neighboring line segments.  A
// brute-force search is subsequently conducted for refined sinusoid
// parameter estimates.  The score function favors long line segments
// that lie close to the sinusoid fit.  Best fit values for the A, phi
// and v_avg parameters are exported to text file output.  And fitted
// sinusoids are drawn on top of WISP-360 panoramas.

// 		v_horizon = v_avg + A sin( 2 PI u/Umax + phi)


//				./fit_horizons

// ==========================================================================
// Last updated on 8/8/13; 8/12/13; 8/20/13; 11/21/13
// ==========================================================================

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "general/filefuncs.h"
#include "geometry/linesegment.h"
#include "math/lttwovector.h"
#include "numerical/param_range.h"
#include "video/texture_rectangle.h"
#include "time/timefuncs.h"
#include "video/videofuncs.h"

double sinusoid(double u,double A,double phi,double v_avg,double Umax)
{
   double arg=2*PI*u/Umax+phi;
   double v=v_avg+A*sin(arg);
   return v;
}

double f(double phi,double u1,double u2,double u3,double umax)
{
   double term1=sin(2*PI*u1/umax+phi);
   double term2=sin(2*PI*u2/umax+phi);
   double term3=sin(2*PI*u3/umax+phi);
   double numer=term2-term1;
   double denom=term3-term2;
   double frac=numer/denom;
   return frac;
}

double fprime(double phi,double u1,double u2,double u3,double umax)
{
   double term1=sin(2*PI*u1/umax+phi);
   double term2=sin(2*PI*u2/umax+phi);
   double term3=sin(2*PI*u3/umax+phi);
   double numer=term2-term1;
   double denom=term3-term2;

   double costerm1=cos(2*PI*u1/umax+phi);
   double costerm2=cos(2*PI*u2/umax+phi);
   double costerm3=cos(2*PI*u3/umax+phi);
   
   double Bignumer=denom*(costerm2-costerm1)-numer*(costerm3-costerm2);
   double Bigdenom=sqr(denom);

   double frac=Bignumer/Bigdenom;
   return frac;
}


// ==========================================================================
int main(int argc, char *argv[])
// ==========================================================================
{
   using std::cin;
   using std::cout;
   using std::endl;
   using std::flush;
   using std::map;
   using std::ofstream;
   using std::string;
   using std::vector;

// On Dunmeyer's Ubuntu 12.4 box:

   string date_string="05202013";
   cout << "Enter date string (e.g. 05202013 or 05222013):" << endl;
   cin >> date_string;
   filefunc::add_trailing_dir_slash(date_string);

   string DIME_subdir="/data/DIME/";
   string MayFieldtest_subdir=DIME_subdir+"panoramas/May2013_Fieldtest/";
//   string FSFdate_subdir=MayFieldtest_subdir+"05202013/";
   string FSFdate_subdir=MayFieldtest_subdir+date_string;
   cout << "FSFdate_subdir = " << FSFdate_subdir << endl;

   int scene_ID=29;
   cout << "Enter scene ID:" << endl;
   cin >> scene_ID;
   string scene_ID_str=stringfunc::integer_to_string(scene_ID,2);
   string panos_subdir=FSFdate_subdir+"Scene"+scene_ID_str+"/";
   cout << "panos_subdir = " << panos_subdir << endl;

   string raw_images_subdir=panos_subdir+"raw/";
   string horizons_subdir=panos_subdir+"horizons/";
   string linesegments_subdir=horizons_subdir+"linesegments_files/";

   string sinusoid_params_filename=horizons_subdir+"sinusoid_params.dat";
   ofstream outstream;

   if (filefunc::fileexist(sinusoid_params_filename))
   {
      filefunc::appendfile(sinusoid_params_filename,outstream);
   }
   else
   {
      filefunc::openfile(sinusoid_params_filename,outstream);
      outstream << 
         "# image_basename       A       phi       v_avg       max_score" 
                << endl << endl;
   }

   vector<string> image_filenames=filefunc::image_files_in_subdir(
      raw_images_subdir);
   timefunc::initialize_timeofday_clock();

   int iter_start=0;
   cout << "Enter starting panorama ID:" << endl;
//   cin >> iter_start;
//   int iter_stop=0;
   int iter_stop=image_filenames.size()-1;
   int iter_step=1;
//   int iter_step=25;
//   int iter_step=50;
//   int iter_step=100;
   for (int iter=iter_start; iter<=iter_stop; iter += iter_step)
   {
      double progress_frac=double(iter-iter_start)/
         double(iter_stop-iter_start);
      outputfunc::print_elapsed_and_remaining_time(progress_frac);

      string image_filename=image_filenames[iter];
//      cout << "image_filename = " << image_filename << endl;
      string image_basename=filefunc::getbasename(image_filename);
      cout << "image_basename = " << image_basename << endl;

// Import line segments calculated via program
// EXTRACT_HORIZON_SEGMENTS:

      string separator_chars="_.";
      vector<string> substrings=stringfunc::decompose_string_into_substrings(
         image_basename,separator_chars);
//      string framenumber_str=substrings[1];
      string framenumber_str=substrings[2];
//      cout << "framenumber_str = " << framenumber_str << endl;
      string linesegments_filename=
         linesegments_subdir+"linesegments_"+framenumber_str+".dat";
//      cout << "linesegments_filename = " << linesegments_filename << endl;
      filefunc::ReadInfile(linesegments_filename);

      vector<linesegment> long_orig_segments;
      for (unsigned int i=0; i<filefunc::text_line.size(); i++)
      {
         vector<double> column_values=stringfunc::string_to_numbers(
            filefunc::text_line[i]);
         threevector V1(column_values[1],column_values[2]);
         threevector V2(column_values[3],column_values[4]);
         linesegment curr_segment(V1,V2);
         long_orig_segments.push_back(curr_segment);
      }
      cout << "Original number of long segments =" << long_orig_segments.size()
           << endl;

// Ignore any line segment which lies outside 0.4 < V < 0.65:

      vector<linesegment> candidate_segments;
      for (unsigned int i=0; i<long_orig_segments.size(); i++)
      {
         linesegment curr_segment=long_orig_segments[i];
         double v1=curr_segment.get_v1().get(1);
         double v2=curr_segment.get_v2().get(1);
         if (v1 < 0.4 || v1 > 0.65 || v2 < 0.4 || v2 > 0.65) continue;
//         if (v1 < 0.35 || v1 > 0.65 || v2 < 0.35 || v2 > 0.65) continue;

// Ignore any line segment with large absolute slope values:

         double u1=curr_segment.get_v1().get(0);
         double u2=curr_segment.get_v2().get(0);
         double du=u2-u1;
         if (nearly_equal(du,0)) continue;

// Ignore any line segment which endpoints lie in obvious non-ocean
// parts of WISP images:

// May 2013 FSF imagery:
         
         double min_u_non_ocean=9.9;   // 21780 / 2200
         double max_u_non_ocean=10.25; // 22560 / 2200

// Feb 2013 Deer Island imagery:

//         double min_u_non_ocean=1.54545;	// 3400 / 2200
//         double max_u_non_ocean=9.54545;	// 21000 / 2200

         if (u1 > min_u_non_ocean && u1 < max_u_non_ocean)
         {
//            cout << "u1 = " << u1 << " is not in ocean" << endl;
            continue;
         }
         if (u2 > min_u_non_ocean && u2 < max_u_non_ocean)
         {
//            cout << "u2 = " << u2 << " is not in ocean" << endl;
            continue;
         }

         double dv=v2-v1;
      
         if (du < 0)
         {
            du *=-1;
            dv *=-1;
         }
         double dv_du=dv/du;
         if (fabs(dv_du) > 1) continue;

         candidate_segments.push_back(curr_segment);
      }

      cout << "Number candidate segments #1= " << candidate_segments.size()
           << endl;
      if (candidate_segments.size() < 5) continue;

// Compute point-to-line distances between neighboring candidate
// segments.  Then reject any candidate segment whose adjacent
// neighbor makes too strong an angle relative to it:

      const double max_dtheta=10*PI/180;
      vector<linesegment> candidate_segments2;
      for (unsigned int i=0; i<candidate_segments.size()-1; i++)
      {
         linesegment curr_segment=candidate_segments[i];
         linesegment next_segment=candidate_segments[i+1];
      
         double perp_dist=curr_segment.point_to_line_distance(
            next_segment.get_midpoint());
         double separation_dist=
            (next_segment.get_midpoint()-
            curr_segment.get_midpoint()).magnitude();
         double dtheta=atan2(perp_dist,separation_dist);

         if (dtheta > max_dtheta) continue;
         candidate_segments2.push_back(curr_segment);
      }
      cout << "Number candidate segments #2 = " << candidate_segments2.size()
           << endl;
      if (candidate_segments2.size() < 2) continue;

// Construct initial estimates for A, phi and v_avg parameters
// entering into sinusoidal horizon function

//		v = v_avg + A sin (2*PI/Umax * u + phi)

      double Umax=double(40000)/double(2200);

// Perform brute-force search for horizon sinusoid parameters: 

      double max_score=-1;
      double best_A,best_phi,best_vavg;
      
      double Astart=0.01;
      double Astop=0.04;
      param_range A(Astart,Astop,5);
   
      double phi_start=-PI;
      double phi_stop=PI;
      param_range phi(phi_start,phi_stop,19);

      double vavg_start=0.48;
      double vavg_stop=0.52;
      param_range v_avg(vavg_start,vavg_stop,9);

      int n_iters=20;
      for (int iter=0; iter<n_iters; iter++)
      {
//         cout << "  iter = " << iter << " of " << n_iters << endl;
         cout << iter << " " << flush;

         while (A.prepare_next_value())
         {
            while (phi.prepare_next_value())
            {
               while (v_avg.prepare_next_value())
               {

// Score match between each candidate line segment with current
// sinusoidal horizon function:

                  double curr_score=0;
                  int n_nearby_segments=0;
                  for (unsigned int s=0; s<candidate_segments2.size(); s++) 
                  {
                     linesegment curr_segment=candidate_segments2[s];
                     twovector p_start(curr_segment.get_v1());
                     twovector p_stop(curr_segment.get_v2());

                     double dv_start=p_start.get(1)-sinusoid(
                        p_start.get(0),A.get_value(),phi.get_value(),
                        v_avg.get_value(),Umax);
                     double dv_stop=p_stop.get(1)-sinusoid(
                        p_stop.get(0),A.get_value(),phi.get_value(),
                        v_avg.get_value(),Umax);

// Ignore current segment if it lies too far away from current
// sinusoid function:

                     double max_dv=0.05;
                     if (fabs(dv_start) > max_dv || fabs(dv_stop) > max_dv)
                        continue;

// Weight current segment's contribution to score function by its
// length.  Exponentially deweight segment by its endpoints distances
// from the sinusoid function:

                     curr_score += curr_segment.get_length()*
                        ( exp(-fabs(dv_start)/max_dv) + 
                        exp(-fabs(dv_stop)/max_dv) );

                     n_nearby_segments++;
                  } // loop over index s labeling candidate segments

                  int min_n_nearby_segments=0.30*candidate_segments2.size();
                  if (n_nearby_segments < min_n_nearby_segments) continue;

                  if (curr_score > max_score)
                  {
                     max_score=curr_score;
                     A.set_best_value();
                     phi.set_best_value();
                     v_avg.set_best_value();

//                     cout << "max_score = " << max_score << endl;
//                     cout << "Best A value = " << A.get_best_value() << endl;
//                     cout << "Best phi value = " << phi.get_best_value()
//                        *180/PI << endl;
//                     cout << "Best v_avg value = " << v_avg.get_best_value() 
//                          << endl;
//                     cout << endl;
                  }

               } // v_avg while loop
            } // phi while loop
         } // A while loop

//         double frac=0.55;
         double frac=0.66;
         A.shrink_search_interval(A.get_best_value(),frac);
         phi.shrink_search_interval(phi.get_best_value(),frac);
         v_avg.shrink_search_interval(v_avg.get_best_value(),frac);

      } // loop over iter index

      best_A=A.get_best_value();
      best_phi=phi.get_best_value();
      best_vavg=v_avg.get_best_value();

      if (best_A < 0)
      {
         best_A *= -1;
         best_phi += PI;
      }
      best_phi=basic_math::phase_to_canonical_interval(best_phi,0,2*PI);

      cout << endl;
      cout << "max_score = " << max_score << endl;
      cout << "Best A value = " << best_A << endl;
      cout << "Best phi value = " << best_phi*180/PI << endl;
      cout << "Best v_avg value = " << best_vavg << endl;

// Save fitted sinusoid parameters to output text file:

      outstream << image_basename << "   "
                << best_A << "   "
                << best_phi*180/PI << "   "
                << best_vavg << "   "
                << max_score 
                << endl;
 
      string banner="Initializing texture rectangles";
      outputfunc::write_banner(banner);

      texture_rectangle* grey_texture_rectangle_ptr=
         new texture_rectangle(image_filename,NULL);
      int width=grey_texture_rectangle_ptr->getWidth();
      int height=grey_texture_rectangle_ptr->getHeight();

      Umax=double(width)/double(height);
      double Vmax=1;
      cout << "Umax = " << Umax << " Vmax = " << Vmax << endl;

      texture_rectangle* texture_rectangle_ptr=new texture_rectangle(
         width,height,1,3,NULL);
      string blank_filename="blank.jpg";
      texture_rectangle_ptr->generate_blank_image_file(
         width,height,blank_filename,0.5);

      int R,G,B;
      for (int pu=0; pu<width; pu++)
      {
         for (int pv=0; pv<height; pv++)
         {
            grey_texture_rectangle_ptr->get_pixel_RGB_values(pu,pv,R,G,B);
            texture_rectangle_ptr->set_pixel_RGB_values(pu,pv,R,G,B);
         }

// Draw sinusoidal horizon function on top of WISP-360 panorama:

         double u=double(pu)/double(height);
         double v=sinusoid(u,best_A,best_phi,best_vavg,Umax);
         int pv_horizon=height*(1-v);
      
         int R_horizon=255;
         int G_horizon=0;
         int B_horizon=0;

         texture_rectangle_ptr->set_pixel_RGB_values(
            pu,pv_horizon-2,R_horizon,G_horizon,B_horizon);
         texture_rectangle_ptr->set_pixel_RGB_values(
            pu,pv_horizon,R_horizon,G_horizon,B_horizon);
         texture_rectangle_ptr->set_pixel_RGB_values(
            pu,pv_horizon+1,R_horizon,G_horizon,B_horizon);
         texture_rectangle_ptr->set_pixel_RGB_values(
            pu,pv_horizon+2,R_horizon,G_horizon,B_horizon);

      } // loop over pu index

      string horizon_filename=horizons_subdir+"horizon_"+image_basename;
      texture_rectangle_ptr->write_curr_frame(horizon_filename);

//   int n_channels=texture_rectangle_ptr->getNchannels();
//   cout << "n_channels = " << n_channels << endl;

// Superpose randomly colored surviving line segments onto WISP-360
// panorama containing fitted horizon function:

      int segment_color_index=-1;	// random segment coloring   
      videofunc::draw_line_segments(
         candidate_segments2,texture_rectangle_ptr,
         segment_color_index);

      string segments_filename=horizons_subdir+"segments_"+image_basename;
      texture_rectangle_ptr->write_curr_frame(segments_filename);

      delete texture_rectangle_ptr;
      delete grey_texture_rectangle_ptr;

      banner="Exported "+segments_filename;
      outputfunc::write_big_banner(banner);

   } // loop over iter index labeling panoramas

   filefunc::closefile(sinusoid_params_filename,outstream);

   string banner="Exported sinusoid parameters to "+sinusoid_params_filename;
   outputfunc::write_banner(banner);

   banner="Finished running program FIT_HORIZONS";
   outputfunc::write_big_banner(banner);
   outputfunc::print_elapsed_time();   
}
