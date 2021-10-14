#include<iostream>
#include<float.h>
#include<stdint.h>
#include<stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <vector>
#include <ctime>       
#include <random>
#include <algorithm>
#include <iterator>
#include <math.h>
#include <time.h>
#include <fstream>
#include <iomanip>
#include <sys/time.h>
#include <atomic>
#include<list>
#include<queue>
#include<stack>
#include<string.h>

#define vntp "VERTEX NOT PRESENT"
#define entp "EDGE NOT PRESENT"
#define ventp "VERTEX OR EDGE NOT PRESENT"
#define ep "EDGE PRESENT"
#define eadd "EDGE PutED"
#define er "EDGE REMOVED"
#define ef "EDGE FOUND"
#define eupdt "EDGE UPDATED"
#define vupdt "VERTEX UPDATED"
#define vp "VERTEX PRESENT"
#define vadd "VERTEX PutED"
#define vr "VERTEX REMOVED"
#define pp "PATH PRESENT"
#define pntp "PATH NOT PRESENT"

using namespace std;



inline int is_marked_ref(long i){
  return (int) (i & 0x1L);
}

inline long get_unmarked_ref(long w){
  return w & ~0x1L;
}

inline long get_marked_ref(long w){
  return w | 0x1L;
}


// ENode structure
typedef struct ENode{
	int key; // immutable key field
	//atomic <double> wt;// weight of the edge
	atomic<struct VNode *>pointv; // pointer to its vertex
	atomic<struct ENode *>enext; // pointer to the next ENode
}elist_t;

// VNode structure
typedef struct VNode{
	int key; // immutable key field
	//atomic<double> wt; // weight of the vertex
	atomic<struct VNode *>vnext; // pointer to the next VNode
	atomic<struct ENode *>enext; // pointer to the EHead
	atomic <int> ecount; // counter for edge operations
}vlist_t;

int th = 8;

enum OPType {
  DATA  = 0,
  DEAD = 1,
  INS = 2,
  DEL = 3,
  FREEZE = 4
};
// FSetNode structure
typedef struct FSet{
        bool ok;
        long size;
        //atomic<VNode *>head;	
        struct VNode *head;	
}FSet;

// FSetNode structure
typedef struct FSetOp{
        OPType otype;
        int key;
        bool done;
        bool response;	
}FSetOp;

typedef struct  HNode {
        FSet *buckets;
        long size;
        atomic<HNode *>pred;
        long used;
        long new_resize;
  }HNode;    
typedef struct HSNode{
        int ct;
  	atomic<HNode *>next;
  }HSNode;
  
class LFGraph{
public:
  HSNode * Head;
  void initFSetOp(FSetOp *hnode, OPType otype, int &key){
        hnode->otype = otype;
        hnode->key = key;
        hnode->done = false;
        hnode->response = false;
  }
  VNode * initFSet(){
        vlist_t * vTail = (vlist_t*) malloc(sizeof(vlist_t));
          vTail ->key = INT_MAX;
          vTail ->vnext = NULL;
         vlist_t * vHead = (vlist_t*) malloc(sizeof(vlist_t));
          vHead ->key = INT_MIN;
          vHead ->vnext = NULL; 
        vHead->vnext.store(vTail);  
     return vHead;   
  }
  void initHNode(int size){
  Head = new HSNode;
  HNode  *head = new HNode;//[size];
  VNode * bkt;
  head->buckets = new FSet[th*size+1];
  for(long i=0; i<=th*size; i++){
      bkt = initFSet();//new VNode;
      head->buckets[i].head = bkt;//initFSet();// = new Fset;//[size];
      head->buckets[i].ok = true;
      head->buckets[i].size = 2;
      }
   //newbuckets = initFSet();
   //head->ok
   head->size = th*size;
   head->used = 0;
   head->pred = NULL;
    Head->ct = 1;
   Head->next = head; 
  }
  void initHNode(HNode *t, int size){
   //t = new HNode;//[size];
   t->buckets = new FSet[size];
   VNode * bkt;
   for(long i=0; i<size; i++){
      bkt = initFSet();//new VNode;
      t->buckets[i].head = bkt;//initFSet();// = new Fset;//[size];
      t->buckets[i].ok = true;
      t->buckets[i].size = 0;
      }
   //newbuckets = initFSet();
   //head->ok
   t->size = size;
   t->used = 0;
   t->pred = NULL;
  }
  
