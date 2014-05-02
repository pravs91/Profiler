/*
 * PerformanceTimer.cpp
 *
 *  Created on: Mar 19, 2014
 *      Author: pravs
 */

#include"PerformanceTimer.h"
#include <map>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
using namespace std;

typedef struct timer_T{
	double flop, memOps, time_in_sec,time_in_ms;
	double bandwidth,flopRate;
	bool calcFlops,calcBW;
	bool startFlag,stopFlag;
	int numRuns;
	string name;
	PerformanceTimer<double> timer;
	double averageTime;
	double averageFR,averageBW;
	double timeImprovement,FRImprovement,BWImprovement;
	struct timer_T* next;
	struct timer_T* prev;
	struct timer_T* nestedHead;
	struct timer_T* nestedTail;
	struct timer_T* nestedNext;


	double inclusiveTime;
	timer_T(){
		flop = memOps = time_in_sec = time_in_ms = 0;
		bandwidth = flopRate = 0;
		averageTime = averageFR = averageBW = 0;
		numRuns = 0;
		timeImprovement = FRImprovement = BWImprovement = 0;
		calcFlops = calcBW = false;
		next = prev = NULL;
		inclusiveTime = 0;
		startFlag = stopFlag = false;
		nestedHead = nestedTail = nestedNext = NULL;
	}

	void start(){
		numRuns++;
		startFlag = true;
		timer.start();
	}

	void stop(){
		timer.stop();
		stopFlag = true;
		time_in_sec += timer();
		time_in_ms = time_in_sec * 1000.0;
	}

	void reset(){
		flop = memOps = 0;
		bandwidth = flopRate = 0;
		numRuns = 0;
		time_in_sec = time_in_ms = 0;
	}

	void computeFlops(){
		if(calcFlops)
			flopRate = flop * numRuns / (time_in_sec * 1.0e9);
	}

	void computeBW(){
		if(calcBW)
			bandwidth = memOps * numRuns / (time_in_sec * 1024.0 * 1024.0 * 1024.0);
	}

}timer_T;

typedef struct compare{
	string name;
	double time;
	double flopRate;	
	double bandwidth;

	compare(){
		time = flopRate = bandwidth = 0;
	}
}compare;

//template<typename T>
class Profiler
{
public:
	int numberOfTimers;// fd;
	double totalTime;
	std::map<string,timer_T> timers;
	std::map<string,compare> reference;
	std::map<string,int> fileMap;
	timer_T* root;
	bool debug;
	bool refFlag;
	int dashCount;
	int tabCount;
	
	Profiler(){//constructor to initialize the class members
		root = new timer_T();
		numberOfTimers = 0;
		debug = true;
		totalTime = 0;
		refFlag = false;
		dashCount = -1;
		tabCount = 0;
	}

	Profiler(string chosen){//constructor to choose the timer to use
		root = new timer_T();
		numberOfTimers = 0;
		debug = true;
		totalTime = 0;
		refFlag = false;
		dashCount = -1;
		tabCount = 0;
		if(chosen.compare("OPENMP")==0){
			choice = OPENMP_TIMER;
		}
		else if(chosen.compare("CLOCK")==0){
			choice = CLOCK_TIMER;
		}

	}

public:

