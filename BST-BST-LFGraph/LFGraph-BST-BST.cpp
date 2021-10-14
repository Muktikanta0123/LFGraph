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
#include<stack>
#include<queue>

#define TRUE 1
#define FALSE 0
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

inline long unset_mark(long i){
  i &= ~0x1L;
  return i;
}

inline long set_mark(long i){
  i |= 0x1L;
  return i;
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
	void * volatile op;
	struct Lock_Free_BST_ENode * volatile left;
	struct Lock_Free_BST_ENode *volatile right;
} LF_BST_ENode;

typedef struct EChild_Compare_And_Swap_Operation {
	bool is_left;
	LF_BST_ENode * volatile expected;
	LF_BST_ENode * volatile update;
} EChild_CAS_OP;

typedef struct ERelocate_Operation {
	int volatile state;
	LF_BST_ENode * volatile dest;
	void *dest_op;
	int remove_key;
	int replace_key;
} ERelocate_OP;

typedef struct Lock_Free_BST_Node {
	int volatile key;
	void * volatile op;
	struct Lock_Free_BST_Node * volatile left;
	struct Lock_Free_BST_Node *volatile right;
	LF_BST_ENode *base_root;// = NULL;
        std::vector<LF_BST_ENode *> *rlist;
} LF_BST_Node;

typedef struct Child_Compare_And_Swap_Operation {
	bool is_left;
	LF_BST_Node * volatile expected;
	LF_BST_Node * volatile update;
} Child_CAS_OP;

typedef struct Relocate_Operation {
	int volatile state;
	LF_BST_Node * volatile dest;
	void *dest_op;
	int remove_key;
	int replace_key;
} Relocate_OP;




int MAX_THREADS;

LF_BST_Node *base_root;// = NULL;
std::vector<LF_BST_Node *> *rlist;
void test_ptr_functions();


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


typedef struct ENode{
	int key; // immutable key field
	//atomic <double> wt;// weight of the edge
	atomic<struct BSTNode *>pointv; // pointer to its vertex
	atomic<struct ENode *>parent;
  	atomic<struct ENode *>left;
	atomic<struct ENode *>right;
}elist_t;

typedef struct ERootNode{
        int val;
  	atomic<ENode *>next;
	
  }RootENode;

typedef struct BSTNode{
        int val;
  	atomic<struct BSTNode *>parent;
  	atomic<struct BSTNode *>left;
	atomic<struct BSTNode *>right;
	atomic<struct ERootNode *>next;
	atomic <int> ecount;
  }BNode;
typedef struct BSTRootNode{
        int val;
  	atomic<BNode *>next;
	
  }RootNode;

class LFGraph{
public:
        void init(int n);
        LF_BST_Node *create_LF_node(int key, int n);
        void addV(int key, int thread_num, int n);
        int findV(int key, LF_BST_Node *&pred, void *&pred_op, LF_BST_Node *&curr, void *&curr_op, LF_BST_Node *auxRoot, int thread_num);
        bool removeV(int key, int thread_num);
        char* ContainsV(int key, int thread_num);


        void helpV(LF_BST_Node *pred, void *pred_op, LF_BST_Node *curr, void *curr_op, int thread_num);
        void helpVChildCAS(Child_CAS_OP *op, LF_BST_Node *dest, int thread_num);
        void helpVMarked(LF_BST_Node *pred, void *pred_op, LF_BST_Node *curr, int thread_num);
        void helpVRelocateMarked(Relocate_OP *op, LF_BST_Node *pred, void *pred_op, LF_BST_Node *curr, int thread_num);
        bool helpVRelocate(Relocate_OP *op, LF_BST_Node *pred, void *pred_op, LF_BST_Node *curr, int thread_num);


        LF_BST_ENode *create_LF_Enode(int key);
        char* addE(int key1, int key2, int thread_num);
        int findE(int key, LF_BST_ENode *&pred, void *&pred_op, LF_BST_ENode *&curr, void *&curr_op, LF_BST_ENode *auxRoot, LF_BST_Node *curr1, int thread_num);
        char* removeE(int key1, int key2, int thread_num);
        char* ContainsE(int key1, int key2, int thread_num);