 void freeHash(HNode *t){
  HNode  *head = t;
  VNode *bkt, *bktnext;
    for(long i=0; i<head->size; i++){
      bkt = head->buckets[i].head;
      bktnext = bkt->vnext;
      while(bktnext){
        free(bkt);
        bkt = bktnext;
        bktnext = bktnext->vnext;
        
      }
      
    }
    delete[]head->buckets;
  }
  elist_t* createE(int key){
          elist_t * newe = (elist_t*) malloc(sizeof(elist_t));
          //assert(newe != NULL);
          newe ->key = key;
          //newe ->wt.store(wt);
          newe ->pointv.store(NULL);
          newe ->enext.store(NULL);
         return newe;
        }
 vlist_t* createV(int key, int n){
          elist_t *EHead = createE(INT_MIN); //create Edge Head
         elist_t *ETail = createE(INT_MAX); // create Edge Tail
          EHead ->enext.store(ETail); 
          vlist_t * newv = (vlist_t*) malloc(sizeof(vlist_t));
         // assert(newv != NULL);
          newv ->key = key;
          //newv ->state = state;
          //newv ->wt.store(wt);
          newv ->vnext.store(NULL);
          newv->ecount.store(0); // init ecounter to zero
          newv ->enext.store(EHead);
         return newv;
 }
    
 // Find pred and curr for VNode(key)     
void locateVPlus(vlist_t *startV, vlist_t ** n1, vlist_t ** n2, int key, int tid){
       vlist_t *succv, *currv, *predv;
      retry:
       while(true){
        predv = startV;
        currv = (vlist_t *) get_unmarked_ref((long)predv->vnext.load());// predv->vnext.load();
        while(true){
         succv = currv->vnext.load();
         while(currv->vnext.load() != NULL && is_marked_ref((long) succv) && currv->key < key ){ 
           if(!predv->vnext.compare_exchange_strong(currv, (vlist_t *) get_unmarked_ref((long)succv), memory_order_seq_cst)){
           goto retry;
           }
           currv = (vlist_t *) get_unmarked_ref((long)succv);
           succv = currv->vnext.load(); 
         }
         if(currv->key >= key){
          (*n1) = predv;
          (*n2) = currv;
          return;
         }
         predv = currv;
         currv = (vlist_t *) get_unmarked_ref((long)succv);
        }
     }  
 } 
    