	void start(string timerName,string parent){//attach to a particular parent, to support interleaved timers

		std::map<string,timer_T>::iterator timerIter;
		timerIter = timers.find(timerName);
		if(timerIter != timers.end()){
			timers[timerName].start();
			timers[timerName].stopFlag = false;
		}
		else{
			timer_T* temp = new timer_T();
			temp -> name = timerName;
			timers[timerName] = *temp;
			if(parent.compare("")==0){//if parent is empty string, add timer to root (1st level)
				if(root->nestedHead == NULL){//first timer added to root
					root->nestedHead = &timers[timerName];
					root->nestedTail = &timers[timerName];
					timers[timerName].prev = root;
				}
				else{//add timers to tail of root
					root->nestedTail->nestedNext = &timers[timerName];
					root->nestedTail = &timers[timerName];
					timers[timerName].prev = root;
				}
				numberOfTimers++;
				timers[timerName].start();
				return;
			}
			timers[parent].next = &timers[timerName];
			timers[timerName].prev = &timers[parent];
			if(timers[parent].nestedHead == NULL){
				timers[parent].nestedHead = &timers[timerName];
				timers[parent].nestedTail = &timers[timerName];
			}
			else{
				timers[parent].nestedTail->nestedNext = &timers[timerName];
				timers[parent].nestedTail = &timers[timerName];
			}
			numberOfTimers++;
			timers[timerName].start();		
		}
		
	}
	void start(string timerName){//function to start the user defined timer
		std::map<string,timer_T>::iterator timerIter;
		timerIter = timers.find(timerName);
		if(timerIter != timers.end()){
			timers[timerName].start();
			timers[timerName].stopFlag = false;
			//*****tentative solution, should check if it breaks anything else******//
			timer_T* current = &timers[timerName];//current = timer to start
			if(current != NULL && current -> prev != NULL)
				current -> prev -> next = &timers[timerName];
			//*****end of tentative solution******
		}
		else{
			struct timer_T* traverse = root->nestedHead;
		
			while(traverse != NULL){
				timer_T* current = traverse;
				if(!current->stopFlag){//check if timer has not been stopped
					while(current -> next != NULL){//go until currently active timer
						current = current -> next;
					}
					timer_T* temp = new timer_T();
					temp -> name = timerName;
					timers[timerName] = *temp;//insert timer into hash
					timers[current->name].next = &timers[timerName];//doubly link previous timer and new timer
					timers[timerName].prev = current;
					if(timers[current->name].nestedHead == NULL){//update nested list of parent timer
						timers[current->name].nestedHead = &timers[timerName];
						timers[current->name].nestedTail = &timers[timerName];
					}
					else{//insert at tail of nested list
						timers[current->name].nestedTail->nestedNext = &timers[timerName];
						timers[current->name].nestedTail = &timers[timerName];
					}
					numberOfTimers++;
					timers[timerName].start();
					break;
				}
				traverse = traverse->nestedNext;
			
			}
			if(traverse == NULL){//add a new timer to root
				timer_T* temp = new timer_T();
				temp -> name = timerName;
				numberOfTimers++;
				timers[timerName] = *temp;
				if(root->nestedHead == NULL){//first timer added to root
					root->nestedHead = &timers[timerName];
					root->nestedTail = &timers[timerName];
					timers[timerName].prev = root;
				}
				else{//add timers to tail of root
					root->nestedTail->nestedNext = &timers[timerName];
					root->nestedTail = &timers[timerName];
					timers[timerName].prev = root;
				}
				timers[timerName].start();
			}
		}
	}
	void stop(string timerName){//stop the user-defined timer
		std::map<string,timer_T>::iterator timerIter;
		timerIter = timers.find(timerName);
		if(timerIter != timers.end()){//timer already exists
			if(timers[timerName].stopFlag){
				if(debug)
					cout << "Timer " << timers[timerName].name << " already stopped" << endl;
				return;
			}				
			timers[timerName].stop();
			timer_T* current = &timers[timerName];//current = timer to stop
		/*	timer_T* nested = timers[timerName].next;
			while(nested != NULL && !nested->stopFlag){//stop all nested timers
				nested->stop();
				nested = nested->next;
			}*/
			if(current != NULL && current -> prev != NULL)//update next pointer of parent timer to NULL
				current->prev->next = NULL;
		}
		else{
			if(debug)
				std::cout << "Undefined timer '" << timerName << "'" << endl;
		}
	}

	void reset(){//reset all timer data
		std::map<string,timer_T>::iterator timerIter;
		tabCount = 0;
		dashCount = -1;
		for(timerIter = timers.begin();timerIter != timers.end();timerIter++){
			timerIter->second.reset();
		}
	}
	
	double getTime(string timerName,string unit){//get the time taken in seconds or ms
		std::map<string,timer_T>::iterator timerIter;
		timerIter = timers.find(timerName);
		if(timerIter != timers.end()){//timer already exists
			if(unit.compare("sec")==0)
				return timers[timerName].time_in_sec;
			else if(unit.compare("ms")==0)
				return timers[timerName].time_in_ms;
			else
				if(debug)
					std::cout << "Specify a valid unit 'sec' or 'ms'" << endl;
				return -1;
		}
		else{
			if(debug)
				std::cout << "Undefined timer '" << timerName << "'" << endl;
			return -1;
		}
	}

