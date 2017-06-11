#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "main.h"
#include "lcgrand.h"

#define total_ras 100*100
#define number_of_preamble 30
#define num_ue 1000

typedef enum rar_e{
	conventional=1,
	ext_rar_2,
	ext_rar_3,
	ext_rar_4
}rar_t;

typedef enum events_e{
	event_ra_period,
	event_stop,
	num_normal_event
}events_t;

typedef struct ue_s{
	
    float arrival_time;
    int preamble_index;
    int is_active;
    int retransmit_counter;
    int backoff_counter;
    struct ue_s *next;
}ue_t;

typedef struct preamble_s{
    int num_selected_rar[4];
	ue_t *rar_ue_list[4];
}preamble_t;

//FILE *infile;
//FILE *outfile;


//  temp static configuration
float sim_time;
float stop_time;
float time_next_event[num_normal_event];
int next_event_type;
float mean_interarrival;
float ra_period = 0.01f;
rar_t rar_type;
ue_t ue_list[1000];
int max_retransmit = 6;
preamble_t preamble_table[30];
int failed=0, attempt=0, success=0, collide=0, once_attempt_success=0, retransmit=0, once_attempt_collide=0, trial=0, ras=0;
int back_off_window_size = 20;

int main(int argc, char *argv[]){
	mean_interarrival = 0.05f;
	rar_type = ext_rar_4;
    initialize();
	    
	do{
	    timing();
	        
	    switch(next_event_type){
	        case event_ra_period:
                ra_procedure();
	            ue_backoff_process();
	            break;
	        case event_stop:
	            report();
				break;
	        default:
	            ue_arrival(next_event_type);
	            break;
	    }
	}while(event_stop != next_event_type);
	
	printf("finished");
	return 0;
}

float exponetial(float mean){
    return -mean * log(lcgrand(1));
}

void ue_backoff_process(void){
    int i;
    for(i=0;i<num_ue;++i){
        if(ue_list[i].is_active == 0x1){
            if(ue_list[i].backoff_counter == 0x0){
                ue_selected_preamble(i);
            }else{
                ue_list[i].backoff_counter -= 0x1;
            }
        }
    }
}

void ra_procedure(void){
	
    int i;
    int rar;
    int collide_counter=0;
    ue_t *iterator, *iterator1;
    
	static int debug = 0;
    ++ras;
    
    // RAR process
    for(i=0;i<30;++i){
        for(rar=0;rar<4;++rar){
            if(preamble_table[i].num_selected_rar[rar] > 1){
                //  collision
                collide += preamble_table[i].num_selected_rar[rar];
                iterator = preamble_table[i].rar_ue_list[rar];
                while((ue_t *)0 != iterator){
                    if(iterator->retransmit_counter == 0){
                        once_attempt_collide += 1;
                    }
                    iterator = iterator->next;
                }
                
                iterator = preamble_table[i].rar_ue_list[rar];
                while((ue_t *)0 != iterator){
                    if(iterator->retransmit_counter >= max_retransmit){
                        //  failed
                        ++failed;
                        iterator->retransmit_counter = 0;
                        iterator->backoff_counter = 0;
                        iterator->is_active = 0x0;
                        iterator->arrival_time = sim_time + exponetial(mean_interarrival);
                        
                    }else{
                        ++retransmit;
                        //  retransmit
                        iterator->retransmit_counter += 1;
                        //  uniform backoff
                        iterator->backoff_counter = (int)(back_off_window_size*lcgrand(3+collide_counter));
                        ++collide_counter;
                    }
                    iterator1 = iterator;
                    iterator = iterator->next;
                    iterator1->next = (ue_t *)0;
                }
            }else if(preamble_table[i].num_selected_rar[rar] == 1){
                //  success
                
                ++success;
                
                if(preamble_table[i].rar_ue_list[rar]->retransmit_counter == 0){
                    once_attempt_success+=1;
                }
                
                preamble_table[i].rar_ue_list[rar]->is_active = 0x0;
                preamble_table[i].rar_ue_list[rar]->retransmit_counter = 0x0;
                preamble_table[i].rar_ue_list[rar]->backoff_counter = 0;
                preamble_table[i].rar_ue_list[rar]->arrival_time = sim_time + exponetial(mean_interarrival);
                preamble_table[i].rar_ue_list[rar]->next = (ue_t *)0;
                
            }
            // clear rar table
            preamble_table[i].rar_ue_list[rar] = (ue_t *)0;
            preamble_table[i].num_selected_rar[rar] = 0;
        }
    }
    time_next_event[event_ra_period] = sim_time + ra_period;
}