     // add a new vertex in the vertex-list
 bool PutV(VNode *vHead, int key, int NT, int tid){
      vlist_t* predv, *currv;
      while(true){
        locateVPlus(vHead, &predv, &currv, key,tid); // find the location, <pred, curr>
        if(currv->key == key){
           return false;//(char*) vp; // key already present
          
        }         
        else{
           vlist_t *newv = createV(key, NT); // create a new vertex node
           newv->vnext.store(currv);  
           
           if(predv->vnext.compare_exchange_strong(currv, newv, memory_order_seq_cst)) {// PutEd in the vertex-list
               return true;//(char *) vadd; //
           }
          } 
      }
  }
   
 
 
// Deletes the vertex from the vertex-list
bool RemoveV(VNode *vHead, int key, int tid){
        vlist_t* predv, *currv, *succv;
           while(true){
        	locateVPlus(vHead, &predv, &currv, key,tid);
		if(currv->key != key){
			return false;
			}//(char*) vntp; // key is not present}
		succv = currv->vnext.load(); 
		if(!is_marked_ref((long) succv)){
		  if(!currv->vnext.compare_exchange_strong(succv, (vlist_t*)get_marked_ref((long)succv), memory_order_seq_cst)){ // logical deletion
		    continue;
		    }
		   if(predv->vnext.compare_exchange_strong(currv, succv, memory_order_seq_cst)){ // physical deletion
                       break;
        	 }
        	}	
	} 
   return true;//(char*) vr;
}
  // Find pred and curr for ENode(key)    in the edge-list 
void locateEPlus(vlist_t **startE, elist_t ** n1, elist_t ** n2, int key, int tid){
       elist_t *succe, *curre, *prede;
       vlist_t *tv;
retry: while(true){
        prede = (*startE)->enext;
        curre = (elist_t*)get_unmarked_ref((long)prede->enext.load());
        while(true){
         succe = curre->enext.load();
         tv = curre->pointv.load();
     
        //helping: delete one or more enodes which are marked
         while(curre->enext.load() != NULL && is_marked_ref((long) succe) && !is_marked_ref((long)tv->vnext.load()) &&  curre->key < key ){ 
           (*startE)->ecount.fetch_add(1, memory_order_relaxed);
           if(!prede->enext.compare_exchange_strong(curre, (elist_t*)get_unmarked_ref((long)succe), memory_order_seq_cst)){
           goto retry;
           }
           curre = (elist_t*)get_unmarked_ref((long)succe);
           succe = curre->enext.load(); 
           tv = curre->pointv.load();
         }


         if(curre->key >= key){
          (*n1) = prede;
          (*n2) = curre;
          return;
         }
         prede = curre;
         curre =(elist_t*)get_unmarked_ref((long)succe);
        }
       }  
    } 

void locateV(vlist_t ** n1, int key) {
HNode *t = Head->next.load();
    FSet b = t->buckets[key % t->size];
    VNode * vhead = b.head;
    if(vhead && vhead->vnext.load() && vhead->vnext.load()->key != INT_MAX ) {
       HNode *s = t->pred.load();
       b = (s == NULL)
                ? t->buckets[key % t->size]
                : s->buckets[key % s->size]; 
    }
    VNode * curr_bucket_head = b.head;
    vlist_t *currv = curr_bucket_head->vnext;
    while(currv->vnext.load() && currv->key < key){
           currv =  (vlist_t *) get_unmarked_ref((long)currv->vnext.load());
          } 
      (* n1) = currv; 
      return;   
}



bool ContainsVPlus(vlist_t ** n1, vlist_t ** n2, int key1, int key2){
     vlist_t *curr1, *pred1, *curr2, *pred2;
     locateV( &curr1, key1); 
     if((!curr1 || !curr1->vnext.load()) || curr1->key != key1)
	return false; // key1 is not present in the vertex-list
     locateV(&curr2, key2); 
     if((!curr2 || !curr2->vnext.load()) || curr2->key != key2)
        return false; // key2 is not present in the vertex-list
     (*n1) = curr1; 
     (*n2) = curr2; 
    return true;    
 }	
// add a new edge in the edge-list    
 char * PutE1(int key1, int key2, int tid){
               elist_t* prede, *curre;
               vlist_t *u,*v;
               
               bool flag = ContainsVPlus(&u, &v, key1, key2);
               if(flag == false){           
                  return (char*)vntp; // either of the vertex is not present
               }   
               while(true){
                locateEPlus(&u, &prede, &curre, key2,tid);
                 if(is_marked_ref((long) curre->enext.load())) {
                     continue;
                  } 
                if(curre->key == key2){
                     return (char*)ep; // edge already present
                  }
                 else{  
                   elist_t *newe = createE(key2);// create a new edge node
                   newe->enext.store(curre);  // connect newe->next to curr
                   newe->pointv.store(v); // points to its vertex
                   if(prede->enext.compare_exchange_strong(curre, newe, memory_order_seq_cst)){  // insertion
                      u->ecount.fetch_add(1, memory_order_relaxed);
                      return (char*)eadd;
              }
             } 
         } // End of while           
     }    
  
  // add a new edge in the edge-list    
 char * PutE(int key1, int key2, int tid){
               elist_t* prede, *curre;
               vlist_t *u,*v;
               //stin>>sleeptime;
               //cout<<sleeptime<<" ";
               //usleep(sleeptime);
               bool flag = ContainsVPlus(&u, &v, key1, key2);
               if(flag == false){          
                  return (char*)vntp; // either of the vertex is not present
               }   
               while(true){

                locateEPlus(&u, &prede, &curre, key2,tid);
                 if(is_marked_ref((long) curre->enext.load())) {
                     continue;
                  } 
                if(curre->key == key2){
                     return (char*)ep; // edge already present
                  }
                 else{  
                   elist_t *newe = createE(key2);// create a new edge node
                   newe->enext.store(curre);  // connect newe->next to curr
                   newe->pointv.store(v); // points to its vertex
                   if(prede->enext.compare_exchange_strong(curre, newe, memory_order_seq_cst)){  // insertion
                      u->ecount.fetch_add(1, memory_order_relaxed);
                      return (char*)eadd;
              }
             } 
         } // End of while           
     }    
// Deletes an edge from the edge-list if present
 char* RemoveE(int key1, int key2, int tid){
               elist_t* prede, *curre, *succe;
               vlist_t *u,*v;
               //stin>>sleeptime;
               //usleep(sleeptime);
               bool flag = ContainsVPlus(&u, &v, key1, key2);
               if(flag == false){         
                  return (char*)vntp; // either of the vertex is not present
               }   
               while(true){
              
               locateEPlus(&u, &prede, &curre, key2, tid);
                if(curre->key != key2){
                     return (char*)entp; // edge already present
                   } 
               succe = curre->enext.load();
	       if(!is_marked_ref((long) succe)){
	         if(!curre->enext.compare_exchange_strong(succe, (elist_t*)get_marked_ref((long)succe), memory_order_seq_cst)){ //  logical deletion
		    continue;
		    }
		  u->ecount.fetch_add(1, memory_order_relaxed); // increament the counter
		  if(!prede->enext.compare_exchange_strong(curre, succe, memory_order_seq_cst)){ // physical deletion
			break;
	          }
	      }

	}
	return (char*)er;      
 }    
 //Contains ENode       
 char* ContainsE(int key1, int key2, int NT){
          elist_t *curre;
          vlist_t *u,*v;
          bool flag = ContainsVPlus(&u, &v, key1, key2);
          if(flag == false){             
               return (char*)vntp; // either of the vertex is not present
          }   
          curre = u->enext.load(); 
          while(curre->enext.load() && curre->key < key2){
           curre =  (elist_t*)get_unmarked_ref((long)curre->enext.load());
          }
	 if((curre->enext.load()) && curre->key == key2 && !is_marked_ref((long) curre->enext.load()) && !is_marked_ref((long) u->vnext.load()) && !is_marked_ref((long) v->vnext.load())){
	        return (char*)ep;
	        }
	  else {
	        return (char*)ventp;
          }   
    } 