	void setFlop(string timerName,double f){//set flop count
		std::map<string,timer_T>::iterator timerIter;
		timerIter = timers.find(timerName);
		if(timerIter != timers.end()){//timer already exists
			timers[timerName].flop = f;
			timers[timerName].calcFlops = true;
			if(timers[timerName].time_in_sec != 0)
				timers[timerName].computeFlops();
		}
		else{
			if(debug)
				std::cout << "Undefined timer '" << timerName << "'" << endl;
			return;
		}
	}

	void setMemory(string timerName,double m){//set memory transfer count for BW calculation
		std::map<string,timer_T>::iterator timerIter;
		timerIter = timers.find(timerName);
		if(timerIter != timers.end()){//timer already exists
			timers[timerName].memOps = m;
			timers[timerName].calcBW = true;
			if(timers[timerName].time_in_sec != 0)
				timers[timerName].computeBW();
		}
		else{
			if(debug)
				std::cout << "Undefined timer '" << timerName << "'" << endl;
			return;
		}		
	}	
	void dottedLine(int count,int fd){
		int i;
		for(i=0;i<count;i++)
			write(fd,"----",4);
	}
	
	
	void printReport(){
		DFSprint(root);
	}

	void printReport(string timerName){
		std::map<string,timer_T>::iterator timerIter;
		timerIter = timers.find(timerName);
		if(timerIter != timers.end())
			DFSprint(&timerIter->second);
		else{
			if(debug)
				cout << "Undefined timer '" << timerName << "'" << endl;
		}
	}
	void printConciseStdOut(string timerName){//prints concise report to stdout
		int std = STDOUT_FILENO;
		char buffer[1024];
		memset(buffer,0,1024);
		if(refFlag)
			sprintf(buffer,"%-20s\t%12s\t%16s\t%12s\t%12s\t%12s\t%12s\t%12s\t%12s\t%12s\n","Timer Name","Time(s)","Time(ms)","t_Nested(s)","Nested %","Flop_Rate","Bandwidth","t(change)","FR(change)","BW(change)");
		else
			sprintf(buffer,"%-20s\t%12s\t%16s\t%12s\t%12s\t%12s\t%12s\t\n","Timer Name","Time(s)","Time(ms)","t_Nested(s)","Nested %","Flop_Rate","Bandwidth");
	
		write(std,buffer,strlen(buffer));
		if(timerName.compare("")==0){
			DFSconcise(root,std);
			return;
		}
		std::map<string,timer_T>::iterator timerIter;
		timerIter = timers.find(timerName);
		
		if(timerIter != timers.end()){
			DFSconcise(&timerIter->second,std);
			write(std,"\n",1);
		}
		else{
			if(debug)
				cout << "Undefined timer '" << timerName << "'" << endl;
		}
	}