        void helpE(LF_BST_ENode *pred, void *pred_op, LF_BST_ENode *curr, void *curr_op, LF_BST_Node *curr1, int thread_num);
        void helpEChildCAS(EChild_CAS_OP *op, LF_BST_ENode *dest, LF_BST_Node *curr1, int thread_num);
        void helpEMarked(LF_BST_ENode *pred, void *pred_op, LF_BST_ENode *curr, LF_BST_Node *curr1, int thread_num);
        void helpERelocateMarked(ERelocate_OP *op, LF_BST_ENode *pred, void *pred_op, LF_BST_ENode *curr, int thread_num);
        bool helpERelocate(ERelocate_OP *op, LF_BST_ENode *pred, void *pred_op, LF_BST_ENode *curr, LF_BST_Node *curr1, int thread_num);
        void InOrderVisitGraph();
        void InOrderVisit();
        void InOrderVisitE(LF_BST_ENode *base_root);
};
    
    
LF_BST_ENode * LFGraph :: create_LF_Enode(int key)
{
	LF_BST_ENode *newNode = new LF_BST_ENode;
	newNode->key = key;
	newNode->op = NULL;
	newNode->left = (LF_BST_ENode *) SET_NULL(NULL);
	newNode->right = (LF_BST_ENode *) SET_NULL(NULL);
	return newNode;
}
    
LF_BST_Node * LFGraph :: create_LF_node(int key, int n)
{
	LF_BST_Node *newNode = new LF_BST_Node;
	newNode->key = key;
	newNode->op = NULL;
	newNode->base_root = create_LF_Enode(-1);
        newNode->rlist = new  std::vector<LF_BST_ENode *>[n];
	newNode->left = (LF_BST_Node *) SET_NULL(NULL);
	newNode->right = (LF_BST_Node *) SET_NULL(NULL);
	
	return newNode;
}
void LFGraph :: init(int n){
 base_root = create_LF_node(-1, n);
 rlist = new  std::vector<LF_BST_Node *>[n];
}
void LFGraph :: helpV(LF_BST_Node *pred, void *pred_op, LF_BST_Node *curr, void *curr_op, int thread_num)
{
	if(GET_FLAG(curr_op) == CHILDCAS) {
		helpVChildCAS( ( (Child_CAS_OP *) UNFLAG(curr_op) ), curr, thread_num);
	}
	else if(GET_FLAG(curr_op) == RELOCATE) {
		helpVRelocate( (Relocate_OP *) UNFLAG(curr_op), pred, pred_op, curr, thread_num);
	}
	else if(GET_FLAG(curr_op) == MARK) {
		helpVMarked(pred, pred_op, curr, thread_num);
	}
}

void LFGraph :: helpE(LF_BST_ENode *pred, void *pred_op, LF_BST_ENode *curr, void *curr_op, LF_BST_Node *curr1, int thread_num)
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


void LFGraph :: helpVChildCAS(Child_CAS_OP *op, LF_BST_Node *dest, int thread_num)
{
	LF_BST_Node **address = op->is_left ? (LF_BST_Node **)&dest->left : (LF_BST_Node **)&dest->right;
	if (__sync_bool_compare_and_swap(address, op->expected, op->update)) {

		if (UNFLAG(op->expected) != NULL) {
			std::vector<LF_BST_Node *>::iterator rlist_vec_itr;

			if (std::find(rlist[thread_num].begin(), rlist[thread_num].end(),
			    (LF_BST_Node *)UNFLAG(op->expected)) == rlist[thread_num].end()) {
				rlist[thread_num].push_back((LF_BST_Node *)UNFLAG(op->expected));
			}
		}

	
	}

	__sync_bool_compare_and_swap(&dest->op, SET_FLAG(op, CHILDCAS), SET_FLAG(op, NONE));
}