   void initHash(int n, int NT,int tid){
   int i, v_;
    for( i=1;i<=n;i++){
         //v_ = rand()%n;
         //if(v_ != 0)
          insertinit(i, NT,tid);
     }
  } 

  void insertinit(int key, int NT,int tid) {
    HNode *t = Head->next.load();
    FSet b = t->buckets[key % t->size];
    PutV(b.head, key, NT,tid);  
    t->used++;  
}

void PrintResize(){
   cout<<"resize:"<<Head->ct<<endl;
}

  bool addV(int key, int NT, int tid) {
    int result = apply(INS, key, NT,tid);
    //HNode *h = Head->next.load();
    //if(h->used > (h->size*th)){     //simple heuristic for shrinking
    //    resize(&h, true);
    //    Head->ct++;
    //}
    return result;
}

bool removeV(int key, int NT, int tid) {
    int result = apply(DEL, key, NT,tid);
    //simple heuristic for shrinking
   //  HNode *h = Head->next.load();
   // if(h->used < (h->size/2)){
   //     resize(&h, false);
   //     Head->ct--;
   // }
    return result;//apply(DEL, key);
}


bool hasMember(VNode * curr_bucket_head, int  key){
    vlist_t *currv = curr_bucket_head;
    while(currv->vnext.load() != NULL ){
           if(currv->key == key && !is_marked_ref((long) currv->vnext.load()))
             return true;//currv->state != DEL;
           currv =  (vlist_t *) get_unmarked_ref((long)currv->vnext.load());
          }
      return false;//(char*) vntp;   
}

 
int getResponse(VNode *vhead, int type, int key, int NT, int tid)
    {
        return (type == INS) ? PutV(vhead, key, NT,tid) : RemoveV(vhead, key, tid);
    }

bool hasMember_old(VNode * curr_bucket_head,int  key){
    vlist_t *currv = curr_bucket_head->vnext, *predv;
    while(currv->vnext.load() && currv->key < key){
           currv =  (vlist_t *) get_unmarked_ref((long)currv->vnext.load());
          }
          vlist_t *succv = currv->vnext.load();
	  if((currv->vnext.load()) && currv->key == key && !is_marked_ref((long) succv)){
	        return true;//(char*) vp;
	     }   
	  else {
	        return false;//(char*) vntp;
     }   
}

int x;
bool ContainsV(int key) {
HNode *t = Head->next.load();
    FSet b;// = t->buckets[key % t->size];
    //VNode * vhead = b.head;
    //if(vhead && vhead->vnext.load() && vhead->vnext.load()->key != INT_MAX ) {
       HNode *s = t->pred.load();
       b = (s == NULL)
                ? t->buckets[key % t->size]
                : s->buckets[key % s->size]; 
    //}
    //cout<<b.head->vnext.load()->key;
    return hasMember(b.head, key);
 }
 bool apply(int type, int key, int NT, int tid) {
    // VNode *h = createV(key, type);
    int a=1;
        while (true) {
               
            HNode *t = Head->next.load();;
            int    i = key % t->size;
             FSet b  = t->buckets[i];
             VNode * vhead = b.head;
            // if the b is empty, help finish resize
            if (vhead && vhead->vnext.load() && vhead->vnext.load()->key == INT_MAX && a == 1){
                helpResize(t, i);
                a++;
               //cout<<vhead->key<<" "<<vhead->vnext.load()->key;
               } 
            // otherwise enlist at b
            else //if (h->state != FREEZE)
                return getResponse(vhead, type,key, NT,tid);
          // cout<<"apply i:"<<i<<endl;  
           //cin>>i;   
        }
       // cout<<"end";
}

