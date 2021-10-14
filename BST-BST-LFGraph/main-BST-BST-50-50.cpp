/*
 * File:lfgraphmain.h
 *  
 *
 * Author(s):
 *   Muktikanta Sa   <muktikanta.sa@gmail.com>
 *   
 *   
 * Description:
 *   implementation of a graph using LF-BST
 * Copyright (c) 2017.
 * last Updated: 17/10/2017
 *
*/
#include <chrono>
#include <unistd.h>
#include <sys/resource.h>
 #include"LFGraph-BST-BST.cpp"

time_t start1,end1;
atomic<long> vertexID;
double seconds;
 typedef struct 
{
    int     secs;
    int     usecs;
}TIME_DIFF;

TIME_DIFF * my_difftime (struct timeval * start, struct timeval * end)
{
	TIME_DIFF * diff = (TIME_DIFF *) malloc ( sizeof (TIME_DIFF) );
 
	if (start->tv_sec == end->tv_sec) 
	{
        	diff->secs = 0;
        	diff->usecs = end->tv_usec - start->tv_usec;
    	}
   	else 
	{
        	diff->usecs = 1000000 - start->tv_usec;
        	diff->secs = end->tv_sec - (start->tv_sec + 1);
        	diff->usecs += end->tv_usec;
        	if (diff->usecs >= 1000000) 
		{
        	    diff->usecs -= 1000000;
	            diff->secs += 1;
	        }
	}
        return diff;
}

struct timeval tv1, tv2;
TIME_DIFF * difference;

int NTHREADS;
atomic <int> ops;
atomic <int> opsUpdate;
atomic <int> opsLookup;
atomic <int> opsapp;
typedef struct infothread{
  long tid;
  LFGraph G;
  
}tinfo;

int  nops;
int itr = 0;
void* pthread_call(void* t)
{
        tinfo *ti=(tinfo*)t;
        long Tid = ti->tid;
        LFGraph G1=ti->G;
        int u, v;
      
        
        while(true)
        {
                if(ops > nops)
                     break;
              int op = rand()%1000;

                if(op > 0 && op <125 )
                {
                                v = rand() %vertexID;          
                                vertexID++;

                                G1.addV(v, Tid, NTHREADS);
                                opsUpdate++;
                                ops++;


                }

                else if(op >=125 && op <250 )
                {
                        l2:     v = rand() % (vertexID); 
                                if(v == 0)
                                        goto l2;

                                //G1.RemoveV(v);
                                G1.removeV(v, Tid);
                                opsUpdate++;
                                ops++;

                                
                }
                else if(op >= 250 && op < 375 )
                {
                l3:             u = (rand() % (vertexID));            
                                v = (rand() % (vertexID));
                                if(u == v || u == 0 || v == 0)
                                        goto l3;

                                G1.removeE(u,v, Tid);
                                opsUpdate++;
                                ops++;


                }
                else if(op >=375 && op <500 )
                {
                l4:             u = (rand() % (vertexID));  
                                v = (rand() % (vertexID));
                                if(u == v || u == 0 || v == 0)
                                        goto l4;

                                G1.addE(u,v,Tid);
                                opsUpdate++;
                                ops++;


                }
                else if(op >= 500 && op <750 )
                {
                l5:             u = (rand() % (vertexID));  
                                v = (rand() % (vertexID));
                                if(u == v || u == 0 || v == 0)
                                        goto l5;

                                G1.ContainsE(u,v,Tid);
                                opsLookup++;
                                ops++;


                }
                else if(op >= 750 && op < 1000 )
                {

                l6:             v = rand() % (vertexID);   
                                if(v == 0)
                                        goto l6;

                               G1.ContainsV(v, Tid);
                               opsLookup++;
                               ops++;


                }
        }               //end of while loop
}




int main(int argc, char*argv[]) //command line arguments - #threads, #vertices initially, #time in seconds
{
        
        vertexID = 1;
        int i;
        if(argc < 3)
        {
                cout << "Enter 3 command line arguments - #threads, #operations, #init vertices" << endl;
                return 0;
        }
	LFGraph sg;
  	NTHREADS = atoi(argv[1]);
  	sg.init(NTHREADS);
    MAX_THREADS = atoi(argv[1]);
        
        long initial_vertices = atoi(argv[3]);  // initial number of vertices
        nops = atoi(argv[2]);
        
        vertexID = initial_vertices;           // or +1?
	
        pthread_t *thr = new pthread_t[NTHREADS];
        pthread_attr_t attr;
        pthread_attr_init (&attr);
        pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);
 	    ops = 0;
        opsUpdate = 0;
        opsLookup = 0;
        
        
        gettimeofday(&tv1,NULL);

        for (i=0;i < NTHREADS;i++)
        {
              tinfo *t =(tinfo*) malloc(sizeof(tinfo));
                t->tid = i;
                t->G = sg;
                pthread_create(&thr[i], &attr, pthread_call, (void*)t);
        }

        for (i = 0; i < NTHREADS; i++)
        {
                pthread_join(thr[i], NULL);
        }
        gettimeofday(&tv2,NULL);
        delete []thr;
	difference = my_difftime (&tv1, &tv2);
	int dig = 1;
	int temp = difference->usecs;
	while(temp>=10)
	{	
		dig++;
		temp = temp/10;
	}
	temp =1;
	for(i=1;i<=dig;i++)
		temp = temp * 10;
	double duration = (double) difference->secs + ((double)difference->usecs / (double)temp);

	cout<<"Number of Threads:"<<NTHREADS<<endl;
    	cout << "Duration (gettimeofday() function): " << duration <<" secs."<<endl;
        
        cout << "Total operations: " << ops <<endl;
        cout << "Total Update operations: " << opsUpdate <<endl;
        cout << "Total Lookup operations: " << opsLookup <<endl;

       return 0;
}


