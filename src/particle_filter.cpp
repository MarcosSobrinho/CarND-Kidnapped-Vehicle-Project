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
#include <array>

#include "helper_functions.h"

using std::string;
using std::vector;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
  /**
   * TODO: Set the number of particles. Initialize all particles to 
   *   first position (based on estimates of x, y, theta and their uncertainties
   *   from GPS) and all weights to 1. 
   * TODO: Add random Gaussian noise to each particle.
   * NOTE: Consult particle_filter.h for more information about this method 
   *   (and others in this file).
   */

  std::normal_distribution<double> dist_x(x, std[0]);
  std::normal_distribution<double> dist_y(y, std[1]);
  std::normal_distribution<double> dist_t(theta, std[2]);
  
  //std::array<Particle, num_particles> test; 

  for(int i = 0; i < num_particles; ++i){
    Particle sample;
    sample.x = dist_x(gen);
    sample.y = dist_y(gen);
    sample.theta = dist_t(gen);
    sample.weight = 1.0;
    particles.push_back(sample);
  }
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
  std::normal_distribution<double> dist_x(0.0, std_pos[0]);
  std::normal_distribution<double> dist_y(0.0, std_pos[1]);
  std::normal_distribution<double> dist_t(0.0, std_pos[2]);
  
  double vel = velocity/yaw_rate;
  std::cout << vel << std::endl;

  if(std::isinf(vel)){
    double vel = velocity/yaw_rate;
    std::cout << vel << std::endl;
    for (auto& p : particles){
      p.weight = 1.0;
      double pred_t = p.theta;
      double pred_x = p.x + velocity*delta_t*cos(pred_t);
      double pred_y = p.y + velocity*delta_t*sin(pred_t);
      
      p.x = pred_x + dist_x(gen);
      p.y = pred_y + dist_y(gen);
      p.theta = pred_t + dist_t(gen);
    }
  }
  else {
    for (auto& p : particles){
      p.weight = 1.0;
      double pred_t = p.theta + delta_t*yaw_rate;
      double pred_x = p.x + vel*(sin(pred_t) - sin(p.theta));
      double pred_y = p.y + vel*(cos(p.theta) - cos(pred_t));

      p.x = pred_x + dist_x(gen);
      p.y = pred_y + dist_y(gen);
      p.theta = pred_t + dist_t(gen);
    }
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
  const double sig_x = std_landmark[0];
  const double sig_y = std_landmark[1];

  const double gauss_norm = 1.0 / (2.0*M_PI*sig_x*sig_y);

  for (auto& p : particles){
    //transform particle obs to map coord
    vector<LandmarkObs> predicted;
    for (auto& obs : observations){
      LandmarkObs new_pred;
      new_pred.x = p.x + obs.x*cos(p.theta) - obs.y*sin(p.theta);
      new_pred.y = p.y + obs.x*sin(p.theta) + obs.y*cos(p.theta);
      predicted.push_back(new_pred);
    }

    //associate particle obs to landmark
    vector<Map::single_landmark_s> associasons;

    for (auto& pred : predicted){
          double min_diff;
          bool first_iter = true;
          Map::single_landmark_s association;

      for (auto& mark : map_landmarks.landmark_list){
        double diff = pow(pred.x - mark.x_f, 2) + pow(pred.y - mark.y_f, 2);

        if(first_iter || (diff < min_diff)){
          min_diff = diff;
          association = mark;
        }
        first_iter = false;
      }

      associasons.push_back(association);
    }

    //update weight
    for (unsigned int i = 0; i < predicted.size(); ++i){
      auto c_p = predicted[i];
      auto c_a = associasons[i];

      double exponent = (pow(c_p.x -c_a.x_f, 2) / (2 * pow(sig_x, 2)))
               + (pow(c_p.y -c_a.y_f, 2) / (2 * pow(sig_y, 2)));
      p.weight *= (gauss_norm * exp(-exponent));
    }
  }
}

void ParticleFilter::resample() {
  /**
   * TODO: Resample particles with replacement with probability proportional 
   *   to their weight. 
   * NOTE: You may find std::discrete_distribution helpful here.
   *   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
   */
  vector<double> weights(num_particles);
  for (int i = 0; i < num_particles; ++i){
    weights[i] = particles[i].weight;
  }

  std::discrete_distribution<> d(weights.begin(), weights.end());

  vector<Particle> new_particles;
  for (int i = 0; i < num_particles; ++i){
    new_particles.push_back(particles[d(gen)]);
  }

  particles = std::move(new_particles);
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