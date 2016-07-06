============================================================================
GEO triangulation notes
============================================================================
Last updated on 5/15/13; 6/16/13; 6/20/13; 7/2/13
============================================================================

Preliminaries
-------------

*.  Create new subdirectory of bundler/GEO/
(e.g. mains/geo/bundler/GEO/20120105_1402/).  We will refer to this
"bundler" subdirectory as BUNDLER_IO_SUBDIR in this README file.

*.  Place raw images' jpeg files into a new raw_images/ subdirectory of
BUNDLER_IO_SUBDIR.  Create soft link from BUNDLER_IO_SUBDIR/images/ to
BUNDLER_IO_SUBDIR/raw_images/ .

*.  Copy relevant part of Ross Anderson's metadata file into
BUNDLER_IO_SUBDIR.  Create soft link from this metadata file to
BUNDLER_IO_SUBDIR/camera_metadata.txt .

*.  Run mains/photosynth/generate_peter_inputs .

FLIR imagery and metadata programs
----------------------------------

*.  Execute the auto-generated run_visualize_flir_metadata script:

 ~/programs/c++/svn/projects/src/mains/photosynth/run_visualize_FLIR_metadata

Program mains/aerosynth/VISUALIZE_FLIR_METADATA is a variant of
PARSE_FLIR_METADATA.  It reads in the metadata ascii file generated by Ross
Anderson's program which ran on the Mac-Mini along with the FLIR for Twin
Otter flights.  It extracts aircraft GPS, aircraft orientation and FLIR
pointing information from this metadata file. This program generates a TDP
file containing the aircraft's GPS track, line-of-sight rays from the
aircraft towards the ground, and Uhat-Vhat direction vectors.
VISUALIZE_FLIR_METADATA also outputs package files containing low-defn
frame filenames and their hardware derived camera parameters.  Finally, it
exports an executable script for program SEPARATED_PACKAGES.

*.  Execute the auto-generated run_crop_analog_frames script:

 ~/programs/c++/svn/projects/src/mains/photosynth/run_crop_analog_frames

Program mains/geo/CROP_ANALOG_FRAMES reads in a set of package files
generated by program VISUALIZE_FLIR_FRAMES. It extracts their
low-definition JPG frames and crops each one so that it contains (mostly)
FLIR pixel content without screen metadata.  The cropped low-defn frames
are exported to bundler_IO_subdir/cropped_lowdefn_frames/.  A new soft link
is generated from bundler_IO_subdir/cropped_lowdefn_frames/ to
bundler_IO_subdir/images.

CROP_ANALOG_FRAMES also generates list_tmp.txt and trivial bundle.out files
corresponding to the cropped low-defn video frames.

*.  Execute the auto-generated run_separated_packages:

 ~/programs/c++/svn/projects/src/mains/geo/run_separated_packages

Program mains/geo/SEPARATED_PACKAGES imports an entire set of video frames
for some flight.  It then queries the user to enter the maximum permissible
angular separation between subsampled frames which will be bundle adjusted.
On 5/7/13, we empirically found that setting this angular threshold to 5
degs yielded quite good reconstruction results for GEO pass 20120105_1402!
SEPARATED_PACKAGES exports the angularly separated package filenames to an
output text file which can serve as input to progrmas RESTRICTED_ASIFT and
TRIANGULATE.  It also outputs the image filenames corresponding to those
packages to bundler_IO_subdir/list_tmp.txt.


Feature matching and bundle adjustment programs
-----------------------------------------------

*.  Execute the auto-generated run_restricted_asift:

 ~/programs/c++/svn/projects/src/mains/geo/run_restricted_asift

Program mains/geo/RESTRICTED_ASIFT is a variant of program ASIFTVID.  It
performs expensive feature matching only between pairs of aerial video
frames whose hardware estimates for camera orientations indicate that the
separation angle between image planes is less than some threshold value.

*.  Execute the auto-generated run_triangulate:

 ~/programs/c++/svn/projects/src/mains/geo/run_triangulate

Program mains/geo/TRIANGULATE imports SIFT/ASIFT feature tracks generated
by program RESTRICTED_ASIFTVID.  It also takes in package files for the n
video frames processed by ASIFTVID which contain hardware-based camera
parameters.  TRIANGULATE backprojects each 2D feature into a 3D ray.  It
then computes multi-line intersection points for each feature track.
Triangulated 3D points are exported to a features_3D.txt file, and
correlated 2D/3D feature information is written photograph_features.html .

TRIANGULATE exports hardware-based camera parameters and
triangulated ground points to bundle_raw.out.  BUNDLER is
subsequently called to perform (re)bundle adjustment with camera
focal parameters held fixed at their previously calibrated values.
Bundle-adjusted camera positions are written to an output text
file.

*.  For Bundler output, re-run generate_peter_inputs.  Then execute
run_photo_sizes, run_thumbnails, run_mini_convert, run_bundler_photos,
run_write_viewbundler_script and run_viewbundler as described in
mains/photosynth/README.view_bundler_output.

*.  Execute the auto-generated run_gpsfit:

 ~/programs/c++/svn/projects/src/mains/geo/run_gpsfit

Program mains/geo/GPSFIT reads in reconstructed camera posns generated by
program PARSE_SSBA in UTM coords.  It also reads in a text file containing
hardware GPS measurements for the camera generated by program TRIANGULATE.
If the two input files are NOT in precise correspondence, GPSFIT generates
a new version of the GPS measurements file whose entries do match those in
the reconstructed camera posns file.