 void freeze(FSet *b){
     if(b->ok == true){
       b->ok = false;
     }
   //return
   }
FSet helpResize(HNode *t, int i) {
    FSet b = t->buckets[i];
    HNode *s = t->pred.load();
    FSet new_bucket,new_bucket2;
    //std::unordered_set new_set;
    int curr_size = t->size;
//cout<<"initBucket, size, pred:"<<i<<" "<<b.size<<" "<<s<<endl;
 //int curr_size = t->size;
    VNode * vhead = b.head;
    if( vhead && vhead->vnext.load() && vhead->vnext.load()->key != INT_MAX &&  s != NULL) { //b.size == 0 &&
        int prev_size = s->size;
      //  cout<<"prev & curr size:"<<prev_size<< " "<<curr_size<<endl;
        if((curr_size*2) == (prev_size)) {
             //new_bucket = s->buckets[i % prev_size];
             s->buckets[i % prev_size] = b;
             new_bucket = s->buckets[i % prev_size];
             freeze(&b);
             freeze(&new_bucket);
             //new_bucket = b;
             new_bucket2 = split(&new_bucket, i, prev_size);
             b.ok = true;
             new_bucket.ok = true;
             s->buckets[i] =  new_bucket;
             s->buckets[i+curr_size] =  new_bucket2;
             
             //cout<<"slit i:"<<i<<endl;
        } else {
            new_bucket = s->buckets[i];
            FSet ith = t->buckets[i];
            FSet ith_ = t->buckets[i + prev_size];
            freeze(&new_bucket);
            freeze(&ith);
            freeze(&ith_);
            FSetUnion(&new_bucket, &ith, &ith_); // merg two sets
            s->buckets[i] = new_bucket;
            //cout<<"merg i:"<<i<<endl;
        }
    }
    return t->buckets[i];//.head;
}   
void resize(HNode **t, bool grow){
    //calculate new size: grow or shrink 
    (*t)->new_resize +=1;
    int new_size = grow ? (*t)->size*2 : (*t)->size/2;
    HNode *pnext = new HNode;
   // cout<<"\nnew_size & grow:"<<new_size<<" "<<grow<<endl;
    if(new_size>1 || grow == true){
     initHNode(pnext, new_size);
     pnext->used = (*t)->used;
     (*t)->pred.store(pnext);
    }
    if((*t)->size >1 && grow == true){
      for(int i=0; i < (*t)->size; i++){
            //migrate each bucket from old to the new
            helpResize((*t),  i);
        }
      (*t) = pnext;  
      (*t)->pred .store(NULL); 
      (*t)->size = new_size;
    }
    else if((*t)->size >1 && grow == false){
      for(int i=0; i < new_size; i++){
            //migrate each bucket from old to the new
            helpResize((*t),  i);
        }
      (*t) = pnext;  
      (*t)->pred.store(NULL); 
      (*t)->size = new_size;
    }
    HNode *h = Head->next.load();
    Head->next.compare_exchange_strong(h, (*t), memory_order_seq_cst);
    //head=(*t);
}
 
FSet split(FSet *old_set, int i, int osize){
   FSet new_set;// = new FSet;
   VNode * pred1 = (*old_set).head, *curr1;
   curr1 = pred1->vnext.load();
   new_set.head = initFSet();
   new_set.ok = true;
   new_set.size = 0;
   VNode * pred2 = (new_set).head, *curr2;
   curr2 = pred2->vnext.load();
   //int osize = *old_set%
   while(curr1->vnext != NULL){
   //cout<<"key:"<<curr1->key<<endl;
  // cin>>x;
   
     if(curr1->key%osize != i){
        pred2->vnext = curr1;
        pred1->vnext.store(curr1->vnext);
        curr1->vnext = curr2;
        //pred2->vnext->vnext = curr2;
        curr1 = pred1->vnext;
        pred2 = pred2->vnext; 
        continue;
     }
     pred1 = curr1;
     curr1 = curr1->vnext;
     
   }
   //cout<<"split end"<<endl;
   return new_set;
}
void FSetUnion(FSet *new_set, FSet *tmp_set,FSet *tmp_set2){
new_set->head = initFSet();
   new_set->ok = true;
   //new_set.size = 0;
  VNode * pred1 = (new_set)->head, *curr1;
  curr1 = pred1->vnext.load();
  VNode * pred2 = (tmp_set)->head, *curr2;
  curr2 = pred2->vnext.load();
  VNode * pred3 = (tmp_set2)->head, *curr3;
  curr3 = pred3->vnext.load();
  while(curr2 ->vnext != NULL && curr3->vnext != NULL){
        if(curr2->key < curr3->key){
                pred1->vnext = curr2;
                pred2->vnext.store(curr2->vnext);
                curr2->vnext =curr1;
                pred1 = curr2;
                curr2 = pred2->vnext;
        }
        else if(curr2->key > curr3->key){
                pred1->vnext = curr3;
                pred3->vnext.store(curr3->vnext);
                curr3->vnext =curr1;
                pred1 = curr3;
                curr3 = pred3->vnext;
        }
       }
       if(curr2 ->vnext != NULL && curr3->vnext == NULL){
        while(curr2 ->vnext != NULL){
                pred1->vnext = curr2;
                pred2->vnext.store(curr2->vnext);
                curr2->vnext =curr1;
                pred1 = curr2;
                curr2 = pred2->vnext;
          }
        }
        if(curr2 ->vnext == NULL && curr3->vnext != NULL){
        while(curr3 ->vnext != NULL){
                pred1->vnext = curr3;
                pred3->vnext.store(curr3->vnext);
                curr3->vnext =curr1;
                pred1 = curr3;
                curr3 = pred3->vnext;
          }
        }
  }
 

// init of graph 
void  initGraph(int n, int NT,int tid){
 int i,j, e=0, v_;
     for( i=1;i<=n+1;i++){
         //v_ = rand()%n+1;
         insertinit(i,NT, tid);
     }
  for(i=1;i<=n;i= i+2){
         for( j=i+1; j<=n; j=j+2){
          int u = rand()%n +1;
          int v = rand()%n +1;
          if(u!=v){
          PutE(u,v,tid); e++;}
       }   
    } 
  cout<<"edges:"<<e<<endl;
 } 
 
