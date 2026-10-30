/* Copyright 2013-2019 Homegear GmbH
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#ifndef HEATINGCONTROLLER_H_
#define HEATINGCONTROLLER_H_

#include <homegear-node/INode.h>

#define SAMPLE_INTERVAL 60.0

namespace MyNode {

class HeatingController {
 public:
  struct ControllerSettings {
    double Kp = 50.0; // % valve state per °C
    double TN = 7200.0; // integration time in seconds
    double Td = 900.0; // derivative time in seconds
    double RC_w_LP = 1200.0; // LP time constant for w
    double y_LP_RC = 180.0; // LP time constant for y
    double uvalve_LP_RC = 180.0; // Lp time constant for uvalve

    double getKptn() const { return Kp * (SAMPLE_INTERVAL / TN); } // I gain
    double getKptd() const { return Kp * (Td / SAMPLE_INTERVAL); } // D gain
    double getUValveLpWeight() const { return std::exp(-SAMPLE_INTERVAL / uvalve_LP_RC); }
    double getWLpWeight() const { return std::exp(-SAMPLE_INTERVAL / RC_w_LP); }
    double getYLpWeight() const { return std::exp(-SAMPLE_INTERVAL / y_LP_RC); }
  };

  void setControllerSettings(ControllerSettings &value) { _controllerSettings = value; }
  double getCurrentTemperature() { return _currentTemperature; }
  void setCurrentTemperature(double value) { _currentTemperature = value; }
  double getSetPointTemperature() { return _setPointTemperature; }
  void setSetPointTemperature(double value) { _setPointTemperature = value; }

  explicit HeatingController(const std::shared_ptr<Flows::Output> &output);
  ~HeatingController();

  void setSetUValve(std::function<void(int32_t position)> value) { _setUValve.swap(value); }

  void tick();
  Flows::PVariable serialize();
  void unserialize(const Flows::PVariable& value);
 private:
  std::shared_ptr<Flows::Output> _out;

  // inverse conservative valve characteristic
  // input and output in the range of 0..100%
  // formula for generating this characteristic/LUT:
  // antivalveLUT(i) = -RC * LN(1 - (i / U_0))
  // with RC = 30, U_0 = 100/(1-EXP(-100/RC))
  const std::array<int32_t, 101> _antivalveLUT{{0,
                                                0, 1, 1, 1, 1, 2, 2, 2, 3, 3,
                                                3, 4, 4, 4, 5, 5, 5, 6, 6, 6,
                                                7, 7, 8, 8, 8, 9, 9, 9, 10, 10,
                                                11, 11, 11, 12, 12, 13, 13, 14, 14, 15,
                                                15, 16, 16, 17, 17, 18, 18, 19, 19, 20,
                                                20, 21, 21, 22, 23, 23, 24, 25, 25, 26,
                                                27, 27, 28, 29, 30, 30, 31, 32, 33, 34,
                                                35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
                                                46, 47, 48, 50, 51, 53, 55, 57, 59, 61,
                                                63, 65, 68, 71, 74, 78, 82, 87, 93, 100}};

  // conservative valve characteristic
  // input and output in the range of 0..100%
  // formula for generating this characteristic/LUT:
  // valveLUT(i) = U_0 * (1 - EXP(-i / RC))
  // with RC = 30, U_0 = 100/(1-EXP(-100/RC))
  const std::array<double, 101> _valveLUT{{0,
                                           3.4, 6.69, 9.87, 12.94, 15.92, 18.8, 21.58, 24.27, 26.88, 29.4,
                                           31.83, 34.19, 36.47, 38.67, 40.8, 42.86, 44.86, 46.79, 48.65, 50.46,
                                           52.2, 53.89, 55.52, 57.1, 58.63, 60.11, 61.54, 62.92, 64.26, 65.55,
                                           66.8, 68.01, 69.18, 70.31, 71.41, 72.47, 73.49, 74.48, 75.44, 76.36,
                                           77.26, 78.13, 78.97, 79.78, 80.56, 81.32, 82.05, 82.76, 83.45, 84.11,
                                           84.76, 85.38, 85.98, 86.56, 87.12, 87.66, 88.19, 88.7, 89.19, 89.67,
                                           90.13, 90.57, 91, 91.42, 91.82, 92.21, 92.59, 92.95, 93.3, 93.64,
                                           93.97, 94.29, 94.6, 94.9, 95.19, 95.47, 95.74, 96, 96.25, 96.49,
                                           96.73, 96.96, 97.18, 97.39, 97.6, 97.8, 97.99, 98.18, 98.36, 98.54,
                                           98.71, 98.87, 99.03, 99.18, 99.33, 99.47, 99.61, 99.74, 99.87, 100}};

  std::function<void(int32_t position)> _setUValve;

  //{{{ Publicly accessible
  std::atomic<double> _currentTemperature{21.0};
  std::atomic<double> _setPointTemperature{21.0};
  //}}}

  // general settings
  double _theta0 = 20.0;

  // PI controller parameters
  ControllerSettings _controllerSettings;

  // controller variables
  // w is relative to 20°C, not absolute (linearization around operating point)
  // wDeg is absolute in °C
  double _w = 0.0; // setpoint temperature (in °C) w(k)
  //private $wold; // old setpoint temerature w(k-1)
  double _w_LP = 0.0; // IIR LP filtered w(k)
  double _w_LP2 = 0.0; // semi-LP filtered w(k)
  double _w_LPold = 0.0; // LP filtered w(k-1)
  double _uLin = 0.0; // (theoretical) linear manipulated value u(k) (desired hot water flow in percent); in the range of -50 to +50
  double _uLinOld = 0.0; // (theoretical) linear manipulated value u(k-1); in the range of -50 to +50
  int32_t _hgUValve = 50; // in the range of 0 to +100
  int32_t _uvalve = 50; // (physical) valve setting u_S(k); in the range of 0 to +100
  double _uvalve_LP = 50; // LP filtered (physical) valve setting u_S(k); in the range of 0 to +100
  double _uvalve_LPold = 50; // old LP filtered (physical) valve setting u_S(k-1); in the range of 0 to +100
  // usage: uvalve = antivalve(uLin);
  double _uhat = 0.0; // reconstructed actual hot water flow û(k); in the range of -50 to +50
  // usage: uhat = valve(uvalve);
  double _uhatold = 0.0; // û(k-1); in the range of -50 to +50
  double _e = 0.0; // control deviation e(k)
  double _eOld = 0.0; // e(k-1)
  double _eOold = 0.0; // e(k-2)
  bool _firststart = true;
  double _y = 0.0; // y(k)
  //private $yOld = 0.0; // y(k-1)
  //private $yOold = 0.0; // y(k-2)
  double _y_LP = 0.0; // IIR LP filtered y(k)
  double _y_LPold = 0.0; // LP filtered y(k-1)

  int32_t antivalve(double uLin);
  double valve(int32_t uvalve);
  static double piController(double e, double eOld, double eOold, double uLinOld, double kp, double kptn, double kptd);
};

}

#endif
