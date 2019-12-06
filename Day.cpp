#include <Arduino.h>
#include "Day.h"

/******* Constructors *******/
Day::Day(){}
Day::Day(bool dayActive, bool mistActive, bool sprayActive, uint8_t startTime_H, 
		uint8_t startTime_M, uint8_t mistDuration, uint8_t sprayDuration){
	_dayActive = dayActive;
	_mistActive = mistActive;
	_sprayActive = sprayActive;
	_startTime_H = startTime_H;
  	_startTime_M = startTime_M;
	_mistDuration = mistDuration;
	_sprayDuration = sprayDuration;
}

/********** Setters *********/
void Day::setDayActive(bool dayActive){
	_dayActive = dayActive;
}

void Day::setMistActive(bool mistActive){
	_mistActive = mistActive;
}

void Day::setSprayActive(bool sprayActive){
	_sprayActive = sprayActive;
}

void Day::setStartTime_H(uint8_t startTime_H){
	_startTime_H = startTime_H;
}

void Day::setStartTime_M(uint8_t startTime_M){
	_startTime_M = startTime_M;
}

void Day::setMistDuration(uint8_t mistDuration){
	_mistDuration = mistDuration;
}

void Day::setSprayDuration(uint8_t sprayDuration){
	_sprayDuration = sprayDuration;
}

void Day::setOffSet_Temp(uint8_t t_offSet){
	_offSet_temp = t_offSet;
}

void Day::setOffSet_humid(uint8_t h_offSet){
	_offSet_humid = h_offSet;
}

void Day::setAll(bool dayActive, bool mistActive, bool sprayActive, uint8_t startTime_H, 
		uint8_t startTime_M, uint8_t mistDuration, uint8_t sprayDuration){
	_dayActive = dayActive;
	_mistActive = mistActive;
	_sprayActive = sprayActive;
	_startTime_H = startTime_H;
	_startTime_M = startTime_M;
	_mistDuration = mistDuration;
	_sprayDuration = sprayDuration;
	_offSet_temp = 0;
	_offSet_humid = 0;
}

/********** Getters *********/
bool Day::getDayActive(){
	return _dayActive;
}

bool Day::getMistActive(){
	return _mistActive;
}

bool Day::getSprayActive(){
	return _sprayActive;
}

uint8_t Day::getStartTime_H(){
	return _startTime_H;
}

uint8_t Day::getStartTime_M(){
	return _startTime_M;
}

uint8_t Day::getMistDuration(){
	return _mistDuration;
}

uint8_t Day::getSprayDuration(){
	return _sprayDuration;
}

uint8_t Day::getOffSet_Temp(){
	return _offSet_temp;
}
uint8_t Day::getOffSet_humid(){
	return _offSet_humid;
}
