/**
 * particle_filter.cpp
 *
 * Created on: Dec 12, 2016
 * Author: Tiffany Huang
 */

#include "particle_filter.h"

#include <math.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "helper_functions.h"

using std::string;
using std::vector;
using std::normal_distribution;
using std::uniform_real_distribution;
using namespace std;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
  /**
   * TODO: Set the number of particles. Initialize all particles to 
   *   first position (based on estimates of x, y, theta and their uncertainties
   *   from GPS) and all weights to 1. 
   * TODO: Add random Gaussian noise to each particle.
   * NOTE: Consult particle_filter.h for more information about this method 
   *   (and others in this file).
   */
  num_particles = 101;  // TODO: Set the number of particles
   
  default_random_engine gen;
  
  // This line creates a normal (Gaussian) distribution for x
  normal_distribution<double> dist_x(0.0, std[0]);
  
  // This line creates a normal (Gaussian) distribution for y
  normal_distribution<double> dist_y(0.0, std[1]);
  
  // This line creates a normal (Gaussian) distribution for theta
  normal_distribution<double> dist_theta(0.0, std[2]);
  
   for (int i = 0; i < num_particles; i++) 
   {
     Particle p;
     p.id=i;
     p.x=x;
     p.y=y;
     p.theta=theta;
     p.weight = 1.0;
     
     //noise
     p.x+=dist_x(gen);
     p.y+=dist_y(gen);
     p.theta+=dist_theta(gen);
     
     particles.push_back(p);
   }
  
  // The filter is now initialized.
  is_initialized = true;

}

void ParticleFilter::prediction(double delta_t, double std_pos[], 
                                double velocity, double yaw_rate) {
  /**
   * TODO: Add measurements to each particle and add random Gaussian noise.
   * NOTE: When adding noise you may find std::normal_distribution 
   *   and std::default_random_engine useful.
   *  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
   *  http://www.cplusplus.com/reference/random/default_random_engine/
   */
  
  // This line creates a normal (Gaussian) distribution for x
  normal_distribution<double> dist_x(0.0, std_pos[0]);
  
  // This line creates a normal (Gaussian) distribution for y
  normal_distribution<double> dist_y(0.0, std_pos[1]);
  
  // This line creates a normal (Gaussian) distribution for theta
  normal_distribution<double> dist_theta(0.0, std_pos[2]);
  
  default_random_engine gen;
  
   for (int i = 0; i < num_particles; i++) 
   {
     Particle p = particles[i];
     if(fabs(yaw_rate) < 0.00001)
     {
      particles[i].x += velocity *delta_t * cos(p.theta);
      particles[i].y += velocity *delta_t * sin(p.theta);
     }
     else
     {
      particles[i].x += velocity / yaw_rate * (sin(p.theta+yaw_rate*delta_t) - sin(p.theta));
      particles[i].y += velocity / yaw_rate * (cos(p.theta) - cos(p.theta+yaw_rate*delta_t));
      particles[i].theta += yaw_rate*delta_t;
     }
    particles[i].x += dist_x(gen);
    particles[i].y += dist_y(gen);
    particles[i].theta += dist_theta(gen);
  }
}

