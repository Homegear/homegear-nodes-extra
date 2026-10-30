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

#include "HeatingCoolingController.h"

namespace MyNode {

HeatingCoolingController::HeatingCoolingController(const std::shared_ptr<Flows::Output> &output) {
  _out = output;

  _w = 21.0 - _theta0;
  _w_LP = _w; // IIR LP filtered w(k)
  _w_LP2 = _w; // semi-LP filtered w(k)
  _w_LPold = _w; // LP filtered w(k-1)

  _firststart = true;
}

HeatingCoolingController::~HeatingCoolingController() = default;

Flows::PVariable HeatingCoolingController::serialize() {
  try {
    Flows::PVariable object = std::make_shared<Flows::Variable>(Flows::VariableType::tStruct);
    object->structValue->emplace("w", std::make_shared<Flows::Variable>(_w));
    object->structValue->emplace("w_LP", std::make_shared<Flows::Variable>(_w_LP));
    object->structValue->emplace("w_LP2", std::make_shared<Flows::Variable>(_w_LP2));
    object->structValue->emplace("w_LPold", std::make_shared<Flows::Variable>(_w_LPold));
    object->structValue->emplace("uLin", std::make_shared<Flows::Variable>(_uLin));
    object->structValue->emplace("uLinOld", std::make_shared<Flows::Variable>(_uLinOld));
    object->structValue->emplace("hgUValve", std::make_shared<Flows::Variable>(_hgUValve));
    object->structValue->emplace("uvalve", std::make_shared<Flows::Variable>(_uvalve));
    object->structValue->emplace("uvalve_LP", std::make_shared<Flows::Variable>(_uvalve_LP));
    object->structValue->emplace("uvalve_LPold", std::make_shared<Flows::Variable>(_uvalve_LPold));
    object->structValue->emplace("uhat", std::make_shared<Flows::Variable>(_uhat));
    object->structValue->emplace("uhatold", std::make_shared<Flows::Variable>(_uhatold));
    object->structValue->emplace("e", std::make_shared<Flows::Variable>(_e));
    object->structValue->emplace("eOld", std::make_shared<Flows::Variable>(_eOld));
    object->structValue->emplace("eOold", std::make_shared<Flows::Variable>(_eOold));
    object->structValue->emplace("y", std::make_shared<Flows::Variable>(_y));
    object->structValue->emplace("y_LP", std::make_shared<Flows::Variable>(_y_LP));
    object->structValue->emplace("y_LPold", std::make_shared<Flows::Variable>(_y_LPold));
    return object;
  }
  catch (const std::exception &ex) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return std::make_shared<Flows::Variable>();
}

void HeatingCoolingController::unserialize(const Flows::PVariable &value) {
  try {
    if (!value->structValue->empty()) _firststart = false;

    auto valueIterator = value->structValue->find("w");
    if (valueIterator != value->structValue->end()) _w = valueIterator->second->floatValue;
    valueIterator = value->structValue->find("w_LP");
    if (valueIterator != value->structValue->end()) _w_LP = valueIterator->second->floatValue;
    valueIterator = value->structValue->find("w_LP2");
    if (valueIterator != value->structValue->end()) _w_LP2 = valueIterator->second->floatValue;
    valueIterator = value->structValue->find("w_LPold");
    if (valueIterator != value->structValue->end()) _w_LPold = valueIterator->second->floatValue;
    valueIterator = value->structValue->find("uLin");
    if (valueIterator != value->structValue->end()) _uLin = valueIterator->second->floatValue;
    valueIterator = value->structValue->find("uLinOld");
    if (valueIterator != value->structValue->end()) _uLinOld = valueIterator->second->floatValue;
    valueIterator = value->structValue->find("hgUValve");
    if (valueIterator != value->structValue->end()) _hgUValve = valueIterator->second->integerValue;
    valueIterator = value->structValue->find("uvalve");
    if (valueIterator != value->structValue->end()) _uvalve = valueIterator->second->integerValue;
    valueIterator = value->structValue->find("uvalve_LP");
    if (valueIterator != value->structValue->end()) _uvalve_LP = valueIterator->second->floatValue;
    valueIterator = value->structValue->find("uvalve_LPold");
    if (valueIterator != value->structValue->end()) _uvalve_LPold = valueIterator->second->floatValue;
    valueIterator = value->structValue->find("uhat");
    if (valueIterator != value->structValue->end()) _uhat = valueIterator->second->floatValue;
    valueIterator = value->structValue->find("uhatold");
    if (valueIterator != value->structValue->end()) _uhatold = valueIterator->second->floatValue;
    valueIterator = value->structValue->find("e");
    if (valueIterator != value->structValue->end()) _e = valueIterator->second->floatValue;
    valueIterator = value->structValue->find("eOld");
    if (valueIterator != value->structValue->end()) _eOld = valueIterator->second->floatValue;
    valueIterator = value->structValue->find("eOold");
    if (valueIterator != value->structValue->end()) _eOold = valueIterator->second->floatValue;
    valueIterator = value->structValue->find("y");
    if (valueIterator != value->structValue->end()) _y = valueIterator->second->floatValue;
    valueIterator = value->structValue->find("y_LP");
    if (valueIterator != value->structValue->end()) _y_LP = valueIterator->second->floatValue;
    valueIterator = value->structValue->find("y_LPold");
    if (valueIterator != value->structValue->end()) _y_LPold = valueIterator->second->floatValue;
  }
  catch (const std::exception &ex) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

//{{{ supplemental functions
int32_t HeatingCoolingController::antivalve(double uLin) {
  try {
    int32_t intu = std::lround(uLin);

    if (intu > 50) intu = 50;
    else if (intu < -50) intu = -50;
    return _antivalveLUT.at(intu + 50);
  }
  catch (const std::exception &ex) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return 0;
}

double HeatingCoolingController::valve(int32_t uvalve) {
  try {
    int32_t uvalvelimited = uvalve;

    if (uvalvelimited > 100) uvalvelimited = 100;
    else if (uvalvelimited < 0) uvalvelimited = 0;
    return _valveLUT.at(uvalvelimited) - 50;
  }
  catch (const std::exception &ex) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return 0;
}
//}}}

// PI controller tick formula
double HeatingCoolingController::piController(double e, double eOld, double eOold, double uLinOld, double kp, double kptn, double kptd) {
  // P part.
  double uLin = uLinOld + kp * e - kp * eOld;

  // I part. valve not in saturation => integrator enabled
  if (((uLinOld >= -50) && (uLinOld <= 50)) || ((uLinOld <= -50) && (e >= 0)) || ((uLinOld >= 50) && (e <= 0))) {
    uLin = uLin + kptn * e;
  }

  // D part. only for rising or slowly dropping temperature = dropping or slowly rising e(k)
  if ((e - eOld) <= (0.1 * 60)) // _Ts
  {
    uLin = uLin + kptd * (e - 2 * eOld + eOold);
    /*if(($kptd * ($e - 2 * $eOld + $eOold)) < (-20)) // Beschränkung des Ausschlags durch den D-Anteil
       $uLin = $uLin - 20;
   else
       $uLin = $uLin + $kptd * ($e - 2 * $eOld + $eOold);*/
  }

  return uLin;
}

void HeatingCoolingController::tick() {
  try {
    // setpoint value update
    //_wold = _w;
    _w = _setPointTemperature - _theta0;
    // low pass filtering of w in order to stabilize control loop
    _w_LPold = _w_LP;
    _w_LP = _controllerSettings.getWLpWeight() * _w_LPold + (1 - _controllerSettings.getWLpWeight()) * _w;
    _w_LP2 = 0.5 * _w + 0.5 * _w_LP;

    // control deviation
    //_yhatold = _yhat;
    //_yOld = _y;
    _y_LPold = _y_LP;
    _y_LP = _controllerSettings.getYLpWeight() * _y_LPold + (1 - _controllerSettings.getYLpWeight()) * _y;
    _eOold = _eOld;
    _eOld = _e;
    _e = _w_LPold - _y_LP;
    if (_firststart == 1) {
      _eOld = _e;
      _eOold = _e;
      _firststart = false;
    }

    // controller tick and (less aggressive) valve update
    _uLinOld = _uLin;
    _uLin = piController(_e, _eOld, _eOold, _uLinOld, _controllerSettings.Kp, _controllerSettings.getKptn(), _controllerSettings.getKptd());
    _uvalve = antivalve(_uLin);
    _uvalve_LPold = _uvalve_LP;
    _uvalve_LP = _controllerSettings.getUValveLpWeight() * _uvalve_LPold + (1 - _controllerSettings.getUValveLpWeight()) * _uvalve;
    _setUValve((int32_t)_uvalve_LP);
    _uhat = valve(_hgUValve);

    _y = _currentTemperature - _theta0;
  }
  catch (const std::exception &ex) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

}