	void printConcise(string timerName,string file){//print concise report
		const char *fileName = file.c_str();
		int fd;
		std::map<string,int>::iterator fileIter;
		fileIter = fileMap.find(file);

		if(file.compare("STDOUT")==0){
			printConciseStdOut(timerName);
			return;
		}
		else{
			if(fileIter == fileMap.end()){//file does not exist in file hash, so open / create & truncate
				fileMap[file] = 1;
				fd = open(fileName,O_CREAT | O_RDWR | O_TRUNC , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
			}
			else //open file in append mode
				fd = open(fileName,O_RDWR | O_APPEND); 
		}
		if(fd<0){
			if(debug)
				cout << "Cannot open file '" << file << "'" << endl;
			return;
		}
		char buffer[1024];
		memset(buffer,0,1024);
		if(refFlag)
			sprintf(buffer,"%-20s\t%12s\t%16s\t%12s\t%12s\t%12s\t%12s\t%12s\t%12s\t%12s\n","Timer Name","Time(s)","Time(ms)","t_Nested(s)","Nested %","Flop_Rate","Bandwidth","t(change)","FR(change)","BW(change)");
		else
			sprintf(buffer,"%-20s\t%12s\t%16s\t%12s\t%12s\t%12s\t%12s\t\n","Timer Name","Time(s)","Time(ms)","t_Nested(s)","Nested %","Flop_Rate","Bandwidth");
				
		write(fd,buffer,strlen(buffer));
		if(timerName.compare("")==0){
			DFSconcise(root,fd);
			return;
		}
		std::map<string,timer_T>::iterator timerIter;
		timerIter = timers.find(timerName);
		
		if(timerIter != timers.end()){
			DFSconcise(&timerIter->second,fd);
			write(fd,"\n",1);
		}
		else{
			if(debug)
				cout << "Undefined timer '" << timerName << "'" << endl;
		}
	}

	void printNestedStdout(string timerName){//function to print nested timers to stdout
		int std = STDOUT_FILENO;
		char buffer[1024];
		memset(buffer,0,1024);
	
		if(timerName.compare("")==0){
			DFSnestedPrint(root,std);
			return;
		}
		std::map<string,timer_T>::iterator timerIter;
		timerIter = timers.find(timerName);
		
		if(timerIter != timers.end()){
			DFSnestedPrint(&timerIter->second,std);
			write(std,"\n",1);
		}
		else{
			if(debug)
				cout << "Undefined timer '" << timerName << "'" << endl;
		}
	}
	void printNestedTimers(string timerName,string file){//function to print nested timers to file.
		const char *fileName = file.c_str();
		int fd;
		std::map<string,int>::iterator fileIter;
		fileIter = fileMap.find(file);

		if(file.compare("STDOUT")==0){
			printNestedStdout(timerName);
			return;
		}
		else{
			if(fileIter == fileMap.end()){//file does not exist in file hash, so open / create & truncate
				fileMap[file] = 1;
				fd = open(fileName,O_CREAT | O_RDWR | O_TRUNC , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
			}
			else //open file in append mode
				fd = open(fileName,O_RDWR | O_APPEND); 
		}
		if(fd<0){
			if(debug)
				cout << "Cannot open file '" << file <<"'" <<endl;
			return;
		}
		char buffer[1024];
		memset(buffer,0,1024);
		if(timerName.compare("")==0){
			DFSnestedPrint(root,fd);
			return;
		}
		std::map<string,timer_T>::iterator timerIter;
		timerIter = timers.find(timerName);
		
		if(timerIter != timers.end()){
			DFSnestedPrint(&timerIter->second,fd);
			write(fd,"\n",1);
		//	close(fd);
		}
		else{
			if(debug)
				cout << "Undefined timer '" << timerName << "'" << endl;
		}		
	}	

	void DFSnestedPrint(timer_T* timerObj,int filedes){//DFS for printing nested timers
		char buffer[1024];
		memset(buffer,0,1024);
		if(timerObj -> nestedHead == NULL){
			dashCount++;
			dottedLine(dashCount,filedes);
			sprintf(buffer,"%s",timerObj -> name.c_str());	
			write(filedes,buffer,strlen(buffer));
			write(filedes,"\n",1);
			if(dashCount != 0)
				dashCount--;

			return;
		}
		dashCount++;
		dottedLine(dashCount,filedes);
		sprintf(buffer,"%s",timerObj -> name.c_str());	
		write(filedes,buffer,strlen(buffer));				
		write(filedes,"\n",1);
		struct timer_T* temp = timerObj -> nestedHead;
		while(temp != NULL){
			DFSnestedPrint(temp,filedes);
			temp = temp -> nestedNext;	
		}
		if(dashCount != 0)
			dashCount--;
		
	}

	void conciseHelper(timer_T* timerObj,int filedes){//helper function to print for DFSconcise
		char buffer[1024];
		memset(buffer,0,1024);
		sprintf(buffer,"%-20s\t%12f\t%16f\t",timerObj -> name.c_str(), timerObj -> time_in_sec, timerObj -> time_in_ms);
		write(filedes,buffer,strlen(buffer));
		if(timerObj -> inclusiveTime != 0){
			sprintf(buffer,"%12f\t%12f %\t",timerObj->inclusiveTime, (timerObj -> inclusiveTime / timerObj -> time_in_sec) * 100);
			write(filedes,buffer,strlen(buffer));
			timerObj -> inclusiveTime = 0;
		}
		else{
			sprintf(buffer,"%12s\t%12s\t","N/A","N/A");
			write(filedes,buffer,strlen(buffer));	
		}
		if(timerObj -> calcFlops){
			sprintf(buffer,"%12f\t",timerObj -> flopRate);
			write(filedes,buffer,strlen(buffer));
		}
		else{
			sprintf(buffer,"%12s\t","N/A");
			write(filedes,buffer,strlen(buffer));	
		}
		if(timerObj -> calcBW){
			sprintf(buffer,"%12f\t",timerObj -> bandwidth);
			write(filedes,buffer,strlen(buffer));
		}
		else{
			sprintf(buffer,"%12s\t","N/A");
			write(filedes,buffer,strlen(buffer));	
		}
		if(refFlag){
			if(timerObj -> timeImprovement != 0){
				sprintf(buffer,"%12f\t",timerObj->timeImprovement);
				write(filedes,buffer,strlen(buffer));	
			}
			else{
				sprintf(buffer,"%12s\t","N/A");
				write(filedes,buffer,strlen(buffer));	
			}
			if(timerObj -> FRImprovement != 0){
				sprintf(buffer,"%12f\t",timerObj->FRImprovement);
				write(filedes,buffer,strlen(buffer));	
			}
			else{
				sprintf(buffer,"%12s\t","N/A");
				write(filedes,buffer,strlen(buffer));	
			}
			if(timerObj -> BWImprovement != 0){
				sprintf(buffer,"%12f\t",timerObj->BWImprovement);
				write(filedes,buffer,strlen(buffer));	
			}
			else{
				sprintf(buffer,"%12s\t","N/A");
				write(filedes,buffer,strlen(buffer));	
			}
		}
		
	}	
	void DFSconcise(timer_T* timerObj,int filedes){
		char buffer[1024];
		memset(buffer,0,1024);
		if(timerObj -> nestedHead == NULL){
			conciseHelper(timerObj,filedes);
			write(filedes,"\n",1);	
			return;
		}
		struct timer_T* temp = timerObj->nestedHead;
		while(temp!=NULL){
			DFSconcise(temp,filedes);
			timerObj -> inclusiveTime += temp -> time_in_sec;
			temp = temp -> nestedNext;
		}
		
		if(timerObj == root){
			sprintf(buffer,"%f\n",root->inclusiveTime);
			write(filedes,buffer,strlen(buffer));
			root->inclusiveTime = 0;
			write(filedes,"\n",1);
			return;
		}
		conciseHelper(timerObj,filedes);
		write(filedes,"\n",1);
			
	}

	void printHelper(timer_T* timerObj,int tabCount){//helper function to print for printReport
		dottedLine(tabCount,1);
		cout << "***************" << endl;
		dottedLine(tabCount,1);
		cout << "Timer Name: " << timerObj->name << endl;
		dottedLine(tabCount,1);
		cout << "Time taken: " << timerObj->time_in_sec << "s = " << timerObj->time_in_ms << "ms" << endl;
		if(timerObj -> inclusiveTime != 0){
			dottedLine(tabCount,1);
			cout << "Nested timers' time: " << timerObj->inclusiveTime << endl;
			dottedLine(tabCount,1);
			cout << "Nested timers' percentage: " << (timerObj -> inclusiveTime / timerObj -> time_in_sec) * 100 << " %" << endl;
			timerObj -> inclusiveTime = 0;
		}
		if(timerObj->calcFlops){
			dottedLine(tabCount,1);
			cout << "Flop rate: " << timerObj -> flopRate << endl;
		}
		if(timerObj->calcBW){
			dottedLine(tabCount,1);
			cout << "Bandwidth: " << timerObj -> bandwidth << endl;
		}		
	}	
	void DFSprint(timer_T* timerObj){//printing timing report using depth first search
		if(timerObj->nestedHead == NULL){
			printHelper(timerObj,tabCount);		
			return;				
		}
		tabCount++;	
		struct timer_T* temp = timerObj->nestedHead;
		while(temp!=NULL){
			DFSprint(temp);
			timerObj -> inclusiveTime += temp -> time_in_sec;
			temp = temp -> nestedNext;
		}
		
		if(timerObj == root){
			cout << "***************" <<endl;
			cout << "Total time taken: " << root -> inclusiveTime << endl;
			root -> inclusiveTime = 0;
			return;
		}
		if(tabCount != 0)
			tabCount--;
		printHelper(timerObj,tabCount);			
	}

	void computeAverage(){//compute average time, fr, bw for each timer
		std::map<string,timer_T>::iterator timerIter;
		for(timerIter = timers.begin();timerIter != timers.end();timerIter++){
			timerIter->second.averageTime = timerIter->second.time_in_sec/timerIter->second.numRuns;
			timerIter->second.averageFR = timerIter->second.flopRate/timerIter->second.numRuns;
			timerIter->second.averageBW = timerIter->second.bandwidth/timerIter->second.numRuns;
		}
	}
	void dumpReference(string file){//dump data to reference file
		const char* fileName = file.c_str();
		char buffer[1024];
		memset(buffer,0,1024);
		int fd = open(fileName,O_CREAT | O_RDWR | O_TRUNC , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if(fd<0){
			if(debug)
				cout << "Cannot open file '" << file <<"'" <<endl;
			return;
		}
		std::map<string,timer_T>::iterator timerIter;
		computeAverage();
		for(timerIter = timers.begin();timerIter != timers.end();timerIter++){
			sprintf(buffer,"%f %f %f %s\n",timerIter->second.averageTime,timerIter->second.averageFR,timerIter->second.averageBW,timerIter->second.name.c_str());
			write(fd,buffer,strlen(buffer));
		}
		
	}

	void readReference(const char* fileName){//read data from reference file into hash
		FILE *fp;
		char *line = (char*) malloc(sizeof(char) * 1024);
		memset(line,0,1024);
		fp = fopen(fileName,"r");
		char* name = (char*) malloc(sizeof(char) * 255);
		memset(name,0,255);
		double t,fr,bw;
		if(fp == NULL){
			if(debug)
				cout << "Cannot open file '" << fileName << "'" << endl;
			return;
		}
		while(1){
			if(fgets(line,1024,fp)==NULL)
				break;
			sscanf(line,"%lf %lf %lf %[^\t\n]",&t,&fr,&bw,name);
			compare* temp = new compare();
			temp -> name = name;
			temp -> time = t;
			temp -> flopRate = fr;
			temp -> bandwidth = bw;
			reference[name] = *temp;
		}
	}
	
	void computeImprovement(){//compute the percentage improvement for current run vs reference data
		std::map<string,timer_T>::iterator timerIter;
		std::map<string,compare>::iterator compIter;
		computeAverage();
		for(timerIter=timers.begin();timerIter!=timers.end();timerIter++){
			compIter = reference.find(timerIter->second.name);
			if(compIter != reference.end()){
				timerIter->second.timeImprovement = (compIter->second.time - timerIter->second.averageTime) * 100/compIter->second.time;
				if(compIter->second.flopRate != 0)
					timerIter->second.FRImprovement = (compIter->second.flopRate - timerIter->second.averageFR) * 100/compIter->second.flopRate;
				if(compIter->second.bandwidth != 0)
					timerIter->second.BWImprovement = (compIter->second.bandwidth - timerIter->second.averageBW) * 100/compIter->second.bandwidth;
	
			}
			else{
				cout << "Cannot find reference data for timer '" << timerIter->second.name << "'" << endl;
			}	
		}
		
	}

	void comparisonReport(string timerName,string newFile,string refFile){//generate comparison report
		refFlag = true;
		const char* refName = refFile.c_str();
		readReference(refName);
		std::map<string,timer_T>::iterator iter;
		iter=timers.find(timerName);
		if(iter!=timers.end() || timerName.compare("")==0){
			computeImprovement();
			printConcise(timerName,newFile);
		}
		else{	
			if(debug)
				cout << "Undefined timer '" << timerName << "'" << endl;		
		}
	}

};





