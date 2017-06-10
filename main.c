#include <stdio.h>
#include <math.h>
#include "lcgrand.h"

#define EXT_RAR_1 1
#define EXT_RAR_2 2
#define EXT_RAR_3 3
#define EXT_RAR_4 4 

#define total_ras 100*100
#define number_of_preamble 30
#define EXT_RAR EXT_RAR_3
#define num_UE 500



typedef enum events_e{
	event_ra_period,
	event_stop,
	num_normal_event,
}events_t;

typedef struct ue_s{
    float arrival_time;
    int preamble_index;
    int is_active;
    int retransmit_counter;
    int backoff_counter;
    
    //rar
    struct ue_s *next;
}ue_t;

typedef struct preamble_s{
    int num_selected;
    int num_selected_rar[EXT_RAR];
    ue_t *rar_ue_list[EXT_RAR];
}preamble_t;

int next_event_type;

float area_num_in_q;
float area_num_in_s;
float area_server_status;
float mean_interarrival;
float mean_service;
float sim_time;
float time_last_event;
float time_last_start_service;
float time_next_event[num_normal_event];
float total_of_delays;
float total_of_delays_system;
float stop_time;

FILE *infile;
FILE *outfile;

void initialize(void);
void timing(void);
void report(void);
void update_time_average_status(void);
float exponetial(float mean);
void ra_procedure(void);
void ue_arrival(int next_event_type);
void ue_backoff_process(void);
void ue_selected_preamble(int ue_id);

//  temp static configuration
ue_t ue_list[num_UE];  
float ra_period = 0.01f;
int max_retransmit = 6;
preamble_t preamble_table[number_of_preamble] = {0};
int failed=0, attempt=0, success=0, collide=0, once_attempt_success=0, retransmit=0, once_attempt_collide=0, trial=0, ras=0;
int back_off_window_size = 20;

int test1=0;
int test2=0;
int test3=0;

int main(void){
    //infile = fopen("mm1k.in", "r");
    //outfile = fopen("mm1k.out", "w");
    //while(!feof(infile)){
	
		//fscanf(infile, "%f %f %d %f", &mean_interarrival, &mean_service, &system_size, &stop_time);
	    //queue_size = system_size - num_server;
//	    stop_time = STOP;
	    mean_interarrival = 0.05f;
        initialize();
	    
	    do{
	        timing();
	        update_time_average_status();
	        
	        //printf("[%f] %d\n", sim_time, next_event_type);
	        switch(next_event_type){
	            case event_ra_period:
	                //printf("%f ra test1 %d\n", sim_time, test1);
                    ra_procedure();
	                ue_backoff_process();
	                break;
	            case event_stop:
	                //printf("%f stop test1 %d\n", sim_time, test1);
	            	report();
					break;
	            default:
	                //printf("ue:%d arrival\n", next_event_type-num_normal_event);
	                ++test1;
	                ue_arrival(next_event_type);
	                break;
	        }
	    }while(event_stop != next_event_type);
	    
	//}
	
	//fclose(infile);
	//fclose(outfile);
    return 0;
}