 // init of graph from file
void initGraphFromFile(string file, int NT, int tid){
  ifstream cinn(file);
  long n,m;
  int u, v, v_;
  cinn>>n>>m;
  //cout<<n<<" "<<m<<endl;
  
  int i,j, e=0;
  //cin>>i;
  for(i=1;i<=n;i++){
        //v_ = rand()%n;
        insertinit(i,NT, tid);
   }
   //cout<<
  for(j=1; j<=m; j = j+1){
	cinn>>u>>v;
	PutE1(u,v, tid); 
	e++;
      }   
  //cout<<"Edge:"<<e<<endl;
} 


void PrintHash(){

        HNode *tmp = Head->next.load();
     
 for(int i=0; i<Head->next.load()->size; i++){
   cout<<"\nbuckets ["<<i<<"]:"<<tmp->buckets[i].ok<<" "<<tmp->buckets[i].size;
   FSet  fsetnode = tmp->buckets[i];
   //VNode * curr = fsetnode.head;
    vlist_t *temp1 = fsetnode.head;
	elist_t *temp2;
	while(temp1 != NULL){
		if(temp1->key == INT_MAX || temp1->key == INT_MIN)
		  cout << temp1->key<<endl;
		else{
		cout << temp1->key;//<<" "<<temp1->ecount;
		temp2 = (elist_t*)get_unmarked_ref((long)temp1->enext.load());
		while(temp2 != NULL)
		{
			cout << "["<<temp2->key; // print edge node
			if(temp2->key != INT_MAX && temp2->key != INT_MIN) 
			  cout<<","<<(temp2->pointv.load())->key<<"]";//<<temp2->wt<<"]"; // print it's vertex node
			temp2 = (elist_t*)get_unmarked_ref((long)temp2->enext.load());
		}
		}
		cout << endl;
		temp1 = (vlist_t*)get_unmarked_ref((long)temp1->vnext.load());
	   }
   }
   cout<<"\nsize,used && new_size, pred: "<< tmp->size<<" "<<tmp->used<<" "<< tmp->new_resize<<" "<<tmp->pred.load()<<endl;
 }
 
};  