void LFGraph :: helpEChildCAS(EChild_CAS_OP *op, LF_BST_ENode *dest, LF_BST_Node *curr, int thread_num)
{
	LF_BST_ENode **address = op->is_left ? (LF_BST_ENode **)&dest->left : (LF_BST_ENode **)&dest->right;
	if (__sync_bool_compare_and_swap(address, op->expected, op->update)) {

		if (UNFLAG(op->expected) != NULL) {
			std::vector<LF_BST_ENode *>::iterator rlist_vec_itr;

			if (std::find(curr->rlist[thread_num].begin(), curr->rlist[thread_num].end(),
			    (LF_BST_ENode *)UNFLAG(op->expected)) == curr->rlist[thread_num].end()) {
				curr->rlist[thread_num].push_back((LF_BST_ENode *)UNFLAG(op->expected));
			}
		}

	
	}

	__sync_bool_compare_and_swap(&dest->op, SET_FLAG(op, CHILDCAS), SET_FLAG(op, NONE));
}



void LFGraph :: helpVMarked(LF_BST_Node *pred, void *pred_op, LF_BST_Node *curr, int thread_num)
{
	LF_BST_Node *new_ref;
	Child_CAS_OP *cas_op;

	if(IS_NULL(curr->left)) {
		
		if(IS_NULL(curr->right)) {
			new_ref = (LF_BST_Node *) SET_NULL((void *) curr);
		}
		else {
			new_ref = curr->right;
		}
	}
	else {
		new_ref = curr->left;
	}


	cas_op = new Child_CAS_OP;
	cas_op->is_left = (curr == pred->left);
	cas_op->expected = curr;
	cas_op->update = new_ref;

	if(__sync_bool_compare_and_swap(&pred->op, pred_op, SET_FLAG((void *) cas_op, CHILDCAS))) {
		helpVChildCAS(cas_op, pred, thread_num);
	} else {
		delete cas_op;

	}

}