void ue_backoff_process(void){
    int i;
    for(i=0;i<num_UE;++i){
        if(ue_list[i].is_active == 0x1){
            if(ue_list[i].backoff_counter == 0x0){
                ue_selected_preamble(i);
            }else{
                //printf("*");
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
    
    ++ras;
    
    // RAR process
    for(i=0;i<number_of_preamble;++i){
        
        for(rar=0;rar<EXT_RAR;++rar){
            //test2+=preamble_table[i].num_selected_rar[rar];
            
            /*if(preamble_table[i].num_selected != 0)
            if(preamble_table[i].rar_ue_list[rar]->retransmit_counter == 0){
                ++test1;
            }*/
            
            if(preamble_table[i].num_selected_rar[rar] > 1){
                //  collision
                collide += preamble_table[i].num_selected_rar[rar];
                
                //*******************
                iterator = preamble_table[i].rar_ue_list[rar];
                while((ue_t *)0 != iterator){
                    if(iterator->retransmit_counter == 0){
                        once_attempt_collide += 1;
                        test2+=1;
                    }
                    iterator = iterator->next;
                }
                
                //*******************
                
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
                
                //*******************
                if(preamble_table[i].rar_ue_list[rar]->retransmit_counter == 0){
                    once_attempt_success+=1;
                    test2+=1;
                }
                //*******************
                
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
        // clear preamble table
        preamble_table[i].num_selected = 0x0;
    }
    //printf("%d %d\n", attempt, test2);
    //if(attempt != test2)system("pause");
    
    time_next_event[event_ra_period] = sim_time + ra_period;
}

void ue_selected_preamble(int ue_id){
    ue_t *iterator;
    int preamble_index = (int)(number_of_preamble*lcgrand(2));
    int rar_index = (int)(EXT_RAR*lcgrand(3));
    
    /*if(ue_list[ue_id].retransmit_counter == 0){
        ++attempt;
    }*/
    
    ++trial;
    
    ue_list[ue_id].is_active = 0x1;
    ue_list[ue_id].preamble_index = preamble_index;
    preamble_table[preamble_index].num_selected += 1;
    
    //  choose RAR
    preamble_table[preamble_index].num_selected_rar[rar_index] += 1;
    if( (ue_t *)0 == preamble_table[preamble_index].rar_ue_list[rar_index] ){
        preamble_table[preamble_index].rar_ue_list[rar_index] = &ue_list[ue_id];
    }else{
        iterator = preamble_table[preamble_index].rar_ue_list[rar_index];
        while( (ue_t *)0 != iterator->next ){
            iterator = iterator->next;
        }
        iterator->next = &ue_list[ue_id];
        ue_list[ue_id].next = (ue_t *)0;
    }
}


void ue_arrival(int next_event_type){
    int ue_id = next_event_type - num_normal_event;
    ++attempt;
    ue_selected_preamble(ue_id);
}


void initialize(void){
    int i;
    sim_time = 0.0f;
    failed=0;
    time_last_event = 0.0f;
    time_last_start_service = 0.0f;
    total_of_delays = 0.0f;
    total_of_delays_system = 0.0f;
    area_num_in_q = 0.0f;
    area_num_in_s = 0.0f;
    area_server_status = 0.0f;
    
    time_next_event[event_ra_period] = sim_time + ra_period;//+ exponetial(mean_interarrival);
    //time_next_event[event_depart] = 1.0e+30;
    time_next_event[event_stop] = stop_time;
    for(i=0;i<num_UE;++i){
        ue_list[i].is_active = 0x0;
        ue_list[i].backoff_counter = 0x0;
        ue_list[i].retransmit_counter = 0x0;
        ue_list[i].preamble_index = 0x0;
        ue_list[i].arrival_time = sim_time + exponetial(mean_interarrival);
    }
}

void timing(void){ 
    int i;
    float min_time_next_event = time_next_event[event_ra_period];
    next_event_type = event_ra_period;
    
    /*for(i=0;i<num_normal_event;++i){
        if(time_next_event[i] < min_time_next_event){
            min_time_next_event = time_next_event[i];
            next_event_type = i;

        }
    }*/
    
    for(i=0;i<num_UE;++i){
        if( 0x0 == ue_list[i].is_active){
            if(ue_list[i].arrival_time < min_time_next_event){
                min_time_next_event = ue_list[i].arrival_time;
                next_event_type = num_normal_event+i;
            }
        }
        
        //  TODO priority queue, using heap, implemented by array
    }
    
    if(ras >= total_ras){
        next_event_type = event_stop;
        return;
    }
    
    sim_time = min_time_next_event;
    //printf("%f %f\n", sim_time, min_time_next_event);
    //system("pause");
}

void report(void){ 
    
    int num_ras = total_ras;//(int)(stop_time / ra_period);
    float avg_num_attempt = (float)attempt/num_ras;
    float avg_num_success = (float)success/num_ras;
    float avg_num_collide = (float)collide/num_ras;
    float avg_num_once_success = (float)once_attempt_success/num_ras;
    float avg_num_once_collide = (float)once_attempt_collide/num_ras;
    printf("----------------\nUE: %d\nRAR: %d\n----------------\n", num_UE, EXT_RAR);
    printf("total attempt : %d\n", attempt);
    printf("once success  : %d\n", once_attempt_success);
    printf("once collide  : %d\n", once_attempt_collide);
    printf("\n\n");
    printf("avg. success: %f\n", avg_num_success);     //  between 0-M transmission
    /*printf("total trial  : %d\n", trial);       //  retransmission+attemp
    printf("total success: %d\n", success);     //  between 0-M transmission
    printf("total failed : %d\n", failed);      //  after M retransmission still failed
    printf("retransmit:%d\n", retransmit);
    */
    printf("\n\n");
    printf("avg. prob. success: %f\n", (float)success/trial);
    printf("avg. prob. collide: %f\n", (float)collide/trial);
    /*printf("avg. num attempt: %f\n", avg_num_attempt);
    printf("avg. num success: %f\n", avg_num_success);
    printf("avg. num collide: %f\n", avg_num_collide);*/
    printf("\n\n");
    printf("once\n");
    printf("avg. prob. success: %f\n", (float)once_attempt_success/attempt);
    printf("avg. prob. collide: %f\n", (float)once_attempt_collide/attempt);
    printf("\n\n");
    printf("total RAs      : %d\n", num_ras);
    
    printf("test1: %d\ntest2: %d\ntest3: %d\n", test1, test2, test3);
}

void update_time_average_status(void){ 

}

float exponetial(float mean){
    return -mean * log(lcgrand(1));
}
