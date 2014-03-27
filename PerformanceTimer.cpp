/*
 * myTimer.cpp
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

using namespace std;

typedef struct timer_T{
	double flop, memOps, time_in_sec,time_in_ms;
	double bandwidth,flopRate;
	bool calcFlops,calcBW;
	bool startFlag,stopFlag;
	int numRuns;
	string name;
	PerformanceTimer<double> timer;

	struct timer_T* next;
	struct timer_T* prev;
	struct timer_T* nestedHead;
	struct timer_T* nestedTail;
	struct timer_T* nestedNext;
	
	double inclusiveTime;
	timer_T(){
		flop = memOps = time_in_sec = time_in_ms = 0;
		bandwidth = flopRate = 0;
		numRuns = 0;
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



template<typename T>
class Profiler
{
public:
	int numberOfTimers;
	double totalTime;
	std::map<string,timer_T> timers;
	timer_T* root;
	
	Profiler(){//constructor to initialize the class members
		root = new timer_T();
		numberOfTimers = 0;
		totalTime = 0;
	}
public:

	void start(string timerName){
		std::map<string,timer_T>::iterator timerIter;
		timerIter = timers.find(timerName);
		if(timerIter != timers.end()){
			timers[timerName].start();
			timers[timerName].stopFlag = false;
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
				cout << "Timer " << timers[timerName].name << " already stopped" << endl;
				return;
			}				
			timers[timerName].stop();
			timer_T* current = &timers[timerName];//current = timer to stop
			timer_T* nested = timers[timerName].next;
			while(nested != NULL && !nested->stopFlag){//stop all nested timers
				nested->stop();
				nested = nested->next;
			}
			if(current != NULL && current -> prev != NULL)//update next pointer of parent timer to NULL
				current->prev->next = NULL;
		}
		else{
			std::cout << "Undefined timer '" << timerName << "'" << endl;
		}
	}

	void reset(){//reset all timer data
		std::map<string,timer_T>::iterator timerIter;
		for(timerIter = timers.begin();timerIter != timers.end();timerIter++){
			timerIter->second.reset();
		}
	}
// void PrintError(string error){ if(dodebug) cout error
	double getTime(string timerName,string unit){//get the time taken in seconds or ms
		std::map<string,timer_T>::iterator timerIter;
		timerIter = timers.find(timerName);
		if(timerIter != timers.end()){//timer already exists
			if(unit.compare("sec")==0)
				return timers[timerName].time_in_sec;
			else if(unit.compare("ms")==0)
				return timers[timerName].time_in_ms;
			else
				std::cout << "Specify a valid unit 'sec' or 'ms'" << endl;
				return -1;
		}
		else{
			std::cout << "Undefined timer '" << timerName << "'" << endl;
			return -1;
		}
	}

	void setFlop(string timerName,double f){//set flop count
		timers[timerName].flop = f;
		timers[timerName].calcFlops = true;
		timers[timerName].computeFlops();
	}

	void setMemory(string timerName,double m){//set memory transfer count for BW calculation
		timers[timerName].memOps = m;
		timers[timerName].calcBW = true;
		timers[timerName].computeBW();
	}

	void printReport(){
		DFSprint(root);
	}
	void printReport(string timerName){
		std::map<string,timer_T>::iterator timerIter;
		timerIter = timers.find(timerName);
		DFSprint(&timerIter->second);
	}
	
	void insertTab(int count){
		int i;
		for(i = 0;i < count; i++)
			cout<<"-----";
	}	
	
	void DFSprint(timer_T* timerObj){
		static int tabCount = 0;
		int j;
		if(timerObj->nestedHead == NULL){
			insertTab(tabCount);
			cout << "***************" << endl;
			insertTab(tabCount);
			cout << "Timer Name: " << timerObj->name << endl;
			insertTab(tabCount);
			cout << "Time taken: " << timerObj->time_in_sec << "s = " << timerObj->time_in_ms << "ms" << endl;
			if(timerObj -> inclusiveTime != 0){
				insertTab(tabCount);
				cout << "Nested timers' time: " << timerObj->inclusiveTime << endl;
				insertTab(tabCount);
				cout << "Nested timers' percentage: " << (timerObj -> inclusiveTime / timerObj -> time_in_sec) * 100 << " %" << endl;
			}
			return;				
		}
		tabCount++;
		int i;
		
		struct timer_T* temp = timerObj->nestedHead;
		while(temp!=NULL){
			DFSprint(temp);
			timerObj -> inclusiveTime += temp -> time_in_sec;
			temp = temp -> nestedNext;
		}
		
		if(timerObj == root){
			cout << "***************" <<endl;
			cout << "Total time taken: " << root -> inclusiveTime << endl;
			return;
		}
		if(tabCount != 0)
			tabCount--;
		insertTab(tabCount);
		cout << "***************" << endl;
		insertTab(tabCount);
		cout << "Timer Name: " << timerObj->name << endl;
		insertTab(tabCount);
		cout << "Time taken: " << timerObj->time_in_sec << "s = " << timerObj->time_in_ms << "ms" << endl;
		insertTab(tabCount);
		cout << "Nested timers' time: " << timerObj->inclusiveTime << endl;
		insertTab(tabCount);
		cout << "Nested timers' percentage: " << (timerObj -> inclusiveTime / timerObj -> time_in_sec) * 100 << " %" << endl;
		if(timerObj->calcFlops){
			insertTab(tabCount);
			cout << "Flop rate: " << timerObj -> flopRate << endl;
		}
		if(timerObj->calcBW){
			insertTab(tabCount);
			cout << "Bandwidth " << timerObj -> bandwidth << endl;
		}		
	}

};



int main(){
	Profiler<int> obj;
	int i,j;
//	obj.DebugReport(1);

	obj.start("head's parent");
		for(j = 0;j<2;j++){
			obj.start("head");
			for(i = 0;i<2;i++){
				obj.start("A");
				sleep(1);
//	obj.stop("A");
					obj.start("B");
						obj.start("D");
//	obj.stop("A");
						sleep(2);
						obj.stop("D");
					obj.stop("B");

					obj.start("C");
					sleep(1);
					obj.stop("C");

				obj.stop("A");
			}
			obj.start("M");
				obj.start("N");
				sleep(1);
				obj.stop("N");
			obj.stop("M");
//	obj.setFlop("A",2);
//	obj.setMemory("M",10);
			for(i = 0;i < 3;i++){
				obj.start("test");
				sleep(1);
				obj.stop("test");
			}
			obj.stop("head");
	}
	obj.stop("head's parent");
	obj.printReport();

//	cout << "M's time: " << obj.getTime("M","ms") << endl;
//	cout << "test's time: " << obj.getTime("test","ms") << endl;

	return 0;
}
