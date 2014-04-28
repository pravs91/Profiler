#include <stdio.h>
#include <iostream>
#include <string>
#include "PerformanceTimer.h"
#include "PerformanceTimer.cpp"

/*int main(){//interleaved timers
	Profiler obj;
	obj.start("head");
	sleep(2);
		obj.start("A");
		sleep(1);
		obj.start("B","");
		sleep(1);
		obj.stop("A");
		sleep(1);
		obj.stop("B");
	obj.stop("head");
	obj.printReport("head");
	obj.printConcise("head",NULL);
	return 0;

}
*/
//start and stop example
/*int main(){
	Profiler obj;
	obj.start("A");
		// do something here....
	obj.stop("A");

	obj.start("B");
		//do some more stuff...
	obj.stop("B"); 
	return 0;
}
*/
//
//nested timers example....
/*
int main(){
	Profiler obj("OPENMP");
	int i;
	for(i=0;i<2;i++){
	obj.start("Head");
	//master function "Head" that has several functions within it
	sleep(2);
		obj.start("A");
		//function A that has functions B and D as children.
		sleep(1);	
			obj.start("B");
			//function B...has C
			sleep(1);
				obj.start("C");
				//function C...
				sleep(3);
				obj.stop("C");
			obj.stop("B");
			
			obj.start("D");
			//function D...belongs to A
			sleep(1);
			obj.stop("D");
		
		obj.stop("A");
	obj.stop("Head");
	}

	obj.stop("fake");

	obj.comparisonReport("","comparison.txt","ref.txt");

//	obj.dumpReference("ref.txt");
//	obj.printReport();
//	obj.printReport("A");
//	obj.printReport("D");	
	obj.printNestedTimers("","output1.txt");
	obj.printConcise("","output1.txt");
	obj.printConcise("B","STDOUT");
	obj.printConcise("A","output1.txt");
//	cout << "A took " << obj.getTime("A","sec") << "s = " << obj.getTime("A","ms") << "ms to complete" << endl;
	return 0;			
}
*/

int main(){
	Profiler obj("OPENMP");
	int a = 1, b=1,c=1;
	int i;
	for(i=0;i<3;i++){
	obj.start("head");
		if(a){
			obj.start("A");
			sleep(1);
			obj.setFlop("A",2);
			obj.stop("A");
			a = 0;
		}
		else if(b){
			obj.start("B");
			sleep(1);
			obj.setMemory("B",10);
			obj.stop("B");
			b = 0;
		}
		else {
			obj.start("C");
			sleep(2);
			obj.setFlop("C",5);
			obj.setMemory("C",8);
			obj.stop("C");
		}
			obj.start("last");
			sleep(1);
			obj.stop("last");
	obj.stop("head");
	}
	obj.start("head");
		obj.start("another nested timer");
		sleep(1);
		obj.stop("another nested timer");
	obj.stop("head");
	obj.printReport();
	obj.printConcise("","report.txt");
	


	return 0;




}