void ue_arrival(int next_event_type){
    int ue_id = next_event_type - num_normal_event;
    ++attempt;
    ue_selected_preamble(ue_id);
}


void ue_selected_preamble(int ue_id){
    ue_t *iterator2;
    int preamble_index;
    int rar_index;
    
    ++trial;
    
    preamble_index = (int)(number_of_preamble*lcgrand(2));
    rar_index = (int)(4*lcgrand(3));
    ue_list[ue_id].is_active = 0x1;
    ue_list[ue_id].preamble_index = preamble_index;
    
    //  choose RAR
    preamble_table[preamble_index].num_selected_rar[rar_index] += 1;
    if( (ue_t *)0 == preamble_table[preamble_index].rar_ue_list[rar_index] ){
        preamble_table[preamble_index].rar_ue_list[rar_index] = &ue_list[ue_id];
		ue_list[ue_id].next = (ue_t *)0;
    }else{
        iterator2 = preamble_table[preamble_index].rar_ue_list[rar_index];
        
        while( (ue_t *)0 != iterator2->next ){
            iterator2 = iterator2->next;
        }
        iterator2->next = &ue_list[ue_id];
        ue_list[ue_id].next = (ue_t *)0;
    }
}

void initialize(void){
    int i, j;
    sim_time = 0.0f;
    failed = 0;
    
    //ue_list = (ue_t *)malloc(num_ue*sizeof(ue_t));
    time_next_event[event_ra_period] = sim_time + ra_period;

    time_next_event[event_stop] = stop_time;
    for(i=0;i<num_ue;++i){
        ue_list[i].is_active = 0;
        ue_list[i].backoff_counter = 0;
        ue_list[i].retransmit_counter = 0;
        ue_list[i].preamble_index = 0;
        ue_list[i].arrival_time = sim_time + exponetial(mean_interarrival);
        ue_list[i].next = (ue_t *)0;
    }
    for(i=0;i<30;++i){
    	for(j=0;j<4;++j){
    		preamble_table[i].num_selected_rar[j] = 0;
    		preamble_table[i].rar_ue_list[j] = (ue_t *)0;
		}
	}
}

void timing(void){ 
    int i;
    float min_time_next_event = time_next_event[event_ra_period];
    next_event_type = event_ra_period;
    
    for(i=0;i<num_ue;++i){
        if( 0x0 == ue_list[i].is_active){
            if(ue_list[i].arrival_time < min_time_next_event){
                min_time_next_event = ue_list[i].arrival_time;
                next_event_type = num_normal_event+i;
            }
        }
        
        //  TODO priority queue, using heap, implemented by array
        //	O(1), only need to check the root of heap 
    }
    
    if(ras >= total_ras){
        next_event_type = event_stop;
        return;
    }
    
    sim_time = min_time_next_event;
}

void report(void){ 
    int num_ras = total_ras;//(int)(stop_time / ra_period);
    float avg_num_attempt = (float)attempt/num_ras;
    float avg_num_success = (float)success/num_ras;
    float avg_num_collide = (float)collide/num_ras;
    float avg_num_once_success = (float)once_attempt_success/num_ras;
    float avg_num_once_collide = (float)once_attempt_collide/num_ras;
    printf("----------------\nUE: %d\nRAR: %d\n----------------\n", num_ue, rar_type);
    printf("total attempt : %d\n", attempt);
    printf("once success  : %d\n", once_attempt_success);
    printf("once collide  : %d\n", once_attempt_collide);
    printf("\n\n");
    printf("avg. success: %f\n", avg_num_success);
    printf("\n\n");
    printf("avg. prob. success: %f\n", (float)success/trial);
    printf("avg. prob. collide: %f\n", (float)collide/trial);
    printf("\n\n");
    printf("once\n");
    printf("avg. prob. success: %f\n", (float)once_attempt_success/attempt);
    printf("avg. prob. collide: %f\n", (float)once_attempt_collide/attempt);
    printf("\n\n");
    printf("total RAs      : %d\n", num_ras);
}





