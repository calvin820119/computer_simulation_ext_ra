#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "main.h"
#include "lcgrand.h"

    static float sim_time;
    static float stop_time;
    static float time_next_event[num_normal_event];
    static int next_event_type;

int main(int argc, char *argv[]){

    ext_ra_inst_t ext_ra_inst;
	
    initialize(&ext_ra_inst);
	    
	do{
	    timing(&ext_ra_inst);
	    printf("%f\n", sim_time);
	    switch(next_event_type){
	        case event_ra_period:
                ra_procedure(&ext_ra_inst);
	            ue_backoff_process(&ext_ra_inst);
	            break;
	        case event_stop:
	            report(&ext_ra_inst);
		    break;
	        default:
	            ue_arrival(&ext_ra_inst, next_event_type);
	            break;
	    }
	
	}while(event_stop != next_event_type);
	
	printf("finished\n");
	return 0;
}

float exponetial(float mean){
    //return mean;
    return -mean * log(lcgrand(1));
}

void ue_backoff_process(ext_ra_inst_t *inst){
    int i;
    for(i=0;i<1000;++i){
        if(inst->ue_list[i].is_active == 1){
            if(inst->ue_list[i].backoff_counter == 0){
                ue_selected_preamble(inst, i);
            }else{
                inst->ue_list[i].backoff_counter -= 1;
            }
        }
    }
}

void ra_procedure(ext_ra_inst_t *inst){
	
    int i;
    int rar;
    ue_t *iterator, *iterator1;
    
    inst->ras+=1;    
    
    // RAR process
    for(i=0;i<30;++i){
        for(rar=0;rar<4;++rar){
            if(inst->preamble_table[i].num_selected_rar[rar] > 1){
                //  collision
                inst->collide += inst->preamble_table[i].num_selected_rar[rar];
                iterator = inst->preamble_table[i].rar_ue_list[rar];
                while((ue_t *)0 != iterator){
                    if(iterator->retransmit_counter == 0){
                        inst->once_attempt_collide += 1;
                    }
                    iterator = iterator->next;
                }
                
                iterator = inst->preamble_table[i].rar_ue_list[rar];
                while((ue_t *)0 != iterator){
                    if(iterator->retransmit_counter >= inst->max_retransmit){
                        //  failed
                        inst->failed += 1;
                        iterator->retransmit_counter = 0;
                        iterator->backoff_counter = 0;
                        iterator->is_active = 0x0;
                        iterator->arrival_time = sim_time + exponetial(inst->mean_interarrival);
                        
                    }else{
                        inst->retransmit += 1;
                        //  retransmit
                        iterator->retransmit_counter += 1;
                        //  uniform backoff
                        iterator->backoff_counter = (int)(inst->back_off_window_size*lcgrand(4));
                    }
                    iterator1 = iterator;
                    iterator = iterator->next;
                    iterator1->next = (ue_t *)0;
                }
            }else if(inst->preamble_table[i].num_selected_rar[rar] == 1){
                //  success
                
                inst->success+=1;
                
                if(inst->preamble_table[i].rar_ue_list[rar]->retransmit_counter == 0){
                    inst->once_attempt_success+=1;
                }
                
                inst->preamble_table[i].rar_ue_list[rar]->is_active = 0x0;
                inst->preamble_table[i].rar_ue_list[rar]->retransmit_counter = 0x0;
                inst->preamble_table[i].rar_ue_list[rar]->backoff_counter = 0;
                inst->preamble_table[i].rar_ue_list[rar]->arrival_time = sim_time + exponetial(inst->mean_interarrival);
                inst->preamble_table[i].rar_ue_list[rar]->next = (ue_t *)0;
                
            }
            // clear rar table
            inst->preamble_table[i].rar_ue_list[rar] = (ue_t *)0;
            inst->preamble_table[i].num_selected_rar[rar] = 0;
        }
    }
    time_next_event[event_ra_period] = sim_time + inst->ra_period;

}




void ue_arrival(ext_ra_inst_t *inst, int next_event_type){
    int ue_id = next_event_type - num_normal_event;
    inst->attempt+=1;
    ue_selected_preamble(inst, ue_id);
}


