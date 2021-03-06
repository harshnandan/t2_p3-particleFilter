/*
 * particle_filter.cpp
 *
 *  Created on: Dec 12, 2016
 *      Author: Tiffany Huang
 */

#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <math.h> 
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>

#include "particle_filter.h"

using namespace std;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
	// TODO: Set the number of particles. Initialize all particles to first position (based on estimates of 
	//   x, y, theta and their uncertainties from GPS) and all weights to 1. 
	// Add random Gaussian noise to each particle.
	// NOTE: Consult particle_filter.h for more information about this method (and others in this file).

	// initialize number of particles
	num_particles = 100;
	// set initialized falg to true
	is_initialized = true;
	// generate normal distributions
	default_random_engine gen;
	normal_distribution<double> dist_x(x, std[0]);
	normal_distribution<double> dist_y(y, std[1]);
	normal_distribution<double> dist_theta(theta, std[2]);

	for(int i=0; i<num_particles; i++){
		// initialize particles
		Particle p = {i, dist_x(gen), dist_y(gen), dist_theta(gen), 1};
		particles.push_back(p);
	}

}

void ParticleFilter::prediction(double delta_t, double std_pos[], double velocity, double yaw_rate) {
	// TODO: Add measurements to each particle and add random Gaussian noise.
	// NOTE: When adding noise you may find std::normal_distribution and std::default_random_engine useful.
	//  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
	//  http://www.cplusplus.com/reference/random/default_random_engine/

	default_random_engine gen;
	normal_distribution<double> dist_posx(0, std_pos[0]);
	normal_distribution<double> dist_posy(0, std_pos[1]);
	normal_distribution<double> dist_theta(0, std_pos[2]);

	for(int i=0; i<num_particles; i++){
		Particle p = particles[i];
		// update position/orientation information for all particles
		// ovet dt
		if (abs(yaw_rate) > 0.01){
			p.x = p.x + (velocity/yaw_rate)*(sin(p.theta + yaw_rate*delta_t)-sin(p.theta)) + dist_posx(gen);
			p.y = p.y + (velocity/yaw_rate)*(cos(p.theta)-cos(p.theta + yaw_rate*delta_t)) + dist_posy(gen);
			p.theta = p.theta + yaw_rate*delta_t +  + dist_theta(gen);
		}else{
			p.x = p.x + velocity*cos(p.theta)*delta_t + dist_posx(gen);
			p.y = p.y + velocity*sin(p.theta)*delta_t + dist_posy(gen);
			p.theta = p.theta + dist_theta(gen);
		}
		particles[i] = p;
	}
}

void ParticleFilter::dataAssociation(std::vector<LandmarkObs> predicted, std::vector<LandmarkObs>& observations) {
	// TODO: Find the predicted measurement that is closest to each observed measurement and assign the 
	//   observed measurement to this particular landmark.
	// NOTE: this method will NOT be called by the grading code. But you will probably find it useful to 
	//   implement this method and use it as a helper during the updateWeights phase.

}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
		const std::vector<LandmarkObs> &observations, const Map &map_landmarks) {
	// TODO: Update the weights of each particle using a mult-variate Gaussian distribution. You can read
	//   more about this distribution here: https://en.wikipedia.org/wiki/Multivariate_normal_distribution
	// NOTE: The observations are given in the VEHICLE'S coordinate system. Your particles are located
	//   according to the MAP'S coordinate system. You will need to transform between the two systems.
	//   Keep in mind that this transformation requires both rotation AND translation (but no scaling).
	//   The following is a good resource for the theory:
	//   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
	//   and the following is a good resource for the actual equation to implement (look at equation 
	//   3.33
	//   http://planning.cs.uiuc.edu/node99.html
	double sig_x = std_landmark[0];
	double sig_y = std_landmark[1];

	// update weight for each particle
	for(int i=0; i<num_particles; i++){
		Particle p = particles[i];
		p.weight = 1;
		vector<int> association_vec;
		vector<double> sense_x_vec;
		vector<double> sense_y_vec;

		// process each observation
		for(int j=0; j<observations.size(); j++){
			// calculate landmark position in map coordinates
			LandmarkObs obs = observations[j];
			double xm = p.x + cos(p.theta) * obs.x - sin(p.theta) * obs.y;
			double ym = p.y + sin(p.theta) * obs.x + cos(p.theta) * obs.y;

			double min_dist = 100000;
			int land_idx = 0;
			float mu_x;
			float mu_y;
			// find closest landmark
			for(int k=0; k<map_landmarks.landmark_list.size(); k++){
				double dist_val = dist(map_landmarks.landmark_list[k].x_f,
									   map_landmarks.landmark_list[k].y_f,
									   xm, ym);
				if (dist_val < min_dist){
					min_dist = dist_val;
					land_idx = map_landmarks.landmark_list[k].id_i;
					mu_x = map_landmarks.landmark_list[k].x_f;
					mu_y = map_landmarks.landmark_list[k].y_f;
				}
			}
			// update landmark association data
			association_vec.push_back(land_idx);
			sense_x_vec.push_back(mu_x);
			sense_y_vec.push_back(mu_y);
			// calculate probability of match between observed and landmark position
			double exponent= ( (xm - mu_x)*(xm - mu_x) /(2*sig_x*sig_x)) +
					((ym - mu_y)*(ym - mu_y)/(2*sig_y*sig_y));
			p.weight *= (1/(2 * M_PI * sig_x * sig_y)) * exp(-exponent);
		}
		SetAssociations(p, association_vec, sense_x_vec, sense_y_vec);
		particles[i] = p;
	}
}

void ParticleFilter::resample() {
	// TODO: Resample particles with replacement with probability proportional to their weight. 
	// NOTE: You may find std::discrete_distribution helpful here.
	//   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution

	// isolate weights in one vector
	vector<double> w_vec;
	for(int i=0; i<num_particles; i++){
		Particle p = particles[i];
		w_vec.push_back( p.weight );
	}

	// use discrete distribution to resample
	discrete_distribution<> dist_resample(w_vec.begin(), w_vec.end());
	default_random_engine gen;

	// resample
	vector<Particle> resampled_particles;
	for(int i=0; i<num_particles; i++){
		int pIdx = dist_resample(gen);
		resampled_particles.push_back(particles[pIdx]);
	}
	particles = resampled_particles;

}

void ParticleFilter::SetAssociations(Particle& particle, const std::vector<int>& associations,
                                     const std::vector<double>& sense_x, const std::vector<double>& sense_y)
{
    //particle: the particle to assign each listed association, and association's (x,y) world coordinates mapping to
    // associations: The landmark id that goes along with each listed association
    // sense_x: the associations x mapping already converted to world coordinates
    // sense_y: the associations y mapping already converted to world coordinates

    particle.associations= associations;
    particle.sense_x = sense_x;
    particle.sense_y = sense_y;

    //return particle
}

string ParticleFilter::getAssociations(Particle best)
{
	vector<int> v = best.associations;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<int>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseX(Particle best)
{
	vector<double> v = best.sense_x;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseY(Particle best)
{
	vector<double> v = best.sense_y;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
