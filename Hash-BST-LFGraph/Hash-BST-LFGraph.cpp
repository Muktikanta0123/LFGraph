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

 ofstream coutt("lf_hash.txt");
 

inline int is_marked_ref(long i){
  return (int) (i & 0x1L);
}

inline long get_unmarked_ref(long w){
  return w & ~0x1L;
}

inline long get_marked_ref(long w){
  return w | 0x1L;
}
#define ONE 0x00000001
#define TWO 0x00000002
#define THREE 0x00000003

void *SET_FLAG(void *ptr, int state)
{
	ptr = (void *) ((uintptr_t)ptr | (uintptr_t)state);
	return ptr;
}

int GET_FLAG(void *ptr)
{
	int flag = ((uintptr_t) ptr & (uintptr_t)THREE);
	return flag;
}

void *UNFLAG(void *ptr)
{
	ptr = (void *)((uintptr_t)ptr & ~(uintptr_t)THREE);
	return ptr;
}

void *SET_NULL(void *ptr)
{
	ptr = (void *) ((uintptr_t)ptr | (uintptr_t)ONE);
	return ptr;
}

bool IS_NULL(void *ptr)
{
	int val = ((uintptr_t)ptr & (uintptr_t)ONE);
	if(val == 1) {
		return true;
	}
	return false;
}

 
int i;
enum inld{LEFT, RIGHT};

enum flag_type {
	NONE = 0,
	MARK,
	CHILDCAS,
	RELOCATE
};

enum find_result_type {
	ABORT = 0,
	NOTFOUND_L,
	NOTFOUND_R,
	FOUND
};

enum operation_state {
	ONGOING = 0,
	SUCCESSFUL,
	FAILED
};

typedef struct Lock_Free_BST_ENode {
	int volatile key;
	//atomic <double> wt;// weight of the edge
	void * volatile op;
	struct Lock_Free_BST_ENode * volatile left;
	struct Lock_Free_BST_ENode *volatile right;
	atomic<struct VNode *>pointv; // pointer to its vertex	
} ENode;

typedef struct EChild_Compare_And_Swap_Operation {
	bool is_left;
	ENode * volatile expected;
	ENode * volatile update;
} EChild_CAS_OP;

typedef struct ERelocate_Operation {
	int volatile state;
	ENode * volatile dest;
	void *dest_op;
	int remove_key;
	int replace_key;
} ERelocate_OP;

// VNode structure
typedef struct VNode{
	int key; // immutable key field
	//atomic<double> wt; // weight of the vertex
	atomic<struct VNode *>vnext; // pointer to the next VNode
	//atomic<struct ENode *>enext; // pointer to the EHead
	ENode *base_root;// = NULL;
        //std::vector<ENode *> *rlist;
	atomic <int> ecount; // counter for edge operations
	
}vlist_t;


int MAX_THREADS;


int *VIntp,*EIntp;
int *VActul,*EActul;

long *edgeCount;