void ParticleFilter::dataAssociation(vector<LandmarkObs> predicted, 
                                     vector<LandmarkObs>& observations) {
  /**
   * TODO: Find the predicted measurement that is closest to each 
   *   observed measurement and assign the observed measurement to this 
   *   particular landmark.
   * NOTE: this method will NOT be called by the grading code. But you will 
   *   probably find it useful to implement this method and use it as a helper 
   *   during the updateWeights phase.
   */
  for (unsigned int i = 0; i < observations.size(); i++) 
  {
    
    // grab current observation
    LandmarkObs o = observations[i];

    // init minimum distance to maximum possible
    double min_dist = numeric_limits<double>::max();

    // init id of landmark from map placeholder to be associated with the observation
    int map_id = -1;
    
    for (unsigned int j = 0; j < predicted.size(); j++) 
    {
      // grab current prediction
      LandmarkObs p = predicted[j];
      
      // get distance between current/predicted landmarks
      double cur_dist = dist(o.x, o.y, p.x, p.y);

      // find the predicted landmark nearest the current observed landmark
      if (cur_dist < min_dist) 
      {
        min_dist = cur_dist;
        map_id = p.id;
      }
    }

    // set the observation's id to the nearest predicted landmark's id
    observations[i].id = map_id;
  }

}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
                                   const vector<LandmarkObs> &observations, 
                                   const Map &map_landmarks) {
  /**
   * TODO: Update the weights of each particle using a mult-variate Gaussian 
   *   distribution. You can read more about this distribution here: 
   *   https://en.wikipedia.org/wiki/Multivariate_normal_distribution
   * NOTE: The observations are given in the VEHICLE'S coordinate system. 
   *   Your particles are located according to the MAP'S coordinate system. 
   *   You will need to transform between the two systems. Keep in mind that
   *   this transformation requires both rotation AND translation (but no scaling).
   *   The following is a good resource for the theory:
   *   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
   *   and the following is a good resource for the actual equation to implement
   *   (look at equation 3.33) http://planning.cs.uiuc.edu/node99.html
   */
   for(int i=0; i<num_particles; i++)
   {
    
    /* 
       Step0, convert map.landmark_list to the
              LandmarkObs structure
    */
    vector<LandmarkObs> ldmk_ObsMap;
     	
    for(unsigned int j=0; j<map_landmarks.landmark_list.size(); j++)
    {
      LandmarkObs lm;
      lm.id= map_landmarks.landmark_list[j].id_i;
      lm.x = map_landmarks.landmark_list[j].x_f;
      lm.y = map_landmarks.landmark_list[j].y_f;
      if( fabs(lm.x-particles[i].x) < sensor_range && fabs(lm.y-particles[i].y) < sensor_range )
      {
        ldmk_ObsMap.push_back(lm);
      }
    }
    /*
       Step1, transformation from OBSx to TOBSx.
              The observations are given in the VEHICLE'S coordinate system.
              These codes would first convert the VEHICLE's coordinate 
              obs to the MAP's coord.
    */
    vector<LandmarkObs> vec_tObs;
     
    for(unsigned int j=0; j<observations.size(); j++)
    {
      LandmarkObs tObs;
      
      tObs.x  = particles[i].x + observations[j].x*cos(particles[i].theta) -
                                 observations[j].y*sin(particles[j].theta);
      tObs.y  = particles[i].y + observations[j].x*sin(particles[i].theta) +
                                 observations[j].y*cos(particles[j].theta);
      vec_tObs.push_back(tObs);
    }
    /*
       Step2, finding the nearest landmarks for each TOBSx.
              each TOBs should have a ID for a landmark, which 
              is closest to this landmark.
    */
    dataAssociation(ldmk_ObsMap, vec_tObs); 
     
    /*
       Step3, calculating the particle's final weight
    */
    particles[i].weight = 1.0;
    vector<double> vec_senseX;
    vector<double> vec_senseY;
    vector<int> vec_associations;
    
    for(int j=0; j<vec_tObs.size(); j++)
    {
      double ox = vec_tObs[j].x;
      double oy = vec_tObs[j].y;
      int    associateId = vec_tObs[j].id;
      double ux,uy;
      for (unsigned int k = 0; k < ldmk_ObsMap.size(); k++) {
        if (ldmk_ObsMap[k].id == associateId) {
        ux = ldmk_ObsMap[k].x;
        uy = ldmk_ObsMap[k].y;
        }
      }
      double up = (ox - ux)*(ox - ux)/(2*std_landmark[0]*std_landmark[0]) +
                  (oy - uy)*(oy - uy)/(2*std_landmark[1]*std_landmark[1]);
      
      double Pxy = 1.0 / (2*M_PI*std_landmark[0]*std_landmark[1]) * pow(M_E, -1*up);
   
      particles[i].weight *= Pxy;
    }
    //SetAssociations(particles[i], vec_associations, vec_senseX, vec_senseY);
     
   }
}

void ParticleFilter::resample() {
  /**
   * TODO: Resample particles with replacement with probability proportional 
   *   to their weight. 
   * NOTE: You may find std::discrete_distribution helpful here.
   *   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
   */
  default_random_engine gen;
  vector<Particle> new_particles;
  
  vector<double> weights;
  for(int i=0; i<num_particles; i++)
  {
    weights.push_back(particles[i].weight);
  }
  uniform_real_distribution<double> unirealdist(0.0, 1.0);
  int index   = int(unirealdist(gen) * num_particles);
  double beta = 0.0;
  double mw   = *max_element(weights.begin(), weights.end());
  for(int i=0; i<num_particles; i++)
  {
    beta += unirealdist(gen) * 2.0 * mw;
    while(beta > weights[index])
    {
      beta -= weights[index];
      index = (index+1) % num_particles;
    }
   new_particles.push_back(particles[index]);
  }
  particles = new_particles;
}

void ParticleFilter::SetAssociations(Particle& particle, 
                                     const vector<int>& associations, 
                                     const vector<double>& sense_x, 
                                     const vector<double>& sense_y) {
  // particle: the particle to which assign each listed association, 
  //   and association's (x,y) world coordinates mapping
  // associations: The landmark id that goes along with each listed association
  // sense_x: the associations x mapping already converted to world coordinates
  // sense_y: the associations y mapping already converted to world coordinates
  particle.associations= associations;
  particle.sense_x = sense_x;
  particle.sense_y = sense_y;
}

string ParticleFilter::getAssociations(Particle best) {
  vector<int> v = best.associations;
  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<int>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}

string ParticleFilter::getSenseCoord(Particle best, string coord) {
  vector<double> v;

  if (coord == "X") {
    v = best.sense_x;
  } else {
    v = best.sense_y;
  }

  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<float>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}