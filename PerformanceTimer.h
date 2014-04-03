/*
 * PerformanceTimer.h
 *
 *  Created on: Mar 19, 2014
 *      Author: pravs
 */

#ifndef PERFORMANCETIMER_H_
#define PERFORMANCETIMER_H_

#if (((defined WIN32)|| (defined WIN64)) && !(defined(__MINGW32__) || defined(__CYGWIN__)))
#include <time.h>
# ifndef NOMINMAX
#  define NOMINMAX
# endif
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <time.h>
# undef WIN32_LEAN_AND_MEAN
# undef NOMINMAX
#else
#include<sys/time.h>
#endif
#include <omp.h>
#include <iostream>
#include <stdio.h>

using namespace std;

enum timer_option_t {OPENMP_TIMER = 1, CLOCK_TIMER = 2};

timer_option_t choice;


template<typename real_type>
class PerformanceTimer
{
#if (((defined WIN32)|| (defined WIN64))  && !(defined(__MINGW32__) || defined(__CYGWIN__)))

	private:
		LARGE_INTEGER m_start;
		LARGE_INTEGER m_end;
		LARGE_INTEGER m_freq;
		bool init;
    		clock_t begin, end;	
		double begin_omp,end_omp;

	public:
		PerformanceTimer():init(true);

	public:
		 void start(){
			if(choice == CLOCK_TIMER){//use clock()
				begin = clock();
			}
			else if(choice == OPENMP_TIMER){//use OpenMP timer
				begin_omp = omp_get_wtime();
			}
			else{//defaults to windows timer
				 if(init)
		     		 {
		     		   QueryPerformanceFrequency(&m_freq);
		    		    init = false;
		   		   }
		   		   QueryPerformanceCounter(&m_start);
				}
		    }

						/// Stops the timer
		    void stop(){
			if(choice == CLOCK_TIMER){
				end = clock();		
			}
			else if(choice == OPENMP_TIMER){
				end_omp = omp_get_wtime();
			}
			else{//defaults to windows timer
		     		 QueryPerformanceCounter(&m_end);
			}
		    }

		    real_type operator()()const{
			real_type t1,t2;
			if(choice == CLOCK_TIMER){
				t1 = static_cast<real_type>(begin);
				t2 = static_cast<real_type>(end);
				return (t2-t1)/CLOCKS_PER_SEC;
			}
			else if(choice == OPENMP_TIMER){
				t1 = static_cast<real_type>(begin_omp);
				t2 = static_cast<real_type>(end_omp);
				return t2-t1;
			}
			else{
		          real_type end = static_cast<real_type>(m_end.QuadPart);
		          real_type start = static_cast<real_type>(m_start.QuadPart);
		          real_type freq = static_cast<real_type>(m_freq.QuadPart);
		          return (end - start)/ freq;
			}
		   }
#else

  private:
    struct timeval m_start;
    struct timeval m_end;
    clock_t begin, end;	
	double begin_omp,end_omp;

    #ifdef SGI
     time_t			 m_tz;
	#else
     struct	timezone m_tz;
	#endif
  public:
				/// Start the timer
    void start() {
	if(choice == CLOCK_TIMER){
		begin = clock();
	}
	else if(choice == OPENMP_TIMER){
		begin_omp = omp_get_wtime();
	}
	else{//defaults to gettimeofday on linux
		gettimeofday(&m_start, &m_tz); 
	}
    }

				/// Stops the timer
    void stop(){ 
	if(choice == CLOCK_TIMER){
		end = clock();		
	}
	else if(choice == OPENMP_TIMER){
		end_omp = omp_get_wtime();
	}
	else{
		gettimeofday(&m_end,   &m_tz);
	}
    }

				/// Get the timer value, with the () operator.
    real_type operator()()const
    { real_type t1,t2;
	if(choice == CLOCK_TIMER){
		t1 = static_cast<real_type>(begin);
		t2 = static_cast<real_type>(end);
		return (t2-t1)/CLOCKS_PER_SEC;
	}
	else if(choice == OPENMP_TIMER){
		t1 = static_cast<real_type>(begin_omp);
		t2 = static_cast<real_type>(end_omp);
	}
	else{		
		t1 =  static_cast<real_type>(m_start.tv_sec) + static_cast<real_type>(m_start.tv_usec)/(1000*1000);
		t2 =  static_cast<real_type>(m_end.tv_sec) + static_cast<real_type>(m_end.tv_usec)/(1000*1000);
	}
      return t2-t1;
    }

#endif

};






#endif /* PERFORMANCETIMER_H_ */