int napp=0;
int th = 8;
enum OPType {
  DATA  = 0,
  DEAD = 1,
  INS = 2,
  DEL = 3,
  FREEZE = 4
};
// FSetNode structure
typedef struct FSetnode{
        bool ok;
        int size;
        //atomic<struct VNode *>head;	
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
        int size;
        atomic<HNode *>pred;
        int used;
        int new_resize;
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
  
  void freeHash(HNode *t){
  HNode  *head = t;
  VNode *bkt, *bktnext;
    for(int i=0; i<t->size; i++){
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
  
  void initHNode(HNode *t, int size){
   //t = new HNode;//[size];
   t->buckets = new FSet[size];
   for(int i=0; i<size; i++){
      t->buckets[i].head = initFSet();// = new Fset;//[size];
      t->buckets[i].ok = true;
      t->buckets[i].size = 0;
      }
   //newbuckets = initFSet();
   //head->ok
   t->size = size;
   t->used = 0;
   t->pred = NULL;
  }
  
  ENode* createE(int key){
          ENode * newe = (ENode*) malloc(sizeof(ENode));
          //assert(newe != NULL);
          newe ->key = key;
          //newe ->wt.store(wt);
         newe->op = NULL;
	newe->pointv.store(NULL);
	
	newe->left = (ENode *) SET_NULL(NULL);
	newe->right = (ENode *) SET_NULL(NULL);
         return newe;
        }
 vlist_t* createV(int key, int n){
          vlist_t * newv = (vlist_t*) malloc(sizeof(vlist_t));
          newv ->key = key;
          newv->base_root = createE(-1);
          newv ->vnext.store(NULL);
          newv->ecount.store(0); // init ecounter to zero
         return newv;
 }
void PrintResize(){
   cout<<"resize:"<<Head->ct<<endl;
}
 // Find pred and curr for VNode(key)     
void locateVPlus(vlist_t *startV, vlist_t ** n1, vlist_t ** n2, int key){
       vlist_t *succv, *currv, *predv;
      retry:
       while(true){
        predv = startV;
        currv = (vlist_t *) get_unmarked_ref((long)predv->vnext.load());// predv->vnext.load();
        while(true){
         succv = currv->vnext.load();
         while(currv->vnext.load() != NULL && is_marked_ref((long) succv) && currv->key < key ){ 
           if(!predv->vnext.compare_exchange_strong(currv, (vlist_t *) get_unmarked_ref((long)succv), memory_order_seq_cst))
           goto retry;
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
 bool PutV(VNode *vHead, int key, int n, int tid){
      vlist_t* predv, *currv;
      while(true){
        locateVPlus(vHead, &predv, &currv, key); // find the location, <pred, curr>
        if(is_marked_ref((long) predv->vnext.load())){
          continue;
        }
        
        if(currv->key == key){
        
           return false;//(char*) vp; // key already present
          
        }         
        else{
           vlist_t *newv = createV(key,n); // create a new vertex node
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
        	locateVPlus(vHead, &predv, &currv, key);
		if(currv->key != key)
			return false;//(char*) vntp; // key is not present
		succv = currv->vnext.load(); 
		if(!is_marked_ref((long) succv)){
		  if(!currv->vnext.compare_exchange_strong(succv, (vlist_t*)get_marked_ref((long)succv), memory_order_seq_cst)) // logical deletion
		    continue;
		   if(predv->vnext.compare_exchange_strong(currv, succv, memory_order_seq_cst)){ // physical deletion
                       break;
        	}
        	}	
	} 
   return true;//(char*) vr;
}
void helpE(ENode *pred, void *pred_op, ENode *curr, void *curr_op, VNode *curr1, int thread_num)
{
	if(GET_FLAG(curr_op) == CHILDCAS) {
		helpEChildCAS( ( (EChild_CAS_OP *) UNFLAG(curr_op) ), curr, curr1, thread_num);
	}
	else if(GET_FLAG(curr_op) == RELOCATE) {
		helpERelocate( (ERelocate_OP *) UNFLAG(curr_op), pred, pred_op, curr, curr1, thread_num);
	}
	else if(GET_FLAG(curr_op) == MARK) {
		helpEMarked(pred, pred_op, curr, curr1, thread_num);
	}
}
void helpEChildCAS(EChild_CAS_OP *op, ENode *dest, VNode *curr, int thread_num)
{
	ENode **address = op->is_left ? (ENode **)&dest->left : (ENode **)&dest->right;
	__sync_bool_compare_and_swap(address, op->expected, op->update);
	__sync_bool_compare_and_swap(&dest->op, SET_FLAG(op, CHILDCAS), SET_FLAG(op, NONE));
}

void helpEMarked(ENode *pred, void *pred_op, ENode *curr, VNode *curr1,int thread_num)
{
	ENode *new_ref;
	EChild_CAS_OP *cas_op;
	if(IS_NULL(curr->left)) {
		
		if(IS_NULL(curr->right)) {
			new_ref = (ENode *) SET_NULL((void *) curr);
		}
		else {
			new_ref = curr->right;
		}
	}
	else {
		new_ref = curr->left;
	}


	cas_op = new EChild_CAS_OP;
	cas_op->is_left = (curr == pred->left);
	cas_op->expected = curr;
	cas_op->update = new_ref;

	if(__sync_bool_compare_and_swap(&pred->op, pred_op, SET_FLAG((void *) cas_op, CHILDCAS))) {
		helpEChildCAS(cas_op, pred, curr1, thread_num);
	} else {
		delete cas_op;

	}

}
bool  helpERelocate(ERelocate_OP *op, ENode *pred, void *pred_op, ENode *curr, VNode *curr1, int thread_num)
{
	int seen_state = op->state;
	

	if(seen_state == ONGOING) {
		
	void *seen_op = __sync_val_compare_and_swap(&op->dest->op, op->dest_op, SET_FLAG((void *) op, RELOCATE));
	if( (seen_op == op->dest_op) || (seen_op == SET_FLAG((void *) op, RELOCATE)) ) {
			__sync_bool_compare_and_swap(&op->state, ONGOING, SUCCESSFUL);
			seen_state = SUCCESSFUL;
		}
		else {
			seen_state = __sync_val_compare_and_swap(&op->state, ONGOING, FAILED);
		}

	}
	
	if(seen_state == SUCCESSFUL) {
		__sync_bool_compare_and_swap(&op->dest->key, op->remove_key, op->replace_key);
		__sync_bool_compare_and_swap(&op->dest->op, SET_FLAG((void *) op, RELOCATE), SET_FLAG((void *) op, NONE));
	}
	
	bool result = (seen_state == SUCCESSFUL);

	if(op->dest == curr) {
		return result;
	}

	__sync_bool_compare_and_swap(&curr->op, SET_FLAG((void *) op, RELOCATE), SET_FLAG((void *) op, result ? MARK : NONE));

	if(result) {
		if(op->dest == pred) {
			pred_op = SET_FLAG((void *) op, NONE);
		}

		helpEMarked(pred, pred_op, curr, curr1, thread_num);
	}

	return result;
}



void locateV_old(vlist_t ** n1, int key) {
HNode *t = Head->next.load();
    FSet curr_bucket = Head->next.load()->buckets[key % t->size];
    if(!curr_bucket.ok) {
        HNode *prev_hnode = t->pred.load();
        if(prev_hnode != NULL) {
            curr_bucket = prev_hnode->buckets[key % prev_hnode->size];
        }
    }
    VNode * curr_bucket_head = curr_bucket.head;
    vlist_t *currv = curr_bucket_head->vnext;
    while(currv->vnext.load() && currv->key < key){
           currv =  (vlist_t *) get_unmarked_ref((long)currv->vnext.load());
          }
          
      (* n1) = currv; 
      return;   
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
char*  ContainsE(int key1, int key2, int thread_num){
	ENode *pred, *curr;
	void *pred_op, *curr_op;
	VNode *pred1, *curr1;
	void *pred_op1, *curr_op1;
	VNode *pred2, *curr2;
	void *pred_op2, *curr_op2;
	bool flag = ContainsVPlus(&curr1, &curr2, key1, key2);
               if(flag == false){             
                  return (char*)vntp; // either of the vertex is not present
               }   
              if(is_marked_ref((long) curr1->vnext.load()) || is_marked_ref((long) curr2->vnext.load()) )
                   return (char*)vntp;
	        
		int result = findE(key2, pred, pred_op, curr, curr_op, curr1->base_root, curr1, thread_num);

		if(result == FOUND) 
		{
			return (char*)ep;
		}
		else
		  return (char*) entp;
		
}

int  findE(int key, ENode *&pred, void *&pred_op, ENode *&curr, void *&curr_op, ENode *auxRoot, VNode *curr1, int thread_num)
{
	int result, curr_key;
	ENode *next, *last_right;
	void *last_right_op;

retry:

	result = NOTFOUND_R;
	curr = auxRoot;
	curr_op = curr->op;
	if(GET_FLAG(curr_op) != NONE) {
		if(auxRoot == curr1->base_root) {
			helpEChildCAS(((EChild_CAS_OP *)UNFLAG(curr_op)), curr, curr1, thread_num);
			goto retry;
		}
		else {
			return ABORT;
		}
	}
	next = curr->right;
	last_right = curr;
	last_right_op = curr_op;

	while(!IS_NULL(next) && next != NULL) {
		pred = curr;
		pred_op = curr_op;
		curr = next;
		curr_op = curr->op;
		
		if(GET_FLAG(curr_op) != NONE) {
			helpE(pred, pred_op, curr, curr_op, curr1, thread_num);
			goto retry;
		}
		
		curr_key = curr->key;

		if(key < curr_key) {
			result = NOTFOUND_L;
			next = curr->left;
		}
		else if(key > curr_key) {
			result = NOTFOUND_R;
			next = curr->right;
			last_right = curr;
			last_right_op = curr_op;
		}
		else {
			result = FOUND;
			break;
		}
	}
	if( (result != FOUND) && (last_right_op != last_right->op) ) {
		goto retry;
	}
	if(curr_op != curr->op) {
		goto retry;
	}
	return result;
}


bool ContainsVPlus(vlist_t ** n1, vlist_t ** n2, int key1, int key2){
     vlist_t *curr1, *pred1, *curr2, *pred2;
     locateV( &curr1, key1); 
     if((!curr1->vnext.load()) || curr1->key != key1)
	return false; 
     locateV(&curr2, key2); 
     if((!curr2->vnext.load()) || curr2->key != key2)
        return false; 
     (*n1) = curr1; 
     (*n2) = curr2; 
    return true;    
 }	
char* PutE(int key1, int key2, int thread_num)
{
	ENode *pred, *curr, *newNode;
	void *pred_op, *curr_op;
	VNode *pred1, *curr1;
	void *pred_op1, *curr_op1;
	VNode *pred2, *curr2;
	void *pred_op2, *curr_op2;
	EChild_CAS_OP *cas_op;
	bool flag = ContainsVPlus(&curr1, &curr2, key1, key2);
               if(flag == false){             
                  return (char*)vntp; // either of the vertex is not present
               }   
               while(true){
              if(is_marked_ref((long) curr1->vnext.load()) || is_marked_ref((long) curr2->vnext.load()) )
                   return (char*)vntp;

		int result = findE(key2, pred, pred_op, curr, curr_op, curr1->base_root, curr1, thread_num);

		if(result == FOUND) 
		{
			return (char*) ep;
		}

		newNode = createE(key2);
                newNode->pointv.store(curr2);
		bool is_left = (result == NOTFOUND_L);

		ENode *old = is_left ? curr->left : curr->right;
		cas_op = new EChild_CAS_OP;
		cas_op->is_left = is_left;
		cas_op->expected = old;
		cas_op->update = newNode;
		if(__sync_bool_compare_and_swap(&curr->op, curr_op, SET_FLAG((void *) cas_op, CHILDCAS))) {
			helpEChildCAS(cas_op, curr, curr1, thread_num);
			curr1->ecount.fetch_add(1, memory_order_relaxed);
			return (char*) eadd;
		} else {
			delete newNode;
			delete cas_op;
		}
	}
}
char* RemoveE(int key1, int key2, int thread_num)
{
	ENode *pred, *curr, *replace;
	void *pred_op, *curr_op, *replace_op; 
	ERelocate_OP *reloc_op;
	
	VNode *pred1, *curr1;
	void *pred_op1, *curr_op1;
	VNode *pred2, *curr2;
	void *pred_op2, *curr_op2;
	bool flag = ContainsVPlus(&curr1, &curr2, key1, key2);
               if(flag == false){             
                  return (char*)vntp; // either of the vertex is not present
               }   
               while(true){
              if(is_marked_ref((long) curr1->vnext.load()) || is_marked_ref((long) curr2->vnext.load()) )
                   return (char*)vntp;

		if(findE(key2, pred, pred_op, curr, curr_op, curr1->base_root, curr1,  thread_num) != FOUND) {
			return (char*) entp;
		}
		if( IS_NULL(curr->right) || IS_NULL(curr->left) ) {
			if(__sync_bool_compare_and_swap(&curr->op, curr_op, SET_FLAG(curr_op, MARK))) {
				helpEMarked(pred, pred_op, curr, curr1, thread_num);
				curr1->ecount.fetch_add(1, memory_order_relaxed);
				return (char*) er;
			}
		}
		else {
			if( (findE(key2, pred, pred_op, replace, replace_op, curr, curr1, thread_num) == ABORT) || (curr->op != curr_op) ) {
				continue;
			}
			reloc_op = new ERelocate_OP;
			reloc_op->state = ONGOING;
			reloc_op->dest = curr;
			reloc_op->dest_op = curr_op;
			reloc_op->remove_key = key2;
			reloc_op->replace_key = replace->key;
			if(__sync_bool_compare_and_swap(&replace->op, replace_op, SET_FLAG((void *) reloc_op, RELOCATE))) {
				if(helpERelocate(reloc_op, pred, pred_op, replace, curr1, thread_num)) {
				curr1->ecount.fetch_add(1, memory_order_relaxed);
					return (char*) er;
				} else {
					delete reloc_op;
				}
			} else {
				delete reloc_op;
			}
		}
	}
}

void freeze(FSet *b){
     if(b->ok == true){
       b->ok = false;
     }
   //return
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
    PutV(b.head, key, NT, tid);  
    t->used++;  
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

FSet getBucket(int key) {
    return Head->next.load()->buckets[key];
}
 
//bool getResponse( FSetOp *op){
 // return op->response;
//}
 
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
void  initGraph(int n, int NT, int tid){
 int i,j, e=0, v_;
     for( i=0;i<n;i++){
         //v_ = rand()%n+1;
         insertinit(i,NT,tid);
     }
  for(i=0;i<n;i= i+2){
         for( j=i+1; j<n; j=j+2){
          int u = rand()%n;
          int v = rand()%n;
          if(u!=v){
          PutE(u,v,2); e++;}
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
  cout<<n<<" "<<m<<endl;
  
  int i,j, e=0;
  //cin>>i;
  for(i=1;i<=n;i++){
        //v_ = rand()%n;
        insertinit(i,NT, tid);
   }
   //cout<<
  for(j=1; j<=m; j = j+1){
	cinn>>u>>v;
	PutE(u,v,tid); 
	e++;
      }   
  cout<<"Edge:"<<e<<endl;
} 
void  InOrderVisitE(ENode *base_root) {
    stack<ENode*> s;
   ENode * curr = base_root;//->next.load();
    curr = curr->right; 
   while (!IS_NULL(curr)  || s.empty() == false) 
    { 
    //cout<<curr->key<< curr->left->key << curr->right->key;
   // cin>>i;
    while (!IS_NULL(curr)) {
       s.push(curr);
       curr = curr->left;
       //cout<<curr->key;
      }
     curr = s.top(); 
     s.pop(); 
     cout << curr->key << " ";  
     curr = curr->right; 
    } 
}
void PrintHash(){
HNode *tmp = Head->next.load();
 for(int i=0; i<Head->next.load()->size; i++){
   cout<<"\nbuckets ["<<i<<"]:"<<tmp->buckets[i].ok<<" "<<tmp->buckets[i].size;
   FSet  fsetnode = tmp->buckets[i];
   //VNode * curr = fsetnode.head;
    vlist_t *temp1 = fsetnode.head;
	ENode *temp2;
	while(temp1 != NULL){
		if(temp1->key == INT_MAX || temp1->key == INT_MIN)
		  cout << temp1->key<<endl;
		else{
		cout << temp1->key;//<<" "<<temp1->ecount;
		InOrderVisitE(temp1->base_root);
		//cout << endl;
	        }	
		temp1 = (vlist_t*)get_unmarked_ref((long)temp1->vnext.load());
	}   
   }
   cout<<"\nsize,used && new_size, pred: "<< tmp->size<<" "<<tmp->used<<" "<< tmp->new_resize<<" "<<tmp->pred.load()<<endl;
 }
 
void PrintHash2(){

        HNode *tmp = Head->next.load();
     
 for(int i=0; i<Head->next.load()->size; i++){
   cout<<"\nbuckets ["<<i<<"]:"<<tmp->buckets[i].ok<<" "<<tmp->buckets[i].size;
   FSet  fsetnode = tmp->buckets[i];
   VNode * curr = fsetnode.head;
    while(curr){
      cout<<" "<<curr->key<<"->";
      curr = curr->vnext;
    }
   }
   cout<<"\nsize,used && new_size, pred: "<< tmp->size<<" "<<tmp->used<<" "<< tmp->new_resize<<" "<<tmp->pred.load()<<endl;
 }
 void PrintHash1(HNode *tmp){

        //HNode *tmp = head;
     
 for(int i=0; i<tmp->size; i++){
   cout<<"\nbuckets ["<<i<<"]:"<<tmp->buckets[i].ok<<" "<<tmp->buckets[i].size;
   FSet  fsetnode = tmp->buckets[i];
   VNode * curr = fsetnode.head;
    while(curr){
      cout<<" "<<curr->key<<"->";
      curr = curr->vnext;
    }
   }
   cout<<"\nsize,used && new_size: "<< tmp->size<<" "<<tmp->used<<" "<< tmp->new_resize<<endl;
 }
   

};  



