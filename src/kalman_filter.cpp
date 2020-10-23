//#include "kalman_filter.h"
//
//using Eigen::MatrixXd;
//using Eigen::VectorXd;
//
///*
// * Please note that the Eigen library does not initialize
// *   VectorXd or MatrixXd objects with zeros upon creation.
// */
//
//KalmanFilter::KalmanFilter() {}
//
//KalmanFilter::~KalmanFilter() {}
//
//void KalmanFilter::Init(VectorXd &x_in, MatrixXd &P_in, MatrixXd &F_in,
//                        MatrixXd &H_in, MatrixXd &R_in, MatrixXd &Q_in) {
//  x_ = x_in;
//  P_ = P_in;
//  F_ = F_in;
//  H_ = H_in;
//  R_ = R_in;
//  Q_ = Q_in;
//}
//
//void KalmanFilter::Predict() {
//  /**
//   * TODO: predict the state
//   */
//}
//
//void KalmanFilter::Update(const VectorXd &z) {
//  /**
//   * TODO: update the state by using Kalman Filter equations
//   */
//}
//
//void KalmanFilter::UpdateEKF(const VectorXd &z) {
//  /**
//   * TODO: update the state by using Extended Kalman Filter equations
//   */
//}
#include "FusionEKF.h"
#include <iostream>
#include "Eigen/Dense"
#include "tools.h"
#include <math.h>

using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::cout;
using std::endl;
using std::vector;

/**
 * Constructor.
 */
FusionEKF::FusionEKF() {
  is_initialized_ = false;

  previous_timestamp_ = 0; //1477010442500000;

  // initializing matrices
  R_laser_ = MatrixXd(2, 2);
  R_radar_ = MatrixXd(3, 3);
  H_laser_ = MatrixXd(2, 4);
  Hj_ = MatrixXd(3, 4);

  //measurement covariance matrix - laser
  R_laser_ << 0.0225, 0,
          0, 0.0225;

  //measurement covariance matrix - radar
  R_radar_ << 0.09, 0, 0,
          0, 0.0009, 0,
          0, 0, 0.09;

  /**
   * TODO: Finish initializing the FusionEKF.
   * TODO: Set the process and measurement noises
   */
  H_laser_ << 1, 0, 0, 0,
          0, 1, 0, 0;


  // state covariance matrix P
  ekf_.P_ = MatrixXd(4, 4);
  ekf_.P_ << 1, 0, 0, 0,
          0, 1, 0, 0,
          0, 0, 1000, 0,
          0, 0, 0, 1000;



  // set the acceleration noise components
//   float noise_ax = 9;
//   float noise_ay = 9;
}

/**
 * Destructor.
 */
FusionEKF::~FusionEKF() {}

void FusionEKF::ProcessMeasurement(const MeasurementPackage &measurement_pack) {
  /**
   * Initialization
   */
  if (!is_initialized_) {
    /**
     * TODO: Initialize the state ekf_.x_ with the first measurement.
     * TODO: Create the covariance matrix.
     * You'll need to convert radar from polar to cartesian coordinates.
     */

    // first measurement
    cout << "EKF: " << endl;
    ekf_.x_ = VectorXd(4);
    ekf_.x_ << 1, 1, 1, 1;

    if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
      // TODO: Convert radar from polar to cartesian coordinates
      //         and initialize state.
      float rou = measurement_pack.raw_measurements_[0];
      float theta = measurement_pack.raw_measurements_[1];
      //float rou_dot = measurement_pack.raw_measurements_[2];
      ekf_.x_(0) = rou * cos(theta);
      ekf_.x_(1) = rou * sin(theta);
      for (int i = 0; i <= 1; ++i) {  // encounter special situation x or y is close to 0
        if (fabs(ekf_.x_(i)) < 1e-4) {
          ekf_.x_(i) = 1e-4;
        }
      }
    }
    else if (measurement_pack.sensor_type_ == MeasurementPackage::LASER) {
      // TODO: Initialize state.
      ekf_.x_(0) = measurement_pack.raw_measurements_[0];
      ekf_.x_(1) = measurement_pack.raw_measurements_[1];
    }

    // done initializing, no need to predict or update
    is_initialized_ = true;
    return;
  }

  /**
   * Prediction
   */

  /**
   * TODO: Update the state transition matrix F according to the new elapsed time.
   * Time is measured in seconds.
   * TODO: Update the process noise covariance matrix.
   * Use noise_ax = 9 and noise_ay = 9 for your Q matrix.
   */
  float dt = (measurement_pack.timestamp_ - previous_timestamp_) / 1000000.0;
  if (dt > 1) {
    dt = 0.05;  // justify the first timestep;
  }
  previous_timestamp_ = measurement_pack.timestamp_;

  float dt_2 = dt * dt;
  float dt_3 = dt_2 * dt;
  float dt_4 = dt_3 * dt;

  // the initial transition matrix F_
  ekf_.F_ = MatrixXd(4, 4);
  ekf_.F_ << 1, 0, dt, 0,
          0, 1, 0, dt,
          0, 0, 1, 0,
          0, 0, 0, 1;

  // set the process covariance matrix Q
  float noise_ax = 9;
  float noise_ay = 9;
  ekf_.Q_ = MatrixXd(4, 4);
  ekf_.Q_ <<  dt_4/4*noise_ax, 0, dt_3/2*noise_ax, 0,
          0, dt_4/4*noise_ay, 0, dt_3/2*noise_ay,
          dt_3/2*noise_ax, 0, dt_2*noise_ax, 0,
          0, dt_3/2*noise_ay, 0, dt_2*noise_ay;

  ekf_.Predict();
  std::cout << "aftpred" << dt << ekf_.x_ << "\n" << ekf_.H_<< "\n" << ekf_.Q_ << std::endl;
  /**
   * Update
   */

  /**
   * TODO:
   * - Use the sensor type to perform the update step.
   * - Update the state and covariance matrices.
   */

  if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
    // TODO: Radar updates
    ekf_.H_ = tools.CalculateJacobian(ekf_.x_);
    ekf_.R_ = R_radar_;
    ekf_.UpdateEKF(measurement_pack.raw_measurements_);
  } else {
    // TODO: Laser updates
    ekf_.H_ = H_laser_;
    ekf_.R_ = R_laser_;
    ekf_.Update(measurement_pack.raw_measurements_);
  }

  // print the output
  cout << "x_ = " << ekf_.x_ << endl;
  cout << "P_ = " << ekf_.P_ << endl;
}