void ue_selected_preamble(ext_ra_inst_t *inst, int ue_id){
    ue_t *iterator2;
    int preamble_index;
    int rar_index;
    
    inst->trial+=1;
    
    preamble_index = (int)(number_of_preamble*lcgrand(2));
    rar_index = (int)(4*lcgrand(3));
    
    inst->ue_list[ue_id].is_active = 0x1;
    inst->ue_list[ue_id].preamble_index = preamble_index;
    
    //  choose RAR
    inst->preamble_table[preamble_index].num_selected_rar[rar_index] += 1;
    if( (ue_t *)0 == inst->preamble_table[preamble_index].rar_ue_list[rar_index] ){
        inst->preamble_table[preamble_index].rar_ue_list[rar_index] = &inst->ue_list[ue_id];
		inst->ue_list[ue_id].next = (ue_t *)0;
    }else{
        iterator2 = inst->preamble_table[preamble_index].rar_ue_list[rar_index];
        
        while( (ue_t *)0 != iterator2->next ){
            iterator2 = iterator2->next;
        }
        iterator2->next = &inst->ue_list[ue_id];
        inst->ue_list[ue_id].next = (ue_t *)0;
    }
}

void initialize(ext_ra_inst_t *inst){
    int i, j;
    
    //  simulator
    sim_time = 0.0f;
    time_next_event[event_ra_period] = sim_time + inst->ra_period;
    time_next_event[event_stop] = stop_time;
    next_event_type = event_ra_period;
    
    //  statistic
    inst->failed=0;
    inst->attempt=0;
    inst->success=0;
    inst->collide=0;
    inst->once_attempt_success=0;
    inst->retransmit=0;
    inst->once_attempt_collide=0;
    inst->trial=0;
    
    //  extended RA
    inst->ra_period = 0.01f;
    inst->ras=0;
    inst->max_retransmit = 6;
    inst->back_off_window_size = 20;
    inst->mean_interarrival = 0.05f;
	inst->rar_type = (rar_t)ext_rar_4;
    
    
    for(i=0;i<1000;++i){
        inst->ue_list[i].is_active = 0;
        inst->ue_list[i].backoff_counter = 0;
        inst->ue_list[i].retransmit_counter = 0;
        inst->ue_list[i].preamble_index = 0;
        inst->ue_list[i].arrival_time = sim_time + exponetial(inst->mean_interarrival);
        inst->ue_list[i].next = (ue_t *)0;
    }
    
    for(i=0;i<30;++i){
    	for(j=0;j<4;++j){
    		inst->preamble_table[i].num_selected_rar[j] = 0;
    		inst->preamble_table[i].rar_ue_list[j] = (ue_t *)0;
		}
	}
}

void timing(ext_ra_inst_t *inst){ 
    int i;
    float min_time_next_event = time_next_event[event_ra_period];
    next_event_type = event_ra_period;
    
    for(i=0;i<1000;++i){
        if( 0x0 == inst->ue_list[i].is_active){
            if(inst->ue_list[i].arrival_time < min_time_next_event){
                min_time_next_event = inst->ue_list[i].arrival_time;
                next_event_type = num_normal_event+i;
            }
        }
        
        //  TODO priority queue, using heap, implemented by array
        //	O(1), only need to check the root of heap 
    }
    if(inst->ras >= total_ras){
        next_event_type = event_stop;
        return;
    }
    
    
    //printf("before timing %f\n", sim_time);
    //printf("next event %d\n", next_event_type);
    sim_time = min_time_next_event;
    //printf("after timing %f ras %d\n", sim_time, inst->ras);
    
//    fgetc(stdin);
}

void report(ext_ra_inst_t *inst){ 
    int num_ras = total_ras;//(int)(stop_time / ra_period);
    float avg_num_attempt = (float)inst->attempt/num_ras;
    float avg_num_success = (float)inst->success/num_ras;
    float avg_num_collide = (float)inst->collide/num_ras;
    float avg_num_once_success = (float)inst->once_attempt_success/num_ras;
    float avg_num_once_collide = (float)inst->once_attempt_collide/num_ras;
    printf("----------------\nUE: %d\nRAR: %d\n----------------\n", num_ue, inst->rar_type);
    printf("total attempt : %d\n", inst->attempt);
    printf("once success  : %d\n", inst->once_attempt_success);
    printf("once collide  : %d\n", inst->once_attempt_collide);
    printf("\n\n");
    printf("avg. success: %f\n", avg_num_success);
    printf("\n\n");
    printf("avg. prob. success: %f\n", (float)inst->success/inst->trial);
    printf("avg. prob. collide: %f\n", (float)inst->collide/inst->trial);
    printf("\n\n");
    printf("once\n");
    printf("avg. prob. success: %f\n", (float)inst->once_attempt_success/inst->attempt);
    printf("avg. prob. collide: %f\n", (float)inst->once_attempt_collide/inst->attempt);
    printf("\n\n");
    printf("total RAs      : %d\n", num_ras);
}