GPSFIT reports the average residual between the transformed and measured
paths.  This program also exports the transformed path as a TDP and OSGA
file.

*.  Re-execute run_mini_convert, run_bundler_photos and run_viewbundler
after georegistered version of peter_inputs.pkg is generated by program
GPSFIT.


*.  Program VIDEO_PROPAGATOR is a variant of mains/photosynth/PROPAGATOR.
It pops open a 3D window displaying a bundle-adjusted point cloud aerial
OBSFRUSTA. It also opens a 2D window in which the video frame for the most
currently selected OBSFRUSTUM is displayed.  When the 2D window works in
RUN_MOVIE_MODE, all video frames may easily be viewed by pressing the
left/right arrow keys.

A user may mark some imageplane point of interest within the 2D window
after entering into INSERT_FEATURE_MODE.  A 3D ray is then backprojected
from the current OBSFRUSTUM down towards ground-level.  Feature 0 appears
in all other video frames within the lower left corner of each image plane.
Video tiepoints may be manually established by dragging the zeroth feature
crosshairs from the lower left corner to an appropriate image plane
location in MANIPULATE_FEATURE_MODE.  Within the 3D window, a purple ray
moves as the 2D feature is dragged.

Once 2 or more tiepoints have been established, the user may press 'd'
within the 3D window provided it operates in MANIPULATE_FUSED_DATA_MODE.  A
set of 3D crosshairs then appears at the best-fit intersection of the
backprojected rays.

Video orthorectification programs
---------------------------------

*.  Program GROUNDPLANE imports the sparse point cloud generated by
PARSE_SSBA.  It fits a plane to the cloud and computes the distribution of
point distances to the fitted plane.  Any point that lies more than 5
quartile-widths away from the median distance is assumed to be an outlier.
GROUNDPLANE exports a cleaned TDP file consisting of just inlier points.

*. Program ORTHORECTIFY imports bundle-adjusted triangulated points and
camera parameters for some set of input aerial images.  It fits a world
z-plane to the triangulated points (which we assume provides a good
approximation to the groundplane). ORTHORECTIFY then reprojects each aerial
image onto the ground plane and outputs an orthorectified image.


Video navigation programs
-------------------------

*.  Program CAMGEOREG imports a georegistered TIF file generated from a
Google Earth screen shot plus ground control points.  It also takes in an
aerial video frame. CAMGEOREG extracts SIFT and ASIFT features via calls to
Lowe's SIFT binary and the affine SIFT library.  Consolidated SIFT & ASIFT
interest points and descriptors are exported to key files following Lowe's
conventions.  CAMGEOREG next performs tiepoint matching via homography
estimation and RANSAC on the consolidated sets of image features.  The
best-fit homography matrix and feature tracks labeled by unique IDs are
exported to output text files.

*.  Program FLIR_FOCAL imports 3D/2D tiepoints selected from HAFB ALIRT and
FLIR video frames as well as corresponding GPS camera position
measurements.  FLIR_FOCAL computes world-space angles between all tiepoint
pairs and derives a dimensionless focal parameter from each angle.  We
wrote this utility program in order to calibrate "five deg" f.


-------------------------------------------------------------------
Deprecated programs


*.  Program HOMORECT imports SIFT/ASIFT feature tracks generated by program
ASIFTVID.  It also takes in package files for the n video frames processed
by ASIFTVID which contain hardware-based camera parameters.  Working with
adjacent pairs of video frames, HOMORECT purges any feature which doesn't
reasonably satisfy a homography relationship between temporally neighboring
image planes.  It then backprojects surviving 2D features onto a ground
Z-plane.  HOMORECT recovers parameters for the "next" 3x4 projection matrix
based upon the homography between the backprojected ground plane points and
their "next" imageplane counterparts.

*.  Program LIST_HARDWARE_PACKAGES is a little utility program which we
wrote to generate a list of selected FLIR hardware packages which serve as
inputs to programs RESTRICTED_ASIFT, TRIANGULATE and PARSE_SSBA.

*.  Program ASIFTVID imports a set of image files.  It extracts SIFT and
ASIFT features via calls to Lowe's SIFT binary and the affine SIFT library.
Consolidated SIFT & ASIFT interest points and descriptors are exported to
key files following Lowe's conventions.  ASIFTVID next performs tiepoint
matching via fundamental matrix estimation and RANSAC on the consolidated
sets of image features.  Fundamental matrices and feature tracks labeled by
unique IDs are exported to output text files.  Finally, an html file is
generated which lists features containing at least 2 tiepoint
correspondences.

*.  Program PARSE_SSBA imports the refined structure from motion parameters
file generated by SSBA-3.0.  It resets the cameras' intrinsic and extrinsic
parameters based upon the bundle-adjusted results.  It also exports a
"sparse 3D cloud" corresponding to the bundle-adjusted triangulated points.



*.  Program mains/aerosynth/PARSE_FLIR_METADATA reads in the metadata ascii
file generated by Ross Anderson's program which ran on the Mac-Mini along
with the FLIR for Twin Otter flights.  It extracts aircraft GPS, aircraft
orientation and FLIR pointing information from this metadata file.  After
manipulating and reformating the input metadata, this program exports it to
output files aircraft_metadata.txt and camera_metadata.txt.




============================================================================
"2nd" flight facility rooftop target UTM coordinates from GE:

Easting =   312031
Northing = 4703905