void LFGraph :: helpEMarked(LF_BST_ENode *pred, void *pred_op, LF_BST_ENode *curr, LF_BST_Node *curr1,int thread_num)
{
	LF_BST_ENode *new_ref;
	EChild_CAS_OP *cas_op;


	if(IS_NULL(curr->left)) {
		
		if(IS_NULL(curr->right)) {
			new_ref = (LF_BST_ENode *) SET_NULL((void *) curr);
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

bool LFGraph :: helpVRelocate(Relocate_OP *op, LF_BST_Node *pred, void *pred_op, LF_BST_Node *curr, int thread_num)
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

		helpVMarked(pred, pred_op, curr, thread_num);
	}

	return result;
}

bool LFGraph :: helpERelocate(ERelocate_OP *op, LF_BST_ENode *pred, void *pred_op, LF_BST_ENode *curr, LF_BST_Node *curr1, int thread_num)
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

void LFGraph :: InOrderVisit() {
    stack<LF_BST_Node*> s;
    LF_BST_Node *curr = base_root;//->next.load();
    curr = curr->right; 
   while (!IS_NULL(curr)  || s.empty() == false) 
    { 

    while (!IS_NULL(curr)) {
       s.push(curr);
       curr = curr->left;
      }
     curr = s.top(); 
     s.pop(); 
     cout << curr->key << " ";  
     curr = curr->right; 
    } 
}

void LFGraph :: InOrderVisitE(LF_BST_ENode *base_root) {
    stack<LF_BST_ENode*> s;
   LF_BST_ENode * curr = base_root;//->next.load();
    curr = curr->right; 
   while (!IS_NULL(curr)  || s.empty() == false) 
    { 

    while (!IS_NULL(curr)) {
       s.push(curr);
       curr = curr->left;
      }
     curr = s.top(); 
     s.pop(); 
     cout << curr->key << " ";  
     curr = curr->right; 
    } 
}


void LFGraph :: InOrderVisitGraph() {
    stack<LF_BST_Node*> s;
    LF_BST_Node *curr = base_root;//->next.load();
    curr = curr->right; 
   while (!IS_NULL(curr)  || s.empty() == false) 
    { 

    while (!IS_NULL(curr)) {
       s.push(curr);
       curr = curr->left;

      }
     curr = s.top(); 
     s.pop(); 
     cout <<"\n"<< curr->key << "->{ "; 
      InOrderVisitE(curr->base_root);
      cout<<"}";
     curr = curr->right; 
    }   
}

int LFGraph :: findV(int key, LF_BST_Node *&pred, void *&pred_op, LF_BST_Node *&curr, void *&curr_op, LF_BST_Node *auxRoot, int thread_num)
{
	int result, curr_key;
	LF_BST_Node *next, *last_right;
	void *last_right_op;

retry:


	result = NOTFOUND_R;
	curr = auxRoot;
	curr_op = curr->op;

	if(GET_FLAG(curr_op) != NONE) {
		if(auxRoot == base_root) {
			helpVChildCAS(((Child_CAS_OP *)UNFLAG(curr_op)), curr, thread_num);
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
	
			helpV(pred, pred_op, curr, curr_op, thread_num);
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

char* LFGraph :: ContainsV(int key, int tid){
        LF_BST_Node *pred, *curr;
	void *pred_op, *curr_op;
	int result = findV(key, pred, pred_op, curr, curr_op, base_root, tid);
	if(result == FOUND) 
		{
			return (char*)vp;
		}
		else
		  return (char*) vntp;
}

int LFGraph :: findE(int key, LF_BST_ENode *&pred, void *&pred_op, LF_BST_ENode *&curr, void *&curr_op, LF_BST_ENode *auxRoot, LF_BST_Node *curr1, int thread_num)
{
	int result, curr_key;
	LF_BST_ENode *next, *last_right;
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

char* LFGraph ::  ContainsE(int key1, int key2, int thread_num){

	LF_BST_ENode *pred, *curr;
	void *pred_op, *curr_op;
	LF_BST_Node *pred1, *curr1;
	void *pred_op1, *curr_op1;
	LF_BST_Node *pred2, *curr2;
	void *pred_op2, *curr_op2;

	int result1, result2, result;
        result1 = findV(key1, pred1, pred_op1, curr1, curr_op1, base_root, thread_num);
        result2 = findV(key2, pred2, pred_op2, curr2, curr_op2, base_root, thread_num);

		if(result1 != FOUND || result2 != FOUND ) 
		{
			
			return (char*)vntp;
		}
		

	        if(GET_FLAG(curr_op1) == MARK || GET_FLAG(curr_op2) == MARK){
	        return (char*)vntp;
	        }
	        
		result = findE(key2, pred, pred_op, curr, curr_op, curr1->base_root, curr1, thread_num);

		if(result == FOUND) 
		{

			return (char*)ep;
		}
		else
		  return (char*) entp;
		
}
void LFGraph :: addV(int key, int thread_num, int n)
{
	LF_BST_Node *pred, *curr, *newNode;
	void *pred_op, *curr_op;
	Child_CAS_OP *cas_op;
	int result;

	while (true) {
		result = findV(key, pred, pred_op, curr, curr_op, base_root, thread_num);

		if(result == FOUND) 
		{
			return;
		}

		newNode = create_LF_node(key,n);

		bool is_left = (result == NOTFOUND_L);

		LF_BST_Node *old = is_left ? curr->left : curr->right;

		cas_op = new Child_CAS_OP;
		cas_op->is_left = is_left;
		cas_op->expected = old;
		cas_op->update = newNode;


		if(__sync_bool_compare_and_swap(&curr->op, curr_op, SET_FLAG((void *) cas_op, CHILDCAS))) {

			helpVChildCAS(cas_op, curr, thread_num);
			return;
		} else {
			delete newNode;
			delete cas_op;
		}
	}
}

char* LFGraph :: addE(int key1, int key2, int thread_num)
{
	LF_BST_ENode *pred, *curr, *newNode;
	void *pred_op, *curr_op;
	LF_BST_Node *pred1, *curr1;
	void *pred_op1, *curr_op1;
	LF_BST_Node *pred2, *curr2;
	void *pred_op2, *curr_op2;
	EChild_CAS_OP *cas_op;
	int result1, result2, result;
        result1 = findV(key1, pred1, pred_op1, curr1, curr_op1, base_root, thread_num);
        result2 = findV(key2, pred2, pred_op2, curr2, curr_op2, base_root, thread_num);

		if(result1 != FOUND || result2 != FOUND ) 
		{
			
			return (char*)vntp;
		}
		
	while (true) {
	        if(GET_FLAG(curr_op1) == MARK || GET_FLAG(curr_op2) == MARK){
	        return (char*)vntp;
	        }
	        

		 
		result = findE(key2, pred, pred_op, curr, curr_op, curr1->base_root, curr1, thread_num);

		if(result == FOUND) 
		{
			return (char*) ep;
		}


		newNode = create_LF_Enode(key2);


		bool is_left = (result == NOTFOUND_L);

		LF_BST_ENode *old = is_left ? curr->left : curr->right;

		cas_op = new EChild_CAS_OP;
		cas_op->is_left = is_left;
		cas_op->expected = old;
		cas_op->update = newNode;

		if(__sync_bool_compare_and_swap(&curr->op, curr_op, SET_FLAG((void *) cas_op, CHILDCAS))) {

			helpEChildCAS(cas_op, curr, curr1, thread_num);
			return (char*) eadd;
		} else {
			delete newNode;
			delete cas_op;
		}
	}
}

bool LFGraph :: removeV(int key,  int thread_num)
{
	LF_BST_Node *pred, *curr, *replace;
	void *pred_op, *curr_op, *replace_op; 
	Relocate_OP *reloc_op;

	while(true) {

		if(findV(key, pred, pred_op, curr, curr_op, base_root, thread_num) != FOUND) {
			return false;
		}

		if( IS_NULL(curr->right) || IS_NULL(curr->left) ) {
			if(__sync_bool_compare_and_swap(&curr->op, curr_op, SET_FLAG(curr_op, MARK))) {
				helpVMarked(pred, pred_op, curr, thread_num);
				return true;
			}
		}
		else {

			if( (findV(key, pred, pred_op, replace, replace_op, curr, thread_num) == ABORT) || (curr->op != curr_op) ) {
				continue;
			}

			reloc_op = new Relocate_OP;
			reloc_op->state = ONGOING;
			reloc_op->dest = curr;
			reloc_op->dest_op = curr_op;
			reloc_op->remove_key = key;
			reloc_op->replace_key = replace->key;

			if(__sync_bool_compare_and_swap(&replace->op, replace_op, SET_FLAG((void *) reloc_op, RELOCATE))) {
				if(helpVRelocate(reloc_op, pred, pred_op, replace, thread_num)) {
					return true;
				} else {
					delete reloc_op;
				}
			} else {
				delete reloc_op;
			}
		}
	}

	
	
}

char* LFGraph :: removeE(int key1, int key2, int thread_num)
{
	LF_BST_ENode *pred, *curr, *replace;
	void *pred_op, *curr_op, *replace_op; 
	ERelocate_OP *reloc_op;
	
	LF_BST_Node *pred1, *curr1;
	void *pred_op1, *curr_op1;
	LF_BST_Node *pred2, *curr2;
	void *pred_op2, *curr_op2;
	
	
	int result1, result2;
        result1 = findV(key1, pred1, pred_op1, curr1, curr_op1, base_root, thread_num);
        result2 = findV(key2, pred2, pred_op2, curr2, curr_op2, base_root, thread_num);

		if(result1 != FOUND || result2 != FOUND ) 
		{
			
			return (char*)vntp;
		}
		
	while (true) {
	        if(GET_FLAG(curr_op1) == MARK || GET_FLAG(curr_op2) == MARK){
	        return (char*)vntp;
	        }

		if(findE(key2, pred, pred_op, curr, curr_op, curr1->base_root, curr1,  thread_num) != FOUND) {
			return (char*) entp;
		}

		if( IS_NULL(curr->right) || IS_NULL(curr->left) ) {
			if(__sync_bool_compare_and_swap(&curr->op, curr_op, SET_FLAG(curr_op, MARK))) {
				helpEMarked(pred, pred_op, curr, curr1, thread_num);
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












